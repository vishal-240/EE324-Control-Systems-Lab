
clc;
clear;
close all;

% Read data from CSV file in the same folder
filename = 'Frequency response - Sheet1.csv';

if ~isfile(filename)
    error('CSV file not found. Make sure it is in the current MATLAB folder.');
end

data = readtable(filename, 'VariableNamingRule', 'preserve');

% Extract required columns
f       = data{:, 1};   % Frequency (Hz)
gain_db = data{:, 4};   % Gain (dB)
phase   = data{:, 5};   % Phase (degrees)

% Create figure window
figure('Name', 'Bode Plot', ...
       'NumberTitle', 'off', ...
       'Position', [100, 100, 800, 600]);

% Magnitude Plot
subplot(2,1,1);
semilogx(f, gain_db, '-ob', ...
         'LineWidth', 1.5, ...
         'MarkerFaceColor', 'b');
grid on;
title('Bode Plot: Magnitude and Phase', 'FontSize', 12);
ylabel('Gain (dB)', 'FontSize', 11);
xlim([min(f) max(f)]);

% Phase Plot
subplot(2,1,2);
semilogx(f, phase, '-or', ...
         'LineWidth', 1.5, ...
         'MarkerFaceColor', 'r');
grid on;
xlabel('Frequency (Hz)', 'FontSize', 11);
ylabel('Phase (Degrees)', 'FontSize', 11);
xlim([min(f) max(f)]);
