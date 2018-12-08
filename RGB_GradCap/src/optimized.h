#include <stdio.h>
#include <string.h>
#include <wand/MagickWand.h>

/*
Downsample method
	Description: This method take in an image/animation, crops it to the correct aspect ratio for same and
				 parallel display, downsamples and normalizes it, and saves the resultant files
	Input parameters: There are four input parameters which are as follows...
		char* inputFile:	Pointer to where the input file is stored on the SD card
		char *outputMiddle:	Pointer to where the output file for same image display will be stored
		char *outputLeft:	Pointer to where the output file for parallel display (left image) will be stored
		char *outputRight:	Pointer to where the output file for parallel display (right image) will be stored
	Output parameters: There are no ouput parameters, since three of the input parameters lay out where the
					   processed files from the program will be stored.
*/
void downsample(char* inputFile, char* outputMiddle, char* outputLeft, char* outputRight)
{
	// Pointers for the MagickWands (special objects) used in processing the image/animation
	MagickWand *image = NULL;			// Pointer for input image
	MagickWand *helperMiddle = NULL;	// Pointer for same image processing
	MagickWand *helperLeft = NULL;		// Pointer for parallel left image processing
	MagickWand *helperRight = NULL;		// Pointer for parallel right image processing
	MagickWand *newImageMiddle = NULL;	// Pointer for same image assembly and saving
	MagickWand *newImageLeft = NULL;	// Pointer for parallel left image processing and saving
	MagickWand *newImageRight = NULL;	// Pointer for parallel right image processing and saving

	// Initializes the MagickWand environment
	MagickWandGenesis();

	// Returns a wand required for other methods in the MagickWand API
	image = NewMagickWand();

	if(MagickReadImage(image,inputFile));	// Reads in image/animation to be 

	size_t width,height;	// Variables for holding information regarding input image/animation width and height
	width = MagickGetImageWidth(image);		// Read in image/animation input width
	height = MagickGetImageHeight(image);	// Read in image/animation output width
	size_t scaledWidthS,scaledHeightS;		// Variables for holding output image/animation width and height (same image display)
	size_t scaledWidthP,scaledHeightP;		// Variables for holding output image/animation width and height (parallel image display)
	size_t i;	// Variable for keeping track of GIF frames (for animation processing)

	// Calculate height for same image display image
	if (width >= height) {	// If width is greater than height, crop width
		scaledWidthS = height;
		scaledHeightS = height;
	} else {				// Otherwise, crop height
		scaledWidthS = width;
		scaledHeightS = width;
	}

	// Calculate height for parallel image display left and right images
	if (height*2 < width) {	// If width is greater than twice height, base aspect ratio on height
		scaledHeightP = height;
		scaledWidthP = 2*height;
	} else {				// Otherwise, base aspect ratio on width
		scaledHeightP = width/2;
		scaledWidthP = scaledHeightP*2;
	}

	image = MagickCoalesceImages(image); // COmposites a set of images (for processing animations) for uniform frame processing

	newImageMiddle = NewMagickWand();	// Returns a wand for same image assembly and saving
	newImageLeft = NewMagickWand();		// Returns a wand for parallel left image processing and saving
	newImageRight = NewMagickWand();	// Returns a wand for right image processing and saving

	// Process each frame of image/animation (will be one iteration for images, multiple iterations for animations)
	for(i=0; i<MagickGetNumberImages(image); i++) {
		MagickSetIteratorIndex(image,i);	// Set the iterator to the given position in the image list specified by index parameter

		// Return helper wands for use in processing of same, left, and right images
		helperMiddle = MagickGetImage(image);
		helperLeft = MagickGetImage(image);
		helperRight = MagickGetImage(image);

		// Extract the appropriate region of the image/animation frame (based on calculations performed earlier in program)
		MagickCropImage(helperMiddle,scaledWidthS,scaledHeightS,0,0);
		MagickCropImage(helperLeft,scaledHeightP,scaledHeightP,0,0);
		MagickCropImage(helperRight,scaledHeightP,scaledHeightP,scaledHeightP,0);
		
		// Scale image size to the given dimensions
		MagickScaleImage(helperMiddle,32,32);
		MagickScaleImage(helperLeft,32,32);
		MagickScaleImage(helperRight,32,32);

		// Enhance image/animation fram contrast by letting colors span entire possible range
		MagickNormalizeImage(helperMiddle);
		MagickNormalizeImage(helperLeft);
		MagickNormalizeImage(helperRight);

		// Adjust virtual canvas to match scaled image size (needed because of how .GIF processes frames)
		MagickSetImagePage(helperMiddle, 32, 32, 0, 0);
		MagickSetImagePage(helperLeft, 32, 32, 0, 0);
		MagickSetImagePage(helperRight, 32, 32, 0, 0);

		// Add current adjusted image/animation frame to wands holding output for same, left, and right images
		MagickAddImage(newImageMiddle,helperMiddle);
		MagickAddImage(newImageLeft,helperLeft);
		MagickAddImage(newImageRight,helperRight);

		// Destroy the helper wands before going to next iteration
		DestroyMagickWand(helperMiddle);
		DestroyMagickWand(helperLeft);
		DestroyMagickWand(helperRight);
	}
	
	// Reset iterator for output wands to first frame
	MagickResetIterator(newImageMiddle);
	MagickResetIterator(newImageLeft);
	MagickResetIterator(newImageRight);

	// Compare each image with next in sequence and return maximum bounding region (to avoid corruption of animations)
	helperMiddle = MagickCompareImageLayers(newImageMiddle,CompareAnyLayer);
	helperLeft = MagickCompareImageLayers(newImageLeft,CompareAnyLayer);
	helperRight = MagickCompareImageLayers(newImageRight,CompareAnyLayer);

	// Associate option with the wand (for continuous looping of animations)
	MagickSetOption(helperMiddle,"loop","0");
	MagickSetOption(helperLeft,"loop","0");
	MagickSetOption(helperRight,"loop","0");

	// Write the newly processed images to the locations specified in the input parameters
	if(MagickWriteImages(helperMiddle,outputMiddle,MagickTrue));
	if(MagickWriteImages(helperLeft,outputLeft,MagickTrue));
	if(MagickWriteImages(helperRight,outputRight,MagickTrue));

	// Destroy helper wands
	if(helperMiddle) helperMiddle = DestroyMagickWand(helperMiddle);	
	if(helperLeft) helperLeft = DestroyMagickWand(helperLeft);	
	if(helperRight) helperRight = DestroyMagickWand(helperRight);

	// Destroy remaining wands
	if(newImageMiddle) newImageMiddle = DestroyMagickWand(newImageMiddle);
	if(newImageLeft) newImageLeft = DestroyMagickWand(newImageLeft);
	if(newImageRight) newImageRight = DestroyMagickWand(newImageRight);
	if(image) image = DestroyMagickWand(image);

	// Terminate the MagickWand environment
	MagickWandTerminus();
}
