/* Include the SPI library for the arduino boards */
#include <SPI.h>
 
/* Serial rates for UART */
#define BAUDRATE        115200
 
/* SPI commands */
#define AMT22_NOP       0x00
#define AMT22_RESET     0x60
#define AMT22_ZERO      0x70
 
/* Resolution macros */
#define RES12           12
#define RES14           14
 
/* SPI pins (Updated based on your exact wiring) */
#define ENC_0           48 // Pitch Encoder CS
#define ENC_1           45 // Yaw Encoder CS
#define SPI_MOSI        51
#define SPI_MISO        50
#define SPI_SCLK        52
 
/* L298N Motor Driver Pins */
#define ENA             2  // Pitch Motor PWM
#define IN1             3  // Pitch Motor Dir 1
#define IN2             4  // Pitch Motor Dir 2
#define ENB             7  // Yaw Motor PWM
#define IN3             5  // Yaw Motor Dir 1
#define IN4             6  // Yaw Motor Dir 2
 
/* PID Variables - Will be overwritten by PCB knobs */
float Kp_pitch = 0.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_yaw = 0.0,   Ki_yaw = 0.0,   Kd_yaw = 0.0;
 
float setpoint_pitch = 15.0; // Target pitch in degrees
float setpoint_yaw = -25.0;  // Target yaw in degrees
 
float error_pitch, integral_pitch, prev_error_pitch;
float error_yaw, integral_yaw, prev_error_yaw;

/* Integral Windup Limits */
const float PITCH_INTEGRAL_LIMIT = 100.0;
const float YAW_INTEGRAL_LIMIT = 200.0;
 
// Variables for tracking performance metrics
float max_pitch = 0.0;
float min_yaw = 0.0;
float peak_time_pitch = 0.0;
float peak_time_yaw = 0.0;
float pitch_overshoot_pct = 0.0;
float yaw_overshoot_pct = 0.0;
 
unsigned long lastTime;
unsigned long testStartTime = 0; 
 
void setup()
{
  // Set the modes for the SPI IO
  pinMode(SPI_SCLK, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO, INPUT);
  pinMode(ENC_0, OUTPUT);
  pinMode(ENC_1, OUTPUT);
  
  // Initialize Motor Driver pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
 
  // Initialize the UART serial connection for debugging
  Serial.begin(BAUDRATE);
 
  // Get the CS line high which is the default inactive state
  digitalWrite(ENC_0, HIGH);
  digitalWrite(ENC_1, HIGH);
 
  // 500 kHz is a good speed for our test environment
  SPI.setClockDivider(SPI_CLOCK_DIV32); 
  
  // start SPI bus
  SPI.begin();
  
  lastTime = millis();
  testStartTime = millis();
}
 
