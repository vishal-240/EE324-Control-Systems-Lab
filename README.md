# EE324 – Control Systems Lab (2026)

A record of four hardware-based control systems experiments completed as part of **EE324 – Control Systems Lab**.

The work spans embedded control, robotics, state-space control, experimental system identification, and analog feedback.

## Experiments

| Experiment | Focus | Main implementation |
|---|---|---|
| [01. Dual-Axis Tilt Rotor](experiments/01-dual-axis-tilt-rotor/README.md) | Simultaneous pitch and yaw regulation | Independent PID loops on Arduino Mega |
| [02. Line Follower](experiments/02-line-follower/README.md) | Autonomous track following | PD steering control on ATmega16 / Spark V |
| [03. Inverted Pendulum](experiments/03-inverted-pendulum/README.md) | Upright stabilization and swing-up | LQR balance and energy-based swing-up |
| [04. Active Noise Cancellation](experiments/04-active-noise-cancellation/README.md) | Feedback noise attenuation | Frequency-response measurements and analog compensation |

## Topics and tools

- **Control:** PID, PD, LQR, energy-based swing-up, feedback stability
- **Embedded:** Arduino Mega, ATmega16, AVR C, Arduino C/C++
- **Sensors and actuation:** AMT22 encoders, IR sensors, PWM motor drivers
- **Analysis:** MATLAB, frequency-response analysis, state-space methods
- **Experimental work:** CSV/XLSX measurements, response plots, hardware validation

## Repository layout

Each experiment has its own directory containing the available source code, data, results, and/or group report. Files are organized for discoverability; original code, reports, measurement data, and supplied plots have been copied without editing their contents.

## Notes on results

The experiment READMEs summarize objectives and measured outcomes documented in the accompanying reports. Where a target was not fully achieved, the measured result and limitation are stated rather than presenting it as a successful specification match.

## Academic attribution

Some experiments were completed as group work. The original reports retain their authorship and group attribution. Please consult the reports for experiment-specific contributor details.

This repository is an academic record of the work completed for EE324. Course-provided manuals, problem statements, lecture material, vendor utilities, and generated IDE/build artifacts are intentionally not redistributed here.
