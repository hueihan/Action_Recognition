function s = cns_getgroupnos(m, varargin)

args = varargin;

if ~isempty(args) && isstruct(args{1})
    s = args{1};
    args = args(2 : end);
else
    s = struct;
end

if isempty(args)
    prefix = 'g';
elseif numel(args) == 1
    prefix = args{1};
else
    error('incorrect number of arguments');
end

if isfield(m, 'groups')
    numGroups = numel(m.groups);
else
    numGroups = 0;
end

for g = 1 : numGroups
    if isfield(m.groups{g}, 'name')
        name = [prefix, m.groups{g}.name];
        if isvarname(name)
            s.(name) = g;
        end
    end
end

return;