function [k, u] = cns_getknownfields(m)

def = cns_def(m);

mm = m;
if isfield(m, 'layers'), mm = rmfield(mm, 'layers'); end
if isfield(m, 'groups'), mm = rmfield(mm, 'groups'); end
[k, u] = Separate(mm, cns_reservednames(true), def, []);

for z = 1 : numel(def.layers)
    [k.layers{z}, u.layers{z}] = Separate(m.layers{z}, cns_reservednames(false), def.layers{z}, false);
end

for g = 1 : def.gCount
    z = def.groups{g}.zs(1);
    [k.groups{g}, u.groups{g}] = Separate(m.groups{g}, cns_reservednames(false), def.layers{z}, true);
end

return;

%***********************************************************************************************************************

function [k, u] = Separate(m, reserved, d, group)

if isequal(group, false) && d.auto, group = []; end

known = reserved;

for i = 1 : numel(d.syms)
    name = d.syms{i};
    if d.sym.(name).field && (isempty(group) || (d.sym.(name).group == group))
        known{end + 1} = name;
    end
end

names = fieldnames(m);

k = struct;

for i = 1 : numel(known)
    if ismember(known{i}, names)
        k.(known{i}) = m.(known{i});
    end
end

u = struct;

for i = 1 : numel(names)
    if ~ismember(names{i}, known)
        u.(names{i}) = m.(names{i});
    end
end
    
return;