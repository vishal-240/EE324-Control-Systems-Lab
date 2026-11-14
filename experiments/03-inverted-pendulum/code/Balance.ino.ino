
#include <SPI.h>

#define SPI_MOSI 51
#define SPI_MISO 50
#define SPI_SCLK 52


#define ARM_CS   45   // theta - base/arm encoder (motor shaft)
#define PEND_CS  48   // alpha - pendulum encoder


#define MOTOR_EN  2
#define MOTOR_IN1 3
#define MOTOR_IN2 4

// Flip these (+1 / -1) per the SIGN CHECK above instead of
// rewiring anything.
#define MOTOR_SIGN  1
#define ALPHA_SIGN  1
#define THETA_SIGN  1


#define K1  -10   // theta
#define K2  155.667961   // alpha
#define K3  -5.985342   // theta_dot
#define K4   20.612036   // alpha_dot   


// =========================================================
// CONTROL SETTINGS
// =========================================================

#define LOOP_DT_US        5000UL   // 5 ms -> 200 Hz
#define VMAX              12.0     // motor voltage limit (V)
#define CATCH_ANGLE_DEG   3.0      // engage when |alpha| below
#define ALPHA_LIMIT_DEG   30.0     // disengage above this
#define THETA_LIMIT_DEG   120.0    // arm safety limit
#define VEL_FILTER_ALPHA  0.35     // velocity LPF, 0..1

// L298N + motor deadband: smallest voltage that actually
// makes the motor move. Commands are boosted past this so
// small LQR corrections aren't swallowed by friction and
// the driver's internal drop. Find yours: raise PWM slowly
// with a test sketch until the motor just turns, convert to
// volts (pwm/255*12). Typical: 1.5 - 3.0 V.
#define MOTOR_DEADBAND_V  2.0

// AMT22 14-bit: 16384 counts/rev, direct drive on this rig
#define RAD_PER_COUNT   (2.0 * PI / 16384.0)


#define AMT22_NOP 0x00
#define RES14 14
#define RES12 12


uint16_t armZeroCount  = 0;
uint16_t pendZeroCount = 0;

float theta = 0.0, alpha = 0.0;        // rad
float thetaDot = 0.0, alphaDot = 0.0;  // rad/s (filtered)
float thetaPrev = 0.0, alphaPrev = 0.0;

bool controlActive = false;

unsigned long lastLoopMicros = 0;
int print_variable = 0;


