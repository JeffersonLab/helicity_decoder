/*
 * File:
 *    hdTestVXS
 *
 * Description:
 *    Program to test VXS connections of the helicity generator
 *
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "jvme.h"
#include "dmaPList.h"
#include "hdLib.h"
#include "tiLib.h"
#include "sdLib.h"

int32_t
testClock()
{
  int32_t rval = OK;

  int32_t system = 0, local = 0;
  rval = hdGetClockPLLStatus(&system, &local);

  printf("%s:   system = %d   local = %d\n",
	 __func__, system, local);

  return rval;
}

int32_t
testSyncReset()
{
  int32_t rval = OK;
  volatile uint32_t scalers[9];

  if(hdReadScalers(scalers, 1))
    {
      printf("  Signal scalers:\n");
      printf("    Trig1            = 0x%08x (%d)\n", scalers[4], scalers[4]);
      printf("    Trig2            = 0x%08x (%d)\n", scalers[5], scalers[5]);
      printf("    SyncReset        = 0x%08x (%d)\n", scalers[6], scalers[6]);
    }

  return rval;
}

int32_t
testBusyOut()
{
  int32_t rval = OK;
  uint8_t status = 0, ti_status = 0;

  hdBusy(0);
  hdBusyStatus(&status);
  ti_status = tiGetSWBBusy(0);

  printf("%s:   Busy off     status = %d     ti_status = %d\n",
	 __func__, status, ti_status);

  hdBusy(1);
  hdBusyStatus(&status);
  ti_status = tiGetSWBBusy(0);

  printf("%s:   Busy ON      status = %d     ti_status = %d\n",
	 __func__, status, ti_status);

  hdBusy(0);

  return rval;
}

int32_t
testTrigger1()
{
  int32_t rval = OK;
  volatile uint32_t scalers[9];

  if(hdReadScalers(scalers, 1))
    {
      printf("  Signal scalers:\n");
      printf("    Trig1            = 0x%08x (%d)\n", scalers[4], scalers[4]);
      printf("    Trig2            = 0x%08x (%d)\n", scalers[5], scalers[5]);
      printf("    SyncReset        = 0x%08x (%d)\n", scalers[6], scalers[6]);
    }

  return rval;
}

int32_t
testTrigger2()
{
  int32_t rval = OK;
  volatile uint32_t scalers[9];

  if(hdReadScalers(scalers, 1))
    {
      printf("  Signal scalers:\n");
      printf("    Trig1            = 0x%08x (%d)\n", scalers[4], scalers[4]);
      printf("    Trig2            = 0x%08x (%d)\n", scalers[5], scalers[5]);
      printf("    SyncReset        = 0x%08x (%d)\n", scalers[6], scalers[6]);
    }

  return rval;
}

int
main(int argc, char *argv[])
{
  int stat;
  uint32_t address=0;

  if (argc > 1)
    {
      address = (unsigned int) strtoll(argv[1],NULL,16)&0xffffffff;
    }
  else
    {
      address = 0x00ed0000; // my test module
    }

  printf("\n %s: address = 0x%08x\n", argv[0], address);
  printf("----------------------------\n");

  stat = vmeOpenDefaultWindows();
  if(stat != OK)
    goto CLOSE;

  vmeCheckMutexHealth(1);
  vmeBusLock();

  // TI init + config
  stat = tiInit(0,0,0);
  tiClockReset();
  taskDelay(1);
  tiTrigLinkReset();
  taskDelay(1);
  tiEnableVXSSignals();

  stat = sdInit(0);

  // helicity decoder init + config
  hdInit(address, HD_INIT_VXS, HD_INIT_EXTERNAL_FIBER, 0);

  uint32_t hd_slotnumber = 0;
  hdGetSlotNumber(&hd_slotnumber);
  hdStatus(1);

  sdSetActiveVmeSlots(1 << hd_slotnumber);
  sdStatus(1);

 CLOSE:

  vmeBusUnlock();

  stat = vmeCloseDefaultWindows();
  if (stat != OK)
    {
      printf("vmeCloseDefaultWindows failed: code 0x%08x\n",stat);
      return -1;
    }

  exit(0);
}

#if 0
void
hdReadout()
{
  DMA_MEM_ID vmeIN,vmeOUT;
  extern DMANODE *the_event;
  extern unsigned int *dma_dabufp;
  DMANODE *outEvent;
  int32_t idata, dCnt;

  dmaPFreeAll();
  vmeIN  = dmaPCreate("vmeIN",10240,1,0);
  vmeOUT = dmaPCreate("vmeOUT",0,0,0);

  dmaPStatsAll();

  dmaPReInitAll();

  int timeout=0;
  while((hdBReady(0)!=1) && (timeout<100))
    {
      timeout++;
    }

  if(timeout>=100)
    {
      printf("TIMEOUT!\n");
      goto CLOSE;
    }

  GETEVENT(vmeIN,1);

  vmeDmaConfig(2,5,1);
  dCnt = hdReadBlock(dma_dabufp, 1024>>2,1);
  if(dCnt<=0)
    {
      printf("No data or error.  dCnt = %d\n",dCnt);
    }
  else
    {
      dma_dabufp += dCnt;
    }

  PUTEVENT(vmeOUT);

  outEvent = dmaPGetItem(vmeOUT);

  dCnt = outEvent->length;

  printf("  dCnt = %d\n",dCnt);
  for(idata=0;idata<dCnt;idata++)
    {
      hdDecodeData(LSWAP(outEvent->data[idata]));
      /* if((idata%5)==0) printf("\n\t"); */
      /* printf("  0x%08x ",(unsigned int)LSWAP(outEvent->data[idata])); */
    }
  printf("\n\n");

  dmaPFreeItem(outEvent);

}
#endif

/*
  Local Variables:
  compile-command: "make -k hdTestVXS "
  End:
*/
