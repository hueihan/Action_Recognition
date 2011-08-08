function d = hjpkg_combine(d, v)

if isempty(d.fVals)
  d.fVals = cat(4,d.fVals,v);
else
  [d1,d2,d2,d4] = size(d.fVals);
  s2 = size(v,2);
  if d2 > s2
    tmp = zeros(d1,d2,d2);
    tmp(:,1:s2,1:s2) = v;
    d.fVals  = cat(4,d.fVals,tmp);
  elseif d2 < s2
    tmp = zeros(d1,s2,s2,d4);
    tmp(:,1:d2,1:d2,:) = d.fVals;
    d.fVals  = cat(4,tmp,v);
  else
    d.fVals  = cat(4,d.fVals,v);
  end
end


d.fSizes = [d.fSizes;size(v,2)];
