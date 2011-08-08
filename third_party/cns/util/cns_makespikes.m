function times = cns_makespikes(rates, dt, rdt)

if nargin < 3, rdt = dt; end

siz = size(rates);

times = cns_spikeutil(0, double(rates), dt, round(rdt / dt));

times = reshape(times, [size(times, 1), siz(2 : end)]);

return;