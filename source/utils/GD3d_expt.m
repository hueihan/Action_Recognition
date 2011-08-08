
 function Inseparable_filters = GD3d_expt(speeds,dirs,Ssiz,Tsiz)
 
 
   ndir = length(dirs);
   dirs = [dirs(:) repmat(speeds,[ndir 1])];
   speeds = repmat(speeds,[ndir 1]);
   rs = 6;
   filterS = Generate_derivativeGaussian(Ssiz,rs);
   filterT = Generate_derivativeGaussian(Tsiz,rs);
 
   
   
   % get separable filters
   Separable_filters = zeros(Ssiz*Ssiz*Tsiz,10);
   icount = 1;
   for d1 = 0:3
     for d2 = 0:(3-d1)
       d3 = 3 - d1 - d2;
       Separable_filters(:,icount) = reshape(reshape(filterS(:,d1+1)*filterS(:,d2+1)',[],1)*filterT(:,d3+1)',[],1);
       icount = icount + 1;    
     end
   end

   % get linear weights for separable filters

   
    fac = [1 1 2 6];
    W = zeros(10,ndir);    
    for idir = 1:ndir
      ang_xy_z = atan2(speeds(idir),1);
      ang_x_y = dirs(idir);
      
      x = cos(ang_xy_z).*sin(ang_x_y);
      y = cos(ang_xy_z).*cos(ang_x_y);
      z = sin(ang_xy_z);
      
      icount = 1;
      for d1 = 0:3
	for d2 = 0:(3-d1)
	  d3 = 3-d1-d2;
	  const = fac(4)/(fac(d1+1)*fac(d2+1)*fac(d3+1));
	  W(icount,idir) = const* x^d1 * y^d2 * z^d3;
	  icount = icount + 1;
	end
      end
    end

    % interpolate separable filters into inseparable ones
    Inseparable_filters = Separable_filters*W; % S*S*T by ndir
    % normalize to have L1 norm of 1
    Inseparable_filters =  Inseparable_filters./repmat(sum(abs( Inseparable_filters)),[size(Inseparable_filters,1),1]);
    % reshape to three-dimensional
    Inseparable_filters = reshape( Inseparable_filters,[Ssiz Ssiz Tsiz ndir]);
    
  
  
