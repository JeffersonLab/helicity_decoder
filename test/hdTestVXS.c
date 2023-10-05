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
#include "hdLib.h"
#include "tiLib.h"
#include "sdLib.h"

char *progname;
//
//  Just check status of PLL for system and local clock
//

int32_t
testClock()
{
  int32_t rval = OK;
  printf("%s: ----------------------------------------------------------------------\n", __func__);

  int32_t system = 0, local = 0;
  rval = hdGetClockPLLStatus(&system, &local);

  printf("%s:\n\t system = %d (%s)\n\t local = %d (%s)\n",
	 __func__,
	 system, (system==1) ? "LOCKED" : "NOT LOCKED",
	 local, (local==1) ? "LOCKED" : "NOT LOCKED");

  return rval;
}

//
// TI Sends SyncReset 100 times... verify receiving them in the scalers
//

int32_t
testSyncReset()
{
  int32_t rval = OK;
  volatile uint32_t scalers[9];
  printf("%s: ----------------------------------------------------------------------\n", __func__);

  if(hdReadScalers(scalers, 1))
    {
      printf("%s:\n\t SyncReset = 0x%08x (%d)\n",
	     __func__,
	     scalers[6], scalers[6]);
    }

  int32_t isync;
  for(isync = 0; isync < 100; isync++)
    tiSyncReset(0);

  if(hdReadScalers(scalers, 1))
    {
      printf("%s:\n\t SyncReset = 0x%08x (%d)\n",
	     __func__,
	     scalers[6], scalers[6]);
    }

  return rval;
}

//
// Enable Forced BUSY, check that it is propagated to the TI
//

int32_t
testBusyOut()
{
  int32_t rval = OK;
  uint8_t status = 0, ti_status = 0;
  printf("%s: ----------------------------------------------------------------------\n", __func__);
  hdBusy(0);
  hdBusyStatus(&status);
  ti_status = tiGetSWBBusy(0);

  printf("%s:\n\t Busy off     status = %d     ti_status = %d\n",
	 __func__,
	 status, ti_status);

  hdBusy(1);
  hdBusyStatus(&status);
  ti_status = tiGetSWBBusy(0);

  printf("%s:\n\t Busy ON      status = %d     ti_status = %d\n",
	 __func__,
	 status, ti_status);

  hdBusy(0);

  return rval;
}

//
// TI Sends trig1 1000 times... verify receiving them in the scalers
//

int32_t
testTrigger1()
{
  int32_t rval = OK;
  volatile uint32_t scalers[9];
  printf("%s: ----------------------------------------------------------------------\n", __func__);

  if(hdReadScalers(scalers, 1))
    {
      printf("%s:\n\t Trig1 = 0x%08x (%d)\n",
	     __func__,
	     scalers[4], scalers[4]);
    }

  tiSoftTrig(1, 1000, 100, 0);

  if(hdReadScalers(scalers, 1))
    {
      printf("%s:\n\t Trig1 = 0x%08x (%d)\n",
	     __func__,
	     scalers[4], scalers[4]);
    }


  return rval;
}

//
// TI Sends trig2 1000 times... verify receiving them in the scalers
//

int32_t
testTrigger2()
{
  int32_t rval = OK;
  volatile uint32_t scalers[9];
  printf("%s: ----------------------------------------------------------------------\n", __func__);

  if(hdReadScalers(scalers, 1))
    {
      printf("%s:\n\t Trig2 = 0x%08x (%d)\n",
	     __func__,
	     scalers[5], scalers[5]);
    }

  tiSoftTrig(2, 1000, 100, 0);

  if(hdReadScalers(scalers, 1))
    {
      printf("%s:\n\t Trig2 = 0x%08x (%d)\n",
	     __func__,
	     scalers[5], scalers[5]);
    }


  return rval;
}

#include <readline/readline.h>
int com_quit(char *arg);
int com_help(char *arg);

typedef struct
{
  char *name;			/* User printable name of the function. */
  rl_icpfunc_t *func;		/* Function to call to do the job. */
  char *doc;			/* Documentation for this function.  */
} COMMAND;

COMMAND commands[] = {
  {"help", com_help, "Display this text"},
  {"?", com_help, "Synonym for `help'"},
  {"clock", testClock, "Check Clock PLL Lock"},
  {"syncreset", testSyncReset, "Check SYNCRESET"},
  {"busy", testBusyOut, "Test BUSYOUT"},
  {"trig1", testTrigger1, "Check TRIG1"},
  {"trig2", testTrigger2, "Check TRIG2"},
  {"quit", com_quit, "Quit"},
  {(char *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL}
};
#include "readline_menu.h"

int
main(int argc, char *argv[])
{
  int stat;
  uint32_t address=0;
  progname = dupstr(argv[0]);

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

  // resets required before syncs and trigs
  tiClockReset();
  taskDelay(1);
  tiTrigLinkReset();
  taskDelay(1);

  // sd init
  stat = sdInit(0);

  // helicity decoder init + config
  hdInit(address, HD_INIT_VXS, HD_INIT_EXTERNAL_FIBER, 0);

  uint32_t hd_slotnumber = 0;
  hdGetSlotNumber(&hd_slotnumber);
  hdStatus(1);

  // Enable BUSY from helicity decoder to TI
  sdSetActiveVmeSlots(1 << hd_slotnumber);
  sdStatus(1);

  initialize_readline(progname);	/* Bind our completer. */
  com_help("");
  strcat(progname, ": ");
  readline_menu_loop(progname);


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
  compile-command: "make -k hdTestVXS "
  End:
*/
