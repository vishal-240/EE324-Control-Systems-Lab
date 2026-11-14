# Active Noise Cancellation in Headphones

## Overview
This experiment characterizes a headphone acoustic feedback plant and evaluates an analog feedback controller for active noise cancellation.

## Analysis and control approach
The group report describes measuring plant magnitude and phase using a sinusoidal frequency sweep, analysing the plant response, designing an amplifier and lag compensator, and measuring cascaded open-loop and closed-loop message/noise responses.

For unity negative feedback, the report uses `T_msg = L/(1 + L)` and `T_noise = 1/(1 + L)`, where `L = C(s)G(s)`.

## Reported measurements
The plant was measured at 31 frequency points from 100 Hz to 10 kHz. The report records a plant gain of 8.03 dB and phase of 111° at 100 Hz.

| Frequency | Measured noise response |
|---|---:|
| 100 Hz | −12.77 dB |
| 500 Hz | −6.38 dB |
| 1000 Hz | −6.67 dB |

The 100 Hz result did not reach the 20 dB attenuation target. The report estimates an approximately 13 dB gain margin and notes limitations in measuring the low-frequency gain crossover.

## Files
- `matlab/freq_response.m`, `matlab/TF_Identifier.m` — MATLAB analysis scripts
- `data/Exp4.xlsx` — measurement workbook
- `results/` — supplied frequency-response and closed-loop plots
- `reports/Fri_G4_E4_report.pdf` — group experiment report

Original scripts, workbook, plots, and report are copied without editing their contents.
