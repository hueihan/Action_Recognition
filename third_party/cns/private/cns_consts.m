function c = cns_consts(mode, varargin)

switch mode
case 'layertable', c = LayerTable(varargin{:});
otherwise        , error('invalid mode');
end

return;

%***********************************************************************************************************************

function c = LayerTable(maxSiz2pLen)

c.len    = ceil((5 + maxSiz2pLen) / 2) * 2;
c.gmvOff = 0;
c.mvOff  = 1;
c.gcOff  = 2;
c.cOff   = 3;
c.tOff   = 4;
c.siz2p  = 5;

return;