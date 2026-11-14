
clc;
clear;
close all;

%% =========================================================
%  1. MEASURED FREQUENCY RESPONSE DATA
% ==========================================================

f = [100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, ...
     1200, 1400, 1600, 1800, 2000, 2500, 3000, 3500, 4000, ...
     4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, ...
     8500, 9000, 9500, 10000];

gain_db = [8.0315, 12.0338, 10.3104, 13.2236, 12.8972, ...
           10.0789, 9.4530, 9.3615, 8.6949, 7.6878, ...
           8.7342, 11.0568, 14.2110, 14.1239, 14.2690, ...
           13.1808, 13.5614, 16.6637, 16.1912, 14.1811, ...
           6.6397, 10.9226, 9.3762, 8.9203, 8.8681, ...
           8.2685, 5.9919, 3.1773, 2.7582, 0.5334, -1.6864];

% Phase convention: CH1 - CH2 (output - input)
% TA-approved correction at 4500 Hz: +170 -> -190 degrees

phase_deg = [111, 43, 15, 5, -19, -25, -19.5, -25.7, ...
             -23, -23.7, -21.6, -7, -17.7, -35.4, ...
             -50, -77, -86, -112, -165, -190, ...
             -170, 163, 133, 116, 91, 70, 43, ...
             31, 15.5, -3, -20];

%% =========================================================
%  2. PREPARE COMPLEX FREQUENCY RESPONSE
% ==========================================================

f = f(:);
gain_db = gain_db(:);
phase_deg = phase_deg(:);

gain_linear = 10.^(gain_db/20);

phase_rad = unwrap(deg2rad(phase_deg));

H = gain_linear .* exp(1j*phase_rad);

w = 2*pi*f;

N = length(f);

fprintf('\n============================================\n');
fprintf('MEASURED DATA\n');
fprintf('============================================\n');

fprintf('Number of points: %d\n', N);
fprintf('Frequency range: %.0f to %.0f Hz\n', min(f), max(f));

%% =========================================================
%  3. PLOT ORIGINAL MEASUREMENTS
% ==========================================================

figure('Name','Measured plant frequency response');

subplot(2,1,1);

semilogx(f, gain_db, 'bo-', ...
    'LineWidth',1.5,'MarkerFaceColor','b');

grid on;
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
title('Measured plant magnitude response');

subplot(2,1,2);

semilogx(f, rad2deg(phase_rad), 'ko-', ...
    'LineWidth',1.5,'MarkerFaceColor','k');

grid on;
xlabel('Frequency (Hz)');
ylabel('Phase (degrees)');
title('Measured unwrapped phase response');

%% =========================================================
%  4. CROSS-VALIDATION SETTINGS
% ==========================================================

max_poles = 6;
number_of_folds = 5;

% Interleaved validation folds
fold_id = mod(0:N-1,number_of_folds)+1;

mag_scale = max(std(gain_db),eps);
phase_scale = max(std(rad2deg(phase_rad)),eps);

%% =========================================================
%  5. INITIALIZE RESULTS STORAGE
% ==========================================================

pole_counts = [];
zero_counts = [];
parameter_counts = [];

mean_mag_error = [];
mean_phase_error = [];
mean_combined_score = [];
std_combined_score = [];

valid_model_count = 0;

fprintf('\n============================================\n');
fprintf('5-FOLD CROSS-VALIDATION\n');
fprintf('============================================\n');

fprintf('Maximum poles: %d\n',max_poles);
fprintf('Number of folds: %d\n',number_of_folds);

%% =========================================================
%  6. TEST DIFFERENT POLE-ZERO COMBINATIONS
% ==========================================================

