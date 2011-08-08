function d = hjpkg_sort(d)

[ans, inds] = sort(d.fSizes);

d.fSizes = d.fSizes(inds);

if isfield(d, 'fMap')
    d.fVals = d.fVals(:, inds);
    d.fMap  = d.fMap (:, :, inds);
else
    d.fVals = d.fVals(:, :, :, inds);
end

return;