
% function filter = Generate_derivativeGaussian(siz,rs)
% INPUTS
%     siz    - the size of the 1D filter
%     rs     - the variance of the underlying Gaussian derivative filters
%
% OUTPUTS
%    filter  - a 1D array containing a3rd derivative Gaussian filter
%
% DATE
%     06, June 2008.


function filter = Generate_derivativeGaussian(siz,rs)

 x=[-floor(siz/2):1:floor(siz/2)];
 sigma= siz/rs;
 a = 2*sigma^2;
 b = sqrt(2*pi*sigma^2);
 res = 1/b*exp(-x.^2/a);
 res1= -2.*x./b./a.*exp(-x.^2./a);
 res2 = -2./a./b.*exp(-x.^2/a)+4.*x.^2./(a.^2)./b.*exp(-x.^2./a);
 res3 = 12.*x./(a.^2)./b.*exp(-x.^2./a)-8.*x.^3./(a.^3)./b.*exp(-x.^2./a);
 sigma = 1;
 
 filter = [res(:) res1(:).*sigma res2(:).*sigma^2 res3(:).*sigma^3];
 filter = filter./repmat(sum(abs(filter)),[size(filter,1) 1]);
