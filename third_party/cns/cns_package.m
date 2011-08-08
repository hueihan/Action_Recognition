function varargout = cns_package(method, m, varargin)

if isstruct(m)
elseif ischar(m)
    m = struct('package', {m});
else
    error('invalid m');
end

def = cns_def(m);

if ~ismember(method, def.methods), error('invalid method'); end

global CNS_METHOD;
save_method = CNS_METHOD;
CNS_METHOD = [];

err = [];
try
    [varargout{1 : nargout}] = feval([m.package '_cns'], method, m, varargin{:});
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