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
 *  @param helSignalSrc Helicity signal source
 *     - 0 Internal
 *     - 1 External Fiber
 *     - 2 External Copper
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
hdInit(uint32_t vAddr, uint8_t source, uint8_t helSignalSrc, uint32_t iFlag)
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
  hdReset(1, 1);
  hdSetA32(hdA32Base);

  /* Set Clock, Trigger, Sync Source */
  hdSetSignalSources(source, source, source);

  /* Set helicity source */
  switch(helSignalSrc)
    {
    case HD_INIT_EXTERNAL_FIBER:
      hdSetHelicitySource(0, 0, 0);
      break;

    case HD_INIT_EXTERNAL_COPPER:
      hdSetHelicitySource(0, 1, 0);
      break;

    default:
    case HD_INIT_INTERNAL_HELICITY:
      hdSetHelicitySource(1, 0, 1);
    }

  /* Useful defaults */
  /* blocklevel = 1*/
  hdSetBlocklevel(1);

  /* latency = 0x40 (64 x 8 ns = 512 ns),
     data delay = 0x100 (256 x 8 ns = 2048 ns) */
  hdSetProcDelay(0x100, 0x40);

  /* Enable BERR */
  hdSetBERR(1);

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
  CHECKINIT;

  uint32_t vmeAddr = (uint32_t)(devaddr_t)hdp - hdA24Offset;

#ifndef READHD
#define READHD(_reg)				\
  rv._reg = vmeRead32(&hdp->_reg);
#endif

  HLOCK;
  READHD(version);
  READHD(csr);
  READHD(ctrl1);
  READHD(ctrl2);
  READHD(adr32);
  READHD(blk_size);
  READHD(delay);
  READHD(intr);
  HUNLOCK;

