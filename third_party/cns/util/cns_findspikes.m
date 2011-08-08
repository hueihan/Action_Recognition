function times = cns_findspikes(v, thres, dt, vdt)

if nargin < 2, thres = 0 ; end
if nargin < 3, dt    = 1 ; end
if nargin < 4, vdt   = dt; end

siz = size(v);
if siz(1) < 2, error('not enough samples'); end

times = cns_spikeutil(1, single(v), thres, round(vdt / dt));

times = reshape(times, [size(times, 1), siz(2 : end)]);

return;