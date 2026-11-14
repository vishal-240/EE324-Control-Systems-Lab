# Line Follower

## Overview
This experiment develops an autonomous line-following controller for the Spark V robot using its onboard infrared sensors.

## Control approach
The group report describes a PD steering controller using the left and right sensor readings to calculate tracking error. The ATmega16 ADC reads the sensors, while Timer1 Fast PWM controls the differential-drive motors.

Additional logic described in the report includes dynamic base-speed reduction as steering error increases, intersection handling, and line-loss recovery using the previous error direction. The report notes that the integral term was omitted.

## Hardware and implementation
- Spark V robot based on ATmega16
- Three IR line sensors
- L293D motor driver
- AVR C firmware

## Reported outcome
The report states that the robot followed the supplied continuous track and met the 30-second design requirement. It also notes that battery discharge affected available motor torque for a given PWM command.

## Files
- `code/main.c` — main AVR firmware
- `code/lcd.c` — LCD support code
- `reports/lineFollower_report.pdf` — group experiment report

Original source and report files are copied without editing their contents.
