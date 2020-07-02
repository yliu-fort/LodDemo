clear; clc; close all;

%% Test rayleigh scattering and ground transmittence
fInnerRadius = 1.0;
fOuterRadius = 1.012;
fRayleighScaleHeight = 0.11;
fMieScaleHeight = 0.05;

V = makeOpticalBuffer(fInnerRadius,fOuterRadius,fRayleighScaleHeight,fMieScaleHeight);
fRayleighDensityRatio = getOpticalInterpolant(V,'RayleighDensity');
fRayleighDepth = getOpticalInterpolant(V,'RayleighDepth');

%%
DELTA = 1e-6;
m_nChannels = 4;
nSize = 256;
nSamples = 4096;
fScale = 1.0 / (fOuterRadius - fInnerRadius);

ESun = 1.5;
Kr = 0.0025;
klambda = [0.7 0.546 0.435];
InvWavelength4 = 1.0./ klambda.^4;
v3Kr4PIlambda = 4*pi*Kr*InvWavelength4;
fKrESunlambda = Kr * ESun * InvWavelength4;
groundAlbedo = [0.0 0 0.04];

% Assume sun/view angle = 0
fCos = 1.0;
fAngle = acos(fCos);
vRay = [sin(fAngle) cos(fAngle) 0];	% Ray pointing to the viewpoint

fRayleighDensityRatioRef = @(h) exp(-(h - fInnerRadius) * fScale / fRayleighScaleHeight);
fRayleighDepthRe = @(h)(fRayleighScaleHeight*(exp(-(h - fInnerRadius) * fScale / fRayleighScaleHeight) - exp(-1/fRayleighScaleHeight)));

%% Compute transmittence between view and surface
fCameraHeights = linspace(fInnerRadius, fOuterRadius, 50);

for i = 1:numel(fCameraHeights)
fCameraHeight = fCameraHeights(i);
vCameraPos = [0 fCameraHeight 0];

% Now loop through the sample rays
fSamplePoint = linspace(fInnerRadius, fCameraHeight, nSamples).';
fFar = fCameraHeight - fInnerRadius;
fSampleLength = fFar / nSamples;
fScaledLength = fSampleLength * fScale;

fHeight = fSamplePoint;
Rho = fRayleighDensityRatioRef(fHeight);

fScatter = fRayleighDepthRe(fHeight) ...
    + fRayleighDepthRe(fHeight) ...
    - fRayleighDepthRe(fCameraHeight);

v3Attenuate = exp(-fScatter .* v3Kr4PIlambda);
v3FrontColor = v3Attenuate .* (Rho .* fScaledLength);

inScatterRef(i,:) = fKrESunlambda .* sum(v3FrontColor);
transmittenceRef(i,:) = v3Attenuate(1,:);

end

radianceRef = inScatterRef + groundAlbedo .* transmittenceRef * ESun;
radianceRef = tonemapping(radianceRef);

%% Compute transmittence between view and surface
fCameraHeights = linspace(fInnerRadius, fOuterRadius, 50);

for i = 1:numel(fCameraHeights)
fCameraHeight = fCameraHeights(i);
vCameraPos = [0 fCameraHeight 0];

% Now loop through the sample rays
fSamplePoint = linspace(fInnerRadius, fCameraHeight, nSamples).';
fFar = fCameraHeight - fInnerRadius;
fSampleLength = fFar / nSamples;
fScaledLength = fSampleLength * fScale;

fHeight = fSamplePoint;
Rho = fRayleighDensityRatio.interp(fHeight,fCos);

fScatter = fRayleighDepth.interp(fHeight,fCos) ...
    + fRayleighDepth.interp(fHeight,fCos) ...
    - fRayleighDepth.interp(fCameraHeight,fCos);

v3Attenuate = exp(-fScatter .* v3Kr4PIlambda);
v3FrontColor = v3Attenuate .* (Rho .* fScaledLength);

inScatter(i,:) = fKrESunlambda .* sum(v3FrontColor);
transmittence(i,:) = v3Attenuate(1,:);

end

radiance = inScatter + groundAlbedo .* transmittence * ESun;
radiance = tonemapping(radiance);

%% Display
subplot(2,1,1)
for i = 1:numel(fCameraHeights)
x = [0 1 1 0] + i - 1 ; y = [0 0 1 1] ;
fill(x,y,radianceRef(i,:));
hold on
drawnow
end
title('Reference')
subplot(2,1,2)
for i = 1:numel(fCameraHeights)
x = [0 1 1 0] + i - 1 ; y = [0 0 1 1] ;
fill(x,y,radiance(i,:));
hold on
drawnow
end
title('Lookup Table')

%% error
Res = norm(radiance-radianceRef)

%%
function radiance = tonemapping(radiance)
radiance = (1 - exp(-radiance*10)).^(1.0 / 2.2);
end