for np = 1:max_poles

    for nz = 0:(np-1)

        fprintf('\n--------------------------------------------\n');
        fprintf('Testing %d poles and %d zeros\n',np,nz);
        fprintf('--------------------------------------------\n');

        fold_mag_errors = nan(number_of_folds,1);
        fold_phase_errors = nan(number_of_folds,1);
        fold_scores = nan(number_of_folds,1);

        successful_folds = 0;

        for k = 1:number_of_folds

            validation_indices = (fold_id == k);
            training_indices = ~validation_indices;

            w_train = w(training_indices);
            H_train = H(training_indices);

            w_validation = w(validation_indices);
            H_validation = H(validation_indices);

            try

                data_train = idfrd(H_train,w_train,0);

                model_id = tfest(data_train,np,nz);

                G = tf(model_id);

                if ~isstable(G)
                    fprintf('Fold %d: Unstable model rejected\n',k);
                    continue;
                end

                H_predicted = squeeze(freqresp(G,w_validation));
                H_predicted = H_predicted(:);

                H_validation = H_validation(:);

                if any(~isfinite(H_predicted)) || ...
                   any(abs(H_predicted)<eps)

                    fprintf('Fold %d: Invalid prediction\n',k);
                    continue;
                end

                predicted_mag = 20*log10(abs(H_predicted));
                actual_mag = 20*log10(abs(H_validation));

                mag_error = sqrt(mean( ...
                    (predicted_mag-actual_mag).^2));

                phase_difference = angle( ...
                    H_predicted./H_validation);

                phase_error = sqrt(mean( ...
                    rad2deg(phase_difference).^2));

                combined_score = sqrt( ...
                    0.5*(mag_error/mag_scale)^2 + ...
                    0.5*(phase_error/phase_scale)^2);

                fold_mag_errors(k) = mag_error;
                fold_phase_errors(k) = phase_error;
                fold_scores(k) = combined_score;

                successful_folds = successful_folds+1;

                fprintf(['Fold %d: Mag RMSE = %.3f dB, ' ...
                         'Phase RMSE = %.3f deg, Score = %.4f\n'], ...
                         k,mag_error,phase_error,combined_score);

            catch ME

                fprintf('Fold %d failed: %s\n',k,ME.message);

            end

        end

        if successful_folds ~= number_of_folds

            fprintf('Candidate rejected: incomplete validation.\n');
            continue;

        end

        avg_mag = mean(fold_mag_errors);
        avg_phase = mean(fold_phase_errors);
        avg_score = mean(fold_scores);
        score_std = std(fold_scores);

        parameter_count = np+nz+1;

        valid_model_count = valid_model_count+1;

        pole_counts(valid_model_count) = np;
        zero_counts(valid_model_count) = nz;
        parameter_counts(valid_model_count) = parameter_count;

        mean_mag_error(valid_model_count) = avg_mag;
        mean_phase_error(valid_model_count) = avg_phase;
        mean_combined_score(valid_model_count) = avg_score;
        std_combined_score(valid_model_count) = score_std;

        fprintf('\nAverage validation results:\n');
        fprintf('Magnitude RMSE: %.3f dB\n',avg_mag);
        fprintf('Phase RMSE: %.3f degrees\n',avg_phase);
        fprintf('Combined score: %.4f\n',avg_score);

    end
end

%% =========================================================
%  7. CHECK VALID MODELS
% ==========================================================

if valid_model_count == 0
    error('No model completed all cross-validation folds.');
end

%% =========================================================
%  8. DISPLAY CROSS-VALIDATION RESULTS
% ==========================================================

Results = table( ...
    pole_counts(:), ...
    zero_counts(:), ...
    parameter_counts(:), ...
    mean_mag_error(:), ...
    mean_phase_error(:), ...
    mean_combined_score(:), ...
    std_combined_score(:), ...
    'VariableNames', ...
    {'Poles','Zeros','Parameters', ...
     'MagRMSE_dB','PhaseRMSE_deg', ...
     'MeanScore','ScoreStd'});

Results = sortrows(Results,'MeanScore','ascend');

fprintf('\n============================================\n');
fprintf('CROSS-VALIDATION RESULTS\n');
fprintf('============================================\n');