void loop()
{
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0; // Time step in seconds
  
  if (dt >= 0.02) { // Run control loop at 50Hz (every 20ms)
    lastTime = now;
 
    // --- PCB TUNING LOGIC ---
    // Read the 3 knobs (Values range from 0 to 1023)
    int raw_Kp = analogRead(A0);
    int raw_Ki = analogRead(A1);
    int raw_Kd = analogRead(A2);
 
    // Map the 0-1023 knob values to usable PID ranges
    // Change the 50.0 or 10.0 multipliers if you need higher/lower maximums
    float shared_Kp = (raw_Kp / 1023.0) * 50.0;
    float shared_Ki = (raw_Ki / 1023.0) * 20.0;
    float shared_Kd = (raw_Kd / 1023.0) * 20.0;
 
    // Apply the shared values to both axes
    Kp_pitch = 15.06;
    Kp_yaw   = 4.65;
    Ki_pitch = 1.78;
    Ki_yaw   = 1.0;
    Kd_pitch = 12.43;
    Kd_yaw   = 16.93;
    // ------------------------
 
    // --- KEYBOARD SOFTWARE RESET ---
    if (Serial.available() > 0) {
      char incoming = Serial.read();
      if (incoming == 'R' || incoming == 'r') {
        // Reset Performance Trackers & Timers
        max_pitch = 0.0;
        min_yaw = 0.0;
        peak_time_pitch = 0.0;
        peak_time_yaw = 0.0;
        pitch_overshoot_pct = 0.0;
        yaw_overshoot_pct = 0.0;
        testStartTime = millis(); 
        
        // Reset PID Memory
        integral_pitch = 0.0;
        integral_yaw = 0.0;
        prev_error_pitch = 0.0;
        prev_error_yaw = 0.0;
        
        Serial.println("\n>>> TRACKING & PID RESET. STARTING NEW TEST. <<<");
      }
    }
    // -------------------------------
 
    // 1. Read Encoders
    uint16_t raw_pitch = getPositionSPI(ENC_0, RES14);
    uint16_t raw_yaw = getPositionSPI(ENC_1, RES14);
    
    // 2. Convert to Signed Degrees (-180 to +180)
    float current_pitch = 0.0;
    if (raw_pitch != 0xFFFF) {
      current_pitch = (raw_pitch * 360.0) / 16384.0;
      if (current_pitch > 180.0) {
        current_pitch -= 360.0; 
      }
    }
 
    float current_yaw = 0.0;
    if (raw_yaw != 0xFFFF) {
      current_yaw = (raw_yaw * 360.0) / 16384.0;
      if (current_yaw > 180.0) {
        current_yaw -= 360.0;
      }
    }
 
    // --- PERFORMANCE TRACKING ---
    float current_test_time = (millis() - testStartTime) / 1000.0;
 
    if (current_pitch > max_pitch) {
      max_pitch = current_pitch;
      peak_time_pitch = current_test_time;
      if (max_pitch > setpoint_pitch) {
        pitch_overshoot_pct = ((max_pitch - setpoint_pitch) / setpoint_pitch) * 100.0;
      }
    }
 
    if (current_yaw < min_yaw) {
      min_yaw = current_yaw;
      peak_time_yaw = current_test_time;
      if (min_yaw < setpoint_yaw) {
        yaw_overshoot_pct = (((min_yaw) - (setpoint_yaw)) / (setpoint_yaw)) * 100.0;
      }
    }
    // ----------------------------
 
    // 3. Compute PID for Pitch
    error_pitch = setpoint_pitch - current_pitch;
    integral_pitch += error_pitch * dt;
    integral_pitch = constrain(integral_pitch, -PITCH_INTEGRAL_LIMIT, PITCH_INTEGRAL_LIMIT);
   
    float derivative_pitch = (error_pitch - prev_error_pitch) / dt;
    float output_pitch = (Kp_pitch * error_pitch) + (Ki_pitch * integral_pitch) + (Kd_pitch * derivative_pitch);
    prev_error_pitch = error_pitch;
 
    // 4. Compute PID for Yaw
    error_yaw = setpoint_yaw - current_yaw;
    integral_yaw += error_yaw * dt;
    integral_yaw = constrain(integral_yaw, -YAW_INTEGRAL_LIMIT, YAW_INTEGRAL_LIMIT);
   
    float derivative_yaw = (error_yaw - prev_error_yaw) / dt;
    float output_yaw = (Kp_yaw * error_yaw) + (Ki_yaw * integral_yaw) + (Kd_yaw * derivative_yaw);
    prev_error_yaw = error_yaw;
 
    // 5. Drive Motors
    driveMotor_p(output_pitch, ENA, IN1, IN2);
    driveMotor_y(output_yaw, ENB, IN3, IN4);
    
    // 6. Debugging Output 
    // Format: P_Angle | Y_Angle | Kp | Ki | Kd | P_Over | P_Peak | Y_Over | Y_Peak
    Serial.print("P_Ang:"); Serial.print(current_pitch);
    Serial.print(" Y_Ang:"); Serial.print(current_yaw);
    Serial.print(" Kp:"); Serial.print(shared_Kp);
    Serial.print(" Ki:"); Serial.print(shared_Ki);
    Serial.print(" Kd:"); Serial.print(shared_Kd);
    Serial.print(" P_Ov%:"); Serial.print(pitch_overshoot_pct);
    Serial.print(" P_Pk:"); Serial.print(peak_time_pitch);
    Serial.print(" Y_Ov%:"); Serial.print(yaw_overshoot_pct);
    Serial.print(" Y_Pk:"); Serial.println(peak_time_yaw);
  }
}
 
