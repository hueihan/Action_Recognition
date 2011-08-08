
% function class = voting(ry)
%
% INPUTS
%  ry  -  an array of classified class
%
% OUTPUTS   
%  class - the voting result
%
% DATE
%   2006-12-25 
%
function class = voting(ry)

labels=  unique(ry);

for i=1:length(labels)
  label = labels(i);
  score(i) = length(find(ry==label));
end

[a,b]=max(score);
class=labels(b);
