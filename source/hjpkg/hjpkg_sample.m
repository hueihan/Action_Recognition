function v = hjpkg_sample(patchSizes,thre,vals)

   [nlayer,H,W] = size(vals);
 
if thre==0
   
  y     = ceil(rand*(H-patchSizes)); % y_ind  
  x     = ceil(rand*(W-patchSizes)); % y_ind 
  v = vals(:,y:y+patchSizes-1,x:x+patchSizes-1);
else
    
    tmpvals = vals;
    hsz = patchSizes/2;
    tmpvals(:,1:hsz,:) = 0;
    tmpvals(:,:,1:hsz) = 0;
    tmpvals(:,end-hsz+1:end,:) = 0;
    tmpvals(:,:,end-hsz+1:end) = 0;
    maxvals = squeeze(max(tmpvals,[],1));

    [Y,X] = ind2sub(size(maxvals),find(maxvals>thre));  
    while length(Y)==0
      thre  = max(thre - 0.1, 0);
      [Y,X] = ind2sub(size(maxvals),find(maxvals>thre));  
    end 
    ind = ceil(rand*length(Y));
    y = Y(ind);
    x = X(ind);
    v = vals(:, y - hsz  : y + hsz - 1, x - hsz : x + hsz - 1);
end
    %if 0%sparse
    %  v = Sparsify(v, unknown);
    %end
    
    %if 0%norm
    %  v = Normalize(v, zero, thres, unknown);
    %  if isempty(v), continue; end
    %end



return;


%***********************************************************************************************************************

function v = Sparsify(v, unknown)

% TODO: add additional methods.

vOld = v;
v(:) = unknown;

for j = 1 : size(v, 3)
    for i = 1 : size(v, 2)
        [a, f] = max(vOld(:, i, j));
        v(f, i, j) = a;
    end
end

return;

%***********************************************************************************************************************

function v = Normalize(v, zero, thres, unknown)

inds = (v > unknown);

if zero
    v(inds) = v(inds) - mean(v(inds));
end

norm = sqrt(sum(v(inds) .* v(inds)));
if norm < thres * sqrt(sum(inds))
    v = [];
    return;
end

v(inds) = v(inds) / norm;

return;