disp(Results);

%% =========================================================
%  9. ONE-STANDARD-ERROR MODEL SELECTION
% ==========================================================

best_score = Results.MeanScore(1);

best_score_std = Results.ScoreStd(1);

best_standard_error = best_score_std/sqrt(number_of_folds);

selection_threshold = best_score+best_standard_error;

fprintf('\n============================================\n');
fprintf('ONE-STANDARD-ERROR MODEL SELECTION\n');
fprintf('============================================\n');

fprintf('Best validation score: %.5f\n',best_score);
fprintf('Estimated standard error: %.5f\n',best_standard_error);
fprintf('Selection threshold: %.5f\n',selection_threshold);

acceptable = Results( ...
    Results.MeanScore<=selection_threshold,:);

acceptable = sortrows(acceptable, ...
    {'Parameters','MeanScore'}, ...
    {'ascend','ascend'});

selected_np = acceptable.Poles(1);
selected_nz = acceptable.Zeros(1);

fprintf('\nSelected model:\n');
fprintf('Poles: %d\n',selected_np);
fprintf('Zeros: %d\n',selected_nz);

%% =========================================================
%  10. REFIT SELECTED MODEL USING ALL MEASUREMENTS
% ==========================================================

fprintf('\n============================================\n');
fprintf('FINAL MODEL ESTIMATION\n');
fprintf('============================================\n');

data_all = idfrd(H,w,0);

G_selected_id = tfest(data_all,selected_np,selected_nz);

G_selected = tf(G_selected_id);

fprintf('\nSelected transfer function:\n');
G_selected

fprintf('Poles:\n');
disp(pole(G_selected));

fprintf('Zeros:\n');
disp(zero(G_selected));

fprintf('Stable: %d\n',isstable(G_selected));

fprintf('DC gain (extrapolation only):\n');
disp(dcgain(G_selected));

%% =========================================================
%  11. REFIT TOP THREE CANDIDATES
% ==========================================================

number_to_plot = min(3,height(Results));

top_models = cell(number_to_plot,1);
top_poles = zeros(number_to_plot,1);
top_zeros = zeros(number_to_plot,1);
top_scores = zeros(number_to_plot,1);

for k = 1:number_to_plot

    np = Results.Poles(k);
    nz = Results.Zeros(k);

    top_models{k} = tf(tfest(data_all,np,nz));

    top_poles(k) = np;
    top_zeros(k) = nz;
    top_scores(k) = Results.MeanScore(k);

end

%% =========================================================
%  12. GENERATE TOP MODEL RESPONSES
% ==========================================================

f_model = logspace(log10(100),log10(10000),1500);
w_model = 2*pi*f_model;

mag_top = cell(number_to_plot,1);
phase_top = cell(number_to_plot,1);

phase_measured = rad2deg(phase_rad);

for k = 1:number_to_plot

    H_top = squeeze(freqresp(top_models{k},w_model));
    H_top = H_top(:);

    mag_top{k} = 20*log10(abs(H_top));

    phase_top{k} = unwrap(angle(H_top))*180/pi;

    phase_top{k} = phase_top{k} + ...
        360*round((phase_measured(1)-phase_top{k}(1))/360);

end

%% =========================================================
%  13. MAGNITUDE COMPARISON
% ==========================================================

figure('Name','Cross-validated magnitude comparison');

semilogx(f,gain_db,'ko','MarkerSize',6,'LineWidth',1.3);
hold on;

plot_colors = {'b','r','m'};

legend_entries = cell(number_to_plot+1,1);
legend_entries{1} = 'Measured data';

for k = 1:number_to_plot

    semilogx(f_model,mag_top{k}, ...
        'Color',plot_colors{k},'LineWidth',1.8);

    legend_entries{k+1} = sprintf( ...
        '%dP-%dZ, CV score %.4f', ...
        top_poles(k),top_zeros(k),top_scores(k));

