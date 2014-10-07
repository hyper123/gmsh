%%%%%%%%%%%%%%%%%%%%%%%
%% GetDP vs SmallFEM %%
%% OO2: 4 domains    %%
%%%%%%%%%%%%%%%%%%%%%%%

clear all;
close all;

getdp = ...
    [
        6.426623878829e+01
        4.268678079963e+01
        3.566439763196e+01
        3.221988371428e+01
        3.133792824441e+01
        2.584252656441e+01
        3.285670217894e+00
        1.243757604095e+00
        2.718738552204e-01
        1.446349259717e-01
        9.577169087128e-02
        6.841816302693e-02
        4.616540600836e-02
        2.875090948517e-02
        1.943973132673e-02
        1.176241417862e-02
        4.860720422618e-03
        2.776843896026e-03
        8.596079731461e-04
        4.655050361802e-04
        2.248451731965e-04
        1.119127946177e-04
        5.010305752898e-05
        2.606769094970e-05
        1.260440993963e-05
        7.779532864117e-06
        6.099560466775e-06
        4.366278446378e-06
        1.190569778204e-06
        7.109937447734e-07
        3.552087676550e-07
        2.031545524340e-07
        8.592883718477e-08
        4.000590666625e-08
    ];

smallfem = ...
    [
        4.544309453686e+01
        3.018411469333e+01
        2.521853892191e+01
        2.278289771656e+01
        2.215926159452e+01
        1.827342884849e+01
        2.323323138751e+00
        8.794709913482e-01
        1.922440502029e-01
        1.022724504269e-01
        6.772087032180e-02
        4.837899106643e-02
        3.264390662466e-02
        2.032998996877e-02
        1.374598727770e-02
        8.317298839235e-03
        3.437055842728e-03
        1.963529933306e-03
        6.078360287022e-04
        3.291626233763e-04
        1.589900026430e-04
        7.913454446141e-05
        3.542831305899e-05
        1.843269429076e-05
        8.912680862252e-06
        5.500970000890e-06
        4.313048589758e-06
        3.087432297938e-06
        8.418627137191e-07
        5.027501547427e-07
        2.511714111334e-07
        1.436525411344e-07
        6.076116223135e-08
        2.828860008254e-08
    ];

it = [1:size(smallfem, 1)];

semilogy(it, ...
         getdp,              '-o', 'linewidth', 3, ...
         smallfem,           '-o', 'linewidth', 3, ...
         smallfem * sqrt(2), '*',  'linewidth', 3);
legend({'GetDP', 'SmallFEM', 'SmallFEM * \sqrt{2}'});
title('OO2: 4 domains');
xlabel('Iteration');
ylabel('Residual');

print('oo2.png');