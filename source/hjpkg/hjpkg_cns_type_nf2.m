function varargout = hjpkg_cns_type_nf2(method, varargin)

[varargout{1 : nargout}] = feval(['method_' method], varargin{:});

return;

%***********************************************************************************************************************

function p = method_props

p.methods = {'addlayer'};

p.blockYSize = 16;
p.blockXSize = 16;

return;

%***********************************************************************************************************************

function d = method_fields

d.sz = {'lp', 'type', 'base'};

return;

%***********************************************************************************************************************

function m = method_addlayer(m, z, p)

pz = z - p.pzStep*2;
sz = z - p.pzStep;

m.layers{z}.pz = pz;
m.layers{z}.sz = sz;

m.layers{z}.size{1} = m.layers{pz}.size{1};
m = cns_mapdim(m, z, 2, 'copy', pz);
m = cns_mapdim(m, z, 3, 'copy', pz);
m = cns_mapdim(m, z, 4, 'copy', pz);

return;
