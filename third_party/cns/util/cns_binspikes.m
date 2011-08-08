function bins = cns_binspikes(times, dur, dt, bdt)

if nargin < 4, bdt = dt; end

siz = size(times);

bins = cns_spikeutil(2, single(times), round(dur / bdt), round(bdt / dt));

bins = reshape(bins, [size(bins, 1), siz(2 : end)]);

return;