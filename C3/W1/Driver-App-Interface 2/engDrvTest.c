#include <stdio.h>
#include <stdlib.h>
#include <errnoLib.h>
#include <ioLib.h>

#define SAMPLE 0x00
#define CLEAR 0x01

#define NUM_SENSORS 7

engDrvTest()
{

  int ed, bytes_read;
  char buffer[NUM_SENSORS+1];
  int i, j;
  unsigned char opcode;

  printf("Engineering Driver Test\n");
  fflush(stdout);

  /* initialize driver in case it wasn't on boot */
  engDrv();
  engDevCreate("/dev/eng");

  if((ed = open("/dev/eng", O_RDWR, 0)) == ERROR) {
    printf ("Error opening serial interface\r\n");
    return -1;
  }

  printf("driver intialized and opened\n");
  fflush(stdout);

  /* Command the engineering analog sensor device to sample it's sensors */
  opcode = SAMPLE;
  write(ed, &opcode, 1);
  
  printf("commanded sample\n");
  fflush(stdout);

  for(j=0;j<10;j++) {

    bytes_read = 0;

    /* Read data from 7 8 bit A/D converters 
       for 3 temps, 3 pressures, and bus_volt analog sensors
     */
    bytes_read += read(ed, buffer, 7);

    buffer[NUM_SENSORS] = '\0';

    printf("read %d bytes, hex =>", bytes_read);

    for(i=0;i<NUM_SENSORS;i++) {
      printf(" %2X", (0x000000FF & buffer[i]));
    }
    printf("\n");

  }

  /* Command the engineering analog sensor device to clear it's buffers */
  opcode = CLEAR;
  write(ed, &opcode, 1);

  bytes_read = 0;
  bytes_read += read(ed, buffer, 7);

  buffer[NUM_SENSORS] = '\0';

  printf("read %d bytes, hex =>", bytes_read);

  for(i=0;i<NUM_SENSORS;i++) {
    printf(" %2X", (0x000000FF & buffer[i]));
  }
  printf("\n");

  close(ed);
}
