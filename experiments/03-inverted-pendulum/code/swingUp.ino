// EE324 Controls Lab - Experiment 3
// Swing-up + Balance
//
// how to use (serial monitor, 115200 baud):
//   1. upright is the zero saved with 'z' in controller_lqr3 (same EEPROM).
//      to redo it here: hold the pendulum at its balance point and send 'z'
//   2. let the pendulum hang still, then send 's' - swing up. the arm pumps
//      energy into the pendulum and LQR takes over near the top. if it falls
//      it swings up again
//   3. send 'x' - stop the motor (any time)


#include <SPI.h>
#include <EEPROM.h>

// AMT22 SPI commands
#define AMT22_NOP 0x00

#define RES12 12
#define RES14 14

// SPI pins (Mega)
#define ARM_CS    45
#define PEND_CS   48
#define SPI_MOSI  51
#define SPI_MISO  50
#define SPI_SCLK  52

// L298N
#define MOTOR_EN  2
#define MOTOR_IN1 3
#define MOTOR_IN2 4

// flip these if something goes the wrong way
#define MOTOR_SIGN 1
#define ALPHA_SIGN 1
#define THETA_SIGN 1

// Q = diag([100 400 10 1]), R = 1
#define K1 -10.0
#define K2 155.667961
#define K3 -5.985342
#define K4 20.612036

// swing-up
#define PEND_PERIOD_S 0.1   // period of small swings
#define SWING_GAIN 7.0   // volts per unit of missing energy
#define SWING_VMAX 12.0
#define SWING_SIGN 1
#define SWING_CATCH_DEG 5.0  // LQR inside this angle

#define LOOP_DT_US 4000UL  // 250 Hz
#define VMAX 12.0
#define ALPHA_LIMIT_DEG 30.0  // LQR gives up past this and we swing up again
#define VEL_FILTER 0.35 // low pass filt
#define PRINT_EVERY 20  // print every 20 loops

#define RAD_PER_COUNT (2.0 * PI / 16384.0)  // 14 bit encoder
#define ENC_ERROR 0xFFFF

// upright count is saved in EEPROM: [magic(4)][count(2)][~count(2)]
// same layout as controller_lqr3, so the zero saved there is used here
const int EEPROM_ADDR = 0;
const uint32_t EEPROM_MAGIC = 0x4C515201UL;

enum Mode { IDLE, SWING_UP, BALANCE };
const char *modeName[] = { "idle", "swing", "balance" };
Mode mode = IDLE;

uint16_t armZero = 0;
uint16_t pendZero = 0;  // encoder count at upright

float theta = 0, alpha = 0;
float thetaDot = 0, alphaDot = 0;
float thetaPrev = 0, alphaPrev = 0;

bool zeroSaved = false;
bool filterInit = false;

unsigned long lastLoopTime = 0;
int printCount = PRINT_EVERY - 1;

