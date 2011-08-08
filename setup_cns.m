function setup_cns

%compile cns
addpath(genpath('third_party/cns'));
cns_install;
% compile cns files
addpath('source/hjpkg');
disp('compiling CNS');
cns_build('hjpkg');

