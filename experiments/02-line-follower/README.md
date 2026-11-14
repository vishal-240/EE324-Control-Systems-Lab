# Autonomous Line Follower

**Control:** PD steering · **Platform:** Spark V / ATmega16 · **Sensors:** Three IR white-line sensors · **Actuation:** Differential-drive motors through L293D

## Objective

Design a controller for the Spark V robot to follow the supplied continuous track autonomously, with a stated completion target of 30 seconds.

## Hardware and sensor interfacing

- Spark V robot based on ATmega16
- Three IR white-line sensors: left, centre, right
- L293D motor driver and differential-drive motors
- ISP programmer and track layout

The ATmega16 ADC polls the sensors. With `ADLAR=1`, the firmware uses 8-bit ADC readings to distinguish the black track from the white surface. Timer1 generates 8-bit Fast PWM at approximately 450 Hz; duty cycle controls motor speed.

## Steering controller

The steering error is defined as

\[
e_k=l_k-r_k
\]

and the PD correction uses the current error and its change:

\[
u*k=K_p e_k+K_d(e_k-e*{k-1}).
\]

The derivative term adds damping and reduces oscillation. The integral term was deliberately omitted to avoid integral windup on sharp curves.

### Dynamic speed adjustment

The base speed is reduced as the magnitude of steering error increases. This slows the robot before sharp turns and allows faster travel on straighter sections, reducing momentum-induced slipping.

### Special-case behaviour

- **Intersection:** If all sensors detect black, the robot drives straight at constant speed to clear the crosshair. The report notes this would fail at a T-junction, which was not present on the supplied track.
- **Line lost:** If all sensors detect white, the robot uses the sign of the previous error to perform an asymmetric sweep and reacquire the line.

## Experimental observations

- Initial proportional-only control caused severe oscillations at high speed.
- Adding derivative action provided the necessary damping.
- Dynamic braking reduced momentum-induced slip on sharp turns.
- Stopwatch trials were used to check the 30-second target; it was consistently met.
- Battery discharge reduced available motor torque for the same PWM command.

### Lab result sheet

\(K_p\) = 1; \(K_i\) = 0; \(K_d\) = 0.1

## Files

- [`code/main.c`](code/main.c) — main ATmega16 firmware
- [`code/lcd.c`](code/lcd.c) — LCD support code

## Takeaway

This experiment combines sensor calibration, ADC acquisition, timer-based PWM, and feedback control on a resource-constrained microcontroller. The key implementation decisions were PD steering, dynamic speed scaling, and explicit recovery behaviour for intersections and temporary line loss.
