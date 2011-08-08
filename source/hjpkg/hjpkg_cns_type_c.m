function varargout = hjpkg_cns_type_c1(method, varargin)

[varargout{1 : nargout}] = feval(['method_' method], varargin{:});

return;

%***********************************************************************************************************************

function p = method_props

p.abstract = true;
p.methods  = {'addlayer'};

return;

%***********************************************************************************************************************

function d = method_fields

d.rfCount = {'lc', 'int'}; % read rfCount
d.rsCount = {'lc', 'int'}; % read rfCount
d.specialmin = {'lc','int'};
d.sz = {'lp', 'type', 'base','mv'};
return;

%***********************************************************************************************************************

function m = method_addlayer(m, z, p)

pz = z-1;
for i = 1:p.rsCount
  sz(i) = z - p.pzStep + (i-1);
end
m.layers{z}.pz = sz(1);
m.layers{z}.sz = sz;
m.layers{z}.rsCount = p.rsCount;
m.layers{z}.rfCount = p.rfCount;


if p.rfCount < cns_intmax

 
  m.layers{z}.size{1} = m.layers{sz(1)}.size{1};
  m = cns_mapdim(m, z, 2, 'copy' , sz(1));
  m = cns_mapdim(m, z, 3, 'int' , sz(1), p.rfCount, p.rfStep);
  m = cns_mapdim(m, z, 4, 'int' , sz(1), p.rfCount, p.rfStep);
  m.layers{z}.specialmin  = 1; % return fltmin if see fltmin
else
    m.layers{z}.rfCount = cns_intmax;
    m.layers{z}.size{1} = m.layers{sz(1)}.size{1};
    m = cns_mapdim(m, z, 2, 'copy', sz(1));
    m = cns_mapdim(m, z, 3, 'int' , sz(1), cns_intmax);
    m = cns_mapdim(m, z, 4, 'int' , sz(1), cns_intmax);
    m.layers{z}.specialmin  = 0; % treat fltmin as a value
end

return;
