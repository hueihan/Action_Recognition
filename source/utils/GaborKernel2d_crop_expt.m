function result = GaborKernel2d_crop_expt(theta,R,xi)
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
lambda = lambda0;
sigma = lambda*slratio;


%do meshgrid to create the three dimensional matrix
[x,y] = meshgrid(-n:n,-n:n);

%theta is set to pi - theta for conventional purposes (for ex, a value of 0 would mean rightward motion)
theta = pi - theta; 

%rotation matrix
xt = x * cos(theta) + y * sin(theta);
yt = -x * sin(theta) + y * cos(theta);

%precompute few constants

%components of the cosine factor
xp = xt;
f1 = 2*pi/lambda;  
 
b = 1 / (2*sigma*sigma);  
gamma2 = gamma*gamma;

a = gamma / (2*sigma*sigma*pi);  %normalization for spatial gaussian

%compute the kernel itself
result = a*exp(-b*( xt.*xt +gamma2*yt.*yt)).* cos(f1*xp +  xi);
result = result - mean(result(:));
result = result./sqrt(sum(abs(result(:).^2)));

%result = result./sum(abs(result(:)));
return;