void setup()
{
  Serial.begin(115200);

  pinMode(SPI_SCLK, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO, INPUT);

  pinMode(ARM_CS, OUTPUT);
  pinMode(PEND_CS, OUTPUT);
  digitalWrite(ARM_CS, HIGH);
  digitalWrite(PEND_CS, HIGH);

  SPI.begin();

  // 16 MHz / 32 = 500 kHz
  SPI.setClockDivider(SPI_CLOCK_DIV32);

  pinMode(MOTOR_EN, OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  stopMotor();

  Serial.println();
  Serial.println("====================================");
  Serial.println("INVERTED PENDULUM - LQR BALANCE");
  Serial.println();
  Serial.print("K = [");
  Serial.print(K1); Serial.print(", ");
  Serial.print(K2); Serial.print(", ");
  Serial.print(K3); Serial.print(", ");
  Serial.print(K4); Serial.println("]");
  Serial.println();
  Serial.println("Let pendulum HANG DOWN at rest");
  Serial.println("Zeroing encoders in 3 s...");
  Serial.println("====================================");

  delay(3000);

  armZeroCount  = averagePosition(ARM_CS);
  pendZeroCount = averagePosition(PEND_CS);

  Serial.print("Arm zero count: ");
  Serial.println(armZeroCount);
  Serial.print("Pendulum zero count (hanging): ");
  Serial.println(pendZeroCount);
  Serial.println();
  Serial.println("Now lift pendulum slowly to the top.");
  Serial.println("ALPHA should read 0 at the top -");
  Serial.println("controller engages within +/-10 deg.");
  Serial.println("====================================");

  lastLoopMicros = micros();
}

// =========================================================
// MAIN LOOP - fixed 200 Hz
// =========================================================

void loop()
{
  // Fixed-rate tick: keeps dt constant so the velocity
  // estimates and LQR behave consistently
  while ((micros() - lastLoopMicros) < LOOP_DT_US) { }
  lastLoopMicros += LOOP_DT_US;

  const float dt = LOOP_DT_US / 1e6;

  // =======================================================
  // READ BOTH ENCODERS
  // =======================================================

  uint16_t armEncoder  = getPositionWithRetry(ARM_CS, RES14);
  uint16_t pendEncoder = getPositionWithRetry(PEND_CS, RES14);

  if (armEncoder == 0xFFFF || pendEncoder == 0xFFFF)
  {
    // Bad SPI read: skip this tick (state held; safety
    // limits still protect us on the next good read)
    return;
  }

  // =======================================================
  // COUNTS -> ANGLES (rad)
  // =======================================================

  // theta: arm angle from its zero, wrapped to +/-pi
  theta = THETA_SIGN *
          countsToRad(armEncoder, armZeroCount);

  // alpha: pendulum angle measured from UPRIGHT (top = 0).
  // Encoder is zeroed while hanging down at rest, so shift
  // by 180 deg: top reads 0, bottom reads +/-180.
  alpha = ALPHA_SIGN *
          wrapPi(countsToRad(pendEncoder, pendZeroCount) - PI);

  // =======================================================
  // VELOCITIES (finite difference + low-pass filter)
  // =======================================================

  float thetaDotRaw = wrapPi(theta - thetaPrev) / dt;
  float alphaDotRaw = wrapPi(alpha - alphaPrev) / dt;
  thetaPrev = theta;
  alphaPrev = alpha;

  thetaDot += VEL_FILTER_ALPHA * (thetaDotRaw - thetaDot);
  alphaDot += VEL_FILTER_ALPHA * (alphaDotRaw - alphaDot);

  // =======================================================
  // ENGAGE / DISENGAGE
  // =======================================================

  float alphaDeg = alpha * 180.0 / PI;
  float thetaDeg = theta * 180.0 / PI;

  if (!controlActive)
  {
    if (abs(alphaDeg) < CATCH_ANGLE_DEG)
    {
      controlActive = true;

      // Problem statement: theta is re-defined to 0 deg the
      // moment the balance controller is activated
      armZeroCount = armEncoder;
      theta = 0.0;
      thetaPrev = 0.0;
      thetaDot = 0.0;

      // Also clear the pendulum velocity filter: it holds
      // hand-motion from the lift, which would otherwise
      // produce a large spurious kick at engagement
      alphaPrev = alpha;
      alphaDot = 0.0;

      Serial.println(">>> BALANCE CONTROL ENGAGED <<<");
    }
  }
  else
  {
    if (abs(alphaDeg) > ALPHA_LIMIT_DEG ||
        abs(thetaDeg) > THETA_LIMIT_DEG)
    {
      controlActive = false;
      stopMotor();
      Serial.println(">>> DISENGAGED (limit hit) <<<");
    }
  }

  // =======================================================
  // LQR CONTROL LAW:  u = -K x   (u in Volts)
  // =======================================================

  if (controlActive)
  {
    float u = -(K1 * theta +
                K2 * alpha +
                K3 * thetaDot +
                K4 * alphaDot);

    if (u >  VMAX) u =  VMAX;
    if (u < -VMAX) u = -VMAX;

    setMotorVoltage(MOTOR_SIGN * u);
  }
  else
  {
    stopMotor();
  }

  // =======================================================
  // TELEMETRY every ~100 ms (MODIFIED FOR SERIAL PLOTTER)
  // =======================================================

  print_variable += 1;
  if (print_variable >= 20)
  {
    print_variable = 0;
    
    // Only log data when the controller is actively balancing
    if (controlActive) {
      Serial.print("Theta:");
      Serial.print(thetaDeg, 2);
      Serial.print(",");
      Serial.print("Alpha:");
      Serial.print(alphaDeg, 2);
      Serial.print(",");
      Serial.print("Limit_+3:");
      Serial.print(3.0);
      Serial.print(",");
      Serial.print("Limit_-3:");
      Serial.println(-3.0);
    }
  }
}

// =========================================================
// ANGLE HELPERS
// =========================================================

// Wrap any angle (rad) to (-pi, pi]
float wrapPi(float a)
{
  while (a >  PI) a -= 2.0 * PI;
  while (a < -PI) a += 2.0 * PI;
  return a;
}

// (count - zero) with proper 14-bit wraparound -> radians,
// signed, in (-pi, pi]
float countsToRad(uint16_t count, uint16_t zero)
{
  int16_t rel = (int16_t)((count - zero) & 0x3FFF);
  if (rel > 8191) rel -= 16384;
  return rel * RAD_PER_COUNT;
}

// =========================================================
// MOTOR CONTROL (L298N, Exp 2 style)
// =========================================================

// Motor voltage (-VMAX..VMAX) -> direction pins + PWM,
// with deadband compensation: |v| is remapped from
// 0..VMAX onto DEADBAND..VMAX so even tiny corrections
// actually move the motor.
void setMotorVoltage(float v)
{
  float mag = abs(v);

  if (mag < 0.05)   // genuinely zero command -> coast
  {
    stopMotor();
    return;
  }

  float vEff = MOTOR_DEADBAND_V +
               mag * (VMAX - MOTOR_DEADBAND_V) / VMAX;

  int pwm = constrain((int)(vEff / VMAX * 255.0), 0, 255);

  if (v > 0)
  {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
  }
  else
  {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, HIGH);
  }

  analogWrite(MOTOR_EN, pwm);
}

