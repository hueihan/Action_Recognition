function m = hj_model_multiscale(Iparam, d,s1Par,c1Par,s2Par,c2Par,options)

if nargin < 2
    d.fVals  = [];
    d.fMap   = [];
    d.fSizes = [];
end

%-----------------------------------------------------------------------------------------------------------------------

m = cns_package('new', 'hjpkg');

%-----------------------------------------------------------------------------------------------------------------------
  p = struct;
  p.name = 'ri';
  p.type = 'ri';
  p.size = Iparam(1).size;
  m = cns_package('add', m, p);

  for isiz = 1:length(s1Par.Ssiz) % 12

    p = struct;
    p.name    = 's1';
    p.type    = 'zdp';    
    p.rfCount = s1Par.Ssiz(isiz);
    p.rfStep  = 1;
    p.tfCount = s1Par.Tsiz(isiz);
    p.fCount  = s1Par.ndir*length(s1Par.speed)*length(s1Par.symmetry);
    p.fParams = {s1Par.filter_type,s1Par.symmetry,s1Par.speed};
    p.ndir    = s1Par.ndir;
    p.thres   = s1Par.thres;
    m = cns_package('add', m, p);
  end
  %-----------------------------------------------------------------------------------------------------------------------
  for isiz = 1:length(s1Par.Ssiz) % 12

    p = struct;
    p.name = 'nf1';
    p.type = 'nf1';
    p.pzStep = length(s1Par.Ssiz);
    p.gamma  = s1Par.nf1const;
    m = cns_package('add', m, p);
  end

  %-----------------------------------------------------------------------------------------------------------------------
  for isiz = 1:length(s1Par.Ssiz) % 12
    
    p = struct;
    p.name = 'nf2';
    p.type = 'nf2';
    p.pzStep = length(s1Par.Ssiz);
    m = cns_package('add', m, p);
  end
  %-----------------------------------------------------------------------------------------------------------------------

  for isiz = 1:ceil((length(s1Par.Ssiz)-c1Par.PoolScaleSiz+1)/c1Par.PoolScaleStepSiz) % 6
    p = struct;
    p.name    = 'c1';
    p.type    = 'max';
    p.rfCount = c1Par.PoolSiz(isiz); 
    p.rfStep  = c1Par.PoolStepSiz(isiz);
    p.rsCount = c1Par.PoolScaleSiz; % 2
    p.rsStep  = c1Par.PoolScaleStepSiz;% 2
    p.pzStep  = length(s1Par.Ssiz)-(p.rsStep-1)*(isiz-1);
    m = cns_package('add', m, p);
  end

  if isempty(c2Par)
    return;
  end
%-----------------------------------------------------------------------------------------------------------------------
  pzstep = ceil((length(s1Par.Ssiz)-c1Par.PoolScaleSiz+1)/c1Par.PoolScaleStepSiz); % 6;

%-----------------------------------------------------------------------------------------------------------------------
  for isiz = 1:pzstep

    p = struct;
    p.name       = ['ssvalid']; % same or valid
    p.type       = ['ndot'];
    p.rfCountMin = min(d.fSizes);
    p.rfSpace    = 1;
    p.rfStep     = 1;
    p.fVals      = d.fVals;
    p.fMap       = d.fMap;
    p.fSizes     = d.fSizes;
    p.exponent   = s2Par.exponent;
    p.thres      = s2Par.thres;
    p.pzStep     = pzstep; 
    m = cns_package('add', m, p);
  end
    
%-----------------------------------------------------------------------------------------------------------------------
  for isiz = 1:ceil((pzstep-c2Par.PoolScaleSiz+1)/c2Par.PoolScaleStepSiz) % 6
 
    p = struct; 
    p.name    = 'c2';
    p.type    = 'max';
    if c2Par.PoolSiz> 100
      p.rfCount = inf;
    else      
      p.rfCount = c2Par.PoolSiz; 
    end
 
    p.rsCount = c2Par.PoolScaleSiz;%2    
    p.rfStep  = c2Par.PoolStepSiz;    
    p.rsStep  = c2Par.PoolScaleStepSiz;%2
    p.pzStep  = pzstep-(p.rsStep-1)*(isiz-1);
  
    m = cns_package('add', m, p);
  end	
%-----------------------------------------------------------------------------------------------------------------------
return;
