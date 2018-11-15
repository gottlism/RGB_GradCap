
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef char * renderedImage; //Most likely byte stream for rendered image

typedef struct Nodes {
	renderedImage node1;
	renderedImage node2;
} Nodes;

typedef struct RenderedImages
{
	Nodes nodes;
	int newData;
	pthread_mutex_t node_mutex;
	pthread_cond_t node_cond;
} RenderedImages;

typedef struct State
{
	int parallelState;
	int newData;
	pthread_mutex_t state_mutex;
	pthread_cond_t state_cond;

} State;

typedef struct Images
{
	//Contains references to all the images in a circular format and what image we are currently on
} Images;

// Read in the next image button
//If button pressed, get image, load next image into shared data structure
static void * nextImageButton(void * bufferArg) {

}

//Change state to opposite and flag a rerender
static void * parallelSerialButton(void * bufferArg) {

}

// Shift orientation and flag to be sent
static void * shiftOrientationButton(void * bufferArg) {

}

//When orientation shifts or next image pressed, render image and flag to be sent
static void * downSampleImage(void * bufferArg) {
	//Waits for a layout chagne or a new image to come in
}

//Sends data to nodes after new data has been rendered or Orientation to shift
static void * sendData(void * bufferArg) {

}

int main(void) {
	pthread_t next, parallelSerial, shift, downSample, send;

	State myState;
	state_init(&myState);
	Images myImages;
	init_images(&myImages);
	RenderedImages myRenderedImages;
	init_renderedImages(&myRenderedImages);


}

void state_init (State *state) {
	state->parallelState = 0;
	state->newData = 0;
	pthread_mutex_init(& (state->state_mutex), NULL);
	pthread_cond_init(& (state->state_cond), NULL);
}

void init_images(Images * images) {
	//Read in from folder with image and create a circuilar buffer will all image names.
	//Set the current image as the first one in the buffer
}

void init_renderedImages(RenderedImages * renderedImages) {
	renderedImages->newData = 0;
	pthread_mutex_init(& (renderedImages->node_mutex), NULL);
	pthread_cond_init(& (renderedImages->node_cond), NULL);
}
