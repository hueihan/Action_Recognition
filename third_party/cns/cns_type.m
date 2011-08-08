function varargout = cns_type(method, m, n, varargin)

if isstruct(m)
elseif ischar(m)
    if ~ischar(n), error('invalid n'); end
    m = struct('package', {m});
else
    error('invalid m');
end

if isnumeric(n)
    def = cns_def(m);
    if n < 0
        n = -n;
        if n > def.gCount, error('invalid n'); end
        d = def.layers{def.groups{n}.zs(1)};
    else
        d = def.layers{n};
    end
    type = d.type;
elseif ischar(n)
    if isempty(n), error('invalid n'); end
    def = cns_def(m.package);
    type = n;
    n = [];
    d = def.type.(type);
else
    error('invalid n');
end

if ~isfield(d.method, method), error('invalid method'); end
list = d.method.(method);
first = list{end};
rest  = list(1 : end - 1);

global CNS_METHOD;
save_method = CNS_METHOD;
CNS_METHOD = struct;
CNS_METHOD.method = method;
CNS_METHOD.super  = rest;

err = [];
try
    if isempty(n)
        [varargout{1 : nargout}] = feval([m.package '_cns_type_' first], method, m, varargin{:});
    else
        [varargout{1 : nargout}] = feval([m.package '_cns_type_' first], method, m, n, varargin{:});
    end
catch
    err = lasterror;
end

if isempty(save_method)
    clear global CNS_METHOD;
else
    CNS_METHOD = save_method;
end

if ~isempty(err), rethrow(err); end

return;