#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>


typedef char * RenderedImage; //Data of image to be rendered, this type might change
static int N = 20; //Size of command buffer

typedef struct RawImages
{
	//array of file paths
	int currentFile;
	int arraySize;
}RawImages;

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
	RawImages images;
	CommandBuffer buffer;
} ThreadData;

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

int main (void) {
	RawImages imagePaths;
	loadInImages(&imagePaths);

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
