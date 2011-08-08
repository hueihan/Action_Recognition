  dataset_name = 'WIS';
  isplit = 1;
  srcdir = sprintf('data/%s/video',dataset_name);
  dstdir = sprintf('data/%s/feature_split_%d',dataset_name,isplit);
  if ~exist(dstdir)
    mkdir(dstdir);
  end
  % secific for WIS
  splitdir = [];
  saction = {'bend','jack','jump','pjump','run','side','skip','walk','wave1','wave2'};
  Y           = 144;
  X           = 180;
  T           = 20;

  % can be changed
  iscale   = 1; 
  ndir     = 4;
  speed    = 1;

  npatches = 100;
  patchSizes        = [8,16];
  thre_for_sample_c1 = 0.3;

  % fixed settings
  model_file = @hj_model_multiscale;
  s1siz = [5,7,9,11,13,15,17,19,21,23,25,27];
  nt = [9];
  c1siz = [6,8,10,12,14,16];  
  c1step = [3,4,5,6,7,8];
  overlap = nt-1;
  
  s1scales{1}    = 1:4;
  s1scales{2}    = 5:8;
  s1scales{3}    = 9:12;
  c1scales{1} = 1:2;
  c1scales{2} = 3:4;
  c1scales{3} = 5:6;
  
  c1stage_num = 1+4+4+4+1;
  
  c2stage_num = 1+4+4+4+2+2+1;
   	
  s1Par = ...
      struct('Ssiz',s1siz(s1scales{iscale}),'ndir',ndir,'speed',speed,'symmetry',[0],'Tsiz',nt*ones(4,1),'nf1const',0.1,'filter_type','gabor','thres',0.2);
  c1Par = struct('PoolSiz',c1siz(c1scales{iscale}),'PoolStepSiz',c1step(c1scales{iscale}),'PoolScaleSiz',2,'PoolScaleStepSiz',2);  
     
  s2Par = struct('exponent',1,'thres',0.001);

  c2Par = struct('PoolSiz',200,'PoolStepSiz',1,'PoolScaleSiz',2,'PoolScaleStepSiz',1);

  % for computing c2 of a clip
  Iparam.size = [T,Y,X,1];  
  % for extracting patches
  Iparam1.size = [nt,Y,X,1];

