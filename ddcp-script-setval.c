/* vim: set sw=8 ts=8 si et: */
/* 
 * Linux software to communicate with the DDCP
 * Written by Guido Socher 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

int main(int argc, char *argv[])
{
        char *device;
        char *str;
        int fd;

        if (argc != 3){
                printf("USAGE: ddcp-script-setval \"command\" com-port-device\n");
                printf("Example, linux: ddcp-script-setval \"u=33\" /dev/ttyUSB0\n");
                printf("Example, mac: ddcp-script-setval \"u=33\" /dev/tty.usbserial-*\n");
                exit(0);
        }
        str=argv[1];
        device=argv[2];

        /* Set up io port correctly, and open it... */
        fd = open(device, O_WRONLY );
        if (fd == -1) {
                fprintf(stderr, "ERROR: open for %s failed.\n",device);
                exit(1);
        }
        write(fd,"\r",1); 
        usleep(100000); // commands are polled in the avr and it can take 100ms
        write(fd,str,strlen(str)); 
        write(fd,"\r",1); 
        close(fd);
        usleep(150000);
        return(0);
}
