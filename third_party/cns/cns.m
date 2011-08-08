function varargout = cns(mode, varargin)

persistent G;
if isempty(G)
    G = struct;
    G.sessionOpen = false;
end

G.err = '';

try
    switch mode
    case 'platform', [   varargout{1 : nargout}] = Platform(G, varargin{:});
    case 'init'    , [G, varargout{1 : nargout}] = Init    (G, varargin{:});
    case 'done'    , [G, varargout{1 : nargout}] = Done    (G, varargin{:});
    case 'run'     , [   varargout{1 : nargout}] = Run     (G, varargin{:});
    case 'get'     , [   varargout{1 : nargout}] = Get     (G, varargin{:});
    case 'update'  , [   varargout{1 : nargout}] = Update  (G, varargin{:});
    case 'set'     , [   varargout{1 : nargout}] = Set     (G, varargin{:});
    otherwise      , error('invalid mode');
    end
catch
    G.err = cns_error;
end

err = G.err;
 
if ~G.sessionOpen
    clear G;
end

if ~isempty(err)
 
    error(err);
end

end

%***********************************************************************************************************************

function Platform(G, varargin)

if G.sessionOpen, error('session currently open'); end
if nargin < 2, error('not enough arguments'); end

GetPlatform(varargin{:});

global CNS_Platform;
CNS_Platform = varargin;

end

%***********************************************************************************************************************

function G = Init(G, varargin)

try
    G = Init2(G, varargin{:});
    G.err = '';
catch
    err = cns_error;
    G = Done(G);
    G.err = err;
end

end

%***********************************************************************************************************************

function G = Init2(G, m, varargin)

if nargin < 2, error('not enough arguments'); end

global CNS_Platform;
if nargin == 2
    args = CNS_Platform;
    if isempty(args), args = {}; end
else
    args = varargin;
end
[platform, deviceNo, nice] = GetPlatform(args{:});

g = struct;
g.sessionOpen = false;

if G.sessionOpen

    if ~strcmp(m.package, G.m.package), error('to reinitialize for a new package, call "done" first'); end
    if ~strcmp(platform, G.platform), error('to reinitialize for a new platform, call "done" first'); end
    if deviceNo ~= G.deviceNo, error('to reinitialize for a new device, call "done" first'); end
    if nice ~= G.nice, error('to reinitialize for a new nice value, call "done" first'); end

    G.func(CB(G), 3);

    g.platform = G.platform;
    g.deviceNo = G.deviceNo;
    g.nice     = G.nice;
    g.func     = G.func;
    g.pr       = G.pr;

else

    % Precaution in case the clear in 'Done' didn't happen; see comments there.
    clear mex;

    g.platform = platform;
    g.deviceNo = deviceNo;
    g.nice     = nice;
    g.func     = str2func([m.package '_cns_compiled_' platform]);
    g.pr       = g.func(CB(g), 0, deviceNo, double(nice));

end

g.def = cns_def(m);

if ismember('init', g.def.methods)
    m = feval([m.package '_cns'], 'init', m);
end

g.m = MemModel(m, g.def);

[g.s, h] = InitStart;
[g.s, h] = InitGlobal     (g, g.s, h, m, g.def);
[g.s, h] = InitGroups     (g, g.s, h, m, g.def);
[g.s, h] = InitLayers     (g, g.s, h, m, g.def);
[g.s, h] = ReadAllConsts  (g, g.s, h, m, g.def);
[g.s, h] = ReadAllArrays  (g, g.s, h, m, g.def);
[g.s, h] = ReadAllTextures(g, g.s, h, m, g.def);
[g.s, h] = ReadAllCommon  (g, g.s, h, m, g.def);
[g.s, h] = ReadAllNFields (g, g.s, h, m, g.def);
[g.s, h] = ReadAllSynapses(g, g.s, h, m, g.def);
[g.s, h] = ReadAllSFields (g, g.s, h, m, g.def);
[g.s, h] = MakeLayerTable (g, g.s, h, m, g.def);
[g.s, h] = MakeKernels    (g, g.s, h, m, g.def);
[g.s, h] = Finalize       (g, g.s, h);

g.func(CB(g), 2, g.s, h);

g.sessionOpen = true;

G = g;

mlock;

end

%***********************************************************************************************************************

function G = Done(G)

if ~G.sessionOpen, return; end

G.func(CB(G), 3);
G.func(CB(G), 1);

funcName = func2str(G.func);

G = struct;
G.sessionOpen = false;

munlock;

% Not sure why this is necessary given that we clean everything up in the mex file, explicitly ask for the CUDA
% context to be released, etc.  But this is apparently not enough.  Without a clear, if we run a different package,
% we get the wrong results and no error message!  CUDA must still be hanging on to something.
clear(funcName);

G.err = '';

end

%***********************************************************************************************************************

function [platform, deviceNo, nice] = GetPlatform(platform, niceFlag)

if nargin < 1, platform = 'cuda'; end
if nargin < 2, niceFlag = 'nice'; end

pos = find(isstrprop(platform, 'digit'), 1);
if isempty(pos)
    deviceNo = -1;
else
    if ~all(isstrprop(platform(pos + 1 : end), 'digit')), error('invalid device number'); end
    deviceNo = str2double(platform(pos : end));
    if isnan(deviceNo), error('invalid device number'); end
    platform = platform(1 : pos - 1);
end

switch lower(platform)
case {'cuda', 'gpu'}
    platform = 'cuda';
case 'cpu'
    if deviceNo > 0, error('device %u not supported for cpu', deviceNo); end
    platform = 'cpu';
otherwise
    error('platform "%s" is invalid', platform);
end

switch lower(niceFlag)
case 'mean', nice = false;
case 'nice', nice = true;
otherwise  , error('nice flag "%s" is invalid', niceFlag);
end

end

%***********************************************************************************************************************

function mm = MemModel(m, def)

mm = struct;

names = cns_reservednames(true, true);

for f = 1 : numel(names)
    if isfield(m, names{f}), mm.(names{f}) = m.(names{f}); end
end

names = cns_reservednames(false, true);

for z = 1 : numel(def.layers)
    for f = 1 : numel(names)
        if isfield(m.layers{z}, names{f}), mm.layers{z}.(names{f}) = m.layers{z}.(names{f}); end
    end
end

for g = 1 : def.gCount
    for f = 1 : numel(names)
        if isfield(m.groups{g}, names{f}), mm.groups{g}.(names{f}) = m.groups{g}.(names{f}); end
    end