end

grid on;
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
title('Measured data vs top cross-validated models');
legend(legend_entries,'Location','best');
xlim([100 10000]);

%% =========================================================
%  14. PHASE COMPARISON
% ==========================================================

figure('Name','Cross-validated phase comparison');

semilogx(f,phase_measured,'ko','MarkerSize',6,'LineWidth',1.3);
hold on;

for k = 1:number_to_plot

    semilogx(f_model,phase_top{k}, ...
        'Color',plot_colors{k},'LineWidth',1.8);

end

grid on;
xlabel('Frequency (Hz)');
ylabel('Phase (degrees)');
title('Measured phase vs top cross-validated models');
legend(legend_entries,'Location','best');
xlim([100 10000]);

%% =========================================================
%  15. SELECTED MODEL BODE PLOT
% ==========================================================

figure('Name','Selected plant Bode response');

bode(G_selected);
grid on;

title(sprintf('Selected plant: %d poles, %d zeros', ...
    selected_np,selected_nz));

%% =========================================================
%  16. SELECTED MODEL POLE-ZERO MAP
% ==========================================================

figure('Name','Selected plant pole-zero map');

pzmap(G_selected);
grid on;

title('Pole-zero map of selected plant');

%% =========================================================
%  17. FINAL MODEL VALIDATION
% ==========================================================

f_check = logspace(log10(100),log10(10000),1000);
w_check = 2*pi*f_check;

H_check = squeeze(freqresp(G_selected,w_check));
H_check = H_check(:);

mag_check = 20*log10(abs(H_check));

phase_check = unwrap(angle(H_check))*180/pi;

phase_check = phase_check + ...
    360*round((phase_measured(1)-phase_check(1))/360);

figure('Name','Final plant validation');

subplot(2,1,1);

semilogx(f,gain_db,'ko','MarkerSize',6);
hold on;
semilogx(f_check,mag_check,'b-','LineWidth',1.8);

grid on;
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
legend('Measured data','Selected model');
title('Final plant magnitude validation');

subplot(2,1,2);

semilogx(f,phase_measured,'ko','MarkerSize',6);
hold on;
semilogx(f_check,phase_check,'b-','LineWidth',1.8);

grid on;
xlabel('Frequency (Hz)');
ylabel('Phase (degrees)');
legend('Measured data','Selected model');
title('Final plant phase validation');

%% =========================================================
%  18. PHASE ERROR VALIDATION
% ==========================================================

H_model_measured = squeeze(freqresp(G_selected,w));
H_model_measured = H_model_measured(:);

model_phase = unwrap(angle(H_model_measured))*180/pi;

model_phase = model_phase + ...
    360*round((phase_measured(1)-model_phase(1))/360);

phase_error = model_phase-phase_measured;

phase_error = mod(phase_error+180,360)-180;

phase_RMSE = sqrt(mean(phase_error.^2));

fprintf('\n============================================\n');
fprintf('PHASE VALIDATION\n');
fprintf('============================================\n');

fprintf('Phase RMSE: %.3f degrees\n',phase_RMSE);

PhaseComparison = table( ...
    f,phase_measured,model_phase,phase_error, ...
    'VariableNames', ...
    {'Frequency_Hz','Measured_Phase_deg', ...
     'Model_Phase_deg','Phase_Error_deg'});

disp(PhaseComparison);

figure('Name','Phase fitting error');

semilogx(f,phase_error,'ro-', ...
    'LineWidth',1.4,'MarkerFaceColor','r');

grid on;
xlabel('Frequency (Hz)');
ylabel('Phase error (degrees)');

title(sprintf('Phase fitting error (RMSE = %.2f degrees)', ...
    phase_RMSE));

%% =========================================================
%  19. INITIAL AMPLIFIER GAIN CALCULATION
%  Target: 20 dB noise attenuation at 100 Hz
% ==========================================================

