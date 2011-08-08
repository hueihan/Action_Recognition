function varargout = hjpkg_cns_type_base(method, varargin)

[varargout{1 : nargout}] = feval(['method_' method], varargin{:});

return;

%***********************************************************************************************************************

function p = method_props

p.abstract = true;
p.methods  = {'initlayer'};

p.dims   = {1 2 1 2};
p.dparts = {2 2 1 1};
p.dnames = {'f' 't' 'y' 'x'};
p.dmap   = [false true true true];

return;

%***********************************************************************************************************************

function d = method_fields

d.pz  = {'lp', 'type', 'base'};
d.val = {'cv', 'dflt', 0};

return;

%***********************************************************************************************************************

function m = method_initlayer(m, z)

return;
