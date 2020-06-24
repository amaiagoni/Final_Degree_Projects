function string = coefficientsOctave(central_frequency)

o = octaveFilter(central_frequency,'1 octave','SampleRate',48000);

c = coeffs(o);

b = conv(c.Stage1.Numerator,c.Stage2.Numerator);
a = conv(c.Stage1.Denominator,c.Stage2.Denominator);
v = orderCoefficients(b, a);
freqz(b, a);

string = "const float32_t coefficientsOctave";
string = [string central_frequency];
string = join(string, "");
string = [string "Hz[NUM_TAPS] = {"];
string = join(string, "");
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