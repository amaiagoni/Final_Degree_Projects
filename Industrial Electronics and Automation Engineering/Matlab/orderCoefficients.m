function [v]=orderCoefficients(b,a)
[sos,g]=tf2sos(b,a);
[rows,columns]=size(sos);
g=g^(1/rows);
v=[];
for n=1:rows
    v=[v g*sos(n,1) g*sos(n,2) g*sos(n,3) -sos(n,5) -sos(n,6)];
end