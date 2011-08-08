function m = cns_setrunorder(m, mode, varargin)

switch mode
case 'field', m = Field(m, varargin{:});
otherwise   , error('invalid mode');
end

return;

%***********************************************************************************************************************

function m = Field(m, fields)

if ischar(fields), fields = {fields}; end

def = cns_def(m);

p = repmat(0 , 0, numel(def.layers));
r = repmat(-1, 1, numel(def.layers));

for z = 1 : numel(def.layers)

    a = [];
    for i = 1 : numel(fields)
        a = [a, m.layers{z}.(fields{i})];
    end
    a = unique(a);

    if any(a(:) < 0                ), error('z=%u: invalid previous layer', z); end
    if any(a(:) > numel(def.layers)), error('z=%u: invalid previous layer', z); end
    a = a(a > 0);

    if def.layers{z}.kernel
        p(1 : numel(a), z) = a(:);
    else
        if ~isempty(a), error('z=%u: layers with no kernel cannot have dependencies', z); end
        r(z) = 0;
    end

end

p(ismember(p, find(r == 0))) = 0;
n = 1;

while true

    f = find((r < 0) & all(p == 0, 1));
    if isempty(f), break; end

    r(f) = n;
    n = n + 1;

    p(ismember(p, f)) = 0;

end

if any(r < 0), error('there is a dependency loop'); end

for z = 1 : numel(def.layers)
    m.layers{z}.runOrder = r(z);
end

m.feedforward = true;

return;