void setup()
{
  pinMode(MOTOR_EN, OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  stopMotor();

  pinMode(SPI_SCLK, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO, INPUT);
  pinMode(ARM_CS, OUTPUT);
  pinMode(PEND_CS, OUTPUT);

  Serial.begin(115200);

  // CS idles high
  digitalWrite(ARM_CS, HIGH);
  digitalWrite(PEND_CS, HIGH);

  SPI.setClockDivider(SPI_CLOCK_DIV32);  // 500 kHz
  SPI.begin();

  zeroSaved = loadZero();

  Serial.println("time_ms,theta,alpha,thetaDot,alphaDot,mode");

  lastLoopTime = micros();
}

void loop()
{
  while (micros() - lastLoopTime < LOOP_DT_US) {}
  lastLoopTime += LOOP_DT_US;
  float dt = LOOP_DT_US / 1e6;

  uint16_t armPos = readEncoder(ARM_CS);
  uint16_t pendPos = readEncoder(PEND_CS);
  if (armPos == ENC_ERROR || pendPos == ENC_ERROR)
    return;  // bad read, skip this tick. serial commands wait in the buffer till the next good one

  // serial commands: z = save upright zero, s = swing up, x = stop
  while (Serial.available() > 0)
  {
    char c = tolower(Serial.read());

    if (c == 'z')
    {
      mode = IDLE;
      stopMotor();
      pendZero = pendPos;
      zeroSaved = saveZero();
      filterInit = false;
      lastLoopTime = micros(); 
    }
    else if (c == 's' && zeroSaved)  // needs an upright zero first
      mode = SWING_UP;
    else if (c == 'x')
      mode = IDLE;
  }

  theta = THETA_SIGN * countsToRad(armPos, armZero);
  alpha = ALPHA_SIGN * countsToRad(pendPos, pendZero);

  if (!filterInit)
  {
    filterInit = true;
    thetaPrev = theta;
    alphaPrev = alpha;
    thetaDot = 0;
    alphaDot = 0;
  }

  // velocity = dx/dt, then lpf
  thetaDot += VEL_FILTER * (wrapPi(theta - thetaPrev) / dt - thetaDot);
  alphaDot += VEL_FILTER * (wrapPi(alpha - alphaPrev) / dt - alphaDot);
  thetaPrev = theta;
  alphaPrev = alpha;

  float alphaDeg = alpha * RAD_TO_DEG;

  if (mode == SWING_UP && fabs(alphaDeg) < SWING_CATCH_DEG)
  {
    // theta = 0 from here on. the velocities are kept since the
    // pendulum is still moving when we catch it
    mode = BALANCE;
    armZero = armPos;
    theta = 0;
    thetaPrev = 0;
  }
  else if (mode == BALANCE && fabs(alphaDeg) > ALPHA_LIMIT_DEG)
    mode = SWING_UP;

  float u = 0;
  if (mode == SWING_UP)
    u = swingUpVoltage();
  else if (mode == BALANCE)
    u = -(K1 * theta + K2 * alpha + K3 * thetaDot + K4 * alphaDot);
  u = constrain(u, -VMAX, VMAX);

  if (mode == IDLE)
    stopMotor();
  else
    setMotorVoltage(MOTOR_SIGN * u);

  printCount++;
  if (printCount >= PRINT_EVERY)
  {
    printCount = 0;
    Serial.print(millis());
    Serial.print(",");
    Serial.print(theta * RAD_TO_DEG, 2);
    Serial.print(",");
    Serial.print(alphaDeg, 2);
    Serial.print(",");
    Serial.print(thetaDot * RAD_TO_DEG, 1);
    Serial.print(",");
    Serial.print(alphaDot * RAD_TO_DEG, 1);
    Serial.print(",");
    Serial.println(modeName[mode]);
  }
}


// swing-up

float pendulumEnergy()
{
  float w0 = 2.0 * PI / PEND_PERIOD_S;  // natural freq
  return 0.5 * sq(alphaDot / w0) + cos(alpha) - 1.0;
}

// push the arm whichever way adds energy,
float swingUpVoltage()
{
  float missing = 0.0 - pendulumEnergy();
  float dir = (alphaDot * cos(alpha) >= 0) ? 1.0 : -1.0;

  return SWING_SIGN * dir * constrain(SWING_GAIN * missing, -SWING_VMAX, SWING_VMAX);
}


// ---------- EEPROM (upright zero) ----------

bool loadZero()
{
  uint32_t magic;
  uint16_t count, notCount;
  EEPROM.get(EEPROM_ADDR, magic);
  EEPROM.get(EEPROM_ADDR + 4, count);
  EEPROM.get(EEPROM_ADDR + 6, notCount);

  if (magic != EEPROM_MAGIC) return false;
  if (count > 0x3FFF) return false;
  if (notCount != (uint16_t)~count) return false;

  pendZero = count;
  return true;
}

// clear the magic first and write it last, so if power goes off
// halfway through we don't load garbage next time
bool saveZero()
{
  uint16_t count = pendZero;
  EEPROM.put(EEPROM_ADDR, (uint32_t)0);
  EEPROM.put(EEPROM_ADDR + 4, count);
  EEPROM.put(EEPROM_ADDR + 6, (uint16_t)~count);
  EEPROM.put(EEPROM_ADDR, EEPROM_MAGIC);

  // read it back to check
  return loadZero() && pendZero == count;
}


// helpers

float wrapPi(float a)
{
  while (a > PI) a -= 2.0 * PI;
  while (a < -PI) a += 2.0 * PI;
  return a;
}

// angle of count relative to zero, handles the 16383 -> 0 wraparound
float countsToRad(uint16_t count, uint16_t zero)
{
  int16_t diff = (count - zero) & 0x3FFF;
  if (diff > 8191) diff -= 16384;
  return diff * RAD_PER_COUNT;
}

// voltage -> direction + PWM
void setMotorVoltage(float v)
{
  int pwm = constrain((int)(fabs(v) / VMAX * 255.0), 0, 255);

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
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_EN, 0);
}


// ---------- AMT22 encoder ----------

// tries up to 3 times, returns ENC_ERROR if all of them fail
uint16_t readEncoder(uint8_t encoder)
{
  uint8_t attempts = 0;
  uint16_t pos = getPositionSPI(encoder, RES14);
  while (pos == ENC_ERROR && ++attempts < 3)
    pos = getPositionSPI(encoder, RES14);
  return pos;
}

// reads the position, checks the two check bits
// returns 0xFFFF if the checksum is wrong
uint16_t getPositionSPI(uint8_t encoder, uint8_t resolution)
{
  uint16_t currentPosition;
  bool binaryArray[16];

  // high byte first, keep CS low
  currentPosition = spiWriteRead(AMT22_NOP, encoder, false) << 8;
  delayMicroseconds(3);
  // low byte, release CS after
  currentPosition |= spiWriteRead(AMT22_NOP, encoder, true);

  for (int i = 0; i < 16; i++)
    binaryArray[i] = (0x01) & (currentPosition >> i);

  // checksum from the datasheet: bit 15 = odd parity, bit 14 = even parity
  if ((binaryArray[15] == !(binaryArray[13] ^ binaryArray[11] ^ binaryArray[9] ^ binaryArray[7] ^ binaryArray[5] ^ binaryArray[3] ^ binaryArray[1]))
      && (binaryArray[14] == !(binaryArray[12] ^ binaryArray[10] ^ binaryArray[8] ^ binaryArray[6] ^ binaryArray[4] ^ binaryArray[2] ^ binaryArray[0])))
  {
    currentPosition &= 0x3FFF;  // remove check bits
  }
  else
  {
    currentPosition = 0xFFFF;
  }

  if (resolution == RES12 && currentPosition != 0xFFFF)
    currentPosition = currentPosition >> 2;

  return currentPosition;
}

uint8_t spiWriteRead(uint8_t sendByte, uint8_t encoder, uint8_t releaseLine)
{
  uint8_t data;

  setCSLine(encoder, LOW);
  delayMicroseconds(3);  // datasheet wants a small delay after CS goes low

  data = SPI.transfer(sendByte);
  delayMicroseconds(3);
  setCSLine(encoder, releaseLine);  // HIGH releases, LOW keeps it

  return data;
}

void setCSLine(uint8_t encoder, uint8_t csLine)
{
  digitalWrite(encoder, csLine);
}
