l = 1;
d = 2;

L  = l / 2;
cl = l / d;

Point(1) = {+L, -L, 0, cl};
Point(2) = {+L, +L, 0, cl};
Point(3) = {-L, +L, 0, cl};
Point(4) = {-L, -L, 0, cl};

Point(5) = {0, +L, 0, cl};
Point(6) = {0, -L, 0, cl};

Line(1) = {1, 2};
Line(2) = {2, 5};
Line(3) = {5, 3};
Line(4) = {3, 4};
Line(5) = {4, 6};
Line(6) = {6, 1};

Line(7) = {5, 6};

Line Loop(8) = {4, 5, -7, 3};
Line Loop(9) = {1, 2, 7, 6};

Plane Surface(10) = {8};
Plane Surface(11) = {9};

Physical Line(6) = {1, 2, 3, 4, 5, 6, 7};

Physical Surface(7) = {10, 11};

Transfinite Line {6, 1, 2, 7} = 3 Using Progression 1;
Transfinite Surface {11};
Recombine Surface {11};
