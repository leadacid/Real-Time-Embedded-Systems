/****************************************************************************/
/* Driver: Engineering Subsystem sensor/actuator interface example           /
/*                                                                          */
/* Sam Siewert - 7/22/97                                                    */
/*                                                                          */
/****************************************************************************/

#include "iosLib.h"

typedef struct /* ENGDEV */
{
  DEV_HDR devHdr;
}  ENGDEV;

STATUS engDrv();
STATUS engDevCreate(char *name);
int engOpen (ENGDEV *engDev, char *remainder, int mode);
int engRead(ENGDEV *engDev, char *buffer, int nBytes);
int engWrite(ENGDEV *engDev, char *buffer, int nBytes);
int engIoctl(ENGDEV *engDev, int request, int *arg);
void engISR(ENGDEV *engDev);
