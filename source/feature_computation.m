function feature_computation(setting_file)

         eval(setting_file);
       %****************************************************************************************************
       % Find training indices and sample c1 patches
       %**************************************************************************************************** 
         [Training_set, Testing_set]= feval(sprintf('get_%s_split',dataset_name),isplit,splitdir);
         patches_fname = [dstdir '/patches.mat'];
	 fprintf('extracting patches\n');
         % Build the full CNS network model. 
         if ~exist(patches_fname)
             d = sample_patches(setting_file,Training_set);
	     save(patches_fname,'d');
	 else
             load(patches_fname,'d')
         end
       %**************************************************************************************************
       % Compute feature vectors for videos.
       %**************************************************************************************************
  
         d = hjpkg_sparsify(d);
       
         % Build the full CNS network model. 
	 m = feval(model_file,Iparam,d,s1Par,c1Par,s2Par,c2Par); 

	 % Initialize the model on the GPU.
	 cns('init', m, 'gpu');
	 fprintf('computing c2\n');
        
	 for iaction = 1:length(saction)
	     action = saction{iaction}
	     if ~exist(sprintf('%s/%s',dstdir,action))
               mkdir(sprintf('%s/%s',dstdir,action));
             end		  
	     sets = [Training_set{iaction} Testing_set{iaction}];
	  
	     for ifile = 1:length(sets)
	         srcfname = sprintf('%s/%s/%s',srcdir,action,sets{ifile});
	         dstfname = sprintf('%s/%s/C2_%s.mat',dstdir,action,sets{ifile}(1:end-4))	    
	         if exist(dstfname)
                     continue;
                 end
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

                 sind = [1:T-overlap:nframes-overlap];
	         tind = min(nframes,sind+T-1);
	         Lind = tind - sind + 1;
	         V = single(-255*ones(1,T,Y,X));

	         c2 = single(zeros(npatches,nframes-overlap));
	         c2_sind = sind;
	         c2_tind = (tind-overlap);
	         c2_Lind = c2_tind-c2_sind+1;
	         for i = 1:length(sind)
		     V(1,1:Lind(i),:,:) = shiftdim(ALLV(sind(i):tind(i),:,:),-1);	      
		     cns('set', 1, 'val', V);  		
		     % Compute the feature hierarchy for the image.
		     cns('run');
		     % Retrieve the contents of the C2 layer.
		     tmp = cns('get', c2stage_num, 'val');
		     c2(:,c2_sind(i):c2_tind(i)) = tmp(:,1:c2_Lind(i));
	         end % end of ~eof
	         c2(c2 == cns_fltmin) = 0;
	         save(dstfname,'c2');
	     end % end of ifile
	 end % end of iaction
	  
	 % Release GPU resources.
	 cns('done');
