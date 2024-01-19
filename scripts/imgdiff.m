% A small utility script to compute the root mean square error 
% of an image and the (binarised) difference image.
% Based on https://ch.mathworks.com/matlabcentral/answers/521462-root-mean-square-error-of-two-images

image1 = imread('results-accuracy-zoom-5-no-lod.png'); 
image2 = imread('results-accuracy-zoom-5-lod.png'); 

if ~isequal(size(image1), size(image2))
    error('Images must have the same dimensions.');
end

diff_image = imabsdiff(image1, image2);

rmse = sqrt(mean((image1(:)-image2(:)).^2));
disp(rmse);

diff_image_bin = imbinarize(rgb2gray(diff_image));
imwrite(im2uint8(diff_image), 'results-accuracy-zoom-5-diff.png');
imwrite(im2uint8(diff_image_bin), 'results-accuracy-zoom-5-diff-bin.png');
