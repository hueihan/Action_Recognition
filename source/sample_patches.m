function d = sample_patches(setting_file,Training_set)

       eval(setting_file);
       rand('state',sum(100*clock));
       naction = length(saction);
      
       %*******************************************************************************************
       % Define the model.
       %*******************************************************************************************

       m = feval(model_file,Iparam1,[],s1Par,c1Par,[],[]); 


       % Create an empty dictionary for S2.
       d = struct('fVals', single([]),'fSizes',[]);
       % Initialize the model on the GPU.
       cns('init', m, 'gpu');
       ipatch = 1;
       while 1       
	   fprintf('.');
	   % actually drawn patches
	   n = patchSizes(randi(length(patchSizes)));
           action_ind = randi(length(Training_set));
	   file_ind   = randi(length(Training_set{action_ind})); % file_ind 
	   srcfname = sprintf('%s/%s/%s',srcdir,saction{action_ind},Training_set{action_ind}{file_ind});
	
 % ===========================================================
	   [H, W, nframes] = get_video_info(srcfname);
	   clear openCVread
	   % get all the frames at once 
	   ALLV = single(zeros(nframes,Y,X));
	   i = 1;
	   while 1
	       tmp = single(openCVread(srcfname));  
	       if size(tmp,1)==1;
                   ALLV(i:end,:,:) = [];
                   nframes = size(ALLV,1);  
                   break;
               end
               if size(tmp,1)~=Y | size(tmp,2)~=X
                   tmp = imresize(tmp,[Y X],'nearest');
               end
	       tmp = tmp -  min(tmp(:));
	       tmp = tmp ./ max(tmp(:));		
	       ALLV(i,:,:) = tmp;
	       i = i + 1;
	   end
% =============================================================
	   frame_ind = randi(nframes-nt); 
	   cns('set', 1, 'val', shiftdim(ALLV(frame_ind+[0:nt-1],:,:),-1));  
	   cns('run');	   
	   c1 = squeeze(cns('get', c1stage_num, 'val'));  

           di = hjpkg_sample(n,thre_for_sample_c1,c1);
	   d = hjpkg_combine(d, di);
           ipatch = ipatch + 1;
       
           if ipatch==npatches + 1
               break;
           end
      end % end of ipatch
      cns('done');

function i = randi(n)
  rand('state',sum(100*clock));
  i = max(1, ceil(rand * n));