fprintf('\n============================================\n');
fprintf('INITIAL AMPLIFIER GAIN CALCULATION\n');
fprintf('============================================\n');

% Locate the 100 Hz measurement
idx100 = find(abs(f-100)<1e-9,1);

if isempty(idx100)
    error('100 Hz measurement not found.');
end

% Actual measured plant response at 100 Hz
G_measured_100 = H(idx100);

% Identified plant model response at 100 Hz
G_model_100 = squeeze(freqresp(G_selected,2*pi*100));

% Target noise transfer magnitude
target_noise_ratio = 0.1;

% Required magnitude of 1+L
required_denominator = 1/target_noise_ratio;

% -----------------------------------------
% Gain based on measured plant
% -----------------------------------------

phi_measured = angle(G_measured_100);
mag_measured = abs(G_measured_100);

r_measured = -cos(phi_measured) + ...
    sqrt(cos(phi_measured)^2 + ...
    required_denominator^2-1);

A_measured = r_measured/mag_measured;

% -----------------------------------------
% Gain based on identified model
% -----------------------------------------

phi_model = angle(G_model_100);
mag_model = abs(G_model_100);

r_model = -cos(phi_model) + ...
    sqrt(cos(phi_model)^2 + ...
    required_denominator^2-1);

A_model = r_model/mag_model;

% -----------------------------------------
% Check achieved noise attenuation
% -----------------------------------------

noise_ratio_measured = abs( ...
    1/(1+A_measured*G_measured_100));

noise_attenuation_measured = ...
    -20*log10(noise_ratio_measured);

noise_ratio_model = abs( ...
    1/(1+A_model*G_model_100));

noise_attenuation_model = ...
    -20*log10(noise_ratio_model);

% -----------------------------------------
% Display results
% -----------------------------------------

fprintf('\nACTUAL MEASURED PLANT AT 100 Hz:\n');

fprintf('Magnitude: %.4f V/V\n',mag_measured);
fprintf('Magnitude: %.3f dB\n',20*log10(mag_measured));
fprintf('Phase: %.3f degrees\n',rad2deg(phi_measured));

fprintf('\nIDENTIFIED MODEL AT 100 Hz:\n');

fprintf('Magnitude: %.4f V/V\n',mag_model);
fprintf('Magnitude: %.3f dB\n',20*log10(mag_model));
fprintf('Phase: %.3f degrees\n',rad2deg(phi_model));

fprintf('\nREQUIRED AMPLIFIER GAIN:\n');

fprintf('Using measured plant: %.4f V/V\n',A_measured);
fprintf('Using measured plant: %.3f dB\n',20*log10(A_measured));

fprintf('Using identified model: %.4f V/V\n',A_model);
fprintf('Using identified model: %.3f dB\n',20*log10(A_model));

fprintf('\nCHECKED NOISE ATTENUATION AT 100 Hz:\n');

fprintf('Using measured plant: %.3f dB\n', ...
    noise_attenuation_measured);

fprintf('Using identified model: %.3f dB\n', ...
    noise_attenuation_model);

% Provisional gain estimate
A_initial = A_measured;

fprintf('\nInitial gain estimate from measured data: %.4f V/V\n', ...
    A_initial);

%% =========================================================
%  20. FINAL SUMMARY
% ==========================================================

fprintf('\n============================================\n');
fprintf('FINAL SUMMARY\n');
fprintf('============================================\n');

fprintf('Selected plant: %d poles, %d zeros\n', ...
    selected_np,selected_nz);

fprintf('Plant stability: %d\n',isstable(G_selected));

fprintf('Phase fitting RMSE: %.3f degrees\n',phase_RMSE);

fprintf('Initial amplifier gain estimate: %.4f V/V\n',A_initial);

fprintf('\nTop cross-validation candidates:\n');
disp(Results(1:number_to_plot,:));

fprintf('\nSelected transfer function:\n');
G_selected

fprintf('\nIdentification and initial gain calculation completed.\n');
