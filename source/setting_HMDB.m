  dataset_name = 'HMDB';
  isplit = 1;
  srcdir = sprintf('data/%s/video',dataset_name);
  dstdir = sprintf('data/%s/feature_split_%d',dataset_name,isplit);
  if ~exist(dstdir)
    mkdir(dstdir);
  end
  % secific for HMDB
  splitdir = sprintf('data/%s/testTrainMulti_7030_splits',dataset_name);
  saction =      {'brush_hair','cartwheel','catch','chew','clap','climb','climb_stairs',...
      'dive','draw_sword','dribble','drink','eat','fall_floor','fencing',...
      'flic_flac','golf','handstand','hit','hug','jump','kick_ball',...
      'kick','kiss','laugh','pick','pour','pullup','punch',...
      'push','pushup','ride_bike','ride_horse','run','shake_hands','shoot_ball',...
      'shoot_bow','shoot_gun','sit','situp','smile','smoke','somersault',...
      'stand','swing_baseball','sword_exercise','sword','talk','throw','turn',...
      'walk','wave'};


  Y           = 240;
  X           = 320;
  T           = 10;
  
  % can be changed
  iscale   = 2; 
  ndir     = 4;
  speed    = [1,2];

  npatches = 1000;
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


