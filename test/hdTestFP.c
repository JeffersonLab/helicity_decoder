/*
 * File:
 *    hdTestFP
 *
 * Description:
 *    Program to test Front Panel connections of the helicity generator
 *
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "jvme.h"
#include "tiLib.h"
#include "hdLib.h"
#include "fadcLib.h"

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
testSyncReset(char *char_nsync)
{
  int32_t rval = OK;
  volatile uint32_t scalers[9];
  uint32_t initial_count = 0, final_count = 0;

  int32_t nsync = atoi(char_nsync);
  if (nsync <= 0)
    nsync = 100;


  printf("%s(%d): ----------------------------------------------------------------------\n",
	 __func__, nsync);

  if(hdReadScalers(scalers, 1))
    {
      initial_count = scalers[6];
#ifdef DEBUG
      printf("%s:\n\t SyncReset = 0x%08x (%d)\n",
	     __func__,
	     scalers[6], scalers[6]);
#endif
    }

  int32_t isync;
  for(isync = 0; isync < nsync; isync++)
    faSDC_Sync();

  if(hdReadScalers(scalers, 1))
    {
      final_count = scalers[6];
#ifdef DEBUG
      printf("%s:\n\t SyncReset = 0x%08x (%d)\n",
	     __func__,
	     scalers[6], scalers[6]);
#endif
    }

  printf("\t\tSyncReset sent: %d\n", nsync);
  printf("\t\t initial count: %d\n", initial_count);
  printf("\t\t   final count: %d\n", final_count);
  printf("\t\t  ----------------\n");
  uint32_t diff = final_count - initial_count;
  printf("\t\t   %d %s %d\n", nsync,
	 (diff == nsync) ? "=" : "!=",
	 diff);


  return rval;
}

//
// Enable Forced BUSY, check that it is propagated to the TI
//

int32_t
testBusyOut()
{
  int32_t rval = OK;
  uint8_t status = 0;
  uint16_t sdc_status = 0;
  printf("%s: ----------------------------------------------------------------------\n", __func__);

  hdBusy(0);
  hdBusyStatus(&status);
  sdc_status = faSDC_BusyStatus();

  printf("%s:\n\t Busy off     HD status = %d     sdc_status = 0x%x\n",
	 __func__,
	 status, sdc_status);

  hdBusy(1);
  hdBusyStatus(&status);
  sdc_status = faSDC_BusyStatus();
  printf("%s:\n\t Busy ON      HD status = %d     sdc_status = 0x%x\n",
	 __func__,
	 status, sdc_status);

  hdBusy(0);

  return rval;
}

//
// TI Sends trig1 1000 times... verify receiving them in the scalers
//

int32_t
testTrigger1(char *char_ntrig)
{
  int32_t rval = OK;
  volatile uint32_t scalers[9];
  uint32_t initial_count = 0, final_count = 0;

  int32_t ntrig = atoi(char_ntrig);
  if(ntrig <= 0)
    ntrig = 100;
  if(ntrig > 0xffff)
    ntrig = 0xffff;

  printf("%s(%d): ----------------------------------------------------------------------\n",
	 __func__, ntrig);


  faSDC_Sync();
  tiSyncReset(1);

  hdEnable(0);
  if(hdReadScalers(scalers, 1))
    {
      initial_count = scalers[4];
#ifdef DEBUG
      printf("%s:\n\t Trig1 = 0x%08x (%d)\n",
	     __func__,
	     scalers[4], scalers[4]);
#endif
    }

  int32_t wait_time = 50 * ntrig;
  tiResetBlockReadout();
  tiEnableTriggerSource();

  tiSoftTrig(1, ntrig, 1000, 0);
  usleep(wait_time);
  tiDisableTriggerSource(0);
  hdDisable(0);

  if(hdReadScalers(scalers, 1))
    {
      final_count = scalers[4];
#ifdef DEBUG
      printf("%s:\n\t Trig1 = 0x%08x (%d)\n",
	     __func__,
	     scalers[4], scalers[4]);
#endif
    }

  printf("\t\t    trig1 sent: %d\n", ntrig);
  printf("\t\t initial count: %d\n", initial_count);
  printf("\t\t   final count: %d\n", final_count);
  printf("\t\t  ----------------\n");
  uint32_t diff = final_count - initial_count;
  printf("\t\t   %d %s %d\n", ntrig,
	 (diff == ntrig) ? "=" : "!=",
	 diff);

  return rval;
}

int32_t
status(char *choice)
{
  if((strlen(choice) == 0) | (strcasecmp(choice, "ti") == 0))
    tiStatus(1);

  if((strlen(choice) == 0) | (strcasecmp(choice, "hd") == 0))
    hdStatus(1);

  if((strlen(choice) == 0) | (strcasecmp(choice, "sdc") == 0))
    faSDC_Status(1);

  return 0;
}

int32_t
enable(char *choice)
{
  if((strlen(choice) == 0) | (strcasecmp(choice, "hd") == 0))
    {
      printf("%s: %s\n", __func__, "hd");
      hdEnable();
    }

  return 0;
}

int32_t
disable(char *choice)
{
  if((strlen(choice) == 0) | (strcasecmp(choice, "hd") == 0))
    {
      printf("%s: %s\n", __func__, "hd");
      hdDisable();
    }

  return 0;
}

int32_t
reset(char *choice)
{
  if((strlen(choice) == 0) | (strcasecmp(choice, "hd") == 0))
    {
      printf("%s: %s\n", __func__, "hd");
      hdReset(0, 0);
    }

  return 0;
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
  {"clock", testClock, "Check Clock PLL"},
  {"syncreset", testSyncReset, "Check SYNCRESET"},
  {"busy", testBusyOut, "Test BUSYOUT"},
  {"trig1", testTrigger1, "Check TRIG1"},
  {"status", status, "Status for TI, SD, HD"},
  {"enable", enable, "Enable HD"},
  {"disable", disable, "Disable HD"},
  {"reset", reset, "Reset HD"},
  {"quit", com_quit, "Quit"},
  {(char *) NULL, (rl_icpfunc_t *) NULL, (char *) NULL}
};
#include "readline_menu.h"

int
main(int argc, char *argv[])
{
  int stat;
  uint32_t address=0;
  char *progname = dupstr(argv[0]);

  if (argc > 1)
    {
      address = (unsigned int) strtoll(argv[1],NULL,16)&0xffffffff;
    }
  else
    {
      address = 0x00a80000; // 5 MeV MOTT DAQ Helicity Decoder
    }

  printf("\n %s: address = 0x%08x\n", argv[0], address);
  printf("----------------------------\n");

  stat = vmeOpenDefaultWindows();
  if(stat != OK)
    goto CLOSE;

  vmeCheckMutexHealth(1);
  vmeBusLock();

  // TI init + config
  stat = tiInit(0,0,TI_INIT_SKIP_FIRMWARE_CHECK);
  tiBusyOnBufferLevel(0);
  tiSetBlockBufferLevel(0);
  tiSetTriggerSource(5);
  tiDisableDataReadout();

  // resets required before syncs and trigs
  tiClockReset();
  taskDelay(1);
  tiTrigLinkReset();
  taskDelay(1);
  tiSyncReset(1);

  // sd init
  extern struct fadc_sdc_struct *FASDCp;
  extern u_long fadcA16Offset;
  uint32_t a16addr = 0xeb00;
  u_long laddr = 0;
  int res = vmeBusToLocalAdrs(0x29,(char *)(unsigned long)a16addr,(char **)&laddr);
  if (res != 0)
    {
      printf("%s: ERROR in vmeBusToLocalAdrs(0x29,0x%x,&laddr) \n",
	     __func__, a16addr);
      goto CLOSE;
    }
  fadcA16Offset = laddr-a16addr;
  FASDCp = (struct fadc_sdc_struct *) laddr;
  faSDC_Config(1, 0x2);

  faSDC_Status(1);

  // helicity decoder init + config
  hdSetA32(0x09800000);
  hdInit(address, HD_INIT_FP, HD_INIT_EXTERNAL_FIBER, 0);

  uint32_t hd_slotnumber = 0;
  hdGetSlotNumber(&hd_slotnumber);
  hdStatus(1);


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
  compile-command: "make -k hdTestFP "
  End:
*/
