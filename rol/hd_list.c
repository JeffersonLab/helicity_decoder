/*************************************************************************
 *
 *  hd_list.c - Library of routines for readout and buffering of
 *              events using a JLAB Trigger Interface V3 (TI) with a
 *              Linux VME controller in CODA 3.0.
 *
 *     This is an example readout list for the JLAB helicity decoder
 *
 *
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     10
#define MAX_EVENT_LENGTH   1024*64      /* Size in Bytes */

/* Define TI Type (TI_MASTER or TI_SLAVE) */
#define TI_MASTER
/* EXTernal trigger source (e.g. front panel ECL input), POLL for available data */
#define TI_READOUT TI_READOUT_EXT_POLL
/* TI VME address, or 0 for Auto Initialize (search for TI by slot) */
#define TI_ADDR  0

/* Measured longest fiber length in system */
#define FIBER_LATENCY_OFFSET 0x10

#include "dmaBankTools.h"
#include "tiprimary_list.c" /* Source required for CODA readout lists using the TI */
#include "hdLib.h"

#define HELICITY_DECODER_ADDR 0xed0000
#define HELICITY_DECODER_BANK 0xDEC

/* Define initial blocklevel and buffering level */
#define BLOCKLEVEL 1
#define BUFFERLEVEL 1

#define INTRANDOMPULSER

/****************************************
 *  DOWNLOAD
 ****************************************/
void
rocDownload()
{
  int stat;

  /* Setup Address and data modes for DMA transfers
   *
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */
  vmeDmaConfig(2,5,1);

  /* Define BLock Level */
  blockLevel = BLOCKLEVEL;


  /*****************
   *   TI SETUP
   *****************/

  /*
   * Set Trigger source
   *    For the TI-Master, valid sources:
   *      TI_TRIGGER_FPTRG     2  Front Panel "TRG" Input
   *      TI_TRIGGER_TSINPUTS  3  Front Panel "TS" Inputs
   *      TI_TRIGGER_TSREV2    4  Ribbon cable from Legacy TS module
   *      TI_TRIGGER_PULSER    5  TI Internal Pulser (Fixed rate and/or random)
   */
#ifdef INTRANDOMPULSER
  tiSetTriggerSource(TI_TRIGGER_PULSER); /* TS Internal Pulser */
#else
  tiSetTriggerSource(TI_TRIGGER_TSINPUTS); /* TS Inputs enabled */
  /* Enable set specific TS input bits (1-6) */
  tiEnableTSInput( TI_TSINPUT_1 | TI_TSINPUT_2 );
#endif

  /* Load the trigger table that associates
   *  pins 21/22 | 23/24 | 25/26 : trigger1
   *  pins 29/30 | 31/32 | 33/34 : trigger2
   */
  tiLoadTriggerTable(0);

  tiSetTriggerHoldoff(1,10,0);
  tiSetTriggerHoldoff(2,10,0);

  /* Set initial number of events per block */
  tiSetBlockLevel(blockLevel);

  /* Set Trigger Buffer Level */
  tiSetBlockBufferLevel(BUFFERLEVEL);

  /* Init the SD library so we can get status info */
  stat = sdInit();
  if(stat==0)
    {
      sdSetActiveVmeSlots(0);
      sdStatus(0);
    }

  tiStatus(0);

  /* Initialize the library and module with its internal clock*/
  hdInit(HELICITY_DECODER_ADDR, HD_INIT_INTERNAL, 0, 0);
  hdStatus(1);

  printf("rocDownload: User Download Executed\n");

}

/****************************************
 *  PRESTART
 ****************************************/
