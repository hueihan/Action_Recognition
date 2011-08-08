function varargout = hjpkg_cns(method, varargin)

[varargout{1 : nargout}] = feval(['method_' method], varargin{:});

return;

%***********************************************************************************************************************

function p = method_props

p.methods = {'new', 'add', 'init'};

return;

%***********************************************************************************************************************

function d = method_fields

d = struct;

return;

%***********************************************************************************************************************

function m = method_new(m)

m.layers = {};

return;

%***********************************************************************************************************************

function m = method_add(m, p)

z = numel(m.layers) + 1;

m.layers{z}.name = p.name;
m.layers{z}.type = p.type;

m = cns_type('addlayer', m, z, p);

return;

%***********************************************************************************************************************

function m = method_init(m)

m = cns_setrunorder(m, 'field', 'pz');

%m = cns_setstepnos(m, 'field', 'pz');

%m.independent = true;

for z = 1 : numel(m.layers)
    m = cns_type('initlayer', m, z);
end

return;
