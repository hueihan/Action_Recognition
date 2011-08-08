function result = GaborKernel3d_crop_expt(v,theta,R,nt,xi)
if nargin==4
  xi = 0;
end
bandwidth = 1.5;
slratio = (1/pi) * sqrt( (log(2)/2) ) * ( (2^bandwidth + 1) / (2^bandwidth - 1) );

lambda0 = (R-1)/4/sqrt(1.04)/slratio;

n = floor((R-1)/2);

opt = 1;
%spatiotemporal period
%lambda0 = 2;

%aspect ratio 
gamma = 0.5;


%mean of the temporal Gaussian

t0 = 0;


%calculate the spatial wavelength using the following relation
lambda = lambda0*sqrt(1+v^2);


sigma = lambda*slratio;

%determine the matrix size
nt = floor(nt/2);

%standard deviation of the temporal Gaussian
tau = 2.75*nt/4;

%do meshgrid to create the three dimensional matrix
[x,y,t] = meshgrid(-n:n,-n:n,-nt:nt);

%theta is set to pi - theta for conventional purposes (for ex, a value of 0 would mean rightward motion)
theta = pi - theta; 

%rotation matrix
xt = x * cos(theta) + y * sin(theta);
yt = -x * sin(theta) + y * cos(theta);
tt = t;

%Fix vf depending on whether the kernel has a stationary envelope or moving envelope
if(opt == 0)
    vf = 0;
else 
    vf = v;
end

%precompute few constants

%components of the cosine factor
xp = xt + v*t;
f1 = 2*pi/lambda;  
 
b = 1 / (2*sigma*sigma);  
tau2 = 1/(2*tau*tau);
gamma2 = gamma*gamma;

a = gamma / (2*sigma*sigma*pi);  %normalization for spatial gaussian
t1 = 1/(sqrt(2*pi)*tau);        %normalization for temporal gaussian 

%compute the kernel itself
result = (a*exp(-b*( (xt + vf*(tt)).*(xt + vf*(tt)) +gamma2* (yt.*yt)))).* cos(f1*xp +  xi).*(t1*(exp(-tau2*((tt - t0).*(tt-t0)))));
result = result - mean(result(:));
result = result./sqrt(sum(abs(result(:).^2)));

%result = result./sum(abs(result(:)));
return;

