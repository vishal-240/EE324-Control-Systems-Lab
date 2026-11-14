% Load the data from the CSV file
data = readmatrix('pendulum_data.csv');

% Extract columns
time_ms = data(:, 1);
theta = data(:, 2);
alpha = data(:, 3);

% Convert time from milliseconds to seconds, starting at t=0
time_s = (time_ms - time_ms(1)) / 1000; 

% Create the plot
figure;
hold on; grid on;

% Plot Theta (Arm Angle) and Alpha (Pendulum Angle)
plot(time_s, theta, 'b-', 'LineWidth', 1.5, 'DisplayName', 'Theta (Arm)');
plot(time_s, alpha, 'r-', 'LineWidth', 1.5, 'DisplayName', 'Alpha (Pendulum)');

% Add dashed lines to prove the +/- 3 degree alpha limit
yline(3, 'r--', 'Alpha +3 Limit', 'LabelHorizontalAlignment', 'left');
yline(-3, 'r--', 'Alpha -3 Limit', 'LabelHorizontalAlignment', 'left');

% Formatting for the report
title('Inverted Pendulum: LQR Balance Control Response');
xlabel('Time (seconds)');
ylabel('Angle (degrees)');
legend('Location', 'best');
ylim([-35 35]); % Gives enough room to see Theta's +/- 30 deg limits too