function semilogyRGB(x,spectrum)
semilogy(x, spectrum(:,1),'r',...
     x, spectrum(:,2),'g',...
     x, spectrum(:,3),'b')
end