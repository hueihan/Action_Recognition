function cns_preprocess(base, tpath, fnout)

lines = ReadTop(base, tpath);

fout = fopen(fnout, 'w');
if fout < 0, error('unable to write to "%s"', fnout); end

for i = 1 : numel(lines)
    fprintf(fout, '%s\n', lines{i});
end

fclose(fout);

return;

%***********************************************************************************************************************

function lines = ReadTop(base, tpath)

p = struct;
p.names = {};
p.parts = {};
p.used  = false(0, 0);

found = false;

for i = numel(tpath) : -1 : 1
    fn = ['type_' tpath{i}];
    if ~exist([base '_' fn '.h'], 'file'), continue; end
    ft = ProcessFile(base, fn, @ReadFileType);
    if ft ~= 'p'
        found = true;
        break;
    end
    p = ProcessFile(base, fn, @ReadParts, p);
end

if isempty(p.names)
    if ~found, error('type "%s": no superclass defines a kernel', tpath{end}); end
    if ft ~= 'k', error('type "%s": kernel "%s" is not a complete kernel', tpath{end}, tpath{i}); end
    lines = ProcessFile(base, fn, @Read, 'k', [], 0, 0);
else
    if ~found, error('type "%s": no superclass defines a template', tpath{end}); end
    if ft ~= 't', error('type "%s": kernel "%s" is not a template', tpath{end}, tpath{i}); end
    lines = ProcessFile(base, fn, @ReadTemplate, p);
end

return;

%***********************************************************************************************************************

function varargout = ProcessFile(base, fn, func, varargin)

fp = [base '_' fn '.h'];

fh = fopen(fp, 'r');
if fh < 0, error('unable to open "%s"', fp); end

try
    [varargout{1 : nargout}] = func(base, fh, fgetl(fh), varargin{:});
catch
    fclose(fh);
    error('file "%s": %s', fp, cns_error);
end

fclose(fh);

return;

%***********************************************************************************************************************

function ft = ReadFileType(base, fh, line)

while true
    if isequal(line, -1), break; end
    tok = strtok(line);
    if ~isempty(tok) && isempty(strmatch('//', tok)), break; end
    line = fgetl(fh);
end

if isequal(line, -1)
    ft = 'k';
elseif strcmp(tok, '#TEMPLATE')
    ft = 't';
elseif strcmp(tok, '#PART')
    ft = 'p';
else
    ft = 'k';
end

return;

%***********************************************************************************************************************

function p = ReadParts(base, fh, line, p)

while true
    if isequal(line, -1), return; end
    [tok, rest] = strtok(line);
    if strcmp(tok, '#PART'), break; end
    line = fgetl(fh);
end

names = {};

while true

    [name, rest] = strtok(rest);
    if isempty(name), error('#PART name missing'); end
    if ~isempty(strtrim(rest)), error('#PART names cannot contain spaces'); end

    if ismember(name, names), error('#PART name "%s" repeated', name); end
    names{end + 1} = name;

    [part, line] = Read(base, fh, fgetl(fh), 'p', [], 0, 0);

    n = find(any(strcmpi(name, p.names)), 1);
    if isempty(n), n = numel(p.names) + 1; end

    p.names{n} = name;
    p.parts{n} = part;
    p.used (n) = false;

    if isequal(line, -1), break; end
    [tok, rest] = strtok(line);

end

return;

%***********************************************************************************************************************

function lines = ReadTemplate(base, fh, line, p)

while true
    if isequal(line, -1), return; end
    [tok, rest] = strtok(line);
    if strcmp(tok, '#TEMPLATE'), break; end
    line = fgetl(fh);
end

if ~isempty(strtrim(rest)), error('superfluous #TEMPLATE parameters'); end

[lines, ans, p] = Read(base, fh, fgetl(fh), 't', p, 0, 0);

n = find(~p.used, 1);
if ~isempty(n), error('#PART "%s" is not used by the #TEMPLATE', p.names{n}); end

return;

%***********************************************************************************************************************

function [lines, term, p] = Read(base, fh, line, pmode, p, inc, level)

lines = {};

