# Inverted Pendulum: Swing-Up and LQR Balance

## Overview
This experiment implements upright stabilization of a rotary inverted pendulum using LQR state feedback, together with an energy-based swing-up controller.

## Control approach
The report defines the state as:

`x = [theta, alpha, theta_dot, alpha_dot]^T`

The balance controller uses the LQR law `u = -Kx`. Encoder measurements provide angular positions, and filtered finite differences estimate angular velocities.

The bonus implementation adds a swing-up mode based on an energy-shaping rule, with transitions between swing-up and balance modes.

## Reported implementation parameters
- Arduino Mega
- AMT22 encoders over SPI
- Balance-only loop period: 5 ms (200 Hz)
- Combined swing-up/balance loop period: 4 ms (250 Hz)
- Motor command limit: ±12 V
- LQR gain vector: `[-10, 155.667961, -5.985342, 20.612036]`
- MATLAB design weights: `Q = diag(100, 400, 10, 1)`, `R = 1`

The balance-only sketch and combined swing-up/balance sketch use different loop periods; these are kept distinct here as in the report.

## Reported measurements and limitations
The report records 420 telemetry samples over 41.918 s. The measured arm angle ranged from −14.33° to +31.33°, and the pendulum angle ranged from −3.49° to +4.09°. The report attributes brief excursions beyond the stated limits to an external disturbance during settling-time evaluation.

## Files
- `code/Balance.ino.ino` — balance-only Arduino sketch
- `code/swingUp.ino` — swing-up and balance sketch
- `matlab/matlab.m`, `matlab/plotting.m` — MATLAB scripts
- `data/pendulum_data.csv` — recorded telemetry
- `results/` — supplied images
- `reports/Fri_G4_E3_report.pdf` — group experiment report

Original source, data, images, and report files are copied without editing their contents.
