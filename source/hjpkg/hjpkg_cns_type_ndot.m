function varargout = hjpkg_cns_type_ndot(method, varargin)

[varargout{1 : nargout}] = feval(['method_' method], varargin{:});

return;

%***********************************************************************************************************************

function p = method_props

p.super   = 'ssvalid';
p.methods = {'addlayer'};

p.blockYSize = 16;
p.blockXSize = 20;

return;

%***********************************************************************************************************************

function d = method_fields

d.exponent = {'lc'};
d.thres = {'lc'};
return;

%***********************************************************************************************************************

function m = method_addlayer(m, z, p)

m = cns_super(m, z, p);

m.layers{z}.exponent = p.exponent;
m.layers{z}.thres = p.thres;


return;
