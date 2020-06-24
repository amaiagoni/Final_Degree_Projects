function string = coefficientsAWeighting()

o = weightingFilter('A-weighting','SampleRate',48000);

c = coeffs(o);

[b, a] = sos2tf(c.SOSMatrix, c.ScaleValues);
scaledFilter = tf2sos(b, a);
% freqz(b, a);

[rows,columns]=size(scaledFilter);
v=[];
for n=1:rows
    v=[v scaledFilter(n,1) scaledFilter(n,2) scaledFilter(n,3) -scaledFilter(n,5) -scaledFilter(n,6)];
end

string = "const float32_t coefficientsAWeighting[NUM_TAPS] = {";
for n=1:length(v)
    str = [string v(n)];
    if (n == 1) 
        delimiter = "";
    else
        delimiter = ",";
    end
    string = join(str,delimiter);
end
str = [string "};"];
string = join(str);