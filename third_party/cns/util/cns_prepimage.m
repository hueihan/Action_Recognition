function [val, p] = cns_prepimage(im, bufSize, nSpace)

if ischar(im), im = imread(im); end
if size(im, 3) == 3, im = rgb2gray(im); end
if isinteger(im)
    im = single(im) / single(intmax(class(im)));
else
    im = single(im);
end

siz = size(im);
if any(siz > bufSize)
    siz = round(siz * min(bufSize ./ siz));
    im = imresize(im, siz);
end

if all(siz == bufSize)
    val = im;
else
    val = zeros(bufSize, 'single');
    val(1 : siz(1), 1 : siz(2)) = im;
end

coverage = (siz .* nSpace) / max(siz .* nSpace);
space = coverage ./ siz;
start = 0.5 - 0.5 * (siz - 1) .* space;

p.size  = siz;
p.start = start;
p.space = space;

return;