while true

    if isequal(line, -1)
        if level > 0, error('unmatched #UNROLL_START'); end
        break;
    end

    [tok, rest] = strtok(line);

    if strcmp(tok, '#UNROLL_START')

        [iter, rest] = strtok(rest);
        if isempty(iter), error('#UNROLL_START parameters missing'); end

        [symbol, rest] = strtok(rest);
        lims = strtrim(rest);

        [body, ans, p] = Read(base, fh, fgetl(fh), pmode, p, inc, level + 1);

        lines = [lines, Repeat(body, str2double(iter), symbol, lims, inc, level)];

    elseif strcmp(tok, '#UNROLL_END')

        if ~isempty(strtrim(rest)), error('superfluous #UNROLL_END parameters'); end

        if level == 0, error('unmatched #UNROLL_END'); end

        break;

    elseif strcmp(tok, '#UNROLL_BREAK')

        if ~isempty(strtrim(rest)), error('superfluous #UNROLL_BREAK parameters'); end

        if level == 0, error('unmatched #UNROLL_BREAK'); end

        lines{end + 1} = line;

    elseif strcmp(tok, '#define')

        error('#define is not allowed');

    elseif strcmp(tok, '#include')

        error('#include is not allowed; use #INCLUDE');

    elseif strcmp(tok, '#INCLUDE')

        name = strtrim(rest);
        if isempty(name), error('#INCLUDE name missing'); end

        lines = [lines, ProcessFile(base, ['include_' name], @Read, pmode, p, inc + 1, 0)];

    elseif strcmp(tok, '#TEMPLATE')

        error('#TEMPLATE is not allowed here');

    elseif strcmp(tok, '#PART')

        if pmode == 'k', error('#PART is invalid if not using a #TEMPLATE'); end
        if inc > 0, error('#PART cannot appear inside an #INCLUDE file'); end

        if pmode == 'p', break; end

        [name, rest] = strtok(rest);
        if isempty(name), error('#PART name missing'); end
        if ~isempty(strtrim(rest)), error('#PART names cannot contain spaces'); end

        n = find(strcmp(p.names, name), 1);
        if isempty(n), error('#PART "%s" is missing', name); end

        lines = [lines, p.parts{n}];

        p.used(n) = true;

    else

        lines{end + 1} = line;

    end

    line = fgetl(fh);

end

term = line;

return;

%***********************************************************************************************************************

function lines = Repeat(body, iter, symbol, lims, inc, level)

if isempty(lims)
    lines = RepeatSimple(body, iter, symbol);
else
    lines = RepeatComplex(body, iter, symbol, lims, inc, level);
end

return;

%***********************************************************************************************************************

function lines = RepeatSimple(body, iter, symbol)

lines = {};

lines{end + 1} = 'for (int _ur = 0; _ur < 1; _ur++) {';

for i = 0 : iter - 1

    lines{end + 1} = '{';

    for j = 1 : numel(body)
        if strcmp(strtok(body{j}), '#UNROLL_BREAK')
            lines{end + 1} = 'break';
        elseif isempty(symbol)
            lines{end + 1} = body{j};
        else
            lines{end + 1} = strrep(body{j}, symbol, sprintf('%u', i));
        end
    end

    lines{end + 1} = '}';

end

lines{end + 1} = '}';

return;

%***********************************************************************************************************************

function lines = RepeatComplex(body, iter, symbol, lims, inc, level)

[args{1}, rest] = strtok(lims);
[args{2}, rest] = strtok(rest);
args{3} = strtrim(rest);
args = args(~strcmp(args, ''));
switch numel(args)
case 1, lim1 = '0'    ; cond = '<'    ; lim2 = args{1};
case 2, lim1 = args{1}; cond = '<'    ; lim2 = args{2};
case 3, lim1 = args{1}; cond = args{2}; lim2 = args{3};
end
switch cond
case '<' , extra = '0';
case '<=', extra = '1';
otherwise, error('invalid condition');
end

id = sprintf('%u_%u', inc, level);

lines = {};

lines{end + 1} = '{';
lines{end + 1} = sprintf('int n_%s = ((%s) - (%s) + %s) / %u;', id, lim2, lim1, extra, iter);
lines{end + 1} = sprintf('int i_%s = (%s);', id, lim1);
lines{end + 1} = sprintf('for (int u_%s = 0; u_%s < n_%s; u_%s++) {', id, id, id, id);

for i = 1 : iter
    lines{end + 1} = '{';
    for j = 1 : numel(body)
        if strcmp(strtok(body{j}), '#UNROLL_BREAK')
            lines{end + 1} = sprintf('i_%s = (%s) + %s;', id, lim2, extra);
            lines{end + 1} = 'break';
        else
            lines{end + 1} = strrep(body{j}, symbol, sprintf('i_%s', id));
        end
    end
    lines{end + 1} = '}';
    lines{end + 1} = sprintf('i_%s++;', id);
end

lines{end + 1} = '}';
lines{end + 1} = sprintf('for (; i_%s %s (%s); i_%s++) {', id, cond, lim2, id);

for j = 1 : numel(body)
    if strcmp(strtok(body{j}), '#UNROLL_BREAK')
        lines{end + 1} = 'break';
    else
        lines{end + 1} = strrep(body{j}, symbol, sprintf('i_%s', id));
    end
end
    
lines{end + 1} = '}';
lines{end + 1} = '}';

return;