end

end

%***********************************************************************************************************************

function [s, h] = InitStart

s = struct;
h = struct;

h.cLayerTable = zeros(0, 1); h.sLayerTable = 0;
h.cMVTable    = zeros(0, 1); h.sMVTable    = 1;
h.cVarMeta    = zeros(0, 1); h.sVarMeta    = 2;

h.dData = zeros(0, 1, 'single');

end

%***********************************************************************************************************************

function [s, h] = InitGlobal(G, s, h, m, def)

s.mvOff = TagOff(numel(h.cMVTable), h.sMVTable);
h.cMVTable(end + 1 : end + 2 * numel(def.list.mvm.syms), 1) = 0;

if isfield(m, 'feedforward')
    s.feedforward = double(m.feedforward);
else
    s.feedforward = 0;
end

end

%***********************************************************************************************************************

function [s, h] = InitGroups(G, s, h, m, def)

for g = 1 : numel(def.groups)

    d = def.layers{def.groups{g}.zs(1)};
    c = struct;

    c.mvOff = TagOff(numel(h.cMVTable), h.sMVTable);
    h.cMVTable(end + 1 : end + 2 * numel(d.list.mvg.syms), 1) = 0;

    s.groups{g} = c;

end

end

%***********************************************************************************************************************

function [s, h] = InitLayers(G, s, h, m, def)

s.ySizes  = zeros(1, numel(def.layers));
s.xCounts = zeros(1, numel(def.layers));

s.isType = false(numel(def.layers), 0);

for z = 1 : numel(def.layers)

    d = def.layers{z};

    if d.kernel
        if mod(max(d.blockYSize, 0.5), G.pr.blockYSizeAlign) ~= 0
            error('type "%s": block y size must be a multiple of %u', d.type, G.pr.blockYSizeAlign);
        end
        if mod(max(d.blockYSize * d.blockXSize, 0.5), G.pr.blockSizeAlign) ~= 0
            error('type "%s": block size must be a multiple of %u', d.type, G.pr.blockSizeAlign);
        end
    end

    c = struct;

    c.typeNo = d.typeNo;

    try
        c.t = cns_trans('create', m.layers{z}, d, G.pr.blockYSizeAlign);
    catch
        error('z=%u: %s', z, cns_error);
    end
    c.yCount0 = c.t.siz3 (1);
    c.ySize0  = c.t.siz3a(1);
    c.yCount  = c.t.siz4a(1);
    c.ySize   = c.t.siz4b(1);
    c.xCount  = c.t.siz4a(2);

    if c.yCount0 == c.ySize0
        yCountOpt = c.yCount;
    else
        yCountOpt = c.yCount0;
    end

    if ~isfield(m, 'quiet') || ~m.quiet
        if (yCountOpt < G.pr.blockYSizeAlign) && (c.xCount >= 2 * G.pr.blockYSizeAlign)
            warning('z=%u: for thin layers, yCount > xCount is more efficient', z);
        end
    end

    if d.kernel
        c.blockYSize = OptimizeBlock(d.blockYSize, d.blockXSize, G.pr.blockYSizeAlign, yCountOpt, c.xCount);
    else
        c.blockYSize = 0;
    end

    c.sFlag = any(isfield(m.layers{z}, {'synapseIs', 'synapseZs', 'synapseTs', ...
        d.cat.sc.syms{:}, d.cat.sv.syms{:}}));

    if c.sFlag
        if d.synTypeNo == 0
            error('z=%u: synapses are not defined for this type', z);
        end
        if ~isfield(m.layers{z}, 'synapseIs')
            error('z=%u: field "synapseIs" is missing', z);
        end
        c.sSize = size(m.layers{z}.synapseIs, 1);
        if c.sSize > intmax('uint16')
            error('z=%u: at most %u synapses are allowed', z, intmax('uint16'));
        end
    else
        c.sSize = 0;
    end

    c.st = cns_trans('add', c.t, c.sSize);

    c.mvOff = TagOff(numel(h.cMVTable), h.sMVTable);
    h.cMVTable(end + 1 : end + 2 * numel(d.list.mvl.syms), 1) = 0;

    s.layers{z} = c;

    s.ySizes (z) = c.ySize;
    s.xCounts(z) = c.xCount;
    s.ts     (z) = c.t;

    s.isType(z, 1 : numel(d.isType)) = d.isType;

end

end

%***********************************************************************************************************************

function [s, h] = UpdateGlobal(func, G, s, h, m, def, varargin)

try
    [s, h] = func(G, s, h, m, def, 0, varargin{:});
catch
    error('z=0: %s', cns_error);
end

end

%***********************************************************************************************************************

function [s, h] = UpdateGroups(func, G, s, h, m, def, varargin)

for g = 1 : numel(def.groups)

    z = def.groups{g}.zs(1);

    if g <= def.gCount
        data = m.groups{g};
    else
        data = m.layers{z};
    end

    try
        [s.groups{g}, h] = func(G, s.groups{g}, h, data, def.layers{z}, -g, varargin{:});
    catch
        if g <= def.gCount
            error('group=%u: %s', g, cns_error);
        else
            error('z=%u: %s', z, cns_error);
        end
    end

end

end

%***********************************************************************************************************************

function [s, h] = UpdateLayers(func, G, s, h, m, def, varargin)

for z = 1 : numel(def.layers)

    try
        [s.layers{z}, h] = func(G, s.layers{z}, h, m.layers{z}, def.layers{z}, z, varargin{:});
    catch
        error('z=%u: %s', z, cns_error);
    end

end

end

%***********************************************************************************************************************

function [s, h] = ReadAllConsts(G, s, h, m, def)

h.cData = zeros(0, 1, 'single');

[s, h] = UpdateGlobal(@ReadConsts, G, s, h, m, def, 'mc');
[s, h] = UpdateGroups(@ReadConsts, G, s, h, m, def, 'gc');
[s, h] = UpdateLayers(@ReadConsts, G, s, h, m, def, 'lc');

end

%***********************************************************************************************************************

function [c, h] = ReadConsts(G, c, h, m, d, n, cat)

c.cOff = numel(h.cData);

data = zeros(numel(d.list.(cat).svSyms), 1, 'single');

