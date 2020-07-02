function F = getOpticalInterpolant(in, index)
if( strcmp(index,'RayleighDensity') || strcmp(index,'rrho') )
    F.interpolant = RayleighDensityInterpolant(in);
end
if( strcmp(index,'RayleighDepth') || strcmp(index,'rd') )
    F.interpolant = RayleighDepthInterpolant(in);
end

F.interp = @(x,y)  interpOptical(x,y,F.interpolant);

end

% F(height, theta)
function [F, help] = RayleighDensityInterpolant(in)
[S,T] =ndgrid(in.s,in.t);
V = squeeze(in.buffer(:,:,1));
F = griddedInterpolant(S,T,V);
F.Method = 'linear';
F.ExtrapolationMethod = 'nearest';
help = 's {Rin, Rout}, t {0, 1} - theta {0, 180}';
end

% F(height, theta)
function [F, help] = RayleighDepthInterpolant(in)
[S,T] =ndgrid(in.s,in.t);
V = squeeze(in.buffer(:,:,2));
F = griddedInterpolant(S,T,V);
F.Method = 'linear';
F.ExtrapolationMethod = 'nearest';
help = 's {Rin, Rout}, t {0, 1} - theta {0, 180}';
end

function [s,t] = convertUV(height, fCos)
s = height;
t = (1.0 - fCos)/2;
end

function Vq = interpOptical(height, fCos, in)
[s,t] = convertUV(height, fCos);
if(numel(s) == 1 || numel(t) == 1 || all(size(s) == size(t)))
    pad = 0*s + 0*t;
    func = @(x,y)(in(x,y));
    Vq = arrayfun(func,s+pad,t+pad);
else
    [S,T] = meshgrid(s,t);
    Vq = in(S,T);
end
end