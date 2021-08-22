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

#ifndef __JVME_DEVADDR_T
#define __JVME_DEVADDR_T
typedef unsigned long devaddr_t;
#endif

static volatile HD *hdp=NULL;	  /* pointer to HD memory map */
static volatile uint32_t *hdDatap=NULL; /* pointer to HD data memory map */
static devaddr_t hdA24Offset = 0; /* Offset between VME A24 and Local address space */
static devaddr_t hdA32Offset = 0x09000000; /* Offset between VME A32 and Local address space */
static devaddr_t hdA32Base = 0x09000000; /* VME A32 to use for data */


/* Mutex for thread safe read/writes */
pthread_mutex_t hdMutex = PTHREAD_MUTEX_INITIALIZER;
#define HLOCK   if(pthread_mutex_lock(&hdMutex)<0) perror("pthread_mutex_lock");
#define HUNLOCK if(pthread_mutex_unlock(&hdMutex)<0) perror("pthread_mutex_unlock");

#define CHECKINIT {							\
    if(hdp == NULL)							\
      {                                                                 \
        logMsg("%s: ERROR: Helcity Decoder is not initialized \n",	\
               __func__,2,3,4,5,6);					\
        return ERROR;                                                   \
      }                                                                 \
  }


/**
 * @defgroup Config Initialization/Configuration
 * @defgroup Status Status
 * @defgroup Readout Data Readout
 */

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

#ifndef CHECKOFFSET
#define CHECKOFFSET(_x, _y)						\
  offset = ((uintptr_t) &fbase._y) - base;				\
  expected = _x;							\
  if(offset != expected)						\
  {									\
  printf("%s: ERROR ->%s not at offset = 0x%lx (@ 0x%lx)\n",		\
	 __func__, #_y ,expected, offset);				\
  rval = ERROR;								\
  }
#endif

  CHECKOFFSET(0x04, csr);
  CHECKOFFSET(0x10, adr32);
  CHECKOFFSET(0x30, trig1_scaler);
  CHECKOFFSET(0x74, helicity_history4);

  return rval;
}

/**
 *  @ingroup Config
 *  @brief Initialize the TIp register space into local memory,
 *  and setup registers given user input
 *
 *
 *  @param vAddr  VME Address or Slot Number
 *     - A24 VME Address (0x000016 - 0xffffff)
 *     - Slot number of TI (1 - 21)
 *
 *  @param source  Clock, Trigger, and SyncReset Source
 *     - 0 HD_INIT_INTERNAL  Internal
 *     - 1 HD_INIT_FP        Front Panel (1)
 *     - 2 HD_INIT_VXS       VXS
 *
 *  @param iFlag Initialization bit mask
 *     - 0   Do not initialize the board, just setup the pointers to the registers
 *     - 1   Use Slave Fiber 5, instead of 1
 *     - 2   Ignore firmware check
 *
 *  @return OK if successful, otherwise ERROR.
 *
 */

int32_t
hdInit(uint32_t vAddr, uint8_t source, uint32_t iFlag)
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
  hdReset();
  hdSetA32(hdA32Base);

  /* Set Clock, Trigger, Sync Source */
  hdSetSignalSources(source, source, source);


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

/**
 * @ingroup Status
 * @brief Print some status information module to standard out
 *
 * @param pflag if pflag>0, print out raw registers
 *
 */

