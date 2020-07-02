clear; clc; close all;

%% Aerial perspective
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
%nSize = 256;
nSamples = 64;
fScale = 1.0 / (fOuterRadius - fInnerRadius);

ESun = 1.5;
Kr = 0.0025;
klambda = [0.7 0.546 0.435];
InvWavelength4 = 1.0./ klambda.^4;
v3Kr4PIlambda = 4*pi*Kr*InvWavelength4;
fKrESunlambda = Kr * ESun * InvWavelength4;
groundAlbedo = [0.0 0.0 0.04];

% Assume view angle = 0
v3LightPos = [0 1000 0];
v3LightDir = v3LightPos/norm(v3LightPos);

fCameraAngle = 1.0;
fAngle = acos(fCameraAngle);
v3Ray = [sin(fAngle) cos(fAngle) 0];	% Ray pointing to the viewpoint

fRayleighDensityRatioRef = @(h) exp(-(h - fInnerRadius) * fScale / fRayleighScaleHeight);
fRayleighDepthRe = @(h)(fRayleighScaleHeight*(exp(-(h - fInnerRadius) * fScale / fRayleighScaleHeight) - exp(-1/fRayleighScaleHeight)));

%% Compute transmittence between view and surface
fCameraHeight = interp1([0 1],[fInnerRadius fOuterRadius],0.8);
fCameraAngles = linspace(-1,1,512).'; % -1: look downwards, 1 look upwards
v3CameraPos = [0 fCameraHeight 0];
%fLightAngle = 1.0;

for i = 1:numel(fCameraAngles)
    fCameraAngle = fCameraAngles(i);
    fAngle = acos(fCameraAngle);
    v3Ray = [sin(fAngle) cos(fAngle) 0];	% Ray pointing to the surface
    
    %! compute intersection to the outersphere when out of atmosphere
    v3Start = v3CameraPos;
    
    % Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)
    B = 2.0 * dot(v3CameraPos, v3Ray);
    C = fCameraHeight^2 - fInnerRadius^2;
    fDet = max(0.0, B*B - 4.0 * C);
    fNear = 0.5 * (-B - sqrt(fDet));
    hitGround = (B*B - 4.0 * C) >= 0 && fNear >= 0;
    if(~hitGround)
        C = fCameraHeight^2 - fOuterRadius^2;
        fDet = max(0.0, B*B - 4.0 * C);
        fNear = 0.5 * (-B + sqrt(fDet));
    end
    
    hitGrounds(i,1) = hitGround;
    fNears(i,1) = (fNear);
    
    % Calculate the ray's starting position, then calculate its scattering offset
    v3Pos = v3CameraPos + v3Ray * fNear;
    fFar = fNear;
    
    %scatter3(v3Pos(1),v3Pos(2),v3Pos(3),[],'.'), hold on
    
    % Now loop through the sample rays
    fSampleLength = fFar / nSamples;
    v3SampleRay = v3Ray * fSampleLength;
    fScaledLength = fSampleLength * fScale;
    v3SamplePoint = v3Start + ((1:nSamples).'-0.5).*v3SampleRay;
    
    fHeight = vecnorm(v3SamplePoint.').';
    fLightAngle = dot(v3LightDir+0*v3SamplePoint, v3SamplePoint,2) ./ fHeight;
    if(hitGround)
        fCameraOffset = fRayleighDepth.interp(fCameraHeight, dot(-v3Ray, v3CameraPos) ./ fCameraHeight);
        fCameraAngle = dot(-v3Ray+0*v3SamplePoint, v3SamplePoint,2) ./ fHeight;
    else
        fCameraOffset = fRayleighDepth.interp(fCameraHeight, dot(v3Ray, v3CameraPos) ./ fCameraHeight);
        fCameraAngle = dot(v3Ray+0*v3SamplePoint, v3SamplePoint,2) ./ fHeight;
    end
    fCameraAngleLast(i) = fCameraAngle(end);
    fCameraOffsets(i) = fCameraOffset;
    
    Rho = fRayleighDensityRatio.interp(fHeight,fLightAngle);

    if(hitGround)
        fScatter = fRayleighDepth.interp(fHeight,fLightAngle) ...
            + fRayleighDepth.interp(fHeight,fCameraAngle) ...
            - fCameraOffset;
    else
        fScatter = fRayleighDepth.interp(fHeight,fLightAngle) ...
            - fRayleighDepth.interp(fHeight,fCameraAngle) ...
            + fCameraOffset;
    end
    
    v3Attenuate = exp(-fScatter .* v3Kr4PIlambda);
    v3FrontColor = v3Attenuate .* (Rho .* fScaledLength);
    
    inScatter(i,:) = fKrESunlambda .* sum(v3FrontColor);
    transmittence(i,:) = v3Attenuate(end,:);
    
end

diffuse = max(dot(v3CameraPos, v3LightDir) ./ norm(v3CameraPos),0.0);
radiance = inScatter + hitGrounds .* groundAlbedo .* diffuse .* transmittence * ESun;
radiance = tonemapping(radiance);

%% Display
for i = 1:numel(fCameraAngles)
    x = [0 1 1 0] + i - 1 ; y = [0 0 1 1] ;
    fill(x,y,radiance(i,:),'EdgeColor','None');
    hold on
    drawnow
end
axis tight
title('Lookup Table')

%%
function radiance = tonemapping(radiance)
radiance = (1 - exp(-radiance*10)).^(1.0 / 2.2);
end