// Function to drive L298N motors smoothly
void driveMotor_p(float pid_output, int enPin, int in1, int in2)
{
  // Directly map PID output to PWM, relying on Ki to overcome friction smoothly
  int pwm = constrain(abs((int)pid_output), 0, 255);
 
  if (pid_output > 0.0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (pid_output < 0.0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    // Inside the deadzone, completely turn off
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    pwm = 0;
  }
 
  analogWrite(enPin, pwm);
}

void driveMotor_y(float pid_output, int enPin, int in1, int in2)
{
  // Directly map PID output to PWM, relying on Ki to overcome friction smoothly
  int pwm = constrain(abs((int)pid_output), 0, 225);
 
  if (pid_output > 0.0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (pid_output < 0.0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    // Inside the deadzone, completely turn off
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    pwm = 0;
  }
 
  analogWrite(enPin, pwm);
}

/*
 * -------------------------------------------------------------------------
 * ORIGINAL CUI AMT22 HELPER FUNCTIONS BELOW
 * -------------------------------------------------------------------------
 */
 
uint16_t getPositionSPI(uint8_t encoder, uint8_t resolution)
{
  uint16_t currentPosition;      
  bool binaryArray[16];          
  
  currentPosition = spiWriteRead(AMT22_NOP, encoder, false) << 8;  
  delayMicroseconds(3);
  currentPosition |= spiWriteRead(AMT22_NOP, encoder, true);        
  
  for(int i = 0; i < 16; i++) binaryArray[i] = (0x01) & (currentPosition >> (i));
  
  if ((binaryArray[15] == !(binaryArray[13] ^ binaryArray[11] ^ binaryArray[9] ^ binaryArray[7] ^ binaryArray[5] ^ binaryArray[3] ^ binaryArray[1]))
          && (binaryArray[14] == !(binaryArray[12] ^ binaryArray[10] ^ binaryArray[8] ^ binaryArray[6] ^ binaryArray[4] ^ binaryArray[2] ^ binaryArray[0])))
    {
      currentPosition &= 0x3FFF;
    }
  else
  {
    currentPosition = 0xFFFF; //bad position
  }
  
  if ((resolution == RES12) && (currentPosition != 0xFFFF)) currentPosition = currentPosition >> 2;
  
  return currentPosition;
}
 
uint8_t spiWriteRead(uint8_t sendByte, uint8_t encoder, uint8_t releaseLine)
{
  uint8_t data;
  setCSLine(encoder ,LOW);
  delayMicroseconds(3);
  data = SPI.transfer(sendByte);
  delayMicroseconds(3);
  setCSLine(encoder, releaseLine);
  return data;
}
 
void setCSLine (uint8_t encoder, uint8_t csLine)
{
  digitalWrite(encoder, csLine);
}
 
void setZeroSPI(uint8_t encoder)
{
  spiWriteRead(AMT22_NOP, encoder, false);
  delayMicroseconds(3);
  spiWriteRead(AMT22_ZERO, encoder, true);
  delay(250);
}
 
void resetAMT22(uint8_t encoder)
{
  spiWriteRead(AMT22_NOP, encoder, false);
  delayMicroseconds(3);
  spiWriteRead(AMT22_RESET, encoder, true);
  delay(250);
}