void
rocPrestart()
{
  unsigned short iflag;
  int stat;
  int islot;

  /* Set number of events per block (broadcasted to all connected TI Slaves)*/
  tiSetBlockLevel(blockLevel);
  printf("rocPrestart: Block Level set to %d\n",blockLevel);

  tiStatus(0);

  /* Switch helicity decoder to VXS master clock, trigger, syncreset */
  hdSetSignalSources(HD_INIT_VXS, HD_INIT_VXS, HD_INIT_VXS);

  /* Setting data input (0x100 = 2048 ns) and
     trigger latency (0x40 = 512 ns) processing delays */
  hdSetProcDelay(0x100, 0x40);

  /* Using internal helicity generation for testing */
  hdSetHelicitySource(1, 0, 1);
  hdHelicityGeneratorConfig(2,     /* Pattern = 2 (OCTET) */
			    0,     /* Window Delay = 0 (0 windows) */
			    0x40,  /* SettleTime = 0x40 (512 ns) */
			    0x80,  /* StableTime = 0x80 (1024 ns) */
			    0xABCDEF01); /* Seed */
  hdEnableHelicityGenerator();

  hdStatus(0);

  printf("rocPrestart: User Prestart Executed\n");

}

/****************************************
 *  GO
 ****************************************/
void
rocGo()
{
  int islot;

  /* Get the current Block Level */
  blockLevel = tiGetCurrentBlockLevel();
  printf("rocGo: Block Level set to %d\n",blockLevel);

  /* Enable/Set Block Level on modules, if needed, here */
  hdSetBlocklevel(blockLevel);

  hdEnable();
  hdStatus(0);

  /* Example: How to start internal pulser trigger */
#ifdef INTRANDOMPULSER
  /* Enable Random at rate 500kHz/(2^7) = ~3.9kHz */
  tiSetRandomTrigger(1,0x7);
#elif defined (INTFIXEDPULSER)
  /* Enable fixed rate with period (ns) 120 +30*700*(1024^0) = 21.1 us (~47.4 kHz)
     - Generated 1000 times */
  tiSoftTrig(1,1000,700,0);
#endif
}

/****************************************
 *  END
 ****************************************/
void
rocEnd()
{
  int islot;

  /* Example: How to stop internal pulser trigger */
#ifdef INTRANDOMPULSER
  /* Disable random trigger */
  tiDisableRandomTrigger();
#elif defined (INTFIXEDPULSER)
  /* Disable Fixed Rate trigger */
  tiSoftTrig(1,0,700,0);
#endif

  hdDisable();

  hdStatus(0);

  tiStatus(0);

  printf("rocEnd: Ended after %d blocks\n",tiGetIntCount());

}

/****************************************
 *  TRIGGER
 ****************************************/
void
rocTrigger(int arg)
{
  int dCnt;
  int timeout=0;

  /* Set TI output 1 high for diagnostics */
  tiSetOutputPort(1,0,0,0);

  /* Readout the trigger block from the TI
     Trigger Block MUST be reaodut first */
  dCnt = tiReadTriggerBlock(dma_dabufp);

  if(dCnt<=0)
    {
      printf("No TI Trigger data or error.  dCnt = %d\n",dCnt);
    }
  else
    { /* TI Data is already in a bank structure.  Bump the pointer */
      dma_dabufp += dCnt;
    }

  BANKOPEN(HELICITY_DECODER_BANK, BT_UI4, blockLevel);
  while((hdBReady(0)!=1) && (timeout<100))
    {
      timeout++;
    }

  if(timeout>=100)
    {
      printf("%s: ERROR: TIMEOUT waiting for Helicity Decoder Block Ready\n",
	     __func__);
    }
  else
    {
      dCnt = hdReadBlock(dma_dabufp, 1024>>2,1);
      if(dCnt<=0)
	{
	  printf("%s: ERROR or NO data from hdReadBlock(...) = %d\n",
		 __func__, dCnt);
	}
      else
	{
	  dma_dabufp += dCnt;
	}
    }

  BANKCLOSE;

  /* Set TI output 0 low */
  tiSetOutputPort(0,0,0,0);

}

void
rocCleanup()
{
  int islot=0;

  printf("%s: Reset all Modules\n",__FUNCTION__);

}

/*
  Local Variables:
  compile-command: "make hd_list.so"
  End:
 */