void stopMotor()
{
  analogWrite(MOTOR_EN, 0);
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
}

// =========================================================
// ENCODER ZEROING (average several reads)
// =========================================================

uint16_t averagePosition(uint8_t cs)
{
  uint32_t sum = 0;
  uint8_t n = 0;

  for (uint8_t i = 0; i < 16; i++)
  {
    uint16_t p = getPositionWithRetry(cs, RES14);
    if (p != 0xFFFF)
    {
      sum += p;
      n++;
    }
    delay(5);
  }

  return (n > 0) ? (uint16_t)(sum / n) : 0;
}

// =========================================================
// ENCODER READ WITH RETRY (same as Exp 2)
// =========================================================

uint16_t getPositionWithRetry(uint8_t encoder, uint8_t resolution)
{
  uint16_t position = getPositionSPI(encoder, resolution);

  if (position != 0xFFFF)
    return position;

  delayMicroseconds(10);

  return getPositionSPI(encoder, resolution);
}

// =========================================================
// AMT22 POSITION READ (same as Exp 2)
// =========================================================

uint16_t getPositionSPI(uint8_t encoder, uint8_t resolution)
{
  uint16_t currentPosition;
  bool binaryArray[16];

  digitalWrite(encoder, LOW);
  delayMicroseconds(3);

  currentPosition = ((uint16_t)SPI.transfer(AMT22_NOP)) << 8;
  delayMicroseconds(3);
  currentPosition |= SPI.transfer(AMT22_NOP);

  delayMicroseconds(3);
  digitalWrite(encoder, HIGH);

  for (int i = 0; i < 16; i++)
  {
    binaryArray[i] = (currentPosition >> i) & 0x01;
  }

  // AMT22 checksum
  if (
    (binaryArray[15] ==
      !(binaryArray[13] ^ binaryArray[11] ^
        binaryArray[9]  ^ binaryArray[7]  ^
        binaryArray[5]  ^ binaryArray[3]  ^
        binaryArray[1]))
    &&
    (binaryArray[14] ==
      !(binaryArray[12] ^ binaryArray[10] ^
        binaryArray[8]  ^ binaryArray[6]  ^
        binaryArray[4]  ^ binaryArray[2]  ^
        binaryArray[0]))
  )
  {
    currentPosition &= 0x3FFF;
  }
  else
  {
    return 0xFFFF;
  }

  if (resolution == RES12)
    currentPosition = currentPosition >> 2;

  return currentPosition;
}