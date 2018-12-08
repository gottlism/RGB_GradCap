#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>

#include "libsoc_gpio.h"
#include "libsoc_debug.h"
#include "ImageSend.h"
#include "downsample.h"


static int N = 20; //Max size of command buffer
static int NIPIN = 67;
static int SCPIN = 44;
static int SOPIN = 68;
static int parallelState = 0;
static int orientation = 1;
static pthread_t readCommands, nextImageT, stateChangeT, shiftOrientationT, resizeT;

typedef char * RenderedImage; //Data of image to be rendered

//Circular Buffer that Handles Button Inputs
typedef struct CommandBuffer
{
	char * command; 
	int read; 						//Index of the front of the buffer
  	int write;						//Index of the back of the buffer
  	int count;						//Current size of buffer
  	pthread_mutex_t buffer_mutex;
  	pthread_cond_t buffer_cond;
} CommandBuffer;

typedef struct ThreadData
{
	RenderedImage * renderedImages; //An array of file paths
	CommandBuffer * buffer;			//Command Buffer
	int renderedNumber;				//Amount of Rendered Images
} ThreadData;

/*Moves to the next image*/
static void * nextImageButton(void * bufferArg) {
	//Set up GPIO
  	gpio * pin;
  	pin = libsoc_gpio_request(NIPIN, LS_GPIO_SHARED);
  	libsoc_gpio_set_direction(pin, INPUT);
  	int t, level = 0, prev_level = 0;
  	
  	while(1) {
      level = libsoc_gpio_get_level(pin);

      //If pin level changes, enqueue command					
      if(level == 0 && prev_level == 1) {
          ++t;
          enqueue((struct CommandBuffer *)bufferArg, 'n');
      }
      prev_level = level;
      usleep(100);
  }
  	libsoc_gpio_free(pin);
}

/*Changes the state between single and parallel*/
static void * stateChangeButton(void * bufferArg) {
	//Set up GPIO
	gpio * pin;
	pin = libsoc_gpio_request(SCPIN, LS_GPIO_SHARED);
	libsoc_gpio_set_direction(pin, INPUT);
	int t, level = 0, prev_level = 0;
	
	while(1) {
		level = libsoc_gpio_get_level(pin);

		//If pin level changes, enqueue command				
		if(level==0 && prev_level == 1) {
			++t;
			enqueue((struct CommandBuffer *)bufferArg, 's');
		}
		prev_level = level;
		usleep(100);
	}
	libsoc_gpio_free(pin);
}

/*Changes which screen the parallel gifs are displayed on*/
static void * shiftOrientationButton(void * bufferArg) {
	//Set up GPIO
	gpio * pin;
  	pin = libsoc_gpio_request(SOPIN, LS_GPIO_SHARED);
  	libsoc_gpio_set_direction(pin, INPUT);
	int t, level = 0, prev_level = 0;
  	
  	while(1) {
      level = libsoc_gpio_get_level(pin);

      //If pin level changes, enqueue command			
      if(level==0 && prev_level == 1) {
          ++t;
          enqueue((struct CommandBuffer *)bufferArg, 'o');
      }
      prev_level = level;
      usleep(100);
  }
  libsoc_gpio_free(pin);
}

//Read and handle button input
static void * readCommandsThread(void * dataPack) {
	//Set up passed data
	struct ThreadData * tempDataPack = (struct ThreadData *)dataPack; 	

	char cmd;
	int curImage = 0;
	dispImage(tempDataPack->renderedImages, curImage);			//Display the first image 
	
	while (1) {
		dequeue(tempDataPack->buffer, &cmd);					//Dequeue Command 
		
		//Handle NextImage command
		if (cmd == 'n') {
			//Check for a current image above the amount of rendered images 
			if(++curImage > tempDataPack->renderedNumber) {
			curImage = 0;
		}

	  	dispImage(tempDataPack->renderedImages, curImage);		//Display Next Image 
		}
		
		//Handle StateChange command
		else if (cmd == 's') {
			parallelState = !parallelState;						//Change State
			dispImage(tempDataPack->renderedImages, curImage);	//Display new Images 
		}
		
		//Handle ShiftOrientation command
		else if (cmd == 'o') {
			orientation = !orientation;							//Change Orientation
			dispImage(tempDataPack->renderedImages, curImage);	//Display new images 
		}
	sleep(1);
	}
}

