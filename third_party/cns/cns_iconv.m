function varargout = cns_iconv(m, z, varargin)

if isfield(m.layers{z}, 'size')
    siz = cellfun(@prod, m.layers{z}.size);
else
    siz = [m.layers{z}.yCount, m.layers{z}.xCount];
end

ni = numel(varargin);
no = max(nargout, 1);

if (ni < 1) || (ni > numel(siz)), error('incorrect number of arguments'); end
if no > numel(siz), error('incorrect number of outputs'); end

si = [siz(1 : ni - 1), prod(siz(ni : end))];
so = [siz(1 : no - 1), prod(siz(no : end))];

for i = 1 : ni
    if any(varargin{i}(:) < 1    ), error('index out of range'); end
    if any(varargin{i}(:) > si(i)), error('index out of range'); end
end

if ni == no
    varargout = varargin;
elseif ni == 1
    [varargout{1 : no}] = ind2sub(so, varargin{1});
elseif no == 1
    varargout{1} = sub2ind(si, varargin{:});
else
    [varargout{1 : no}] = ind2sub(so, sub2ind(si, varargin{:}));
end

return;