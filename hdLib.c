/******************************************************************************
 *
 *  hdLib.c -  Driver library for configuration and readout of the
 *                  JLab Helicity Decoder using VxWorks 5.4+ (PPC), or
 *                  Linux (i686, x86_64) VME single board computer.
 *
 *  Authors: Bryan Moffit
 *           Jefferson Lab FEDAQ
 *           October 2020
 *
 *
 */

#ifdef VXWORKS
#include <vxWorks.h>
#include <logLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <semLib.h>
#include <vxLib.h>
#else
#include <unistd.h>
#include <stddef.h>
#endif
#include <pthread.h>
#include <stdio.h>
#include "jvme.h"

#include "hdLib.h"

static volatile HD *hdp;	/* pointer to HD memory map */
static uintptr_t hdA24Offset = 0; /* Offset between VME A24 and Local address space */

/* Mutex for thread safe read/writes */
pthread_mutex_t hdMutex = PTHREAD_MUTEX_INITIALIZER;
#define HLOCK   if(pthread_mutex_lock(&hdMutex)<0) perror("pthread_mutex_lock");
#define HUNLOCK if(pthread_mutex_unlock(&hdMutex)<0) perror("pthread_mutex_unlock");


/**
 *  @ingroup Status
 *  @brief Check the address map for consistency.
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */


int32_t
hdCheckAddresses()
{
  int32_t rval = OK;
  uintptr_t offset = 0, expected = 0, base = 0;
  HD fbase;

  printf("%s:\n\t ---------- Checking helicity decoder address space ---------- \n",
	 __func__);

  base = (uintptr_t) &fbase;

  offset = ((uintptr_t) &fbase.adr32) - base;
  expected = 0x10;
  if(offset != expected)
    {
      printf("%s: ERROR ->adr32 not at offset = 0x%lx (@ 0x%lx)\n",
	     __func__, expected, offset);
      rval = ERROR;
    }

  offset = ((uintptr_t) &fbase.control_scaler[0]) - base;
  expected = 0x30;
  if(offset != expected)
    {
      printf("%s: ERROR ->control_scaler[0] not at offset = 0x%lx (@ 0x%lx)\n",
	     __func__, expected, offset);
      rval = ERROR;
    }

  offset = ((uintptr_t) &fbase.firmware_csr) - base;
  expected = 0x78;
  if(offset != expected)
    {
      printf("%s: ERROR ->firmware_csr not at offset = 0x%lx (@ 0x%lx)\n",
	     __func__, expected, offset);
      rval = ERROR;
    }

  return rval;
}

int32_t
hdInit(uint32_t vAddr, uint32_t iFlag)
{
  uintptr_t laddr;
  uint32_t rval, boardID = 0, fwVersion = 0;
  int32_t stat;
  int32_t noBoardInit=0, noFirmwareCheck=0;
  int32_t supportedVersion = HD_SUPPORTED_FIRMWARE;


  /* Check VME address */
  if((vAddr < 0) || (vAddr > 0xffffff))
    {
      printf("%s: ERROR: Invalid VME Address (%d)\n",__func__,
	     vAddr);
    }
  if(vAddr==0)
    {
      printf("%s: Scanning for Helicity Decoder...\n",__func__);
      vAddr = hdFind();

      if(vAddr==0)
	{
	  printf("%s: ERROR: Unable to find Helcity Decoder\n",__func__);
	  return ERROR;
	}

    }
  if(vAddr < 22)
    {
      /* User enter slot number, shift it to VME A24 address */
      printf("%s: Initializing using slot number %d (VME address 0x%x)\n",
	     __func__,
	     vAddr, vAddr << 19);
      vAddr = vAddr << 19;
    }

  if(iFlag&HD_INIT_NO_INIT)
    {
      noBoardInit = 1;
    }

  stat = vmeBusToLocalAdrs(0x39, (char *)(uintptr_t)vAddr, (char **) &laddr);
  if (stat != 0)
    {
      printf("%s: ERROR: Error in vmeBusToLocalAdrs res=%d \n",
	     __func__,stat);
      return ERROR;
    }
  else
    {
      printf("Helicity Decoder VME (Local) address = 0x%.8x (0x%.8lx)\n",
	     vAddr, laddr);
    }

  hdA24Offset = laddr-vAddr;

  /* Set Up pointer */
  hdp = (HD *)laddr;

  /* Check if this address is readable */
  stat = vmeMemProbe((char *) (&hdp->version), 4, (char *)&rval);

  if (stat != 0)
    {
      printf("%s: ERROR: Helcity Decoder not addressable\n"
	     ,__func__);
      hdp=NULL;
      return ERROR;
    }
  else
    {
      /* Check that it is the helicity decoder */
      if(((rval & HD_VERSION_BOARD_TYPE_MASK) >> 16) != HD_VERSION_BOARD_TYPE)
	{
	  printf("%s: ERROR: Invalid Board ID: 0x%x (rval = 0x%08x)\n",
		 __func__,
		 (rval & HD_VERSION_BOARD_TYPE_MASK)>>16,rval);
	  hdp=NULL;
	  return ERROR;
	}
    }

  boardID = rval;

  fwVersion = boardID & HD_VERSION_FIRMWARE_MASK;

  printf("  Revision 0x%02x  Firmware Version 0x%02x\n",
	 (boardID & HD_VERSION_BOARD_REV_MASK) >> 8,
	 fwVersion);

  if(fwVersion != supportedVersion)
	{
	  if(noFirmwareCheck)
	    {
	      printf("%s: WARN: Firmware type (%d) not supported by this driver.\n"
		     "  Supported type = %d  (IGNORED)\n",
		     __func__, fwVersion, supportedVersion);
	    }
	  else
	    {
	      printf("%s: ERROR: Firmware Type (%d) not supported by this driver.\n"
		     "  Supported type = %d\n",
		     __func__, fwVersion, supportedVersion);

	      hdp=NULL;
	      return ERROR;
	    }
	}

  /* Check if we should exit here, or initialize some board defaults */
  if(noBoardInit)
    {
      return OK;
    }

  /* Reset / initialize stuff here */

  return OK;
}

/**
 *  @ingroup Config
 *  @brief Find the Helicity Decoder within the prescribed "GEO Slot to A24 VME Address"
 *           range from slot 3 to 21.
 *
 *  @return A24 VME address if found.  Otherwise, 0
 */

uint32_t
hdFind()
{
  int islot, stat, hdFound=0;
  unsigned int tAddr, rval;
  unsigned long laddr;

  for(islot = 3; islot<21; islot++)
    {
      tAddr = (islot<<19);

      stat = vmeBusToLocalAdrs(0x39, (char *)(unsigned long)tAddr, (char **)&laddr);

      if(stat != 0)
	continue;

      /* Check if this address is readable */
      stat = vmeMemProbe((char *)(laddr), 4, (char *)&rval);

      if (stat != 0)
	{
	  continue;
	}
      else
	{
	  /* Check that it is the helicity decoder */
	  if(((rval & HD_VERSION_BOARD_TYPE_MASK) >> 16) != HD_VERSION_BOARD_TYPE)
	    {
	      continue;
	    }
	  else
	    {
	      printf("%s: Found Helicity Decoder at 0x%08x\n",
		     __func__,tAddr);
	      hdFound=1;
	      break;
	    }
	}
    }

  if(hdFound)
    return tAddr;
  else
    return 0;

}
