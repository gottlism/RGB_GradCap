#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "libsoc_pwm.h"
#include "libsoc_gpio.h"
#include "libsoc_debug.h"

typedef FILE * RenderedImage; //Data of image to be rendered, this type might change
static int N = 20; //Size of command buffer
static int NIPIN = 20;
static int SCPIN = 21;
static int SOPIN = 22;
static int parallelState = 0;
static pthread_t readCommands, nextImageT, stateChangeT, shiftOrientationT;

typedef struct RawImages
{
	char ** filePaths;
	int currentFile;
	int arraySize;
} RawImages;

typedef struct RenderedImages
{
	RenderedImage node1;
	RenderedImage node2;
} RenderedImages;

typedef struct CommandBuffer
{
	char * command;
	int read;
    int write;
    int count;
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_cond;
} CommandBuffer;

typedef struct ThreadData
{
	RawImages * rawImages;
	RenderedImages * renderedImages;
	CommandBuffer * buffer;
} ThreadData;

static void * nextImageButton(void * bufferArg) {
  gpio * pin;
  pin = libsoc_gpio_request(NIPIN, LS_GPIO_SHARED);
  libsoc_gpio_set_direction(pin, INPUT);
  int t;
  int level;
  level = 0;
  int prev_level;
  prev_level = 0;
  while(1) {
      level = libsoc_gpio_get_level(pin);
      if(level==0 && prev_level == 1) {
          t++;
          enqueue((struct CommandBuffer *)bufferArg, "next image");
          // printf("%s\n", "next image pressed");
      }
      prev_level = level;
      //printf("%d\n", level);
      usleep(100);
  }
  libsoc_gpio_free(pin);
}

static void * stateChangeButton(void * bufferArg) {
	gpio * pin;
	pin = libsoc_gpio_request(SCPIN, LS_GPIO_SHARED);
	libsoc_gpio_set_direction(pin, INPUT);
	int t;
	int level;
	level = 0;
	int prev_level;
	prev_level = 0;
	while(1) {
			level = libsoc_gpio_get_level(pin);
			if(level==0 && prev_level == 1) {
					t++;
					enqueue((struct CommandBuffer *)bufferArg, "state change");
					// printf("%s\n", "state change pressed");
			}
			prev_level = level;
			//printf("%d\n", level);
			usleep(100);
	}
	libsoc_gpio_free(pin);
}

static void * shiftOrientationButton(void * bufferArg) {
	gpio * pin;
  pin = libsoc_gpio_request(SOPIN, LS_GPIO_SHARED);
  libsoc_gpio_set_direction(pin, INPUT);
  int t;
  int level;
  level = 0;
  int prev_level;
  prev_level = 0;
  while(1) {
      level = libsoc_gpio_get_level(pin);
      if(level==0 && prev_level == 1) {
          t++;
          enqueue((struct CommandBuffer *)bufferArg, "orientation shift");
          // printf("%s\n", "shift orientation pressed");
      }
      prev_level = level;
      //printf("%d\n", level);
      usleep(100);
  }
  libsoc_gpio_free(pin);
}

//Read button input
static void * readCommandsThread(void * dataPack) {
	struct ThreadData * tempDataPack = (struct ThreadData *)dataPack;
	char * cmd;
	while (1) {
		pthread_mutex_lock (&(tempDataPack->buffer->buffer_mutex));
		if (tempDataPack->buffer->count == 0) {
			pthread_cond_wait (&(tempDataPack->buffer->buffer_cond), &(tempDataPack->buffer->buffer_mutex));
		}
		dequeue(tempDataPack->buffer, cmd);
		pthread_cond_broadcast (&(tempDataPack->buffer->buffer_cond));
		pthread_mutex_unlock (&(tempDataPack->buffer->buffer_mutex));

		if (strcmp(cmd, "next image") == 0) {
			nextImage(&(tempDataPack->rawImages));
		}

		else if (strcmp(cmd, "state change") == 0) {
			changeState(&(tempDataPack->rawImages));
		}

		else if (strcmp(cmd, "orientation shift") == 0) {
			shiftOrientation(&(tempDataPack->rawImages));
		}
	}
}

int main (void) {
	struct RawImages * imagePaths;
	loadInImages(&imagePaths);
	struct CommandBuffer * cmdBuf;
	struct RenderedImages * rendered;

	struct ThreadData * dataPack;
	dataPack->renderedImages = rendered;
	dataPack->rawImages = imagePaths;
	dataPack->buffer = cmdBuf;

	pthread_create(&nextImageT, (pthread_attr_t*)0, nextImageButton, &(dataPack->buffer));
	pthread_create(&stateChangeT, (pthread_attr_t*)0, stateChangeButton, &(dataPack->buffer));
	pthread_create(&shiftOrientationT, (pthread_attr_t*)0, shiftOrientationButton, &(dataPack->buffer));
	pthread_create(&readCommands, (pthread_attr_t*)0, readCommandsThread, &dataPack);

	pthread_join(nextImageT, 0);
	pthread_join(stateChangeT, 0);
	pthread_join(shiftOrientationT, 0);
	pthread_join(readCommands, 0);

}

//Looks into directory and loads image file paths into Raw Images
void loadInImages(RawImages *r) {
	//doAction
	r-> arraySize = 0; //Change this to number of files in directory
	r->currentFile = 0;

}

void commandBuffer_init(CommandBuffer *b) {
	b->command = (malloc (N*sizeof (char)));
	b-> read = 0;
	b->write = 0;
	b->count = 0;
	pthread_mutex_init(&(b->buffer_mutex),NULL);
	pthread_cond_init (&(b->buffer_cond), NULL);

}

void enqueue (CommandBuffer *b, char command) {
    pthread_mutex_lock (&(b->buffer_mutex));
    while (b->count >= N) {
        pthread_cond_wait (&(b->buffer_cond), &(b->buffer_mutex));
    }
    b->command[b->write] = command;
    b->count++;
    b->write = (b->write + 1) % N;
    pthread_cond_broadcast (&(b->buffer_cond));
    pthread_mutex_unlock (&(b->buffer_mutex));
}

void dequeue (CommandBuffer *b, char *command) {
    pthread_mutex_lock (&(b->buffer_mutex));

    while ((b->count) == 0) {
        pthread_cond_wait (&(b->buffer_cond), &(b->buffer_mutex));
    }

    *command = (b->command[(b->read)]);

    b->read = (b->read + 1) % N;
    b->count--;
    pthread_cond_broadcast (&(b->buffer_cond));
    pthread_mutex_unlock (&(b->buffer_mutex));

}

void nextImage(void * rawImagePaths) {
	struct RawImages * tempRawImages = (struct RawImages *)rawImagePaths;
	tempRawImages->currentFile++;
	if (tempRawImages->currentFile == tempRawImages->arraySize) {
		tempRawImages->currentFile = 0;
	}
	char * curImage = tempRawImages->filePaths[tempRawImages->currentFile];
	//Downsample curImage
	//Send nodes ( ͡° ͜ʖ ͡°)
}

void changeState(void * rawImagePaths) {
	!parallelState;
	//Downsample the current image
	//Send nodes ( ͡° ͜ʖ ͡°)
}

void shiftOrientation(void * renderedImages) {
	struct RenderedImages * tempRenderedImages = (struct RenderedImages *)renderedImages;
	//how do we get the renderedimages/nodes in here
	//Swap the nodes below
	FILE temp = *(tempRenderedImages->node1);
	*(tempRenderedImages->node1) = *(tempRenderedImages->node2);
	*(tempRenderedImages->node2) = temp;
	//Send nodes ( ͡° ͜ʖ ͡°)
}
