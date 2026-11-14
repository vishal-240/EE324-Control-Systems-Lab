# Dual-Axis Tilt Rotor Balancing

## Overview
This experiment implements simultaneous pitch and yaw position control on a dual-axis tilt-rotor setup using an Arduino Mega 2560.

## Control approach
The group report describes independent PID feedback loops for the pitch and yaw axes. Encoder measurements are used for feedback, and motor commands are applied through PWM and an L298N H-bridge. The implementation also includes live gain tuning and integral anti-windup.

## Hardware and interfaces
- Arduino Mega 2560
- AMT22 absolute encoders over SPI
- L298N motor driver
- Two DC motor/propeller assemblies
- Potentiometers for controller-gain adjustment

## Reported results

| Metric | Pitch | Yaw |
|---|---:|---:|
| Kp | 15.06 | 4.85 |
| Ki | 1.78 | 1.00 |
| Kd | 12.43 | 16.93 |
| Peak overshoot | 3.33% | 0.3% |
| Peak time | 8 s | 73 s |
| Settling time | 35 s | 75 s |
| Steady-state error | 0° | 1° |

Both axes met the report's 5% overshoot constraint. The reported peak times exceeded the stated design benchmarks (3.5 s for pitch and 2.5 s for yaw), reflecting the stability/response-speed trade-off discussed in the report.

## Files
- `code/exp2_final.ino` — Arduino implementation
- `data/rotor_angles.csv` — recorded angle data
- `data/raw_data.txt` — raw recorded data
- `results/cleaned_plot.png` — prepared response plot
- `reports/ee324_exp2.pdf` — experiment report

Original source, data, plot, and report files are copied without editing their contents.