#ifndef PREG
#define PREG(_off, _reg)						\
      printf("  %10.18s (0x%04lx) = 0x%08x%s",				\
	     #_reg, (unsigned long)&rv._reg - (unsigned long)&rv, rv._reg, \
	     #_off);
#endif

  printf("\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("STATUS for JLab Helicity Decoder\n");
  if(pflag)
    {
      printf("\n");
      PREG(\t,version);
      PREG(\n,csr);
      PREG(\t,ctrl1);
      PREG(\n,ctrl2);
      PREG(\t,adr32);
      PREG(\n,intr);
      PREG(\t,blk_size);
      PREG(\n,delay);
    }

  printf("\n");
  printf("  Slot    Type     Rev      FW         A24           A32\n");
  printf("  ------------------------------------------------------------------------------\n");
  /*
   *     "   0    0xdec0    0x01    0x01    0xed0000    0x09000000"
   */

  printf("  %2d    ", (rv.intr & HD_INT_GEO_MASK) >> 16);

  printf("0x%04x    ", (rv.version & HD_VERSION_BOARD_TYPE_MASK) >> 16);
  printf("0x%02x    ", (rv.version & HD_VERSION_BOARD_REV_MASK) >> 8);
  printf("0x%02x    ", rv.version & HD_VERSION_FIRMWARE_MASK);
  printf("0x%06x    ", vmeAddr);
  if(rv.adr32 & HD_ADR32_ENABLE)
    printf("0x%08x", (rv.adr32 & HD_ADR32_BASE_MASK) << 16);
  else
    printf("  disabled");
  printf("\n");


  printf("\n");
  printf("                         Configuration\n\n");


  printf("  .Signal Sources..    .Helicity Src.  Block    .Processing Delay. \n");
  printf("  Clk   Trig   Sync    Input   Output  Level        Trigger   Data \n");
  printf("  ------------------------------------------------------------------------------\n");
  /*
   *     "  INT    INT    INT    FIBER     INT       1             64    256"
   */

  printf("  %s    ",
	 (rv.ctrl1 & HD_CTRL1_CLK_SRC_MASK) == HD_CTRL1_CLK_SRC_P0 ? "VXS" :
	 (rv.ctrl1 & HD_CTRL1_CLK_SRC_MASK) == HD_CTRL1_CLK_SRC_FP ? " FP" :
	 (rv.ctrl1 & HD_CTRL1_CLK_SRC_MASK) == HD_CTRL1_CLK_SRC_FP2 ? "FP2" :
	 (rv.ctrl1 & HD_CTRL1_CLK_SRC_MASK) == HD_CTRL1_CLK_SRC_INT ? "INT" : "???");

  printf("%s    ",
	 (rv.ctrl1 & HD_CTRL1_TRIG_SRC_MASK) == HD_CTRL1_TRIG_SRC_P0 ? "VXS" :
	 (rv.ctrl1 & HD_CTRL1_TRIG_SRC_MASK) == HD_CTRL1_TRIG_SRC_FP ? " FP" :
	 (rv.ctrl1 & HD_CTRL1_TRIG_SRC_MASK) == HD_CTRL1_TRIG_SRC_FP2 ? "FP2" :
	 (rv.ctrl1 & HD_CTRL1_TRIG_SRC_MASK) == HD_CTRL1_TRIG_SRC_SOFT ? "INT" : "???");

  printf("%s    ",
	 (rv.ctrl1 & HD_CTRL1_SYNC_RESET_SRC_MASK) == HD_CTRL1_SYNC_RESET_SRC_P0 ? "VXS" :
	 (rv.ctrl1 & HD_CTRL1_SYNC_RESET_SRC_MASK) == HD_CTRL1_SYNC_RESET_SRC_FP ? " FP" :
	 (rv.ctrl1 & HD_CTRL1_SYNC_RESET_SRC_MASK) == HD_CTRL1_SYNC_RESET_SRC_FP2 ? "FP2" :
	 (rv.ctrl1 & HD_CTRL1_SYNC_RESET_SRC_MASK) == HD_CTRL1_SYNC_RESET_SRC_SOFT ?
	 "INT" : "???");

  printf("%s    ",
	 (rv.ctrl1 & HD_CTRL1_USE_INT_HELICITY) ? "INT   " :
	 (rv.ctrl1 & HD_CTRL1_HEL_SRC_MASK) == HD_CTRL1_USE_EXT_CU_IN ? "COPPER" :
	 "FIBER ");

  printf("%s     ",
	 (rv.ctrl1 & HD_CTRL1_INT_HELICITY_TO_FP) ? "INT" : "EXT");

  printf("%3d           ",
	 rv.blk_size & HD_BLOCKLEVEL_MASK);

  printf("%4d   ",
	 rv.delay & HD_DELAY_TRIGGER_MASK);

  printf("%4d",
	   (rv.delay & HD_DELAY_DATA_MASK) >> 16);
  printf("\n");
  printf("\n");


  printf("                          Event       Helicity    Force\n");
  printf("  Decoder     Triggers    Build       Generator   Busy\n");
  printf("  ------------------------------------------------------------------------------\n");
  /*
   *     "  Disabled    Disabled    Disabled    Disabled    Disabled"
  */

  printf("  %s    ",
	 rv.ctrl2 & HD_CTRL2_DECODER_ENABLE ? "ENABLED " : "Disabled");

  printf("%s    ",
	 rv.ctrl2 & HD_CTRL2_GO ? "ENABLED " : "Disabled");

  printf("%s    ",
	 rv.ctrl2 & HD_CTRL2_EVENT_BUILD_ENABLE ? "ENABLED " : "Disabled");

  printf("%s    ",
	 rv.ctrl2 & HD_CTRL2_INT_HELICITY_ENABLE ? "ENABLED " : "Disabled");

  printf("%s    ",
	 rv.ctrl2 & HD_CTRL2_FORCE_BUSY ? "ENABLED " : "Disabled");
  printf("\n");

  printf("\n");
  printf("                         Status\n");
  printf("                                                               Int Buff\n");
  printf("  Helicity   ...Block Data..       Event             Latch     ..Empty.\n");
  printf("  Sequence   Accepted  Ready      Buffer  BERR  BUSY  BUSY      0     1\n");
  printf("  ------------------------------------------------------------------------------\n");
  /*
   *     "        OK        ---    ---       Empty    lo    lo    lo    YES    YES"
   */

  printf("     %s        ",
	 rv.csr & HD_CSR_HELICITY_SEQ_ERROR ? "ERROR" : "   OK");

  printf("%s    ",
	 rv.csr & HD_CSR_BLOCK_ACCEPTED ? "YES" : "---");

  printf("%s    ",
	 rv.csr & HD_CSR_BLOCK_READY ? "YES" : "---");

  printf("%s   ",
	 rv.csr & HD_CSR_EMPTY ? "   Empty" : "NotEmpty");

  printf("%s   ",
	 rv.csr & HD_CSR_BERR_ASSERTED ? " HI" : " lo");

  printf("%s   ",
	 rv.csr & HD_CSR_BUSY ? " HI" : " lo");

  printf("%s    ",
	 rv.csr & HD_CSR_BUSY_LATCHED ? " HI" : " lo");

  printf("%s    ",
	 rv.csr & HD_CSR_INTERNAL_BUF0 ? "YES" : "---");

  printf("%s",
	 rv.csr & HD_CSR_INTERNAL_BUF1 ? "YES" : "---");
  printf("\n");
  printf("\n");

  hdPrintScalers();
  printf("\n");
  hdPrintHelicityGeneratorConfig();
  printf("\n");

  printf("--------------------------------------------------------------------------------\n");
  printf("\n\n");



  return OK;
}

/**
 * @ingroup Config
 * @brief Reset the module
 *
 * @param type Reset Type
 *           0 Soft
 *           1 Hard
 *           2 Both
 * @param clearA32 Flag to clear A32 settings
 *           0 Restore A32 settings after reset
 *           1 Wipe them out.  All of them.
 *
 * @return OK if successful, otherwise ERROR
 */

int32_t
hdReset(uint8_t type, uint8_t clearA32)
{
  int32_t rval = OK;
  uint32_t wreg = 0;
  uint32_t adr32 = 0;
  CHECKINIT;

  switch(type)
    {
    case 0:
      wreg = HD_CSR_SOFT_RESET;
      break;
    case 1:
      wreg = HD_CSR_HARD_RESET;
      break;
    case 2:
    default:
      wreg = HD_CSR_SOFT_RESET | HD_CSR_HARD_RESET;
    }

  clearA32 = clearA32 ? 1 : 0;

  if(!clearA32)
    adr32 = hdGetA32();

  HLOCK;
  vmeWrite32(&hdp->csr, HD_CSR_HARD_RESET);
  HUNLOCK;

  if(!clearA32)
    hdSetA32(adr32);

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

  if(((a32base >> 16) & HD_ADR32_BASE_MASK) == 0)
    {
      printf("%s: ERROR: Invalid a32base (0x%08x)\n",
	     __func__, a32base);
      return ERROR;
    }

  if(hdp != NULL)
    {
      /* If the library has been initialized, configure pointer and register */
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
 * @ingroup Status
 * @brief Get the A32 Base
 *
 * @return OK if successful, otherwise ERROR
 */

uint32_t
hdGetA32()
{
  uint32_t rval;
  HLOCK;
  rval = hdA32Base;
  HUNLOCK;

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
  uint32_t wreg = 0;
  CHECKINIT;


  /* Clock Source */
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

  /* Trigger Source */
  switch(trigSrc)
    {
    case HD_INIT_INTERNAL:
      wreg |= HD_CTRL1_TRIG_SRC_SOFT | HD_CTRL1_SOFT_CONTROL_ENABLE;
      break;

    case HD_INIT_FP:
      wreg |= HD_CTRL1_TRIG_SRC_FP;
      break;

    case HD_INIT_VXS:
      wreg |= HD_CTRL1_TRIG_SRC_P0;
      break;

    default:
      wreg |= HD_CTRL1_TRIG_SRC_SOFT | HD_CTRL1_SOFT_CONTROL_ENABLE;
      printf("%s: Invalid source (%d).  Trigger source set to Internal\n",
	     __func__, trigSrc);
    }

  /* SyncReset Source */
  switch(srSrc)
    {
    case HD_INIT_INTERNAL:
      wreg |= HD_CTRL1_SYNC_RESET_SRC_SOFT | HD_CTRL1_SOFT_CONTROL_ENABLE;
      break;

    case HD_INIT_FP:
      wreg |= HD_CTRL1_SYNC_RESET_SRC_FP;
      break;

    case HD_INIT_VXS:
      wreg |= HD_CTRL1_SYNC_RESET_SRC_P0;
      break;

    default:
      wreg |= HD_CTRL1_SYNC_RESET_SRC_SOFT | HD_CTRL1_SOFT_CONTROL_ENABLE;
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
 * @ingroup Status
 * @brief Get the clock, trigger, and syncreset source for the module
 *
 * @param *clkSrc Address to store Clock Source
 *      0 Internal
 *      1 Front Panel
 *      2 VXS
 * @param *trigSrc Address to store Trigger Source
 *      0 Internal
 *      1 Front Panel
 *      2 VXS
 * @param *srSrc Address to store SyncReset Source
 *      0 Internal
 *      1 Front Panel
 *      2 VXS
 *
 * @return OK if successful, otherwise ERROR
 */

int32_t
hdGetSignalSources(uint8_t *clkSrc, uint8_t *trigSrc, uint8_t *srSrc)
{
  int32_t rval = OK;
  uint32_t rreg = 0, signal = 0;
  CHECKINIT;

  HLOCK;
  rreg = vmeRead32(&hdp->ctrl1);
  HUNLOCK;

  /* Clock Source */
  signal = rreg & HD_CTRL1_CLK_SRC_MASK;
  switch(signal)
    {
    case HD_CTRL1_CLK_SRC_P0:
      *clkSrc = HD_INIT_VXS;
      break;

    case HD_CTRL1_CLK_SRC_FP:
      *clkSrc = HD_INIT_FP;
      break;

    case HD_CTRL1_CLK_SRC_INT:
    default:
      *clkSrc = HD_INIT_INTERNAL;
    }

  /* Trigger Source */
  signal = rreg & HD_CTRL1_TRIG_SRC_MASK;
  switch(signal)
    {
    case HD_CTRL1_TRIG_SRC_P0:
      *trigSrc = HD_INIT_VXS;
      break;

    case HD_CTRL1_TRIG_SRC_FP:
      *trigSrc = HD_INIT_FP;
      break;

    case HD_CTRL1_TRIG_SRC_SOFT:
    default:
      *trigSrc = HD_INIT_INTERNAL;
    }

  /* SyncReset Source */
  signal = rreg & HD_CTRL1_SYNC_RESET_SRC_MASK;
  switch(signal)
    {
    case HD_CTRL1_SYNC_RESET_SRC_P0:
      *srSrc = HD_INIT_VXS;
      break;

    case HD_CTRL1_SYNC_RESET_SRC_FP:
      *srSrc = HD_INIT_FP;
      break;

    case HD_CTRL1_SYNC_RESET_SRC_SOFT:
    default:
      *srSrc = HD_INIT_INTERNAL;
    }

  return rval;
}
/**
 * @ingroup Config
 * @brief Configure helicity source, input, and output, for the module
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

/**
 * @ingroup Status
 * @brief Get the helicity source, input, and output for the module
 *
 * @param *helSrc Address to store Helicity Source
 *      0 External
 *      1 Internal
 * @param *input Address to store External Source signal type
 *      0 Fiber
 *      1 Copper
 * @param *output Address to store helicity signals routed to front panel
 *      0 External
 *      1 Internal
 *
 * @return OK if successful, otherwise ERROR
 */

int32_t
hdGetHelicitySource(uint8_t *helSrc, uint8_t *input, uint8_t *output)
{
  int32_t rval = OK;
  uint32_t rreg = 0;
  CHECKINIT;

  HLOCK;
  rreg = vmeRead32(&hdp->ctrl1) & HD_CTRL1_HEL_SRC_MASK;
  HUNLOCK;

  *helSrc = (rreg & HD_CTRL1_USE_INT_HELICITY) ? 1 : 0;
  *input = (rreg & HD_CTRL1_USE_EXT_CU_IN) ? 1 : 0;
  *output = (rreg & HD_CTRL1_INT_HELICITY_TO_FP) ? 1 : 0;

  return rval;
}

/**
 * @ingroup Config
 * @brief Set the Blocklevel for the module
 *
 * @param blklevel Block level [0-255]
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdSetBlocklevel(uint8_t blklevel)
{
  int32_t rval = OK;
  CHECKINIT;

  HLOCK;
  vmeWrite32(&hdp->blk_size, blklevel);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the blocklevel for the modules
 *
 * @return Blocklevel if successful, otherwise ERROR
 */
int32_t
hdGetBlocklevel()
{
  int32_t rval = OK;
  CHECKINIT;

  HLOCK;
  rval = vmeRead32(&hdp->blk_size) & 0xFF;
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Set the helicity and trigger processing delay for the module
 *
 * @param dataInputDelay Data Input Delay [1-4095]
 *                       1 count = 8ns
 * @param triggerLatencyDelay Trigger Latency Delay [1-4095]
 *                       1 count = 8ns
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdSetProcDelay(uint16_t dataInputDelay, uint16_t triggerLatencyDelay)
{
  int32_t rval = OK;
  uint32_t wreg = 0;
  CHECKINIT;

  if((dataInputDelay == 0) || (dataInputDelay > 0xFFF))
    {
      printf("%s: ERROR: Invalid dataInputDelay (%d)\n",
	     __func__, dataInputDelay);
      return ERROR;
    }
  if((triggerLatencyDelay == 0) || (triggerLatencyDelay > 0xFFF))
    {
      printf("%s: ERROR: Invalid triggerLatencyDelay (%d)\n",
	     __func__, triggerLatencyDelay);
      return ERROR;
    }

  wreg = triggerLatencyDelay | (dataInputDelay << 16);
  HLOCK;
  vmeWrite32(&hdp->delay, wreg);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the programmed helicity and trigger processing delay for the module
 *
 * @param *dataInputDelay Address for Data Input Delay [1-4095]
 *                       1 count = 8ns
 * @param *triggerLatencyDelay Address for Trigger Latency Delay [1-4095]
 *                       1 count = 8ns
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdGetProcDelay(uint16_t *dataInputDelay, uint16_t *triggerLatencyDelay)
{
  int32_t rval = OK;
  uint32_t rreg = 0;
  CHECKINIT;

  HLOCK;
  rreg = vmeRead32(&hdp->delay);

  *dataInputDelay = (rreg & HD_DELAY_DATA_MASK) >> 16;
  *triggerLatencyDelay = rreg & HD_DELAY_TRIGGER_MASK;
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Confirm that programmed processing delays match the latched
 * Write-Read addresses
 *
 * @param pflag Print Flag
 *              Print results to standard output
 *
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdConfirmProcDelay(uint8_t pflag)
{
  int32_t rval = OK;
  uint32_t rreg_programmed = 0, rreg_trigger = 0, rreg_data = 0;
  uint16_t dataInputDelay = 0, triggerLatencyDelay = 0;
  int32_t wraddr = 0, rdaddr = 0, delay = 0;
  CHECKINIT;

  HLOCK;
  rreg_programmed = vmeRead32(&hdp->delay);
  rreg_trigger = vmeRead32(&hdp->latency_confirm);
  rreg_data = vmeRead32(&hdp->delay_confirm);
  HUNLOCK;

  /* Check trigger */
  dataInputDelay = (rreg_programmed & HD_DELAY_DATA_MASK) >> 16;
  rdaddr = rreg_trigger & HD_CONFIRM_READ_ADDR_MASK;
  wraddr = rreg_trigger & HD_CONFIRM_WRITE_ADDR_MASK;

  if (wraddr > rdaddr) delay = wraddr - rdaddr;
  else                 delay = 4096 + wraddr - rdaddr;

  if(dataInputDelay != delay)
    {
      printf("%s: ERROR: Programmed dataInputDelay != wraddr-rdaddr  (0x%04x != 0x%04x)\n",
	     __func__,
	     dataInputDelay, delay);
      rval = ERROR;
    }
  else if(pflag)
    {
      printf("%s: dataInputDelay Confirmed 0x%04x = %s0x%04x - 0x%04x\n",
	     __func__,
	     dataInputDelay,
	     (wraddr > rdaddr) ? "" : "4096 + ",
	     wraddr, rdaddr);
    }

  /* Check data */
  triggerLatencyDelay = rreg_programmed & HD_DELAY_TRIGGER_MASK;
  rdaddr = rreg_data & HD_CONFIRM_READ_ADDR_MASK;
  wraddr = rreg_data & HD_CONFIRM_WRITE_ADDR_MASK;

  if (wraddr > rdaddr) delay = wraddr - rdaddr;
  else                 delay = 4096 + wraddr - rdaddr;

  if(triggerLatencyDelay != delay)
    {
      printf("%s: ERROR: Programmed triggerLatencydelay != wraddr-rdaddr  (0x%04x != 0x%04x)\n",
	     __func__,
	     triggerLatencyDelay, delay);
      rval = ERROR;
    }
  else if(pflag)
    {
      printf("%s: triggerLatencydelay Confirmed 0x%04x = %s0x%04x - 0x%04x\n",
	     __func__,
	     triggerLatencyDelay,
	     (wraddr > rdaddr) ? "" : "4096 + ",
	     wraddr, rdaddr);
    }

  return rval;
}

/**
 * @ingroup Config
 * @brief Enable / Disable BERR Response from the module for Block Read
 *
 * @param enable Enable flag
 *            0 - Disable
 *            1 - Enable
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdSetBERR(uint8_t enable)
{
  int32_t rval = OK;
  CHECKINIT;

  HLOCK;
  if(enable)
    vmeWrite32(&hdp->ctrl1, vmeRead32(&hdp->ctrl1) | HD_CTRL1_BERR_ENABLE);
  else
    vmeWrite32(&hdp->ctrl1, vmeRead32(&hdp->ctrl1) & ~HD_CTRL1_BERR_ENABLE);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the configuration for BERR Response from the module
 *
 * @return 1 if enabled, 0 if disabled, otherwise ERROR
 */
int32_t
hdGetBERR()
{
  int32_t rval = 0;
  CHECKINIT;

  HLOCK;
  rval = (vmeRead32(&hdp->ctrl1) & HD_CTRL1_BERR_ENABLE) ? 1 : 0;
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Enable the decoder for the module.
 *     Useful if triggers will come to the module < 1s after hdEnable().
 *     Call this in prestart.
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdEnableDecoder()
{
  int32_t rval = OK;
  uint32_t wreg = HD_CTRL2_DECODER_ENABLE;
  CHECKINIT;

  HLOCK;
  vmeWrite32(&hdp->ctrl2, vmeRead32(&hdp->ctrl2) | wreg);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Enable the decoder, triggers, and event building for the module
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdEnable()
{
  int32_t rval = OK;
  uint32_t wreg = HD_CTRL2_DECODER_ENABLE | HD_CTRL2_GO | HD_CTRL2_EVENT_BUILD_ENABLE;
  CHECKINIT;

  HLOCK;
  vmeWrite32(&hdp->ctrl2, vmeRead32(&hdp->ctrl2) | wreg);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Disable the decoder, triggers, and event building for the module
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdDisable()
{
  int32_t rval = OK;
  uint32_t wreg = HD_CTRL2_DECODER_ENABLE | HD_CTRL2_GO | HD_CTRL2_EVENT_BUILD_ENABLE;
  CHECKINIT;

  HLOCK;
  vmeWrite32(&hdp->ctrl2, vmeRead32(&hdp->ctrl2) & ~wreg);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Generate a software trigger
 *
 * @param pflag Print Flag
 *              option to print action to standard out
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdTrig(int pflag)
{
  int32_t rval = OK;
  CHECKINIT;

  if(pflag)
    printf("%s: Software Trigger\n", __func__);

  HLOCK;
  vmeWrite32(&hdp->csr, HD_CSR_TRIGGER_PULSE);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Generate a software SyncReset
 *
 * @param pflag Print Flag
 *              option to print action to standard out
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdSync(int pflag)
{
  int32_t rval = OK;
  CHECKINIT;

  if(pflag)
    printf("%s: Software SyncReset\n", __func__);

  HLOCK;
  vmeWrite32(&hdp->csr, HD_CSR_SYNC_RESET_PULSE);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Set the BUSY status of the module
 *
 * @param enable Enable Flag
 *             0 Disable Busy
 *             1 Enable Busy
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdBusy(int enable)
{
  int32_t rval = OK;
  CHECKINIT;

  HLOCK;
  if(enable)
    vmeWrite32(&hdp->ctrl2, vmeRead32(&hdp->ctrl2) | HD_CTRL2_FORCE_BUSY);
  else
    vmeWrite32(&hdp->ctrl2, vmeRead32(&hdp->ctrl2) & ~HD_CTRL2_FORCE_BUSY);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the block ready status
 *
 * @return 1 if block is ready, 0 if not, otherwise ERROR
 */
int32_t
hdBReady()
{
  int32_t rval = 0;
  CHECKINIT;

  HLOCK;
  rval = (vmeRead32(&hdp->csr) & HD_CSR_BLOCK_READY) ? 1 : 0;
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the BERR status
 *
 * @return 1 if module asserted BERR, 0 if not, otherwise ERROR
 */
int32_t
hdBERRStatus()
{
  int32_t rval = 0;
  CHECKINIT;

  HLOCK;
  rval = (vmeRead32(&hdp->csr) & HD_CSR_BERR_ASSERTED) ? 1 : 0;
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the BUSY status
 *
 * @return 1 if module asserted BERR, 0 if not, otherwise ERROR
 */
int32_t
hdBusyStatus(uint8_t *latched)
{
  int32_t rval = 0;
  uint32_t rreg = 0;
  CHECKINIT;

  HLOCK;
  rreg = vmeRead32(&hdp->csr);

  rval = (rreg & HD_CSR_BUSY) ? 1 : 0;

  if(latched != NULL)
    {
      *latched = (rreg & HD_CSR_BUSY_LATCHED) ? 1 : 0;

      if(*latched) /* Clear if it's latched */
	vmeWrite32(&hdp->csr, HD_CSR_BUSY_LATCHED);
    }
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Readout
 * @brief Read a block of events from the module
 *
 * @param   data  - local memory address to place data
 * @param   nwrds - Max number of words to transfer
 * @param   rflag - Readout Flag
 *       -       0 - programmed I/O
 *       -       1 - DMA transfer using Universe/Tempe DMA Engine
 *                    (DMA VME transfer Mode must be setup prior)
 *
 * @return Number of words transferred to data if successful, ERROR otherwise
 *
 */
int32_t
hdReadBlock(volatile unsigned int *data, int nwrds, int rflag)
{
  int32_t rval = OK;
  int32_t ii, dummy=0, iword = 0;
  int32_t dCnt, retVal, xferCount;
  volatile uint32_t *laddr;
  uint32_t vmeAdr, val;
  int32_t ntrig=0, itrig = 0, trigwords = 0;

  CHECKINIT;

  HLOCK;
  if(rflag >= 1)
    { /* Block transfer */
      /* Assume that the DMA programming is already setup.
	 Don't Bother checking if there is valid data - that should be done prior
	 to calling the read routine */

      /* Check for 8 byte boundary for address -
	 insert dummy word*/
      if((devaddr_t) (data)&0x7)
	{
	  *data = LSWAP(HD_DUMMY_WORD);
	  dummy = 1;
	  laddr = (data + 1);
	}
      else
	{
	  dummy = 0;
	  laddr = data;
	}

      vmeAdr = (devaddr_t)hdDatap - hdA32Offset;

      retVal = vmeDmaSend((devaddr_t)laddr, vmeAdr, (nwrds<<2));
      if(retVal != 0)
	{
	  printf("\n%s: ERROR in DMA transfer Initialization 0x%x\n",
		 __func__, retVal);
	  HUNLOCK;
	  return(retVal);
	}

      /* Wait until Done or Error */
      retVal = vmeDmaDone();

      if(retVal > 0)
	{
	  xferCount = ((retVal>>2) + dummy); /* Number of longwords transfered */

	  HUNLOCK;
	  return(xferCount);
	}
      else if (retVal == 0)
	{
	  printf("\n%s: WARNING: DMA transfer returned zero word count 0x%x\n",
		 __func__,
		 nwrds);
	  HUNLOCK;
	  return(nwrds);
	}
      else
	{  /* Error in DMA */
	  printf("\n%s: ERROR: vmeDmaDone returned an Error\n",
		 __func__);
	  HUNLOCK;
	  return(retVal>>2);

	}
    }
  else
    { /* Programmed IO */
      dCnt = 0;
      ii=0;

      /* Check if Bus Errors are enabled. If so then disable for Prog I/O reading */
      uint8_t berr = (vmeRead32(&hdp->ctrl1) & HD_CTRL1_BERR_ENABLE) ? 1 : 0;
      if(berr)
	vmeWrite32(&hdp->ctrl1, vmeRead32(&hdp->ctrl1) & ~HD_CTRL1_BERR_ENABLE);

      /* Read Block Header - should be first word */
      uint32_t bhead = vmeRead32(hdDatap);

      if((bhead&HD_DATA_TYPE_DEFINE)&&((bhead&HD_DATA_TYPE_MASK) == HD_DATA_BLOCK_HEADER))
	{

	  data[dCnt] = LSWAP(bhead); /* Swap back to little-endian */
	  dCnt++;

	}
      else
	{
	  /* We got bad data - Check if there is any data at all */
	  if( (vmeRead32(&hdp->evt_count) & HD_EVENTS_ON_BOARD_MASK) == 0)
	    {
	      printf("%s: FIFO Empty (0x%08x)\n",
		     __func__, bhead);
	      HUNLOCK;
	      return(0);
	    }
	  else
	    {
	      printf("%s: ERROR: Invalid Header Word 0x%08x\n",
		     __func__, bhead);
	      HUNLOCK;
	      return(ERROR);
	    }
	}

      ii=0;
      while(ii<nwrds)
	{
	  val = vmeRead32(hdDatap);
	  data[ii+2] = LSWAP(val);

	  if( (val&HD_DATA_TYPE_DEFINE)
	      && ((val&HD_DATA_TYPE_MASK) == HD_DATA_BLOCK_TRAILER) )
	    break;
	  ii++;

	}
      ii++;
      dCnt += ii;

      /* Re-enabled Bus errors */
      if(berr)
	vmeWrite32(&hdp->ctrl1, vmeRead32(&hdp->ctrl1) | HD_CTRL1_BERR_ENABLE);

      HUNLOCK;
      return dCnt;
    }

  HUNLOCK;

  return rval;
}

/**
 *  @ingroup Readout
 *  @brief Scaler Data readout routine
 *
 *  @param data   - local memory address to place data
 *  @param rflag  - Readout flag
 *            0 - helicity scalers
 *            1 - helicity scalers, trig1, trig2, syncreset, evt_count, blk_count
 *            2 - trig1, trig2, syncreset, evt_count, blk_count
 *
 *
 *  @return Number of uint32 added to data if successful, otherwise ERROR.
 */
int32_t
hdReadScalers(volatile uint32_t *data, int rflag)
{
  int32_t dCnt = 0;
  CHECKINIT;

  HLOCK;
  if(rflag != 2)
    {
      data[dCnt++] = vmeRead32(&hdp->helicity_scaler[0]);
      data[dCnt++] = vmeRead32(&hdp->helicity_scaler[1]);
      data[dCnt++] = vmeRead32(&hdp->helicity_scaler[2]);
      data[dCnt++] = vmeRead32(&hdp->helicity_scaler[3]);
    }
  if(rflag != 0)
    {
      data[dCnt++] = vmeRead32(&hdp->trig1_scaler);
      data[dCnt++] = vmeRead32(&hdp->trig2_scaler);
      data[dCnt++] = vmeRead32(&hdp->sync_scaler);
      data[dCnt++] = vmeRead32(&hdp->evt_count);
      data[dCnt++] = vmeRead32(&hdp->blk_count);
    }
  HUNLOCK;

  return dCnt;
}

/**
 *  @ingroup Readout
 *  @brief Print out Scaler Data to standard out
 *
 *  @return OK if successful, otherwise ERROR.
 */
int32_t
hdPrintScalers()
{
  volatile uint32_t scalers[9];

  if(hdReadScalers(scalers, 1))
    {
      printf("  Helicity Scalers:\n");
      printf("    T_SETTLE falling = 0x%08x (%d)\n", scalers[0], scalers[0]);
      printf("    T_SETTLE rising  = 0x%08x (%d)\n", scalers[1], scalers[1]);
      printf("    PATTERN_SYNC     = 0x%08x (%d)\n", scalers[2], scalers[2]);
      printf("    PAIR_SYNC        = 0x%08x (%d)\n", scalers[3], scalers[3]);
      printf("\n");
      printf("  Signal scalers:\n");
      printf("    Trig1            = 0x%08x (%d)\n", scalers[4], scalers[4]);
      printf("    Trig2            = 0x%08x (%d)\n", scalers[5], scalers[5]);
      printf("    SyncReset        = 0x%08x (%d)\n", scalers[6], scalers[6]);
      printf("\n");
      printf("  Run Scalers:\n");
      printf("    Events           = 0x%08x (%d)\n", scalers[7], scalers[7]);
      printf("    Blocks           = 0x%08x (%d)\n", scalers[8], scalers[8]);
    }

  return OK;
}

/**
 *  @ingroup Readout
 *  @brief Helicity History register readout
 *
 *  @param data   - local memory address to place data
 *                  Bit 0 is the most recent value
 *                  element 0 : PATTERN_SYNC
 *                  element 1 : PAIR_SYNC
 *                  element 2 : reported HELICITY
 *                  element 3 : reported HELICITY at PATTERN_SYNC
 *
 *  @return Number (4) of uint32 added to data if successful, otherwise ERROR.
 */
int32_t
hdReadHelicityHistory(volatile unsigned int *data)
{
  int32_t dCnt = 0;
  CHECKINIT;

  HLOCK;
  data[dCnt++] = vmeRead32(&hdp->helicity_history1);
  data[dCnt++] = vmeRead32(&hdp->helicity_history2);
  data[dCnt++] = vmeRead32(&hdp->helicity_history3);
  data[dCnt++] = vmeRead32(&hdp->helicity_history4);
  HUNLOCK;

  return dCnt;
}

/**
 * @ingroup Status
 * @brief Get the recovered shift register value(s)
 *
 * @param *recovered Address to put current recovered value
 * @param *internalGenerator Address to put current internal generator value
 *
 * @return Blocklevel if successful, otherwise ERROR
 */
int32_t
hdGetRecoveredShiftRegisterValue(uint32_t *recovered, uint32_t *internalGenerator)
{
  int32_t rval = OK;
  int32_t rreg1 = 0, rreg2 = 0;
  CHECKINIT;

  HLOCK;
  rreg1 = vmeRead32(&hdp->recovered_shift_reg);

  if(internalGenerator != NULL)
    rreg2 = vmeRead32(&hdp->generator_shift_reg);

  HUNLOCK;

  *recovered = rreg1 & HD_RECOVERED_SHIFT_REG_MASK;

  if(internalGenerator != NULL)
    *internalGenerator = rreg2 & HD_GENERATOR_SHIFT_REG_MASK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Enable the helicity generator for the module
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdEnableHelicityGenerator()
{
  int32_t rval = OK;
  uint32_t wreg = HD_CTRL2_INT_HELICITY_ENABLE;
  CHECKINIT;

  HLOCK;
  vmeWrite32(&hdp->ctrl2, vmeRead32(&hdp->ctrl2) | wreg);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Disable the helicity generator for the module
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdDisableHelicityGenerator()
{
  int32_t rval = OK;
  uint32_t wreg = HD_CTRL2_INT_HELICITY_ENABLE;
  CHECKINIT;

  HLOCK;
  vmeWrite32(&hdp->ctrl2, vmeRead32(&hdp->ctrl2) & ~wreg);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Configure the internal helicity generator
 *
 * @param pattern Helicity Pattern [0,3]
 *              0 Pair
 *              1 Quartet
 *              2 Octet
 *              3 Toggle
 * @param windowDelay Helicity delay in windows
 * @param settleTime Helicity settle time (1 count = 40ns)
 * @param stableTime Helicity stable time (1 count = 40ns)
 * @param seed Initial pseudorandom sequence seed
*
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdHelicityGeneratorConfig(uint8_t pattern, uint8_t windowDelay,
			  uint16_t settleTime, uint32_t stableTime,
			  uint32_t seed)
{
  int32_t rval = OK;
  uint32_t rreg = 0, wreg1, wreg2, wreg3;
  uint8_t reenable=0;
  CHECKINIT;

  if(pattern > 3)
    {
      printf("%s: ERROR: Invalid pattern (%d)\n",
	     __func__, pattern);
      return ERROR;
    }

  wreg1 = pattern | (windowDelay << 8) | (settleTime << 16);
  wreg2 = stableTime & HD_HELICITY_CONFIG2_STABLE_TIME_MASK;
  wreg3 = seed & HD_HELICITY_CONFIG3_PSEUDO_SEED_MASK;

  HLOCK;
  /* Check if the generator is already enabled */
  rreg = vmeRead32(&hdp->ctrl2);
  reenable = (rreg & HD_CTRL2_INT_HELICITY_ENABLE) ? 1 : 0;

  if(reenable)
    {
      /* Disable generator */
      vmeWrite32(&hdp->ctrl2, rreg & ~HD_CTRL2_INT_HELICITY_ENABLE);
      taskDelay(10);
    }

  vmeWrite32(&hdp->gen_config1, wreg1);
  vmeWrite32(&hdp->gen_config2, wreg2);
  vmeWrite32(&hdp->gen_config3, wreg3);

  taskDelay(10);

  if(reenable)
    {
      /* Reenable generator */
      vmeWrite32(&hdp->ctrl2, rreg | HD_CTRL2_INT_HELICITY_ENABLE);
      taskDelay(10);
    }
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the configured parameeters for the internal helicity generator
 *
 * @param pattern Address to store Helicity Pattern [0,3]
 *              0 Pair
 *              1 Quartet
 *              2 Octet
 *              3 Toggle
 * @param windowDelay Address to store Helicity delay in windows
 * @param settleTime Address to store Helicity settle time (1 count = 40ns)
 * @param stableTime Address to store Helicity stable time (1 count = 40ns)
 * @param seed Initial Address to store pseudorandom sequence seed
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdGetHelicityGeneratorConfig(uint8_t *pattern, uint8_t *windowDelay,
			     uint16_t *settleTime, uint32_t *stableTime,
			     uint32_t *seed)
{
  int32_t rval = OK;
  uint32_t rreg1, rreg2, rreg3;
  CHECKINIT;

  HLOCK;
  rreg1 = vmeRead32(&hdp->gen_config1);
  rreg2 = vmeRead32(&hdp->gen_config2);
  rreg3 = vmeRead32(&hdp->gen_config3);
  HUNLOCK;

  *pattern = rreg1 & HD_HELICITY_CONFIG1_PATTERN_MASK;
  *windowDelay = (rreg1 & HD_HELICITY_CONFIG1_HELICITY_DELAY_MASK) >> 8;
  *settleTime = (rreg1 & HD_HELICITY_CONFIG1_HELICITY_SETTLE_MASK) >> 16;
  *stableTime = rreg2 & HD_HELICITY_CONFIG2_STABLE_TIME_MASK;
  *seed = rreg3 & HD_HELICITY_CONFIG3_PSEUDO_SEED_MASK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Print to standard outevent the configured parameeters for
 * the internal helicity generator
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdPrintHelicityGeneratorConfig()
{
  uint8_t pattern, windowDelay;
  uint16_t settleTime;
  uint32_t stableTime, seed;

  if(hdGetHelicityGeneratorConfig(&pattern, &windowDelay,
				  &settleTime, &stableTime,
				  &seed) == OK)
    {
      printf("\n");
      printf("  Helicity Generator Configuration\n");
      printf("\n");
      printf("               Window Settle                Stable\n");
      printf("    Pattern    Delay  Time                  Time                     Seed\n");
      printf("  ------------------------------------------------------------------------------\n");
      /*
       *     "    PAIR       0      0x0000 (    0 ns)     0x0000000 (        0 ns) 0x00000000"
       */

      printf("    %s  ",
	     (pattern == 0) ? "PAIR   " :
	     (pattern == 1) ? "QUARTET" :
	     (pattern == 2) ? "OCTET  " :
	     (pattern == 3) ? "TOGGLE " : "???????" );

      printf("%3d      ", windowDelay);

      printf("0x%04x (%5d ns)     ", settleTime, settleTime * 8);
      printf("0x%07x (%9d ns) ", stableTime, stableTime * 8);

      printf("0x%08x", seed);
      printf("\n");
    }

  return OK;
}

struct data_struct
{
  uint32_t new_type;
  uint32_t type;
  uint32_t slot_id_hd;
  uint32_t mod_id_hd;
  uint32_t slot_id_tr;
  uint32_t n_evts;
  uint32_t blk_num;
  uint32_t n_words;
  uint32_t evt_num_1;
  uint32_t trig_time;
  uint32_t time_now;
  uint32_t time_1;
  uint32_t time_2;
  uint32_t num_words;
  uint32_t decoder[16];
} hd_data;

void
hdDecodeData(uint32_t data)
{
/* Helicity decoder */

  static uint32_t type_last = 15;	/* initialize to type FILLER WORD */
  static uint32_t time_last = 0;
  static uint32_t decoder_index = 0;
  static uint32_t num_decoder_words = 1;

  static uint32_t slot_id_ev_hd = 0;
  static uint32_t slot_id_dnv = 0;
  static uint32_t slot_id_fill = 0;
  int32_t i_print = 1;

  if(decoder_index)		/* decoder type data word - set by decoder header word */
    {
      hd_data.type = 16;	/*  set decoder data words as type 16 */
      hd_data.new_type = 0;
      if(decoder_index < num_decoder_words)
	{
	  hd_data.decoder[decoder_index - 1] = data;
	  if(i_print)
	    printf("%8X - decoder data(%d) = %d\n", data, (decoder_index - 1),
		   data);
	  decoder_index++;
	}
      else			/* last decoder word */
	{
	  hd_data.decoder[decoder_index - 1] = data;
	  if(i_print)
	    printf("%8X - decoder data(%d) = %d\n", data, (decoder_index - 1),
		   data);
	  decoder_index = 0;
	  num_decoder_words = 1;
	}
    }
  else				/* normal typed word */
    {
      if(data & 0x80000000)	/* data type defining word */
	{
	  hd_data.new_type = 1;
	  hd_data.type = (data & 0x78000000) >> 27;
	}
      else			/* data type continuation word */
	{
	  hd_data.new_type = 0;
	  hd_data.type = type_last;
	}

      switch (hd_data.type)
	{
	case 0:		/* BLOCK HEADER */
	  hd_data.slot_id_hd = (data & 0x7C00000) >> 22;
	  hd_data.mod_id_hd = (data & 0x3C0000) >> 18;
	  hd_data.n_evts = (data & 0x000FF);
	  hd_data.blk_num = (data & 0x3FF00) >> 8;
	  if(i_print)
	    printf
	      ("%8X - BLOCK HEADER - slot = %d  id = %d  n_evts = %d  n_blk = %d\n",
	       data, hd_data.slot_id_hd, hd_data.mod_id_hd, hd_data.n_evts,
	       hd_data.blk_num);
	  break;

	case 1:		/* BLOCK TRAILER */
	  hd_data.slot_id_tr = (data & 0x7C00000) >> 22;
	  hd_data.n_words = (data & 0x3FFFFF);
	  if(i_print)
	    printf("%8X - BLOCK TRAILER - slot = %d   n_words = %d\n",
		   data, hd_data.slot_id_tr, hd_data.n_words);
	  break;

	case 2:		/* EVENT HEADER */
	  if(hd_data.new_type)
	    {
	      slot_id_ev_hd = (data & 0x07C00000) >> 22;
	      hd_data.evt_num_1 = (data & 0x00000FFF);
	      hd_data.trig_time = (data & 0x003FF000) >> 12;
	      if(i_print)
		printf
		  ("%8X - EVENT HEADER - slot = %d  evt_num = %d  trig_time = %d (%X)\n",
		   data, slot_id_ev_hd, hd_data.evt_num_1, hd_data.trig_time,
		   hd_data.trig_time);
	    }
	  break;

	case 3:		/* TRIGGER TIME */
	  if(hd_data.new_type)
	    {
	      hd_data.time_1 = (data & 0x7FFFFFF);
	      if(i_print)
		printf("%8X - TRIGGER TIME 1 - time = %X\n", data,
		       hd_data.time_1);
	      hd_data.time_now = 1;
	      time_last = 1;
	    }
	  else
	    {
	      if(time_last == 1)
		{
		  hd_data.time_2 = (data & 0xFFFFF);
		  if(i_print)
		    printf("%8X - TRIGGER TIME 2 - time = %X\n", data,
			   hd_data.time_2);
		  hd_data.time_now = 2;
		}
	      else if(i_print)
		printf("%8X - TRIGGER TIME - (ERROR)\n", data);

	      time_last = hd_data.time_now;
	    }
	  break;

	case 4:		/* UNDEFINED TYPE */
	  if(i_print)
	    printf("%8X - UNDEFINED TYPE = %d\n", data, hd_data.type);
	  break;

	case 5:		/* UNDEFINED TYPE */
	  if(i_print)
	    printf("%8X - UNDEFINED TYPE = %d\n", data, hd_data.type);
	  break;

	case 6:		/* UNDEFINED TYPE */
	  if(i_print)
	    printf("%8X - UNDEFINED TYPE = %d\n", data, hd_data.type);
	  break;

	case 7:		/* UNDEFINED TYPE */
	  if(i_print)
	    printf("%8X - UNDEFINED TYPE = %d\n", data, hd_data.type);
	  break;

	case 8:		/* DECODER HEADER */
	  num_decoder_words = (data & 0x3F);	/* number of decoder words to follow */
	  hd_data.num_words = num_decoder_words;
	  decoder_index = 1;	/* identify next word as a decoder data word */
	  if(i_print)
	    printf("%8X - DECODER HEADER = %d  (NUM DECODER WORDS = %d)\n",
		   data, hd_data.type, hd_data.num_words);
	  break;

	case 9:		/* UNDEFINED TYPE */
	  if(i_print)
	    printf("%8X - UNDEFINED TYPE = %d\n", data, hd_data.type);
	  break;

	case 10:		/* UNDEFINED TYPE */
	  if(i_print)
	    printf("%8X - UNDEFINED TYPE = %d\n", data, hd_data.type);
	  break;

	case 11:		/* UNDEFINED TYPE */
	  if(i_print)
	    printf("%8X - UNDEFINED TYPE = %d\n", data, hd_data.type);
	  break;

	case 12:		/* UNDEFINED TYPE */
	  if(i_print)
	    printf("%8X - UNDEFINED TYPE = %d\n", data, hd_data.type);
	  break;

	case 13:		/* END OF EVENT */
	  if(i_print)
	    printf("%8X - END OF EVENT = %d\n", data, hd_data.type);
	  break;

	case 14:		/* DATA NOT VALID (no data available) */
	  slot_id_dnv = (data & 0x7C00000) >> 22;
	  if(i_print)
	    printf("%8X - DATA NOT VALID = %d  slot = %d\n", data,
		   hd_data.type, slot_id_dnv);
	  break;

	case 15:		/* FILLER WORD */
	  slot_id_fill = (data & 0x7C00000) >> 22;
	  if(i_print)
	    printf("%8X - FILLER WORD = %d  slot = %d\n", data, hd_data.type,
		   slot_id_fill);
	  break;
	}

      type_last = hd_data.type;	/* save type of current data word */

    }

}

/**
 * @ingroup Config
 * @brief Set the delay after PATTERN_SYNC to generate test trigger.
 *
 * @param delay Delay value [0-262143]
 *            1 count = 8 ns
 *            max = 2.097 us
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdSetInternalTestTriggerDelay(uint32_t delay)
{
  int32_t rval = OK;
  CHECKINIT;

  if(delay > HD_INT_TESTTRIG_DELAY_MASK)
    {
      printf("%s: ERROR: Invalid delay %d (0x%x).  MAX = %d (0x%x)\n",
	     __func__, delay, delay,
	     HD_INT_TESTTRIG_DELAY_MASK, HD_INT_TESTTRIG_DELAY_MASK);
      return ERROR;
    }

  HLOCK;
  vmeWrite32(&hdp->int_testtrig_delay, delay);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Status
 * @brief Get the delay after PATTERN_SYNC to generate test trigger.
 *
 * @param delay Address for Delay value [0-262143]
 *            1 count = 8 ns
 *            max = 2.097 us
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdGetInternalTestTriggerDelay(uint32_t *delay)
{
  int32_t rval = OK;
  CHECKINIT;

  HLOCK;
  *delay = vmeRead32(&hdp->int_testtrig_delay) & HD_INT_TESTTRIG_DELAY_MASK;
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Enable generation of the internal test trigger
 *
 * @param pflag Print Flag
 *           !0 = Print Status to Standard Out
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdEnableInternalTestTrigger(int32_t pflag)
{
  int32_t rval = 0;
  CHECKINIT;

  if(pflag)
    printf("%s: ENABLE\n", __func__);

  HLOCK;
  vmeWrite32(&hdp->ctrl1, vmeRead32(&hdp->ctrl1) | HD_CTRL1_INT_TESTTRIG_ENABLE);
  HUNLOCK;

  return rval;
}

/**
 * @ingroup Config
 * @brief Disable generation of the internal test trigger
 *
 * @param pflag Print Flag
 *           !0 = Print Status to Standard Out
 *
 * @return OK if successful, otherwise ERROR
 */
int32_t
hdDisableInternalTestTrigger(int32_t pflag)
{
  int32_t rval = 0;
  CHECKINIT;

  if(pflag)
    printf("%s: DISABLE\n", __func__);

  HLOCK;
  vmeWrite32(&hdp->ctrl1, vmeRead32(&hdp->ctrl1) & ~HD_CTRL1_INT_TESTTRIG_ENABLE);
  HUNLOCK;

  return rval;
}
