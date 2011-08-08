function [cyCount, cxCount, cyOffs, cxOffs] = cns_textile(pr, yCounts, xCounts)

% NOTE: in the future, if using substeps, we might want to ensure that each substep group is contained in a separate
% rectangle so that it can be updated in one copy.  As if this problem isn't NP enough already.

if isempty(yCounts)
    cyCount = 0;
    cxCount = 0;
    cyOffs  = [];
    cxOffs  = [];
    return;
end

cyOffs = zeros(1, numel(yCounts));
cxOffs = zeros(1, numel(yCounts));

so = [-1  0 pr.maxTexYSize];
sw = [inf 0 inf           ];

[ans, inds] = sort(yCounts .* xCounts, 'descend');

for i = 1 : numel(inds)
    j = inds(i);
    [so, sw, cyOffs(j), cxOffs(j)] = Add(so, sw, yCounts(j), xCounts(j));
end

cyCount = max(cyOffs + yCounts);
cxCount = max(cxOffs + xCounts);

if cxCount > pr.maxTexXSize
    error('maximum texture x size (%u) exceeded', pr.maxTexXSize);
end

return;

%***********************************************************************************************************************

function [so, sw, yOff, xOff] = Add(so, sw, yCount, xCount)

if yCount > so(end)
    error('maximum texture y size (%u) exceeded', so(end));
end

tso = so;
tsw = sw;

while true
    [ans, ind] = min(tsw);
    if tso(ind + 1) - tso(ind) >= yCount, break; end
    if tsw(ind - 1) < tsw(ind + 1)
        tso(ind) = [];
        tsw(ind) = [];
    elseif tsw(ind - 1) == tsw(ind + 1)
        tso(ind : ind + 1) = [];
        tsw(ind : ind + 1) = [];
    else
        tsw(ind) = tsw(ind + 1);
        tso(ind + 1) = [];
        tsw(ind + 1) = [];
    end
end

yOff = tso(ind);
xOff = tsw(ind);

ind1 = find(so <  yOff         , 1, 'last' );
ind2 = find(so >= yOff + yCount, 1, 'first');
nso = [];
nsw = [];

if sw(ind1) ~= xOff + xCount
    nso(end + 1) = yOff;
    nsw(end + 1) = xOff + xCount;
end
if so(ind2) ~= yOff + yCount
    nso(end + 1) = yOff + yCount;
    nsw(end + 1) = sw(ind2 - 1);
elseif sw(ind2) == xOff + xCount
    ind2 = ind2 + 1;
end

so = [so(1 : ind1), nso, so(ind2 : end)];
sw = [sw(1 : ind1), nsw, sw(ind2 : end)];

return;