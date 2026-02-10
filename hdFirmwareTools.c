/* Module: hdFirmwareTools.c
 *
 * Description: Helcity Decoder Firmware Tools Library
 *              Firmware specific functions.
 *
 * Author:
 *        Bryan Moffit and Ed Jastrzembski
 *        JLab Data Acquisition Group
 *
 */

#include "hdLib.h"
#include "hdFirmwareTools.h"

#include <stdlib.h>
#include <pthread.h>

extern pthread_mutex_t hdMutex;
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


extern volatile HD *hdp;

/* #define DEBUGFW */
#define MAX_FW_DATA 0x800000
unsigned char fw_data[MAX_FW_DATA];
int fw_size=0;
unsigned char reverse(unsigned char b);

int
hdFirmwareReadFile(char *filename)
{
  FILE *fwFile = NULL;
  int c=0, idata=0, rval=OK;

  printf(" Opening firmware file: %s\n",filename);
  fwFile = fopen(filename,"r");
  if(fwFile==NULL)
    {
      perror("fopen");
      printf("%s: ERROR opening file (%s) for reading\n",
	     __FUNCTION__,filename);
      return ERROR;
    }

  while( (c = getc(fwFile)) != EOF )
    {
      fw_data[idata] = reverse(c);
      idata++;
      if(idata>=MAX_FW_DATA)
	{
	  printf("%s: ERROR: Firmware size greater than expected: %d (0x%x)\n",
		 __FUNCTION__,idata, idata);
	  rval=ERROR;
	  break;
	}
    }
  fclose(fwFile);

  fw_size = idata++;

#ifdef DEBUGFW
  printf("%s: Firmware size: %d (0x%x)\n",__FUNCTION__,fw_size,fw_size);
#endif

  return rval;
}

int
hdFirmwareEraseEPROM()
{
  unsigned int csr=0, busy=0;
  int iprint=0;

  CHECKINIT;

  HLOCK;
#ifdef DEBUGFW
  printf("%s: BULK ERASE\n",__FUNCTION__);
#endif

  vmeWrite32(&hdp->config_csr, 0xC0000000);	// set up for bulk erase
  csr = vmeRead32(&hdp->config_csr);
#ifdef DEBUGFW
  printf("\n--- CSR = %X\n", csr);
#endif

  vmeWrite32(&hdp->config_data, 0);		// write triggers erase
  csr = vmeRead32(&hdp->config_csr);
#ifdef DEBUGFW
  printf("\n--- CSR = %X\n", csr);
#endif

  printf("     Erasing EPROM\n");

  fflush(stdout);
  do {
    if((iprint%100)==0)
      {
	printf(".");
	fflush(stdout);
      }
    taskDelay(1);
    csr = vmeRead32(&hdp->config_csr);      // test for busy
    busy = (csr & 0x100) >> 8;
    iprint++;
  } while(busy);

  printf(" Done!\n");

  csr = vmeRead32(&hdp->config_csr);
#ifdef DEBUGFW
  printf("\n--- CSR = %X\n", csr);
#endif
  vmeWrite32(&hdp->config_csr, 0);		// set up for read

  HUNLOCK;
  return OK;
}

int
hdFirmwareDownloadConfigData(int print_header)
{
  int iaddr=0, busy=0;
  unsigned int data_word=0, value=0;

  CHECKINIT;

  if(fw_size==0)
    {
      printf("%s: ERROR: FW File not yet loaded\n",
	     __FUNCTION__);
      return ERROR;
    }

#ifdef DEBUGFW
  printf("%s: WRITE CFG DATA\n",__FUNCTION__);
#endif

  HLOCK;
  vmeWrite32(&hdp->config_csr, 0x80000000);	// set up for byte writes

  if(print_header)
    printf("     Writing to EPROM\n");

  fflush(stdout);
  for(iaddr=0; iaddr<fw_size; iaddr++)
    {
      data_word = (iaddr << 8) | fw_data[iaddr];
      vmeWrite32(&hdp->config_data, data_word);

      do {
	value = vmeRead32(&hdp->config_csr);	// test for busy
	busy = (value & 0x100) >> 8;
      } while(busy);

      if((iaddr%100000)==0)
	{
	  printf(".");
	  fflush(stdout);
	}
    }
  printf(" Done!\n");

  vmeWrite32(&hdp->config_csr, 0);			// default state is read
  HUNLOCK;

  return OK;
}

int
hdFirmwareVerifyDownload(int print_header)
{
  unsigned int data_word=0;
  int idata=0, busy=0, error=0;

  CHECKINIT;

  if(fw_size==0)
    {
      printf("%s: ERROR: FW File not yet loaded\n",
	     __FUNCTION__);
      return ERROR;
    }

  HLOCK;
  vmeWrite32(&hdp->config_csr, 0); // Set up for read

  if(print_header)
    printf("     Verifying Data\n");

  fflush(stdout);
  for(idata =0; idata<fw_size; idata++)
    {
      data_word = (idata<<8);
      vmeWrite32(&hdp->config_data, data_word);

      do {
	busy = (vmeRead32(&hdp->config_csr) & 0x100)>>8;
      } while (busy);

      data_word = vmeRead32(&hdp->config_csr) & 0xFF;

      if(data_word != fw_data[idata])
	{
	  printf(" Data ERROR: addr = 0x%06x   data: 0x%02x  !=  file: 0x%02x\n",
		 idata, data_word, fw_data[idata]);
	  error=1;
	}
      if((idata%100000)==0)
	{
	  printf(".");
	  fflush(stdout);
	}
    }
  HUNLOCK;
  printf(" Done!\n");

  if(error)
    return ERROR;
#ifdef DEBUGFW
  else
    printf("%s: Download Verified !\n",__FUNCTION__);
#endif

  return OK;
}

unsigned char
reverse(unsigned char b)
{
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}
