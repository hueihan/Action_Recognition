function s = cns_getlayernos(m, varargin)

args = varargin;

if ~isempty(args) && isstruct(args{1})
    s = args{1};
    args = args(2 : end);
else
    s = struct;
end

if isempty(args)
    prefix = 'z';
elseif numel(args) == 1
    prefix = args{1};
else
    error('incorrect number of arguments');
end

if isfield(m, 'layers')
    numLayers = numel(m.layers);
else
    numLayers = 0;
end

for z = 1 : numLayers
    if isfield(m.layers{z}, 'name')
        name = [prefix, m.layers{z}.name];
        if isvarname(name)
            s.(name) = z;
        end
    end
end

return;