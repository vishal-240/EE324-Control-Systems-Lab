% 1. Define State-Space Matrices
A = [0, 0, 1, 0;
     0, 0, 0, 1;
     0, 71.33, -0.92, 0;
     0, 66.24, -0.32, 0];

B = [0;
     0;
     32.75;
     11.41];

C = eye(4);
D = zeros(4,1);


% 2. Define Weighing Matrices (Q and R)
% State order: [theta, alpha, theta_dot, alpha_dot]

% Q penalises state deviation: theta, alpha, theta_dot, alpha_dot.
% The large weight on alpha keeps the pendulum tightly upright.
% R penalises control effort (motor voltage).
Q = diag([100 400 10 1]);
R = 1;

% 3. Compute LQR Feedback Gain Matrix K
K = lqr(A, B, Q, R);

% Display the calculated gains
disp('Optimal Feedback Gain Matrix K:');
disp(K);



% Closed-loop poles of A - B*K; all should have negative real parts
cl_poles = eig(A - B*K)
