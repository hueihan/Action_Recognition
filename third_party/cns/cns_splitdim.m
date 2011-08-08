function s = cns_splitdim(n, numParts)

if n == 0

    s = zeros(1, numParts);

else

    f = factor(n);
    s = ones(1, numParts);

    for i = numel(f) : -1 : 1
        [ans, j] = min(s);
        s(j) = s(j) * f(i);
    end

    s = sort(s, 'descend');

end

return;