#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<termios.h>   // using the termios.h library

#define RENDER_LOCATION "/home/debian/renderedImages/"

#define SAME "_S.gif"
#define LEFT "_L.gif"
#define RIGHT "_R.gif"

//static const char * SAME = "_s.gif";
//static const char * LEFT = "_l.gif";
//static const  char * RIGHT = "_R.gif";
//static const char * RENDER_LOCATION = "/home/debian/renderedImages/";

void sendImagesSame (char *name) {
    int port;
    char ch = '1';
    char fullPath[100];
    port = setupUART (1);
    snprintf (&fullPath, sizeof(fullPath),"%s%s%s", RENDER_LOCATION, name, SAME);
    printf("%s\n", &fullPath);
    sendFile (port, &fullPath);
    close (port);
    port = setupUART (2);
    sendFile (port, &fullPath);
    write (port, &ch, 1);
    close (port);
    port = setupUART (1);
    write (port, &ch, 1);
    close (port);
}

void sendImagesParallel (char *name, int orientation) {
    char port1Data[100];
    char port2Data[100];
    int port;
    if(orientation) {
        snprintf (&port1Data, sizeof(port1Data),"%s%s%s", RENDER_LOCATION, name, LEFT);
        snprintf (&port2Data,sizeof(port1Data), "%s%s%s", RENDER_LOCATION, name, RIGHT);
    } else {
    	snprintf (&port2Data, sizeof(port1Data),"%s%s%s", RENDER_LOCATION, name, LEFT);
    	snprintf (&port1Data,sizeof(port1Data), "%s%s%s", RENDER_LOCATION, name, RIGHT);
    }
    char ch = '1';
    port = setupUART (1);
    sendFile (port, port1Data);
    close (port);
    port = setupUART (2);
    sendFile (port, port2Data);
    write (port, &ch, 1);
    close (port);
    port = setupUART (1);
    write (port, &ch, 1);
    close (port);

}

void sendFile (int port, char **fileName) {
    unsigned char ch;
    FILE * data;
    data = fopen(fileName, "rb");
    fseek (data, 0L, SEEK_END);
    int size = ftell (data);
    rewind (data);
    printf ("File Size is: %d\n", size);
    write (port, &size, 4);
    for (int i = 0; i < size; i++) {
        ch = getc (data);
        write (port, &ch, 1);

    }
    fclose (data);
    printf("Send File to Node %d:\n ", port); 
}

int setupUART (int node) {
    int file;
    if(node == 1) {
        if((file = open ("/dev/ttyO4", O_RDWR | O_NOCTTY)) < 0) {
            perror ("UART: Failed to open the file.\n");
            return -1;
        }
    } else {
        if((file = open ("/dev/ttyO2", O_RDWR | O_NOCTTY)) < 0) {
            perror ("UART: Failed to open the file.\n");
            return -1;
        }
    }
    printf("Node Setup : %d\n", node); 
    struct termios options;               //The termios structure is vital
    tcgetattr (file, &options);            //Sets the parameters associated with file

    // Set up the communications options:
    //   9600 baud, 8-bit, enable receiver, no modem control lines
    options.c_cflag = B230400 | CS8 | CREAD | CLOCAL;
    options.c_oflag = ONOCR;
    options.c_iflag = IGNPAR | IGNCR;    //ignore partity errors, CR -> newline
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IXON); //tty is the name of the struct termios
    tcflush (file, TCIFLUSH);             //discard file information not transmitted
    tcsetattr (file, TCSANOW, &options);  //changes occur immmediately
    return file;
}
