function out = makeOpticalBuffer(fInnerRadius,...
    fOuterRadius,...
    fRayleighScaleHeight,...
    fMieScaleHeight)
DELTA = 1e-6;
m_nChannels = 4;
nSize = 256;
nSamples = 1024;
fScale = 1.0 / (fOuterRadius - fInnerRadius);
nIndex = 1;

m_pBuffer = zeros(nSize,nSize,m_nChannels);
%Init(nSize, nSize, 1, 4, GL_RGBA, GL_FLOAT);
nfAngles = acos(1.0 - ((0:(nSize-1)) + (0:(nSize-1))) / nSize);
nfHeights = DELTA + fInnerRadius + ((fOuterRadius - fInnerRadius) * (0:(nSize-1))) / nSize;
for nAngle = 0:(nSize-1)
    
    % As the y tex coord goes from 0 to 1, the angle goes from 0 to 180 degrees
    fCos = 1.0 - (nAngle+nAngle) / nSize;
    fAngle = acos(fCos);
    vRay = [sin(fAngle) cos(fAngle) 0];	% Ray pointing to the viewpoint
    
    for nHeight = 0:(nSize-1)
        
        % As the x tex coord goes from 0 to 1, the height goes from the bottom of the atmosphere to the top
        fHeight = DELTA + fInnerRadius + ((fOuterRadius - fInnerRadius) * nHeight) / nSize;
        vPos = [0 fHeight 0];				% The position of the camera
        
        % If the ray from vPos heading in the vRay direction intersects the inner radius (i.e. the planet), then this spot is not visible from the viewpoint
        B = 2.0 * dot(vPos, vRay);
        Bsq = B * B;
        Cpart = dot(vPos, vPos);
        C = Cpart - fInnerRadius*fInnerRadius;
        fDet = Bsq - 4.0 * C;
        bVisible = (fDet < 0 || ((0.5 * (-B - sqrt(fDet)) <= 0) && (0.5 * (-B + sqrt(fDet)) <= 0)));
        
        if(bVisible)
            
            fRayleighDensityRatio = exp(-(fHeight - fInnerRadius) * fScale / fRayleighScaleHeight);
            fMieDensityRatio = exp(-(fHeight - fInnerRadius) * fScale / fMieScaleHeight);
            
        else
            
            % Smooth the transition from light to shadow (it is a soft shadow after all)
            fRayleighDensityRatio = m_pBuffer(nHeight+1, nAngle, 1) * 0.5;
            fMieDensityRatio = m_pBuffer(nHeight+1, nAngle, 3) * 0.5;
        end
        
        % Determine where the ray intersects the outer radius (the top of the atmosphere)
        % This is the end of our ray for determining the optical depth (vPos is the start)
        C = Cpart - fOuterRadius*fOuterRadius;
        fDet = Bsq - 4.0 * C;
        fFar = 0.5 * (-B + sqrt(fDet));
        
        % Next determine the length of each sample, scale the sample ray, and make sure position checks are at the center of a sample ray
        fSampleLength = fFar / nSamples;
        fScaledLength = fSampleLength * fScale;
        vSampleRay = vRay * fSampleLength;
        vPos = vPos + vSampleRay * 0.5;
        
        % Iterate through the samples to sum up the optical depth for the distance the ray travels through the atmosphere
        fRayleighDepth = 0;
        fMieDepth = 0;
        for i=0:(nSamples-1)
            fHeight = norm(vPos);
            fAltitude = (fHeight - fInnerRadius) * fScale;
            min_h = 1./cos(max(fAngle-pi/2, 0))-1;
            if(fAltitude >= min_h)
                fRayleighDepth = fRayleighDepth + exp(-fAltitude / fRayleighScaleHeight);
                fMieDepth = fMieDepth + exp(-fAltitude / fMieScaleHeight);
            end
            vPos = vPos + vSampleRay;
        end
        
        % Multiply the sums by the length the ray traveled
        fRayleighDepth = fRayleighDepth * fScaledLength;
        fMieDepth = fMieDepth * fScaledLength;
        
        
        % Store the results for Rayleigh to the light source, Rayleigh to the camera, Mie to the light source, and Mie to the camera
        m_pBuffer(nHeight+1, nAngle+1, 1) = fRayleighDensityRatio;
        m_pBuffer(nHeight+1, nAngle+1, 2) = fRayleighDepth;
        m_pBuffer(nHeight+1, nAngle+1, 3) = fMieDensityRatio;
        m_pBuffer(nHeight+1, nAngle+1, 4) = fMieDepth;
        
    end
    
end

out.buffer = m_pBuffer;
out.s = nfHeights; % fHeight
out.t = (0:(nSize-1)) / nSize; % fCos



if false
    %% Visualize
    figure(1)
    surf( nfAngles*180/pi, nfHeights, squeeze(m_pBuffer(:,:,1)) );
    xlabel('Angle'), ylabel('Height')
    title('Rayleigh Density Ratio')
    view(2)
    figure(2)
    surf( nfAngles*180/pi, nfHeights, squeeze(m_pBuffer(:,:,2)) );
    xlabel('Angle'), ylabel('Height')
    title('Rayleigh Optical Depth')
    view(2)
end

end