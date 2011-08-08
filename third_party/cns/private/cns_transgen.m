function cns_transgen(mode, d, printf, platform, name, varargin)

p.idims  = [];
p.iparts = [];
p.edims  = [];
p.eparts = [];
for ed = 1 : numel(d.dims)
    for ep = 1 : numel(d.dims{ed})
        p.idims (end + 1) = d.dims  {ed}(ep);
        p.iparts(end + 1) = d.dparts{ed}(ep);
        p.edims (end + 1) = ed;
        p.eparts(end + 1) = ep;
    end
end

p.ac = find((p.idims == 1) & (p.iparts == 1));

switch platform
case 'cuda', p.attr = '__device__';
case 'cpu' , p.attr = 'inline';
end

p.ivars = 'yx';

p.platform = platform;
p.name     = name;

switch mode
case 'handle', HandleFunc (p, printf, varargin{:});
case 'size'  , SizeFuncs  (p, printf, varargin{:});
case 'coord' , CoordFuncs (p, printf, varargin{:});
case 'lookup', LookupFuncs(p, printf, varargin{:});
case 'write' , WriteFunc  (p, printf, varargin{:});
end

return;

%***********************************************************************************************************************

function HandleFunc(p, printf, type)

printf('struct %s_h {\n', p.name);
printf('const unsigned short *siz2p;\n');
if strcmp(type, 'c')
    printf('unsigned short meta[2];\n');
end
printf('};\n');

printf('%s %s_h %s_mh(', p.attr, p.name, p.name);
if strcmp(type, 'c')
    printf('const unsigned short *meta, ');
end
printf('const unsigned short *siz2p) {\n');
printf('%s_h h;\n', p.name);
printf('h.siz2p = siz2p;\n');
if strcmp(type, 'c')
    printf('h.meta[0] = meta[0];\n');
    printf('h.meta[1] = meta[1];\n');
end
printf('return h;\n');
printf('}\n');

return;

%***********************************************************************************************************************

function SizeFuncs(p, printf, hFlags)

if nargin < 3, hFlags = [false true]; end

for handle = hFlags

    p = GetParamInfo(p, handle);

    printf('%s int %s_s%s(%s, unsigned int d) {\n', p.attr, p.name, p.suffix, p.param2);

    printf('return (int)%s[d];\n', p.siz2p);

    printf('}\n');

end

return;

%***********************************************************************************************************************

function CoordFuncs(p, printf)

p = GetParamInfo(p, false);

for c = 1 : numel(p.edims)

    printf('%s int %s_c%s%u(%s, unsigned int y, unsigned int x) {\n', p.attr, p.name, p.suffix, c - 1, p.param2);

    s = p.ivars(p.idims(c));
    if p.iparts(c) < max(p.iparts(p.idims == p.idims(c)))
        s = sprintf('(%s %% %s)', s, ProdSiz2(p, find((p.idims == p.idims(c)) & (p.iparts <= p.iparts(c)))));
    end
    if p.iparts(c) > 1
        s = sprintf('(%s / %s)' , s, ProdSiz2(p, find((p.idims == p.idims(c)) & (p.iparts <  p.iparts(c)))));
    end

    printf('return (int)%s;\n', s);

    printf('}\n');

end

return;

%***********************************************************************************************************************

function LookupFuncs(p, printf, type, varargin)

% TODO: bounds checking for cpu

for handle = [false true]

    p = GetParamInfo(p, handle, type);

    printf('%s float %s_lk%s(', p.attr, p.name, p.suffix);
    if strcmp(type, 'a')
        printf('const float *base, ');
    end
    printf('%s', p.param);
    printf(', int c%u', 0 : numel(p.edims) - 1);
    printf(') {\n');

    BuildYX(p, printf, type, 1, true);
    BuildYX(p, printf, type, 2, true);

    DoLookup(p, printf, type, false, varargin{:});

    printf('}\n');

end

if strcmp(type, 'a')
    hFlags = [true];
else
    hFlags = [false true];
end

for handle = hFlags

    p = GetParamInfo(p, handle, type);

    printf('%s void %s_gc%s(', p.attr, p.name, p.suffix);
    if ismember(type, {'c', 't'})
        printf('%s', p.param);
    else
        printf('%s', p.param2);
    end
    printf(', int c%u', 0 : numel(p.edims) - 1);
    printf(', int &y, int &x) {\n');

    BuildYX(p, printf, type, 1, false);
    BuildYX(p, printf, type, 2, false);

    printf('}\n');

