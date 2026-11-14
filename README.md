# EE324 — Control Systems Lab (2026)

A technical portfolio of four hardware-based control experiments completed as part of **EE324: Control Systems Lab**.

The experiment READMEs collect the objectives, system architecture, hardware, control approach, implementation details, experimental evidence, measured results, and limitations. The original group-report PDFs are not redistributed; relevant diagrams, plots, and result-sheet images from those reports are embedded in the READMEs.

## Experiments

| Experiment | Focus | Implementation |
|---|---|---|
| [Dual-Axis Tilt Rotor](experiments/01-dual-axis-tilt-rotor/README.md) | Simultaneous pitch/yaw regulation | Independent PID loops on Arduino Mega |
| [Line Follower](experiments/02-line-follower/README.md) | Autonomous track following | PD steering on Spark V / ATmega16 |
| [Inverted Pendulum](experiments/03-inverted-pendulum/README.md) | Upright stabilization and swing-up | LQR + energy-based swing-up |
| [Active Noise Cancellation](experiments/04-active-noise-cancellation/README.md) | Analog feedback noise attenuation | Plant identification and compensator design |

## Technologies

PID, PD, LQR, energy shaping, Arduino Mega 2560, ATmega16, AVR C, Arduino C/C++, AMT22 encoders, IR sensors, PWM motor drivers, MATLAB, frequency-response analysis, and state-space control.

## Repository contents

Each experiment contains a self-contained README, relevant source code, data, result plots, and selected figures extracted from its report. Original source files, datasets, and supplied plots are copied byte-for-byte. The figure images extracted from reports are included as separate assets so GitHub can render them inline.

## Academic attribution

These experiments include group work. Original code, datasets, and report figures retain their supplied content. Obtain permission from the other contributors before publicly sharing group-created material.

Course manuals, problem statements, lecture slides, complete group-report PDFs, vendor installers, and generated IDE/build files are intentionally excluded.
