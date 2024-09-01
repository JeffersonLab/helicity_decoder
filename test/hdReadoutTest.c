/*
 * File:
 *    hdReadoutTest
 *
 * Description:
 *    Configure a helicity decoder in internal mode and trigger a readout
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
#include "gslTimerLib.h"

int
main(int argc, char *argv[])
{
  DMA_MEM_ID vmeIN,vmeOUT;
  extern DMANODE *the_event;
  extern unsigned int *dma_dabufp;
  DMANODE *outEvent;
  int32_t idata, dCnt;

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

  /* Create the gslTimer object */
  gslTimer_t gt;

  uint32_t ntimers = 10;
  uint32_t min_time = 0;
  uint32_t max_time = 100000;
  uint32_t bin_size = 10;

  /* Initialize gslTimer */
  gslTimerInit(ntimers, min_time, max_time, bin_size, &gt);


  stat = vmeOpenDefaultWindows();
  if(stat != OK)
    goto CLOSE;

  dmaPFreeAll();
  vmeIN  = dmaPCreate("vmeIN",10240,1,0);
  vmeOUT = dmaPCreate("vmeOUT",0,0,0);

  dmaPStatsAll();

  dmaPReInitAll();

  vmeCheckMutexHealth(1);
  vmeBusLock();

  hdInit(address, HD_INIT_INTERNAL, HD_INIT_INTERNAL_HELICITY, 0);

  hdSetProcDelay(0x100, 0x40);

  hdHelicityGeneratorConfig(2, 0,
			    0x40, 0x80,
			    0xABCDEF01);
  hdEnableHelicityGenerator();

  hdStatus(1);

  hdEnable();
  hdSync(1);
  sleep(1);

  int ireadout = 0, nreads = 1000;
  for(ireadout = 0; ireadout < nreads; ireadout++)
    {
      gslTimerStartTime(&gt);     // Start Time
      hdTrig(0);

      int timeout=0;
      gslTimerRecordTime(&gt);
      while((hdBReady(0)!=1) && (timeout<100))
	{
	  timeout++;
	}
      gslTimerRecordTime(&gt);

      if(timeout>=100)
	{
	  printf("TIMEOUT!\n");
	  goto CLOSE;
	}

      GETEVENT(vmeIN,1);

      vmeDmaConfig(2,5,1);
      gslTimerRecordTime(&gt);
      dCnt = hdReadBlock(dma_dabufp, 1024>>2,1);
      gslTimerRecordTime(&gt);
      if(dCnt<=0)
	{
	  printf("No data or error.  dCnt = %d\n",dCnt);
	}
      else
	{
	  dma_dabufp += dCnt;
	}

      PUTEVENT(vmeOUT);
      gslTimerEndTime(&gt);

      outEvent = dmaPGetItem(vmeOUT);

      dCnt = outEvent->length;

      if(ireadout == 0)
	{
	  printf("  dCnt = %d\n",dCnt);
	  for(idata=0;idata<dCnt;idata++)
	    {
	      hdDecodeData(LSWAP(outEvent->data[idata]));
	      /* if((idata%5)==0) printf("\n\t"); */
	      /* printf("  0x%08x ",(unsigned int)LSWAP(outEvent->data[idata])); */
	    }
	  printf("\n\n");
	}

      dmaPFreeItem(outEvent);
    }

  hdDisable();

  hdStatus(1);
  hdReset(2, 1);
  gslTimerPrintStats(&gt);

  /* Free all memory allocated by the library */
  gslTimerFree(&gt);

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

/*
  Local Variables:
  compile-command: "make -k -B hdReadoutTest"
  End:
 */
