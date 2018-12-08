#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<termios.h>   // using the termios.h library

#define RENDER_LOCATION "/home/debian/renderedImages/"

#define SAME "_S.gif"
#define LEFT "_L.gif"
#define RIGHT "_R.gif"


void sendImagesSame (char *name) {
    int port;
    char ch = '1';          //Transission complete signal 
    char fullPath[100];     //Space for file path 
    port = setupUART (1);   //Setup connection with node 1 

    //Create path for the "same" file 
    snprintf (&fullPath, sizeof(fullPath),"%s%s%s", RENDER_LOCATION, name, SAME);
    printf("%s\n", &fullPath);
    
    sendFile (port, &fullPath); //Send image to node 1 
    close (port);             

    port = setupUART (2);       //Setup connection with node 2
    sendFile (port, &fullPath); //Send image to node 2 
    write (port, &ch, 1);       //Send transmission complete to 1 
    close (port);

    //Resetup with node 1 and send transmission complete 
    port = setupUART (1);
    write (port, &ch, 1);
    close (port);
}

void sendImagesParallel (char *name, int orientation) {
    char port1Data[100];    //Path for image on node 1 
    char port2Data[100];    //Path for image on node 2
    int port;

    //Setup rendered image path to be send tp each individual node 
    if(orientation) {
        snprintf (&port1Data,sizeof(port1Data),"%s%s%s", RENDER_LOCATION, name, LEFT);
        snprintf (&port2Data,sizeof(port1Data), "%s%s%s", RENDER_LOCATION, name, RIGHT);
    } else {
    	snprintf (&port2Data,sizeof(port1Data),"%s%s%s", RENDER_LOCATION, name, LEFT);
    	snprintf (&port1Data,sizeof(port1Data), "%s%s%s", RENDER_LOCATION, name, RIGHT);
    }

    char ch = '1';          //Transmission complete signal
    
    //Set up node 1 transmission and send file 
    port = setupUART (1);   
    sendFile (port, port1Data);
    close (port);
    
    //Set up node 2 transmission and send file and transmission complete signal
    port = setupUART (2);
    sendFile (port, port2Data);
    write (port, &ch, 1);
    close (port);
   
    //Set up node 1 and send transmission complete 
    port = setupUART (1);
    write (port, &ch, 1);
    close (port);

}

void sendFile (int port, char **fileName) {
    unsigned char ch;
    FILE * data;
    data = fopen(fileName, "rb"); //Open image file 
    
    //Get image size 
    fseek (data, 0L, SEEK_END);
    int size = ftell (data);
    rewind (data);
    printf ("File Size is: %d\n", size);
    
    write (port, &size, 4); //Send image size 

    //Send file data 
    for (int i = 0; i < size; i++) {
        ch = getc (data);
        write (port, &ch, 1);

    }
    fclose (data);
    printf("Sent File to Node %d:\n ", port); 
}

int setupUART (int node) {
    int file;
    if(node == 1) {
        //Open up connection with node 1
        if((file = open ("/dev/ttyO4", O_RDWR | O_NOCTTY)) < 0) {
            perror ("UART: Failed to open the file.\n");
            return -1;
        }
    } else {
        //Open up connection with node 2 
        if((file = open ("/dev/ttyO2", O_RDWR | O_NOCTTY)) < 0) {
            perror ("UART: Failed to open the file.\n");
            return -1;
        }
    }
    printf("Node Setup : %d\n", node); 
    struct termios options;               //The termios structure is vital
    tcgetattr (file, &options);          

    // Set up the communications options:
    //   230400 baud, 8-bit, enable receiver, no modem control lines
    options.c_cflag = B230400 | CS8 | CREAD | CLOCAL;
    options.c_oflag = ONOCR;
    options.c_iflag = IGNPAR | IGNCR;    //ignore partity errors, CR -> newline
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IXON); 
    tcflush (file, TCIFLUSH);             //discard file information not transmitted
    tcsetattr (file, TCSANOW, &options);  //changes occur immmediately
    return file;
}
