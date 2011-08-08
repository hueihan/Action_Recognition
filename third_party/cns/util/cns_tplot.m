function cns_tplot(data, dt, varargin)

extra = varargin;

if (numel(extra) >= 1) && isnumeric(extra{1})
    indices = extra{1};
    extra = extra(2 : end);
else
    indices = [];
end

switch numel(indices)
case 0
    first = 1;
    step  = 1;
    last  = Inf;
case 1
    first = 1;
    step  = round(indices(1) / dt);
    last  = Inf;
case 2
    first = round(indices(1) / dt);
    step  = 1;
    last  = round(indices(2) / dt);
case 3
    first = round(indices(1) / dt);
    step  = round(indices(2) / dt);
    last  = round(indices(3) / dt);
otherwise
    error('too many indices');
end

first = max(first, 1);
step  = max(step , 1);
last  = min(last , size(data, 1));

xs = (first : step : last) * dt;
ys = data(first : step : last, :);

plot(xs, ys, extra{:});

return;