for f = 1 : numel(d.list.(cat).svSyms)

    name = d.list.(cat).svSyms{f};
    dd = d.sym.(name);
    pos = dd.pos;

    a = GetField(m, name, [], dd);
    if isfield(dd, 'ptrTypeNo'), CheckPointer(G.s.isType, name, a, dd.ptrTypeNo, true); end

    data(pos, 1) = a;

end

mvOff = RelOff(c.mvOff);

for f = 1 : numel(d.list.(cat).mvSyms)

    name = d.list.(cat).mvSyms{f};
    dd = d.sym.(name);
    pos = 2 * dd.pos - 1;

    a = GetField(m, name, [], dd);
    if isfield(dd, 'ptrTypeNo'), CheckPointer(G.s.isType, name, a, dd.ptrTypeNo, false); end

    h.cMVTable(mvOff + pos    , 1) = numel(h.cData) + numel(data);
    h.cMVTable(mvOff + pos + 1, 1) = numel(a);

    data(end + 1 : end + numel(a), 1) = a;

end

h.cData(end + 1 : end + numel(data), 1) = data;

end

%***********************************************************************************************************************

function [s, h] = ReadAllArrays(G, s, h, m, def)

[s, h] = UpdateGlobal(@ReadArrays, G, s, h, m, def, 'ma');
[s, h] = UpdateGroups(@ReadArrays, G, s, h, m, def, 'ga');
[s, h] = UpdateLayers(@ReadArrays, G, s, h, m, def, 'la');

end

%***********************************************************************************************************************

function [c, h] = ReadArrays(G, c, h, m, d, n, cat)

meta = zeros(0, 1);
data = zeros(0, 1, 'single');

mvOff = RelOff(c.mvOff);

for f = 1 : numel(d.cat.(cat).syms)

    name = d.cat.(cat).syms{f};
    pos = 2 * d.sym.(name).pos - 1;

    [as, siz2ps] = GetArray(m, name, d.sym.(name), G.pr.blockYSizeAlign);

    h.cMVTable(mvOff + pos    , 1) = TagOff(numel(h.cVarMeta) + numel(meta) + 2, h.sVarMeta);
    h.cMVTable(mvOff + pos + 1, 1) = numel(as);

    for i = 1 : numel(as)

        off = numel(h.dData) + numel(data);

        entry = zeros(ceil((2 + numel(siz2ps{i})) / 2) * 2, 1);
        entry(1) = mod(off, 2 ^ 16);
        entry(2) = floor(off / 2 ^ 16);
        entry(2 + 1 : 2 + numel(siz2ps{i})) = siz2ps{i}(:);

        meta(end + 1 : end + numel(entry), 1) = entry;
        data(end + 1 : end + numel(as{i}), 1) = as{i}(:);

    end

end

h.cVarMeta(end + 1 : end + numel(meta), 1) = meta;
h.dData   (end + 1 : end + numel(data), 1) = data;

end

%***********************************************************************************************************************

function [s, h] = ReadAllTextures(G, s, h, m, def)

h.tTexNo   = [];
h.tLayerNo = [];
h.tValueNo = [];
h.tSiz2p   = {};
h.tArray   = {};

[s, h] = UpdateGlobal(@ReadTextures1, G, s, h, m, def, 'mt');
[s, h] = UpdateGroups(@ReadTextures1, G, s, h, m, def, 'gt');
[s, h] = UpdateLayers(@ReadTextures1, G, s, h, m, def, 'lt');

h.tYOff = zeros(1, numel(h.tTexNo));
h.tXOff = zeros(1, numel(h.tTexNo));

h.tTData = cell(def.texCount, 1);

for t = 1 : def.texCount

    inds = find(h.tTexNo == t);

    [yCounts, xCounts] = cellfun(@size, h.tArray(inds));
    [tyCount, txCount, yOffs, xOffs] = cns_textile(G.pr, yCounts, xCounts);
    h.tYOff(inds) = yOffs;
    h.tXOff(inds) = xOffs;

    h.tTData{t} = zeros(tyCount, txCount, 'single');

    for i = 1 : numel(inds)
        h.tTData{t}(yOffs(i) + 1 : yOffs(i) + yCounts(i), xOffs(i) + 1 : xOffs(i) + xCounts(i)) = h.tArray{inds(i)};
    end

end

h = rmfield(h, 'tArray');

[s, h] = UpdateGlobal(@ReadTextures2, G, s, h, m, def, 'mt');
[s, h] = UpdateGroups(@ReadTextures2, G, s, h, m, def, 'gt');
[s, h] = UpdateLayers(@ReadTextures2, G, s, h, m, def, 'lt');

h = rmfield(h, {'tTexNo', 'tLayerNo', 'tValueNo', 'tSiz2p', 'tYOff', 'tXOff'});

end

%***********************************************************************************************************************

function [c, h] = ReadTextures1(G, c, h, m, d, n, cat)

for f = 1 : numel(d.cat.(cat).syms)

    name = d.cat.(cat).syms{f};

    [as, siz2ps] = GetArray(m, name, d.sym.(name), []);

    for i = 1 : numel(as)
        h.tTexNo  (end + 1) = d.sym.(name).resNo;
        h.tLayerNo(end + 1) = n;
        h.tValueNo(end + 1) = i;
        h.tSiz2p  (end + 1) = siz2ps(i);
        h.tArray  (end + 1) = as(i);
    end

end

end

%***********************************************************************************************************************

function [c, h] = ReadTextures2(G, c, h, m, d, n, cat)

meta = zeros(0, 1);

mvOff = RelOff(c.mvOff);

for f = 1 : numel(d.cat.(cat).syms)

    name = d.cat.(cat).syms{f};
    pos = 2 * d.sym.(name).pos - 1;

    inds = ((h.tTexNo == d.sym.(name).resNo) & (h.tLayerNo == n));
    vCount = sum(inds);

    h.cMVTable(mvOff + pos    , 1) = TagOff(numel(h.cVarMeta) + numel(meta) + 2, h.sVarMeta);
    h.cMVTable(mvOff + pos + 1, 1) = vCount;

    for i = 1 : vCount

        ind = find(inds & (h.tValueNo == i));

        entry = zeros(ceil((2 + numel(h.tSiz2p{ind})) / 2) * 2, 1);
        entry(1) = h.tYOff(ind);
        entry(2) = h.tXOff(ind);
        entry(2 + 1 : 2 + numel(h.tSiz2p{ind})) = h.tSiz2p{ind}(:);

        meta(end + 1 : end + numel(entry), 1) = entry;

    end

