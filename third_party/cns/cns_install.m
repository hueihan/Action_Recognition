function cns_install

base = fileparts(mfilename('fullpath'));

mex('-outdir', fullfile(base, 'private'), fullfile(base, 'source', 'cns_initsynapses.cpp'));
mex('-outdir', fullfile(base, 'private'), fullfile(base, 'source', 'cns_limits.cpp'));

if ~exist(fullfile(base, 'util', 'private'), 'dir'), mkdir(fullfile(base, 'util', 'private')); end

mex('-outdir', fullfile(base, 'util', 'private'), fullfile(base, 'source', 'cns_spikeutil.cpp'));

rehash path;

return;