end

p = GetParamInfo(p, true, type);

printf('%s float %s_lc(', p.attr, p.name);
if strcmp(type, 'a')
    printf('const float *base, %s, ', p.param);
end
printf('int y, int x) {\n');

DoLookup(p, printf, type, false, varargin{:});

printf('}\n');

if strcmp(type, 'c')

    p = GetParamInfo(p, false, type);

    printf('%s float %s_ln(%s, int y, int x) {\n', p.attr, p.name, p.param1);

    DoLookup(p, printf, type, true, varargin{:});

    printf('}\n');

end

return;

%-----------------------------------------------------------------------------------------------------------------------

function BuildYX(p, printf, type, id, dec)

nip = max(p.iparts(p.idims == id));

for ip = nip : -1 : 1

    c = find((p.idims == id) & (p.iparts == ip));

    s = sprintf('c%u', c - 1);

    if ip == nip
        if dec, printf('unsigned int '); end
        printf('%s = %s;\n', p.ivars(id), s);
    else
        if c == p.ac, c = numel(p.edims) + 1; end
        printf('%s = %s * %s[%u] + %s;\n', p.ivars(id), p.ivars(id), p.siz2p, c - 1, s);
    end

end

if ismember(type, {'c', 't'})
    printf('%s += %s;\n', p.ivars(id), p.meta{id});
end

return;

%-----------------------------------------------------------------------------------------------------------------------

function DoLookup(p, printf, type, addOffs, tnb)

if strcmp(type, 'a')

    printf('unsigned int start = %s + (%s << 16);\n', p.meta{1}, p.meta{2});
    printf('return base[start + x * %s[%u] + y];\n', p.siz2p, numel(p.edims) + 1);

else

    if addOffs
        printf('y += %s;\n', p.meta{1});
        printf('x += %s;\n', p.meta{2});
    end

    switch p.platform
    case 'cuda', printf('return tex2D(%s, y, x);\n', tnb);
    case 'cpu' , printf('return %s.buf[x * %s.h + y];\n', tnb, tnb);
    end

end

return;

%***********************************************************************************************************************

function WriteFunc(p, printf)

p = GetParamInfo(p, false, 'c');

printf('%s void %s_wn(float *ptr, unsigned int h, %s, int y, int x, float v) {\n', p.attr, p.name, p.param1);

printf('y += %s;\n', p.meta{1});
printf('x += %s;\n', p.meta{2});

printf('ptr[x * h + y] = v;\n');

printf('}\n');

return;

%***********************************************************************************************************************

function p = GetParamInfo(p, handle, type)

if nargin < 3, type = ''; end

if handle
    p.suffix = 'h';
    p.siz2p  = '(unsigned int)h.siz2p';
    p.param  = sprintf('%s_h h', p.name);
    p.param1 = p.param;
    p.param2 = p.param;
else
    p.suffix = '';
    p.siz2p  = '(unsigned int)siz2p';
    if strcmp(type, 'c')
        p.param  = 'const unsigned short *meta, const unsigned short *siz2p';
        p.param1 = 'const unsigned short *meta';
        p.param2 = 'const unsigned short *siz2p';
    else
        p.param  = 'const unsigned short *siz2p';
        p.param1 = p.param;
        p.param2 = p.param;
    end
end

if strcmp(type, 'c')
    if handle
        meta = '(unsigned int)h.meta';
    else
        meta = '(unsigned int)meta';
    end
    p.meta{1} = sprintf('%s[0]', meta);
    p.meta{2} = sprintf('%s[1]', meta);
else
    p.meta{1} = sprintf('%s[-2]', p.siz2p);
    p.meta{2} = sprintf('%s[-1]', p.siz2p);
end

return;

%***********************************************************************************************************************

function s = ProdSiz2(p, cs)

cs(cs == p.ac) = numel(p.edims) + 1;

s = sprintf('%s[%u]', p.siz2p, cs(1) - 1);

if numel(cs) > 1

    for c = cs(2 : end)
        s = sprintf('%s * %s[%u]', s, p.siz2p, c - 1);
    end

    s = sprintf('(%s)', s);

end

return;