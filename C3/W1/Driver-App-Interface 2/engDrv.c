/****************************************************************************/
/* Driver: Engineering Subsystem sensor/actuator interface example           /
/*                                                                          */
/* Sam Siewert - 7/22/97                                                    */
/*                                                                          */
/****************************************************************************/

#include "vxWorks.h"
#include "errnoLib.h"
#include "intLib.h"
#include "ioLib.h"
#include "iosLib.h"
#include "stdlib.h"

#include "engDrv.h"

#define NUM_ANALOGS 7

LOCAL char sample_buffer[NUM_ANALOGS];

LOCAL int engDrvNum;          /* driver number assigned to this driver */
LOCAL ENGDEV engDev =
{
  {NULL},
};


STATUS engDrv()
{

  /* if driver is already installed, then engDrvNum will be > 0 from
     previos call to engDrv()
   */

  if(engDrvNum > 0)
    return (OK);

#if 0
  (void) intConnect(intvec, engInterrupt);

  /* device initialization */

#endif

  engDrvNum = iosDrvInstall(engOpen, (FUNCPTR) NULL, engOpen, (FUNCPTR) NULL, engRead, engWrite, engIoctl);

  return(engDrvNum == ERROR ? ERROR : OK);

}


STATUS engDevCreate(char *name)
{

  int status;

  if(engDrvNum <= 0)
  {
    /* errnoSet(S_ioLib_NO_DRIVER); */
    return (ERROR);
  }

  status = iosDevAdd(&engDev.devHdr, name, engDrvNum);

  srand(41467);

  return status;

}


int engOpen (ENGDEV *engDev, char *remainder, int mode)
{
  
  /* device has no file name or other extension to device special filename */
  if(remainder[0] != 0)
    return ERROR;
  else
    return((int) engDev);
}


int engRead(ENGDEV *engDev, char *buffer, int nBytes)
{
  int i;

  for(i=0;i<nBytes;i++) {
    buffer[i] = sample_buffer[i%NUM_ANALOGS];
  }

  return i;

}


int engWrite(ENGDEV *engDev, char *buffer, int nBytes)
{
  unsigned char sample;
  int i;

  if(buffer[0] == 0x00) {

    /* simulate pseudo sample of analog sensors */

    for(i=0;i<NUM_ANALOGS;i++) {
      sample = (((unsigned char) rand()) % 25) + 65;
      sample_buffer[i] = (char)(0x000000FF & sample);
    }

    return NUM_ANALOGS;
  }
  else if(buffer[0] == 0x01) {
    for(i=0;i<NUM_ANALOGS;i++) {
      sample_buffer[i] = (char)(0x00);
    }
    return NUM_ANALOGS;
  }
  else
    return -1;

}


int engIoctl(ENGDEV *engDev, int request, int *arg)
{
  ;
}


void engISR(ENGDEV *engDev)
{

  /* handle data available or command completion here */

  ;

}
