function cns_path

path = fileparts(mfilename('fullpath'));

addpath(fullfile(path, 'util'));
addpath(path);

return;