/*Downsamples all images in the unrenderedImages folder*/
static void * resize(void * dataPack) {
	//Set up passed data 
	struct ThreadData * tempDataPack = (struct ThreadData *)dataPack;
	
	//Open unredered images folder 
	DIR *d;
	struct dirent *dir;
	d = opendir("./unrenderedImages");

	//Go through every image in folder 
  	while ((dir = readdir(d)) != NULL)
  	{
  	if ( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") ) {
		} else {
			char input[100];	//Input File Name
			char outputS[100];	//Output for same GIF
			char outputL[100];	//Output for left GIF
			char outputR[100];	//Output for right GIF 

			snprintf(&input, sizeof(input) + 1, "%s%s", "./unrenderedImages/", dir->d_name);			//Get input image 
			
			//Cutoff .gif extension
			char * tempName = dir->d_name;
			tempName[strlen(tempName)-4] = 0;
			
			snprintf(&outputS, sizeof(input) + 1, "%s%s%s", "./renderedImages/", tempName, "_S.gif");	//New file path for same GIF
			snprintf(&outputL, sizeof(input) + 1, "%s%s%s", "./renderedImages/", tempName, "_L.gif");	//File path for left GIF
			snprintf(&outputR, sizeof(input) + 1, "%s%s%s", "./renderedImages/", tempName, "_R.gif");	//File path for right GIF
			downsample(&input, &outputS, &outputL,  &outputR);											//Downample input image and ouput three outputted images to directory
			tempDataPack->renderedImages[++tempDataPack->renderedNumber] = tempName; 					//Add base file name ot rendered image data struct and increase rendered images
		}
  }
  closedir(d);
}

int main (void) {
	struct CommandBuffer cmdBuf;
	commandBuffer_init(&cmdBuf);

	RenderedImage rendered[100]; //Allow for 100 images 
	
	//Create and initialize datapack
	struct ThreadData dataPack;
	dataPack.renderedImages = rendered;
	dataPack.buffer = &cmdBuf;
	dataPack.renderedNumber = -1;

	pthread_create(&resizeT, (pthread_attr_t*)0, resize, &dataPack);

	//Don't join other threads while first image hasn't been rendered 
	while(dataPack.renderedNumber == -1) {
		sleep(2);
	}

	pthread_create(&nextImageT, (pthread_attr_t*)0, nextImageButton, dataPack.buffer);
	pthread_create(&stateChangeT, (pthread_attr_t*)0, stateChangeButton, dataPack.buffer);
	pthread_create(&shiftOrientationT, (pthread_attr_t*)0, shiftOrientationButton, dataPack.buffer);
	pthread_create(&readCommands, (pthread_attr_t*)0, readCommandsThread, &dataPack);

	pthread_join(resizeT, 0);
	pthread_join(nextImageT, 0);
	pthread_join(stateChangeT, 0);
	pthread_join(shiftOrientationT, 0);
	pthread_join(readCommands, 0);

}

/*Initializes the command buffer*/
void commandBuffer_init(CommandBuffer *b) {
	b->command = malloc(N*sizeof(char));
	b-> read = 0;
	b->write = 0;
	b->count = 0;
	pthread_mutex_init(&b->buffer_mutex,NULL);
	pthread_cond_init(&b->buffer_cond, NULL);

}

/*Adds char command into the buffer b*/
void enqueue (CommandBuffer *b, char command) {
  	pthread_mutex_lock (&b->buffer_mutex);
  	while (b->count >= N) {
      	pthread_cond_wait (&b->buffer_cond, &b->buffer_mutex);
  	}
  	b->command[b->write] = command;
  	b->count++;
  	b->write = (b->write + 1) % N;
  	pthread_cond_broadcast (&b->buffer_cond);
	pthread_mutex_unlock (&b->buffer_mutex);
}

/*Reads char commands from the buffer b into char* command*/
void dequeue (CommandBuffer *b, char *command) {
    pthread_mutex_lock (&b->buffer_mutex);
    while (b->count == 0) {
        pthread_cond_wait (&b->buffer_cond, &b->buffer_mutex);
    }
    *command = (b->command[b->read]);
    b->read = (b->read + 1) % N;
    b->count--;
    pthread_cond_broadcast(&b->buffer_cond);
    pthread_mutex_unlock(&b->buffer_mutex);
}

/*Sends images to the teensy board*/
void dispImage(RenderedImage * rendImages,  int currentImage) {
	if(!parallelState) {
		sendImagesSame(rendImages[currentImage]);
	} else {
		sendImagesParallel(rendImages[currentImage], orientation);
	}
}
