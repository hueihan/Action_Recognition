function varargout = hjpkg_cns_type_s1(method, varargin)

[varargout{1 : nargout}] = feval(['method_' method], varargin{:});

return;

%***********************************************************************************************************************

function p = method_props

p.abstract = true;
p.methods  = {'addlayer', 'initlayer'};

return;

%***********************************************************************************************************************

function d = method_fields

d.fVals = {'lt', 'dims', {2 1 2 1}, 'dparts', {2 1 1 2}, 'dnames', {'t' 'y' 'x' 'f'}};
d.ndir = {'lc','int'};
d.tfCount = {'lc','int'};
d.thres = {'lc'};
return;

%***********************************************************************************************************************

function m = method_addlayer(m, z, p)

pz = 1;

if m.layers{pz}.size{1} ~= 1
    error('z=%u: previous layer must have only a single feature', z);
end

m.layers{z}.pz = pz;

m.layers{z}.rfCount = p.rfCount;
m.layers{z}.tfCount = p.tfCount;
m.layers{z}.fCount  = p.fCount;
m.layers{z}.fParams = p.fParams;
m.layers{z}.ndir    = p.ndir;
m.layers{z}.thres   = p.thres;

m.layers{z}.size{1} = p.fCount;
m = cns_mapdim(m, z, 2, 'int', pz, p.tfCount, p.rfStep);
m = cns_mapdim(m, z, 3, 'int', pz, p.rfCount, p.rfStep);
m = cns_mapdim(m, z, 4, 'int', pz, p.rfCount, p.rfStep);

return;

%***********************************************************************************************************************

function m = method_initlayer(m, z)

c = m.layers{z};

 
  
switch c.fParams{1}
  
  case 'gabor', c.fVals = GenerateGabor( c.fParams{2},c.fParams{3}, c.ndir, c.tfCount,c.rfCount);

  otherwise   , error('invalid filter type');
end



m.layers{z} = c;

return;

%***********************************************************************************************************************


function fVals = GenerateGabor(symmetry,speed,ndir,Tsiz,Ssiz)

if Tsiz==1
  nfs = length(symmetry)*ndir
    % generate 3D gabor
  fVals = zeros(1,Ssiz,Ssiz,nfs);
  dirs = linspace(0,pi,ndir+1);
  
  f = 1;
    for ism = 1:length(symmetry)
      for j = 1:ndir
	s1units = GaborKernel2d_crop_expt(dirs(j),Ssiz,symmetry(ism));  
	fVals(1,:,:,f) = s1units;	  
	f = f+1;
      end  
    end 
 

else
  nfs = length(speed)*length(symmetry)*ndir;
  % generate 3D gabor
  fVals = zeros(Tsiz,Ssiz,Ssiz,nfs);
  dirs = linspace(0,2*pi,ndir+1);
  
  f = 1;
  for ispd = 1:length(speed)
    for ism = 1:length(symmetry)
      for j = 1:ndir
	s1units = GaborKernel3d_crop_expt(speed(ispd),dirs(j),Ssiz,Tsiz,symmetry(ism));  
	fVals(:,:,:,f) = squeeze(shiftdim(s1units,2));	  
	f = f+1;
      end  
    end 
  end
end
return;



 
 
  
