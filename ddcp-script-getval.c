/* vim: set sw=8 ts=8 si et: */
/* 
 * Linux software to communicate with the DDCP
 * Written by Guido Socher 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int main(int argc, char *argv[])
{
        char *device;
        char c;
        int c_cnt,state;
        int fd;

        if (argc != 2){
                printf("USAGE: ddcp-script-getval com-port-device\n");
                printf("Example, linux: ddcp-script-getval /dev/ttyUSB0\n");
                printf("Example, mac: ddcp-script-getval /dev/tty.usbserial-*\n");
                exit(0);
        }
        device=argv[1];

        /* Set up io port correctly, and open it... */
        fd = open(device, O_RDWR );
        if (fd == -1) {
                fprintf(stderr, "ERROR: open for %s failed.\n",device);
                exit(1);
        }
        write(fd,"\r",1); // send empty line
        usleep(100000); // commands are polled in the avr and it can take 100ms
        write(fd,"\r",1); // send empty line
        usleep(100000); // commands are polled in the avr and it can take 100ms
        state=0;
        while ((c_cnt=read(fd,&c,1))){
                //printf(":0x%x:\n",c); // debug
                if (c=='#') { // find begining of prompt
                        state=1;
                }
                if (state<1) continue;
                if (c=='>') {
                        fputc(c,stdout);
                        state=2;
                }
                if (state>1) break;
                putc(c,stdout);
        }
        close(fd);
        printf("\n");
        usleep(100000); // commands are polled in the avr and it can take 100ms
        return(0);
}