end

h.cVarMeta(end + 1 : end + numel(meta), 1) = meta;

end

%***********************************************************************************************************************

function [s, h] = ReadAllCommon(G, s, h, m, def)

h.tResNo   = [];
h.tLayerNo = [];
h.tArray   = {};

[s, h] = UpdateLayers(@ReadCommon1, G, s, h, m, def);

h.tYOff = zeros(1, numel(h.tResNo));
h.tXOff = zeros(1, numel(h.tResNo));

s.ccYSizes  = zeros(1, def.ccCount);
s.ccXCounts = zeros(1, def.ccCount);
s.cvYSizes  = zeros(1, def.cvCount);
s.cvXCounts = zeros(1, def.cvCount);

h.tCData = cell(def.ccCount, 1);
h.tVData = cell(def.cvCount, 1);

for r = 1 : def.ccCount
    inds = find(h.tResNo == -r);
    ySizes  = s.ySizes (h.tLayerNo(inds));
    xCounts = s.xCounts(h.tLayerNo(inds));
    [s.ccYSizes(r), s.ccXCounts(r), yOffs, xOffs] = cns_textile(G.pr, ySizes, xCounts);
    h.tYOff(inds) = yOffs;
    h.tXOff(inds) = xOffs;
    h.tCData{r} = zeros(s.ccYSizes(r), s.ccXCounts(r), 'single');
    for i = 1 : numel(inds)
        h.tCData{r}(yOffs(i) + 1 : yOffs(i) + ySizes(i), xOffs(i) + 1 : xOffs(i) + xCounts(i)) = h.tArray{inds(i)};
    end
end

for r = 1 : def.cvCount
    inds = find(h.tResNo == r);
    ySizes  = s.ySizes (h.tLayerNo(inds));
    xCounts = s.xCounts(h.tLayerNo(inds));
    [s.cvYSizes(r), s.cvXCounts(r), yOffs, xOffs] = cns_textile(G.pr, ySizes, xCounts);
    h.tYOff(inds) = yOffs;
    h.tXOff(inds) = xOffs;
    h.tVData{r} = zeros(s.cvYSizes(r), s.cvXCounts(r), 'single');
    for i = 1 : numel(inds)
        h.tVData{r}(yOffs(i) + 1 : yOffs(i) + ySizes(i), xOffs(i) + 1 : xOffs(i) + xCounts(i)) = h.tArray{inds(i)};
    end
end

h = rmfield(h, 'tArray');

[s, h] = UpdateLayers(@ReadCommon2, G, s, h, m, def);

h = rmfield(h, {'tResNo', 'tLayerNo', 'tYOff', 'tXOff'});

end

%***********************************************************************************************************************

function [c, h] = ReadCommon1(G, c, h, m, d, n)

for f = 1 : numel(d.cat.cc.syms)
    name = d.cat.cc.syms{f};
    h.tResNo  (end + 1) = -d.sym.(name).resNo;
    h.tLayerNo(end + 1) = n;
    h.tArray  {end + 1} = GetField(m, name, c.t, d.sym.(name));
end

for f = 1 : numel(d.cat.cv.syms)
    name = d.cat.cv.syms{f};
    h.tResNo  (end + 1) = d.sym.(name).resNo;
    h.tLayerNo(end + 1) = n;
    h.tArray  {end + 1} = GetField(m, name, c.t, d.sym.(name));
end

end

%***********************************************************************************************************************

function [c, h] = ReadCommon2(G, c, h, m, d, n)

meta = zeros(2, numel(d.list.c.syms));

for f = 1 : numel(d.cat.cc.syms)
    name = d.cat.cc.syms{f};
    pos = d.sym.(name).pos;
    ind = find((h.tResNo == -d.sym.(name).resNo) & (h.tLayerNo == n));
    meta(1, pos) = h.tYOff(ind);
    meta(2, pos) = h.tXOff(ind);
end

for f = 1 : numel(d.cat.cv.syms)
    name = d.cat.cv.syms{f};
    pos = d.sym.(name).pos;
    ind = find((h.tResNo == d.sym.(name).resNo) & (h.tLayerNo == n));
    meta(1, pos) = h.tYOff(ind);
    meta(2, pos) = h.tXOff(ind);
end

c.tOff = TagOff(numel(h.cVarMeta), h.sVarMeta);

h.cVarMeta(end + 1 : end + numel(meta), 1) = meta(:);

end

%***********************************************************************************************************************

function [s, h] = ReadAllNFields(G, s, h, m, def)

[s, h] = UpdateLayers(@ReadNFields, G, s, h, m, def);

end

%***********************************************************************************************************************

function [c, h] = ReadNFields(G, c, h, m, d, n)

c.ndOff = numel(h.dData);

data = zeros(c.ySize, c.xCount, numel(d.list.n.svSyms), 'single');

for f = 1 : numel(d.list.n.svSyms)

    name = d.list.n.svSyms{f};
    pos = d.sym.(name).pos;

    a = GetField(m, name, c.t, d.sym.(name));

    data(:, :, pos) = a;

end

mvOff = RelOff(c.mvOff);

for f = 1 : numel(d.list.n.mvSyms)

    name = d.list.n.mvSyms{f};
    pos = 2 * d.sym.(name).pos - 1;

    [a, vc] = GetField(m, name, c.t, d.sym.(name));

    h.cMVTable(mvOff + pos    , 1) = size(data, 3);
    h.cMVTable(mvOff + pos + 1, 1) = vc;

    data(:, :, end + 1 : end + vc) = a;

end

h.dData(end + 1 : end + numel(data), 1) = data(:);

end

%***********************************************************************************************************************

function [s, h] = ReadAllSynapses(G, s, h, m, def)

h.dNeurons  = zeros(0, 1, 'uint32');
h.dSynapses = zeros(0, 1, 'uint16');

[s, h] = UpdateLayers(@ReadSynapses, G, s, h, m, def);

end

%***********************************************************************************************************************

function [c, h] = ReadSynapses(G, c, h, m, d, n)

c.nmOff = numel(h.dNeurons );
c.smOff = numel(h.dSynapses) / 4;

