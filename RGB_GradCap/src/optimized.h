#include <stdio.h>
#include <string.h>
#include <wand/MagickWand.h>

void downsample(char* inputFile, char* outputMiddle, char* outputLeft, char* outputRight)
{
	MagickWand *image = NULL;
	MagickWand *helperMiddle = NULL;
	MagickWand *helperLeft = NULL;
	MagickWand *helperRight = NULL;
	MagickWand *newImageMiddle = NULL;
	MagickWand *newImageLeft = NULL;
	MagickWand *newImageRight = NULL;

	MagickWandGenesis();

	image = NewMagickWand();

	if(MagickReadImage(image,inputFile));

	size_t width,height;
	width = MagickGetImageWidth(image);
	height = MagickGetImageHeight(image);
	size_t scaledWidthS,scaledHeightS;
	size_t scaledWidthP,scaledHeightP;
	size_t i;

	//calculate height for 1 by 1
	if (width >= height) {
		scaledWidthS = height;
		scaledHeightS = height;
	} else {
		scaledWidthS = width;
		scaledHeightS = width;
	}

	//calculate height for 1 by 2
	if (height*2 < width) {
		scaledHeightP = height;
		scaledWidthP = 2*height;
	} else {
		scaledHeightP = width/2;
		scaledWidthP = scaledHeightP*2;
	}

	image = MagickCoalesceImages(image);

	newImageMiddle = NewMagickWand();
	newImageLeft = NewMagickWand();
	newImageRight = NewMagickWand();

	for(i=0; i<MagickGetNumberImages(image); i++) {
		MagickSetIteratorIndex(image,i);

		helperMiddle = MagickGetImage(image);
		helperLeft = MagickGetImage(image);
		helperRight = MagickGetImage(image);

		MagickCropImage(helperMiddle,scaledWidthS,scaledHeightS,0,0);
		MagickCropImage(helperLeft,scaledHeightP,scaledHeightP,0,0);
		MagickCropImage(helperRight,scaledHeightP,scaledHeightP,scaledHeightP,0);
		
		MagickScaleImage(helperMiddle,32,32);
		MagickScaleImage(helperLeft,32,32);
		MagickScaleImage(helperRight,32,32);

		MagickNormalizeImage(helperMiddle);
		MagickNormalizeImage(helperLeft);
		MagickNormalizeImage(helperRight);

		MagickSetImagePage(helperMiddle, 32, 32, 0, 0);
		MagickSetImagePage(helperLeft, 32, 32, 0, 0);
		MagickSetImagePage(helperRight, 32, 32, 0, 0);

		MagickAddImage(newImageMiddle,helperMiddle);
		MagickAddImage(newImageLeft,helperLeft);
		MagickAddImage(newImageRight,helperRight);

		DestroyMagickWand(helperMiddle);
		DestroyMagickWand(helperLeft);
		DestroyMagickWand(helperRight);
	}
	
	MagickResetIterator(newImageMiddle);
	MagickResetIterator(newImageLeft);
	MagickResetIterator(newImageRight);

	helperMiddle = MagickCompareImageLayers(newImageMiddle,CompareAnyLayer);
	helperLeft = MagickCompareImageLayers(newImageLeft,CompareAnyLayer);
	helperRight = MagickCompareImageLayers(newImageRight,CompareAnyLayer);

	MagickSetOption(helperMiddle,"loop","0");
	MagickSetOption(helperLeft,"loop","0");
	MagickSetOption(helperRight,"loop","0");

	if(MagickWriteImages(helperMiddle,outputMiddle,MagickTrue));
	if(MagickWriteImages(helperLeft,outputLeft,MagickTrue));
	if(MagickWriteImages(helperRight,outputRight,MagickTrue));

	if(helperMiddle) helperMiddle = DestroyMagickWand(helperMiddle);	
	if(helperLeft) helperLeft = DestroyMagickWand(helperLeft);	
	if(helperRight) helperRight = DestroyMagickWand(helperRight);

	//destroy remaining wands
	if(newImageMiddle) newImageMiddle = DestroyMagickWand(newImageMiddle);
	if(newImageLeft) newImageLeft = DestroyMagickWand(newImageLeft);
	if(newImageRight) newImageRight = DestroyMagickWand(newImageRight);
	if(image) image = DestroyMagickWand(image);

	MagickWandTerminus();
}