function m = cns_mapdim(m, z, dimID, method, varargin)

def = cns_def(m);

d2 = FindDim(def, z, dimID);
name = def.layers{z}.dnames{d2};

switch method
case 'copy'        , m = MapCopy        (m, def, z, name, d2, varargin{:});
case 'pixels'      , m = MapPixels      (m, def, z, name, d2, varargin{:});
case 'scaledpixels', m = MapScaledPixels(m, def, z, name, d2, varargin{:});
case 'int'         , m = MapInt         (m, def, z, name, d2, varargin{:});
case 'win'         , m = MapWin         (m, def, z, name, d2, varargin{:});
otherwise          , error('invalid method');
end

return;

%***********************************************************************************************************************

function m = MapCopy(m, def, z, name, d2, pz)

d1 = FindDim(def, pz, name);

m.layers{z}.size{d2}          = m.layers{pz}.size{d1};
m.layers{z}.([name '_start']) = m.layers{pz}.([name '_start']);
m.layers{z}.([name '_space']) = m.layers{pz}.([name '_space']);

return;

%***********************************************************************************************************************

function m = MapPixels(m, def, z, name, d2, imSize)

if (imSize < 1) || (mod(imSize, 1) ~= 0)
    error('imSize must be a positive integer');
end

m.layers{z}.size{d2}          = imSize;
m.layers{z}.([name '_start']) = 0.5 / imSize;
m.layers{z}.([name '_space']) = 1 / imSize;

return;

%***********************************************************************************************************************

function m = MapScaledPixels(m, def, z, name, d2, baseSize, factor)

if (baseSize < 1) || (mod(baseSize, 1) ~= 0)
    error('baseSize must be a positive integer');
end
if factor < 1
    error('factor must be at least 1');
end

nSpace = factor / baseSize;

if mod(baseSize, 2) == 1
    nSize = 2 * floor((1 - nSpace) / (2 * nSpace)) + 1;
else
    nSize = 2 * floor(1 / (2 * nSpace));
end

if nSize < 1
    nSize  = 0;
    nStart = 0.5;
else
    nStart = 0.5 - nSpace * (nSize - 1) / 2;
end

m.layers{z}.size{d2}          = nSize;
m.layers{z}.([name '_start']) = nStart;
m.layers{z}.([name '_space']) = nSpace;

return;

%***********************************************************************************************************************

function m = MapInt(m, def, z, name, d2, pz, rfSize, rfStep)

% rfSize = unit width in previous layer units.
% rfStep = unit step size in previous layer units.

d1 = FindDim(def, pz, name);

if rfSize >= cns_intmax

    m.layers{z}.size{d2}          = 1;
    m.layers{z}.([name '_start']) = 0.5;
    m.layers{z}.([name '_space']) = 1;

    return;

end

if (rfSize < 1) || (mod(rfSize, 1) ~= 0)
    error('rfSize must be a positive integer');
end
if (rfStep < 1) || (mod(rfStep, 1) ~= 0)
    error('rfStep must be a positive integer');
end

pSize  = m.layers{pz}.size{d1};
pStart = m.layers{pz}.([name '_start']);
pSpace = m.layers{pz}.([name '_space']);

nSpace = pSpace * rfStep;

if mod(pSize, 2) == mod(rfSize, 2)

    % We can place a unit in the center.  The result will have an odd number of units.  Note that if rfStep is even,
    % we would also have the option of making the result have an even number of units, as in the "else" case, but we
    % ignore this possibility.

    nSize = 2 * floor((pSize - rfSize) / (2 * rfStep)) + 1;

else

    % We cannot place a unit in the center, so the result will have an even number of units, and we must place a unit
    % on either side of the center, at the same distance from the center.  This is only possible if rfStep is odd.
    % This really requires a diagram to see.  There are two cases to consider: pSize odd, rfSize even and vice-versa.

    if mod(rfStep, 2) == 0
        error('when the result layer has an even number of units, rfStep must be odd');
    end

    nSize = 2 * floor((pSize - rfSize - rfStep) / (2 * rfStep)) + 2;

end

if nSize < 1
    nSize  = 0;
    nStart = 0.5;
else
    nStart = pStart + (pSpace * (pSize - 1) - nSpace * (nSize - 1)) / 2;
end

m.layers{z}.size{d2}          = nSize;
m.layers{z}.([name '_start']) = nStart;
m.layers{z}.([name '_space']) = nSpace;

return;

%***********************************************************************************************************************

function m = MapWin(m, def, z, name, d2, pz, rfSize, rfStep, rfMargin)

% rfSize   = window width in previous layer units (can be fractional).
% rfStep   = window step size in previous layer units (can be fractional).
% rfMargin = size of margin in previous layer units (can be fractional and/or negative).

if rfSize >= cns_intmax

    m.layers{z}.size{d2}          = 1;
    m.layers{z}.([name '_start']) = 0.5;
    m.layers{z}.([name '_space']) = 1;

    return;

end

if rfSize <= 0
    error('rfSize must be positive');
end
if rfStep <= 0
    error('rfStep must be positive');
end

pSpace = m.layers{pz}.([name '_space']);

rfSize   = rfSize   * pSpace;
rfStep   = rfStep   * pSpace;
rfMargin = rfMargin * pSpace;

nSize = 1 + 2 * floor((1 - rfSize - 2 * rfMargin) / (2 * rfStep));

if nSize < 1
    nSize  = 0;
    nStart = 0.5;
else
    nStart = 0.5 - rfStep * (nSize - 1) / 2;
end

m.layers{z}.size{d2}          = nSize;
m.layers{z}.([name '_start']) = nStart;
m.layers{z}.([name '_space']) = rfStep;

return;

%***********************************************************************************************************************

function dim = FindDim(def, z, dimID)

if isnumeric(dimID)
    dim = dimID;
else
    if isempty(dimID), error('invalid dimension name'); end
    [ans, dim] = ismember(dimID, def.layers{z}.dnames);
    if dim == 0, error('layer %u does not have dimension "%s"', z, dimID); end
end

if ~def.layers{z}.dmap(dim)
    error('dimension %u is not mapped for layer %u', dim, z);
end

return;