if c.sFlag

    if ~cns_trans('sizeis', c.st, m.synapseIs)
        error('synapseIs is incorrectly sized');
    end

    if ~isfield(m, 'synapseZs')
        error('field "synapseZs" is missing');
    end
    if numel(m.synapseZs) == 1
        synapseZs = uint16(m.synapseIs ~= 0) * uint16(m.synapseZs);
    else
        if ~cns_trans('sizeis', c.st, m.synapseZs)
            error('synapseZs must be the same size as synapseIs');
        end
        synapseZs = m.synapseZs;
    end

    if isfield(m, 'synapseTs')
        if numel(m.synapseTs) == 1
            synapseTs = uint16(m.synapseIs ~= 0) * uint16(m.synapseTs);
        else
            if ~cns_trans('sizeis', c.st, m.synapseTs)
                error('synapseTs must be the same size as synapseIs');
            end
            synapseTs = m.synapseTs;
        end
    else
        synapseTs = uint16(m.synapseIs ~= 0);
    end

    [neurons, synapses] = cns_initsynapses(CB(G), n, ...
        cns_trans('pack', c.st, uint32(m.synapseIs)), ...
        cns_trans('pack', c.st, uint16(  synapseZs)), ...
        cns_trans('pack', c.st, uint16(  synapseTs)), ...
        c.ySize, G.s.ts, ...
        uint32(G.s.isType(:, d.synTypeNo)));

elseif d.synTypeNo ~= 0

    neurons  = zeros(1, c.ySize, c.xCount, 'uint32');
    synapses = zeros(0, 'uint16');

else

    neurons  = zeros(0, 'uint32');
    synapses = zeros(0, 'uint16');

end

h.dNeurons (end + 1 : end + numel(neurons ), 1) = neurons (:);
h.dSynapses(end + 1 : end + numel(synapses), 1) = synapses(:);

end

%***********************************************************************************************************************

function [s, h] = ReadAllSFields(G, s, h, m, def)

[s, h] = UpdateLayers(@ReadSFields, G, s, h, m, def);

end

%***********************************************************************************************************************

function [c, h] = ReadSFields(G, c, h, m, d, n)

c.sdOff = numel(h.dData);

if c.sFlag

    data = zeros(c.ySize, c.xCount, c.sSize, numel(d.list.s.svSyms), 'single');

    for f = 1 : numel(d.list.s.svSyms)

        name = d.list.s.svSyms{f};
        pos = d.sym.(name).pos;

        a = GetField(m, name, c.st, d.sym.(name));

        data(:, :, :, pos) = a;

    end

    mvOff = RelOff(c.mvOff);

    for f = 1 : numel(d.list.s.mvSyms)

        name = d.list.s.mvSyms{f};
        pos = 2 * d.sym.(name).pos - 1;

        [a, vc] = GetField(m, name, c.st, d.sym.(name));

        h.cMVTable(mvOff + pos    , 1) = size(data, 4);
        h.cMVTable(mvOff + pos + 1, 1) = vc;

        data(:, :, :, end + 1 : end + vc) = a;

    end

    h.dData(end + 1 : end + numel(data), 1) = data(:);

end

end

%***********************************************************************************************************************

function [s, h] = MakeLayerTable(G, s, h, m, def)

for z = 1 : numel(def.layers)
    g = def.layers{z}.g;
    s.layers{z}.gmvOff = s.groups{g}.mvOff;
    s.layers{z}.gcOff  = s.groups{g}.cOff;
end

[s, h] = UpdateLayers(@MakeLayerTableSub, G, s, h, m, def);

end

%***********************************************************************************************************************

function [c, h] = MakeLayerTableSub(G, c, h, m, d, n)

e = cns_consts('layertable', G.def.maxDims + 2);

c.entry = zeros(1, e.len);

c.entry(e.gmvOff + 1) = c.gmvOff;
c.entry(e.mvOff  + 1) = c.mvOff;
c.entry(e.gcOff  + 1) = c.gcOff;
c.entry(e.cOff   + 1) = c.cOff;
c.entry(e.tOff   + 1) = c.tOff;

siz2p = cns_trans('siz2p', c.t);

c.entry(e.siz2p + 1 : e.siz2p + numel(siz2p)) = siz2p;

h.cLayerTable(end + 1 : end + numel(c.entry), 1) = c.entry';

end

%***********************************************************************************************************************

function [s, h] = MakeKernels(G, s, h, m, def)

rs = zeros(1, 0);
ts = zeros(1, 0);
zs = cell (1, 0);
for z = 1 : numel(def.layers)
    if ~def.layers{z}.kernel, continue; end
    if isfield(m.layers{z}, 'runOrder')
        r = m.layers{z}.runOrder(:)';
        if isempty(r), error('z=%u: invalid runOrder', z); end
        if numel(unique(r)) ~= numel(r), error('z=%u: runOrder values must be unique', z); end
    else
        r = 1;
    end
    t = def.layers{z}.typeNo;
    for i = 1 : numel(r)
        j = find((rs == r(i)) & (ts == t), 1);
        if isempty(j)
            rs(end + 1) = r(i);
            ts(end + 1) = t;
            zs{end + 1} = z;
        else
            zs{j}(end + 1) = z;
        end
    end
end
if ~isequal(unique(rs), 1 : max(rs)), error('runOrder values must be contiguous'); end
[ans, inds] = sortrows([rs' ts']);
rs = rs(inds);
ts = ts(inds);
zs = zs(inds);

h.dBlocks   = zeros(0, 1, 'uint16');
h.hKernelZs = zeros(0, 1, 'uint32');

