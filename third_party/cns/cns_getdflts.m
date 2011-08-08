function m = cns_getdflts(m, n)

if nargin < 2, n = []; end

def = cns_def(m);

if isempty(n) || (n == 0)
    m = GetDflts(m, def, []);
end

for z = 1 : numel(def.layers)
    if isempty(n) || (n == z)
        m.layers{z} = GetDflts(m.layers{z}, def.layers{z}, false);
    end
end

for g = 1 : def.gCount
    if isempty(n) || (n == -g)
        z = def.groups{g}.zs(1);
        m.groups{g} = GetDflts(m.groups{g}, def.layers{z}, true);
    end
end

return;

%***********************************************************************************************************************

function m = GetDflts(m, d, group)

if isequal(group, false) && d.auto, group = []; end

for i = 1 : numel(d.syms)
    name = d.syms{i};
    if d.sym.(name).field && (isempty(group) || (d.sym.(name).group == group)) && ~isfield(m, name)
        dflt = d.sym.(name).value;
        if ~isequalwithequalnans(dflt, NaN), m.(name) = dflt; end
    end
end

return;