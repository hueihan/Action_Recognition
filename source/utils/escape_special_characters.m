
function string = escape_special_characters(string,delimeter)

  delimeter = {'<','>','(',')','!','&',';','[',']'};
  for id = 1:length(delimeter)
      tmp = string;
      inds = strfind(tmp,delimeter{id});
      if ~isempty(inds)
	  str = [];
	  str = [str tmp(1:inds(1)-1) '\' tmp(inds(1))];
	  for i = 2:length(inds)
	      str = [str tmp(inds(i-1)+1:inds(i)-1) '\' tmp(inds(i))];
	  end
	  str = [str tmp(inds(end)+1:end)];	      
	  string = str;
      end	
  end