for k = 1 : numel(rs)

    d = def.layers{zs{k}(1)};
    c = struct;

    c.type      = d.type;
    c.typeNo    = d.typeNo;
    c.runOrder  = rs(k);
    c.blockSize = d.blockYSize * d.blockXSize;

    c.bOff   = numel(h.dBlocks) / 4;
    c.bCount = 0;

    for z = zs{k}

        blockYSize = s.layers{z}.blockYSize;
        blockXSize = c.blockSize / blockYSize;

        yCount0 = s.layers{z}.yCount0;
        ySize0  = s.layers{z}.ySize0;
        yCount  = s.layers{z}.yCount;
        xCount  = s.layers{z}.xCount;

        xs = 0 : blockXSize : xCount - 1;
        ys = 0 : blockYSize : yCount - 1;

        if yCount0 == ySize0
            yc = min(blockYSize, yCount - ys);
        else
            yc = min(blockYSize, yCount0 - mod(ys, ySize0));
        end

        [xsGrid, ysGrid] = meshgrid(xs, ys);
        ycGrid = repmat(yc', 1, numel(xs));

        c.bCount = c.bCount + numel(xsGrid);

        blocks = zeros(4, numel(xsGrid), 'uint16');
        blocks(1, :) = reshape(xsGrid, 1, []);
        blocks(2, :) = reshape(ysGrid, 1, []);
        blocks(3, :) = z - 1;
        blocks(4, :) = reshape(ycGrid, 1, []);

        h.dBlocks(end + 1 : end + numel(blocks), 1) = blocks(:);

    end

    c.zOff   = numel(h.hKernelZs);
    c.zCount = numel(zs{k});

    h.hKernelZs(end + 1 : end + numel(zs{k}), 1) = zs{k}(:) - 1;

    s.kernels{k} = c;

end

end

%***********************************************************************************************************************

function [s, h] = Finalize(G, s, h)

s.groups  = [s.groups{:} ];
s.layers  = [s.layers{:} ];
s.kernels = [s.kernels{:}];

offs = cumsum([numel(h.cLayerTable), numel(h.cMVTable)]);

s = Tag2Abs(s, offs);

s.cMeta = Tag2Abs([h.cLayerTable; h.cMVTable; h.cVarMeta], offs);
if any(s.cMeta > intmax('uint16'))
    error('metadata value exceeds %u', intmax('uint16'));
end
h.cMeta = uint16(s.cMeta);
h = rmfield(h, {'cLayerTable', 'sLayerTable'});
h = rmfield(h, {'cMVTable'   , 'sMVTable'   });
h = rmfield(h, {'cVarMeta'   , 'sVarMeta'   });

end

%***********************************************************************************************************************

function blockYSize = OptimizeBlock(blockYSize, blockXSize, blockYSizeAlign, yCountOpt, xCount)

if blockXSize > xCount

    blockSize = blockYSize * blockXSize;

    blockXSize = xCount;
    while mod(blockSize, blockYSizeAlign * blockXSize) ~= 0
        blockXSize = blockXSize + 1;
    end

    blockYSize = round(blockSize / blockXSize);

elseif blockYSize > yCountOpt

    blockSize = blockYSize * blockXSize;

    blockYSize = ceil(yCountOpt / blockYSizeAlign) * blockYSizeAlign;
    while mod(blockSize, blockYSize) ~= 0
        blockYSize = blockYSize + blockYSizeAlign;
    end

end

end

%***********************************************************************************************************************

function tag = TagOff(rel, seg)

if seg == 0
    tag = rel;
else
    tag = -(seg * 1e10 + rel);
end

end

%***********************************************************************************************************************

function [rel, seg] = RelOff(tag)

if tag < 0
    seg = floor(-tag / 1e10);
    rel = -tag - seg * 1e10;
else
    seg = 0;
    rel = tag;
end

end

%***********************************************************************************************************************

function a = Tag2Abs(a, offs)

if isnumeric(a)

    inds = find(a < 0);
    for i = 1 : numel(inds)
        j = inds(i);
        [rel, seg] = RelOff(a(j));
        a(j) = offs(seg) + rel;
    end

else

    names = fieldnames(a);
    for i = 1 : numel(a)
        for j = 1 : numel(names)
            b = a(i).(names{j});
            if (isnumeric(b) && any(b(:) < 0)) || isstruct(b)
                a(i).(names{j}) = Tag2Abs(b, offs);
            end
        end
    end

end

end

%***********************************************************************************************************************

function [a, vc] = GetField(m, field, t, d)

% Returns a fully padded (siz4b) result (or a scalar).

if isfield(m, field)
    a = m.(field);
    if ~isnumeric(a)
        error('field "%s" must be numeric', field);
    end
elseif ~isequalwithequalnans(d.value, NaN)
    a = d.value;
else
    error('field "%s" is missing', field);
end

if d.int == -2
    a = a - 1;
end

a = single(a);

if isempty(t)

    vc = numel(a);

    if d.multi
        if (ndims(a) ~= 2) || all(size(a) > 1)
            error('field "%s" must be a scalar or vector', field);
        end
        a = a(:);
    else
        if ~isscalar(a)
            error('field "%s" must be a scalar', field);
        end
    end

else

    if d.multi
        vc = min(size(a, 1), numel(a));
        t = cns_trans('add', t, vc);
    else
        vc = 1;
    end

    if isempty(a) && (ndims(a) == 2)
        if ~d.multi
            error('field "%s" cannot be empty', field);
        end
        a = reshape(a, t.siz4b);
    elseif isscalar(a)
    elseif cns_trans('sizeis', t, a)
        a = cns_trans('pack', t, a, true);
    else
        if d.multi
            error('field "%s" must be a scalar, empty, or have size N%s', ...
                field, sprintf('x%u', t.siz1(2 : end)));
        else
            error('field "%s" must be a scalar or have size %u%s', ...
                field, t.siz1(1), sprintf('x%u', t.siz1(2 : end)));
        end
    end

end

end

%***********************************************************************************************************************

function [as, siz2ps] = GetArray(m, field, d, align)

% If align is nonempty, returns fully padded (siz4b) results.  Otherwise results are completely unaligned.

if isfield(m, field)
    as = m.(field);
    if d.multi
        if ~iscell(as), error('"%s" must be a cell array', field); end
        if ~all(cellfun(@isnumeric, as)), error('elements of "%s" must be numeric', field); end
        as = as(:);
    else
        if ~isnumeric(as), error('"%s" must be numeric', field); end
        as = {as};
    end
else
    error('field "%s" is missing', field);
end

if d.int == -2
    for i = 1 : numel(as)
        as{i} = as{i} - 1;
    end
end

if isfield(m, [field '_size'])
    sizes = m.([field '_size'])(:)';
    if d.multi
        if ~iscell(sizes), error('"%s_size" must be a cell array', field); end
        if numel(sizes) ~= numel(as)
            error('"%s_size" must have the same number of elements as "%s"', field, field);
        end
    else
        sizes = {sizes};
    end
else
    for i = 1 : numel(as)
        sizes{i} = size(as{i});
        if (numel(sizes{i}) == 2) && (sizes{i}(2) == 1), sizes{i} = sizes{i}(1); end
        sizes{i}(end + 1 : numel(d.dims)) = 1;
    end
end

for i = 1 : numel(as)

    try
        t = cns_trans('create', struct('size', sizes(i)), d, align);
    catch
        error('"%s": %s', field, cns_error);
    end

    if ~cns_trans('sizeis', t, as{i}), error('"%s" does not match its stated size', field); end

    as    {i} = cns_trans('pack' , t, as{i}, true);
    siz2ps{i} = cns_trans('siz2p', t);

end

end

%***********************************************************************************************************************

function CheckPointer(isType, name, zs, ptrTypeNo, zeroOK)

% Note: zs are zero-based when this is called.

for i = 1 : numel(zs)

    z = zs(i);

    if zeroOK && (z == -1), continue; end

    if (z < 0) || (z >= size(isType, 1)) || (mod(z, 1) ~= 0)
        error('field "%s": invalid layer number', name);
    end

    if ~isType(z + 1, ptrTypeNo)
        error('field "%s": layer number %u is the wrong type', name, z + 1);
    end

end

end

%***********************************************************************************************************************

function varargout = Run(G, totalSteps, varargin)

if ~G.sessionOpen, error('no session open'); end

if nargin < 2
    totalSteps = 1;
else
    if ~isnumeric(totalSteps), error('invalid number of steps'); end
    totalSteps = round(totalSteps);
    if totalSteps < 0, error('invalid number of steps'); end
end

args = varargin;

if ~isempty(args) && isnumeric(args{1})
    sampleRate = args{1};
    args = args(2 : end);
    if sampleRate < 1, error('invalid sample rate'); end
else 
    sampleRate = 1;
end

if ~isempty(args) && isnumeric(args{1})
    bufferSize = args{1};
    args = args(2 : end);
    if bufferSize < 1, error('invalid buffer size'); end
else
    bufferSize = max(totalSteps, 1);
end

if ~isempty(args) && ~iscell(args{1})
    error('"get" parameters must be given as cell arrays');
end

p = GetAllParams(G, 'r', args{:});

if nargout ~= numel(p), error('%u outputs needed', numel(p)); end

if isempty(p)

    G.func(CB(G), 4, totalSteps, totalSteps + 1, []);

else

    for i = 1 : max(ceil(totalSteps / (sampleRate * bufferSize)), 1)

        steps = min(sampleRate * bufferSize, totalSteps - (i - 1) * sampleRate * bufferSize);

        [outs{1 : nargout}] = G.func(CB(G), 4, steps, sampleRate, p);

        for j = 1 : numel(p)
            outs{j} = GetFinalize(p(j), outs{j}, true);
        end

        if i == 1
            varargout = outs;
        else
            for j = 1 : numel(p)
                varargout{j} = cat(1, varargout{j}, outs{j});
            end
        end

    end

end

end

%***********************************************************************************************************************

function varargout = Get(G, varargin)

if ~G.sessionOpen, error('no session open'); end

p = GetAllParams(G, 'g', varargin{:});

if nargout ~= numel(p), error('%u outputs needed', numel(p)); end

for i = 1 : numel(p)
    varargout{i} = G.func(CB(G), 5, p(i));
    varargout{i} = GetFinalize(p(i), varargout{i}, false);
end

end

%***********************************************************************************************************************

function m = Update(G, m)

if ~G.sessionOpen, error('no session open'); end
if nargin < 2, error('not enough arguments'); end

for z = 1 : numel(G.def.layers)

    d = G.def.layers{z};
    fieldNames = [d.cat.cv.syms, d.cat.nv.syms, d.cat.sv.syms];

    for i = 1 : numel(fieldNames)

        p = GetParams(G, 'g', z, fieldNames{i});

        res = G.func(CB(G), 5, p);

        res = GetFinalize(p, res, false);
        
        m.layers{z}.(fieldNames{i}) = res;

    end

end

end

%***********************************************************************************************************************

function Set(G, varargin)

if ~G.sessionOpen, error('no session open'); end

[p, a] = GetAllParams(G, 's', varargin{:});

for i = 1 : numel(p)
    G.func(CB(G), 6, p(i), a{i});
end

end

%***********************************************************************************************************************

function [p, a] = GetAllParams(G, op, varargin)

if ~isempty(varargin) && ~iscell(varargin{1})
    args = {varargin};
else
    args = varargin;
end

p = [];
a = cell(1, numel(args));

for i = 1 : numel(args)

    if ~iscell(args{i}), error('invalid argument'); end

    if i == 1
        [p, a{i}] = GetParams(G, op, args{i}{:});
    else
        [p(i), a{i}] = GetParams(G, op, args{i}{:});
    end

end

end

%***********************************************************************************************************************

function [p, a] = GetParams(G, op, z, fieldName, varargin)

if nargin < 4, error('incorrect number of arguments'); end

args = varargin;

if op == 's'
    if isempty(args), error('z=%u, field "%s": incorrect number of arguments', z, fieldName); end
    a = args{end};
    args = args(1 : end - 1);
    if ~isnumeric(a), error('z=%u, field "%s": invalid value', z, fieldName); end
    a = single(a);
else
    a = single([]);
end

if z == 0
    c = G.s;
    d = G.def.sym.(fieldName);
else
    c = G.s.layers(z);
    d = G.def.layers{z}.sym.(fieldName);
end

switch d.cat
case {'mc', 'gc', 'lc'}, p = GetParams_XC(G, op, z, fieldName, c, d, args{:});
case 'cv'              , p = GetParams_CV(G, op, z, fieldName, c, d, args{:});
case {'nc', 'nv'}      , p = GetParams_N (G, op, z, fieldName, c, d, args{:});
case {'sc', 'sv'}      , p = GetParams_S (G, op, z, fieldName, c, d, args{:});
otherwise
    error('z=%u, field "%s": cannot get/set this field', z, fieldName);
end

if op == 's'
    t = p.t;
    if numel(a) == 1
        a = repmat(a, t.siz4a);
    elseif cns_trans('sizeis', t, a)
        a = cns_trans('pack', t, a);
    elseif isequal(find(t.siz1 ~= 1), 1) && isequal(size(a), [1, t.siz1(1)])
        a = cns_trans('pack', t, a);
    else
        error('z=%u, field "%s": value must be a scalar or have size %u%s', ...
            z, fieldName, t.siz1(1), sprintf('x%u', t.siz1(2 : end)));
    end
end

end

%***********************************************************************************************************************

function p = GetParams_XC(G, op, z, fieldName, c, d, varargin)

if op == 'r'
    error('z=%u, field "%s": cannot retrieve "%s" fields in run mode', z, fieldName, d.cat);
end

q = GetCoords(G, z, fieldName, c, d, false, false, varargin{:});

if ~d.multi
    switch d.cat
    case 'mc', q.fo = q.fo + c.cOff;
    case 'gc', q.fo = q.fo + c.gcOff;
    case 'lc', q.fo = q.fo + c.cOff;
    end
end

p.varType = 3;
p.z       = z;
p.pos     = 1;
p.height  = 1;
p.width   = 1;
p.hOff    = q.fo;
p.wOff    = 0;
p.dOff    = 0;
p.hCount  = q.fc;
p.wCount  = 1;
p.dCount  = 1;
p.t       = q.t;
p.int     = d.int;

end

%***********************************************************************************************************************

function p = GetParams_CV(G, op, z, fieldName, c, d, varargin)

q = GetCoords(G, z, fieldName, c, d, false, true, varargin{:});

p.varType = 0;
p.z       = z;
p.pos     = d.resNo;
p.height  = G.s.cvYSizes (d.resNo);
p.width   = G.s.cvXCounts(d.resNo);
p.hOff    = q.yo + G.s.cMeta(c.tOff + 2 * d.resNo - 1);
p.wOff    = q.xo + G.s.cMeta(c.tOff + 2 * d.resNo);
p.dOff    = 0; % TODO: we ignore "pos".  Should really clean this up.
p.hCount  = q.yc;
p.wCount  = q.xc;
p.dCount  = 1;
p.t       = q.t;
p.int     = d.int;

end

%***********************************************************************************************************************

function p = GetParams_N(G, op, z, fieldName, c, d, varargin)

if (op == 'r') && strcmp(d.cat, 'nc')
    error('z=%u, field "%s": cannot retrieve "%s" fields in run mode', z, fieldName, d.cat);
end

q = GetCoords(G, z, fieldName, c, d, false, true, varargin{:});

p.varType = 1;
p.z       = z;
p.pos     = 1;
p.height  = c.ySize;
p.width   = c.xCount;
p.hOff    = q.yo;
p.wOff    = q.xo;
p.dOff    = q.fo;
p.hCount  = q.yc;
p.wCount  = q.xc;
p.dCount  = q.fc;
p.t       = q.t;
p.int     = d.int;

end

%***********************************************************************************************************************

function p = GetParams_S(G, op, z, fieldName, c, d, varargin)

if (op == 'r') && strcmp(d.cat, 'sc')
    error('z=%u, field "%s": cannot retrieve "%s" fields in run mode', z, fieldName, d.cat);
end

q = GetCoords(G, z, fieldName, c, d, true, true, varargin{:});

if (q.fc > 1) && (q.sc ~= c.sSize)
    error('z=%u, field "%s": cannot select multiple values without selecting all synapses', z, fieldName);
end

p.varType = 2;
p.z       = z;
p.pos     = 1;
p.height  = c.ySize;
p.width   = c.xCount;
p.hOff    = q.yo;
p.wOff    = q.xo;
p.dOff    = q.fo * c.sSize + q.so;
p.hCount  = q.yc;
p.wCount  = q.xc;
p.dCount  = q.fc * q.sc;
p.t       = q.t;
p.int     = d.int;

end

%***********************************************************************************************************************

function q = GetCoords(G, z, fieldName, c, d, hasSyns, hasYX, varargin)

args = varargin;

if d.multi
    pos = c.mvOff + 2 * d.pos - 1;
    fo = G.s.cMeta(pos);
    fc = G.s.cMeta(pos + 1);
    if ~isempty(args)
        e = GetCoordRange(args{1}, fc);
        if isempty(e)
            error('z=%u, field "%s": invalid value number', z, fieldName);
        end
        args = args(2 : end);
        q.fo = fo + e(1) - 1;
        q.fc = e(2) - e(1) + 1;
    else
        q.fo = fo;
        q.fc = fc;
    end
else
    q.fo = d.pos - 1;
    q.fc = 1;
end

if hasSyns
    if ~isempty(args)
        e = GetCoordRange(args{1}, c.sSize);
        if isempty(e)
            error('z=%u, field "%s": invalid synapse number', z, fieldName);
        end
        args = args(2 : end);
        q.so = e(1) - 1;
        q.sc = e(2) - e(1) + 1;
    else
        q.so = 0;
        q.sc = c.sSize;
    end
end

if hasYX
    if ~isempty(args)
        [y, x] = cns_trans('e2yx', c.t, cns_iconv(G.m, z, args{:}));
        args = {};
        q.yo = y - 1;
        q.xo = x - 1;
        q.yc = 1;
        q.xc = 1;
        q.t  = cns_trans('scalar');
    else
        q.yo = 0;
        q.xo = 0;
        q.yc = c.yCount;
        q.xc = c.xCount;
        q.t  = c.t;
    end
else
    q.t = cns_trans('scalar');
end

if ~isempty(args)
    error('z=%u, field "%s": incorrect number of arguments', z, fieldName);
end

if hasSyns, q.t = cns_trans('add', q.t, q.sc); end
if d.multi, q.t = cns_trans('add', q.t, q.fc); end

end

%***********************************************************************************************************************

function e = GetCoordRange(a, n)

e = [];

if ~isnumeric(a) || (numel(a) > 2), return; end
a = double(a);
if any(mod(a, 1) ~= 0), return; end

if isempty(a) || isequal(a, 0)
    e = [1 n];
elseif isscalar(a)
    if (a < 1) || (a > n), return; end
    e = [a a];
else
    if (a(1) < 1       ) || (a(1) > n + 1), return; end
    if (a(2) < a(1) - 1) || (a(2) > n    ), return; end
    e = a(:)';
end

end

%***********************************************************************************************************************

function res = GetFinalize(p, res, time)

t = p.t;

if time, t = cns_trans('add', t, size(res, 2)); end

res = cns_trans('unpack', t, reshape(res, t.siz4a));

if p.int == -2
    res = res + 1;
end

end

%***********************************************************************************************************************

function cb = CB(G)

cb = @Callback;

function varargout = Callback(cbname, varargin)

cbfunc = str2func(cbname);
[varargout{1 : nargout}] = cbfunc(G, varargin{:});

end

end

%***********************************************************************************************************************

function e = CBYX2E(G, z, y, x)

e = cns_trans('yx2e', G.s.layers(z).t, y, x);

end
