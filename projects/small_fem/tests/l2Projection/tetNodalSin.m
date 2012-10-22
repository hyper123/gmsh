close all;
clear all;

%% l2 [Order][Mesh]

%% Sin(10x) + Sin(10y) + Sin(10z)
l2 = ...
    [...
        +1.229975e+00 , +7.127187e-01 , +2.574109e-01 ; ...
        +1.121877e+00 , +2.585136e-01 , +5.192648e-02 ; ...
        +7.401852e-01 , +1.065770e-01 , +7.683434e-03 ; ...
        +3.575730e-01 , +2.097726e-02 , +1.035315e-03 ; ...
        +3.193905e-01 , +6.017459e-03 , +9.351989e-05 ; ...
        +2.918225e-01 , +1.379404e-03 , +1.061236e-05 ; ...
        +5.998994e-02 , +2.352694e-04 , +6.703191e-07 ; ...
        +5.486575e-02 , +5.515433e-05 , +7.893316e-08 ; ...
    ];

%l2 = ...
%    [...
%        +1.229975e+00 , +7.127187e-01 , +2.574109e-01 , +6.358030e-02 , +1.3980%86e-02 ; ...
%        +1.121877e+00 , +2.585136e-01 , +5.192648e-02 , +8.839964e-03 , +1.3733%43e-03 ; ...
%    ];


h = [1, 1/2, 1/4]% 1/8, 1/16];
p = [1:8];

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
loglog(1./h, l2, '-*');
grid;