int32_t
hdStatus(int pflag)
{
  HD rv;

#ifndef READHD
#define READHD(_reg)				\
  rv._reg = vmeRead32(&hdp->_reg);
#endif

#ifndef PREG
#define PREG(_off, _reg)						\
      printf("  %18.18s (0x%04lx) = 0x%08x%s",				\
	     #_reg, (unsigned long)&rv._reg - (unsigned long)&rv, rv._reg, \
	     #_off);
#endif

  return OK;
}

/**
 * @ingroup Config
 * @brief Hard Reset the module
 *
 * @return OK if successful, otherwise ERROR
 */

int32_t
hdReset()
{
  int32_t rval = OK;
  CHECKINIT;

  HLOCK;
  vmeWrite32(&hdp->csr, HD_CSR_HARD_RESET);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Set the A32 Base
 *
 * @return OK if successful, otherwise ERROR
 */

int32_t
hdSetA32(uint32_t a32base)
{
  int32_t rval = OK;
  uint32_t wreg = 0;

  if((a32base & HD_ADR32_BASE_MASK) == 0)
    {
      printf("%s: ERROR: Invalid a32base (0x%08x)\n",
	     __func__, a32base);
      return ERROR;
    }

  if(hdDatap != NULL)
    {
      /* If the pointer had already be defined, redefine it and update
	 the module register
      */
      devaddr_t laddr = 0;
      int32_t res = vmeBusToLocalAdrs(0x09,
				      (char *)(devaddr_t)a32base,
				      (char **)&laddr);
      if (res != 0)
	{
	  printf("%s: ERROR in vmeBusToLocalAdrs(0x09,0x%x,&laddr) \n",
		 __func__, a32base);
	  return ERROR;
	}

      HLOCK;
      hdA32Base = a32base;
      hdA32Offset = laddr - hdA32Base;
      hdDatap = (uint32_t *)(laddr);  /* Set a pointer to the FIFO */

      wreg = ((a32base >> 16) & HD_ADR32_BASE_MASK) | HD_ADR32_ENABLE;

      vmeWrite32(&hdp->adr32, 0);
      vmeWrite32(&hdp->adr32, wreg);
      HUNLOCK;
    }
  else
    {
      HLOCK;
      hdA32Base = a32base;
      HUNLOCK;
    }

  return rval;
}

/**
 * @ingroup Config
 * @brief Set the signal sources for the module
 *
 * @param clkSrc Clock Source
 *      0 Internal
 *      1 Front Panel
 *      2 VXS
 * @param trigSrc Trigger Source
 *      0 Internal
 *      1 Front Panel
 *      2 VXS
 * @param srSrc SyncReset Source
 *      0 Internal
 *      1 Front Panel
 *      2 VXS
 *
 * @return OK if successful, otherwise ERROR
 */

int32_t
hdSetSignalSources(uint8_t clkSrc, uint8_t trigSrc, uint8_t srSrc)
{
  int32_t rval = OK;
  CHECKINIT;

  uint32_t wreg = 0;
  switch(clkSrc)
    {
    case HD_INIT_INTERNAL:
      wreg = HD_CTRL1_CLK_SRC_INT | HD_CTRL1_INT_CLK_ENABLE;
      break;

    case HD_INIT_FP:
      wreg = HD_CTRL1_CLK_SRC_FP;
      break;

    case HD_INIT_VXS:
      wreg = HD_CTRL1_CLK_SRC_P0;
      break;

    default:
      wreg = HD_CTRL1_CLK_SRC_INT;
      printf("%s: Invalid source (%d).  Clock source set to Internal\n",
	     __func__, clkSrc);
    }

  switch(trigSrc)
    {
    case HD_INIT_INTERNAL:
      wreg |= HD_CTRL1_TRIG_SRC_SOFT;
      break;

    case HD_INIT_FP:
      wreg |= HD_CTRL1_TRIG_SRC_FP;
      break;

    case HD_INIT_VXS:
      wreg |= HD_CTRL1_TRIG_SRC_P0;
      break;

    default:
      wreg |= HD_CTRL1_TRIG_SRC_SOFT;
      printf("%s: Invalid source (%d).  Trigger source set to Internal\n",
	     __func__, trigSrc);
    }

  switch(srSrc)
    {
    case HD_INIT_INTERNAL:
      wreg |= HD_CTRL1_SYNC_RESET_SRC_SOFT;
      break;

    case HD_INIT_FP:
      wreg |= HD_CTRL1_SYNC_RESET_SRC_FP;
      break;

    case HD_INIT_VXS:
      wreg |= HD_CTRL1_SYNC_RESET_SRC_P0;
      break;

    default:
      wreg |= HD_CTRL1_SYNC_RESET_SRC_SOFT;
      printf("%s: Invalid source (%d).  SyncReset source set to Internal\n",
	     __func__, srSrc);
    }

  HLOCK;
  vmeWrite32(&hdp->ctrl1,
	     (vmeRead32(&hdp->ctrl1) &
	      ~(HD_CTRL1_CLK_SRC_MASK | HD_CTRL1_INT_CLK_ENABLE |
		HD_CTRL1_TRIG_SRC_MASK | HD_CTRL1_SYNC_RESET_SRC_MASK)) | wreg);
  taskDelay(40);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Set the helicity source for the module
 *
 * @param helSrc Helicity Source
 *      0 External
 *      1 Internal
 * @param input External Source signal type
 *      0 Fiber
 *      1 Copper
 * @param output Which helicity signals routed to front panel
 *      0 External
 *      1 Internal
 *
 * @return OK if successful, otherwise ERROR
 */


int32_t
hdSetHelicitySource(uint8_t helSrc, uint8_t input, uint8_t output)
{
  int32_t rval = OK;
  uint32_t wreg = 0;
  CHECKINIT;

  wreg  = helSrc ? HD_CTRL1_USE_INT_HELICITY : 0;
  wreg |= input ? HD_CTRL1_USE_EXT_CU_IN : 0;
  wreg |= output ? HD_CTRL1_INT_HELICITY_TO_FP : 0;

  HLOCK;
  vmeWrite32(&hdp->ctrl1,
	     (vmeRead32(&hdp->ctrl1) & ~HD_CTRL1_HEL_SRC_MASK) | wreg);
  HUNLOCK;

  return rval;
}
