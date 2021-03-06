close all;
clear all;

h = [1, 0.5, 0.25, 0.125, 0.0625];
p = [1:4];

l2 = ...
    [...
        +1.018273e+00 , +7.339103e-01 , +1.986698e-01 , +5.400421e-02 , +1.330949e-02 ; ...
        +8.820920e-01 , +2.701048e-01 , +5.313349e-02 , +7.105430e-03 , +9.375183e-04 ; ...
        +8.445680e-01 , +1.141926e-01 , +6.927613e-03 , +4.322385e-04 , +2.521169e-05 ; ...
        +2.971150e-01 , +2.314765e-02 , +1.089427e-03 , +3.711183e-05 , +1.170054e-06 ; ...
    ];

P = size(p, 2);
H = size(h, 2);

delta = zeros(P, H - 1);

for i = 1:H-1
  delta(:, i) = ...
    (log10(l2(:, i + 1)) - log10(l2(:, i))) / ...
    (log10(1/h(i + 1))   - log10(1/h(i)));
end

delta

figure;
loglog(1./h, l2', '-*');
grid;
title('quad: Edge');

xlabel('1/h [-]');
ylabel('L2 Error [-]');
