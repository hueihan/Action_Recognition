function varargout = hjpkg_cns_type_ri(method, varargin)

[varargout{1 : nargout}] = feval(['method_' method], varargin{:});

return;

%***********************************************************************************************************************

function p = method_props

p.kernel  = false;
p.methods = {'addlayer'};

return;

%***********************************************************************************************************************

function d = method_fields

d = struct;

return;

%***********************************************************************************************************************

function m = method_addlayer(m, z, p)

m.layers{z}.pz = 0;

m.layers{z}.size{1} = p.size(4);
m = cns_mapdim(m, z, 2, 'pixels', p.size(1));
m = cns_mapdim(m, z, 3, 'pixels', p.size(2));
m = cns_mapdim(m, z, 4, 'pixels', p.size(3));

return;
