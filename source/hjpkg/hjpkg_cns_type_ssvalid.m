function varargout = hjpkg_cns_type_ssvalid(method, varargin)

[varargout{1 : nargout}] = feval(['method_' method], varargin{:});

return;

%***********************************************************************************************************************

function p = method_props

p.abstract = true;
p.methods  = {'addlayer', 'initlayer'};

return;

%***********************************************************************************************************************

function d = method_fields

d.rfCountMin = {'lc', 'int'};
d.rfSpace    = {'lc', 'int'}; 
d.fVals      = {'lt', 'dims', {1 2}, 'dparts', {1 1}, 'dnames', {'p' 'nf'}};
d.fMap2      = {'lt', 'dims', {1 2}, 'dparts', {1 1}, 'dnames', {'p' 'nf'}, 'int'};
d.fSizes     = {'la', 'dims', {1 2}, 'dparts', {1 1}, 'dnames', {'nf' ''}, 'int'}; % la arrays

return;

%***********************************************************************************************************************

function m = method_addlayer(m, z, p)


pz = z-p.pzStep;

if any(p.fSizes < 1), error('invalid fSizes value'); end
if mod(p.rfSpace, 2) == 1
    temp = (mod(p.fSizes, 2) == 1);
    if any(temp) && ~all(temp)
        error('when rfSpace is odd, fSizes must be all even or all odd');
    end
end

if isempty(p.fSizes)
    if isempty(p.fVals), p.fVals = reshape(p.fVals,    0, 0); end
    if isempty(p.fMap ), p.fMap  = reshape(p.fMap , 3, 0, 0); end
end

if size(p.fVals, 2) ~= numel(p.fSizes)
    error('dimension 2 of fVals must match the length of fSizes');
end

if size(p.fMap, 1) ~= 3
    error('dimension 1 of fMap must have size 3');
end
if size(p.fMap, 2) ~= size(p.fVals, 1)
    error('dimension 2 of fMap must match dimension 1 of fVals');
end
if size(p.fMap, 3) ~= size(p.fVals, 2)
    error('dimension 3 of fMap must match dimension 2 of fVals');
end

if m.layers{pz}.size{1} > 65536
    error('previous layer can have at most 65536 features');
end
if any(p.fSizes > 16)
    error('maximum fSizes value cannot exceed 16');
end

m.layers{z}.pz = pz;

m.layers{z}.rfCountMin = p.rfCountMin;
m.layers{z}.rfSpace    = p.rfSpace;
m.layers{z}.fVals      = p.fVals;
m.layers{z}.fMap       = p.fMap;
m.layers{z}.fSizes     = p.fSizes(:);

rfWidthMin = 1 + (p.rfCountMin - 1) * p.rfSpace;

m.layers{z}.size{1} = numel(p.fSizes);
m = cns_mapdim(m, z, 2, 'copy', pz);
m = cns_mapdim(m, z, 3, 'int' , pz, rfWidthMin, p.rfStep);
m = cns_mapdim(m, z, 4, 'int' , pz, rfWidthMin, p.rfStep);

return;

%***********************************************************************************************************************

function m = method_initlayer(m, z)

c = m.layers{z};

fs = shiftdim(c.fMap(1, :, :)) - 1;
ys = shiftdim(c.fMap(2, :, :)) - 1;
xs = shiftdim(c.fMap(3, :, :)) - 1;

cs = sum(fs >= 0, 1);

is = fs;
is = is + ys * 65536;
is = is + xs * 65536 * 16;

c.fMap2 = [cs; is];

m.layers{z} = c;

return;
