function varargout = cns_super(m, varargin)

global CNS_METHOD;
if isempty(CNS_METHOD), error('not in a method call'); end
if isempty(CNS_METHOD.super), error('no superclass implementation to call'); end
method = CNS_METHOD.method;
next   = CNS_METHOD.super{end};
CNS_METHOD.super = CNS_METHOD.super(1 : end - 1);

[varargout{1 : nargout}] = feval([m.package '_cns_type_' next], method, m, varargin{:});

return;