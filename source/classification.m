   function acc = classification(setting_file) 

    eval(setting_file);
    % dataset specification
    c = '1000';
    e = '0.5';
    
    data_fname  = sprintf('%s/train.dat',dstdir);
    test_fname  = sprintf('%s/test.dat',dstdir); 
    model_fname = sprintf('%s/modelodel',dstdir);  
    outfile     = sprintf('%s/pred.tags',dstdir);
    ID_fname    = sprintf('%s/id.mat',dstdir);
    %****************************************************************************************************
    % Find training indices and sample c1 patches
    %**************************************************************************************************** 
      [Training_set, Testing_set]= feval(sprintf('get_%s_split',dataset_name),isplit,splitdir);
      naction = length(saction);
    %****************************************************************************************************
    % collecting training data
    %**************************************************************************************************** 
      if ~exist(data_fname) & ~exist(model_fname)
           icount = 1;
	   for iaction = 1:naction
	       action = saction{iaction};
	       for ifile = 1:length(Training_set{iaction})
	           fprintf('.'); 
		   fnamec2 = sprintf('%s/%s/C2_%s.mat',dstdir,action,Training_set{iaction}{ifile}(1:end-4));
		   load(fnamec2);	
		   c2 = double(c2);
		   c2 = c2./repmat((sum(c2.^2,1).^0.5),[size(c2,1) 1]); 
	           y = iaction*ones(size(c2,2),1);
	           to_svm_light(data_fname,c2,y,icount,'append');
	           icount = icount + 1;
  	       end
	   end
       end
     %****************************************************************************************************
     % collecting testing data
     %**************************************************************************************************** 
       if ~exist(test_fname) & ~exist(outfile)
	   icount = 1;
	   Y = [];
           ID = [];
	   for iaction = 1:naction
	       action = saction{iaction};
	       for ifile = 1:length(Testing_set{iaction})
	           fprintf('.');
	       
	            fnamec2 = sprintf('%s/%s/C2_%s.mat',dstdir,action,Testing_set{iaction}{ifile}(1:end-4)); 
		    load(fnamec2);	
		    c2 = double(c2);
		    c2 = c2./repmat((sum(c2.^2,1).^0.5),[size(c2,1) 1]); 
	            y = iaction*ones(size(c2,2),1);
	            ID = [ID;icount*ones(size(c2,2),1)];
                    Y = [Y;y];
	            to_svm_light(test_fname,c2,y,icount,'append');
	            icount = icount + 1;    
	       end
	   end
	   save(ID_fname,'ID','Y');
       end
     %****************************************************************************************************
     % training and testing
     %**************************************************************************************************** 

       if ~exist(model_fname)
	   cmd = ['!./third_party/svm_hmm/svm_hmm_learn -c ' c ' -e ' e ' --b 100 ' data_fname ' ' model_fname] 
	   eval(cmd);
       end
 
       if ~exist(outfile)
	   cmd = ['!./third_party/svm_hmm/svm_hmm_classify ' test_fname ' ' model_fname ' ' outfile]
	   eval(cmd);
       end

     %****************************************************************************************************
     % evaluating
     %**************************************************************************************************** 
       load(ID_fname);
 
       RY = textread(outfile);
       
       for i = unique(ID)'
	   id = find(ID==i);
	   if ~isempty(id)
	       VRY(i) = voting(RY(id));
	       VY(i) = voting(Y(id));
	   end
       end
       acc = mean(diag(formConfusionM(VY,VRY)));
