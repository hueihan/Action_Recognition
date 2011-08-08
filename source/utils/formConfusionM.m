

% function Matrix = formConfusionM(ytest, ry, nclasses)
%
% to form the confusion matrix from the true labels and predicted labels
% INPUTS
%  ytest  -  true labels
%  ry     -  classified labels
%  nclasses - number of possible classes
%
% OUTPUTS   
%  Matrix - confusionMatrix
%
% DATE
%   2006-09-07 
%
function Matrix = formConfusionM(ytest, ry, nclasses)
 
ytest = ytest(:);
ry = ry(:);

if nargin==2, nclasses = max([ytest; ry]);end

% hard version
if 0
Matrix = zeros(nclasses);

label1 = unique(ytest);
numtrueclass = length(label1);

label2 = unique(ry);
numpredclass = length(label2);

    for i=1:numtrueclass
        truelabel = label1(i);
        ind=find(ytest==truelabel);
        numtruelabel = length(ind);
        predictlabel = ry(ind);
        for j =1:numpredclass
	    predictclass = label2(j);
            numpredictlabel = length(find(predictlabel==predictclass));
            Matrix(truelabel,predictclass) = numpredictlabel/(numtruelabel+eps);
        end
      end
end


% easy version
Matrix = zeros(nclasses);
for i = 1:length(ytest)
  true = ytest(i);
  predict = ry(i);
  Matrix(true,predict) = Matrix(true, predict)+1;
end

for i =1:nclasses
  Matrix(i,:) = Matrix(i,:)/max(1,sum(Matrix(i,:)));
end

