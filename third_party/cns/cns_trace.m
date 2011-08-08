function varargout = cns_trace(m, z, path, varargin)

is = cns_iconv(m, z, varargin{:});
is = unique(is(:));

nc = z;

for i = 1 : numel(path)

    if isempty(is), break; end

    nn = path(i);

    if nn < 0

        nn = -nn;

        if ~isfield(m.layers{nc}, 'synapseIs')
            is = [];
        else
            synIs = m.layers{nc}.synapseIs(:, is);
            synZs = m.layers{nc}.synapseZs(:, is);
            is = unique(synIs(synZs(:) == nn));
        end

    else

        if ~isfield(m.layers{nn}, 'synapseIs')
            is = [];
        else
            syns = (m.layers{nn}.synapseZs == nc) & ismember(m.layers{nn}.synapseIs, is);
            is = find(any(syns(:, :), 1)');
        end

    end

    nc = nn;

end

if isempty(is), is = []; end

[varargout{1 : nargout}] = cns_iconv(m, nc, is);

return;
