/*
 * File:
 *    hdFirmwareUpdate.c
 *
 * Description:
 *    JLab helicity decoder firmware updating program for a single board.
 *
 *
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "jvme.h"
#include "hdLib.h"
#include "hdFirmwareTools.h"

char *progName;
void Usage();

int
main(int argc, char *argv[])
{
  int iflag=0, stat=0;
  int inputchar=10;
  unsigned int hd_address=0;
  char *rbf_filename;

  printf("\nJLAB Helicity Decoder Firmware Update\n");
  printf("----------------------------\n");

  progName = argv[0];

  if(argc<3)
    {
      printf(" ERROR: Must specify two arguments\n");
      Usage();
      exit(-1);
    }
  else
    {
      rbf_filename = argv[1];
      hd_address = (unsigned int) strtoll(argv[2],NULL,16)&0xffffffff;
    }

  stat = hdFirmwareReadFile(rbf_filename);
  if(stat != OK)
    exit(-1);

  vmeSetQuietFlag(1);
  stat = vmeOpenDefaultWindows();
  if(stat != OK)
    goto CLOSE;

  iflag |= HD_INIT_NO_INIT;
  iflag |= HD_INIT_IGNORE_FIRMWARE;

  stat = hdInit(hd_address, HD_INIT_INTERNAL, HD_INIT_EXTERNAL_FIBER, iflag);
  if(stat != OK)
    goto CLOSE;

  unsigned int cfw=0;

  printf("\n");
  cfw = hdGetFirmwareVersion();
  printf(" FPGA Firmware Version: 0x%02x\n",cfw);
  printf("\n");

  printf(" Will update firmware with file: \n   %s\n",rbf_filename);
  printf(" for Helicity Decoder with VME adderss = 0x%08x\n", hd_address);

  printf(" <ENTER> to continue... or q and <ENTER> to quit without update.\n");

  inputchar = getchar();

  if((inputchar == 113) ||
     (inputchar == 81))
    {
      printf(" Quitting without update.\n");
      goto CLOSE;
    }

  vmeBusLock();

  stat = hdFirmwareEraseEPROM();
  if(stat != OK)
    {
      vmeBusUnlock();
      goto CLOSE;
    }

  stat = hdFirmwareDownloadConfigData(1);
  if(stat != OK)
    {
      vmeBusUnlock();
      goto CLOSE;
    }

  stat = hdFirmwareVerifyDownload(1);
  if(stat != OK)
    {
      vmeBusUnlock();
      goto CLOSE;
    }

  vmeBusUnlock();

 CLOSE:

  vmeCloseDefaultWindows();

  return OK;
}


void
Usage()
{
  printf("\n");
  printf("%s <firmware rbf file> <f1TDC VME ADDRESS>\n",progName);
  printf("\n");

}
