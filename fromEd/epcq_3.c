/*  "epcq_3.c" - for Helicity Decoder; firmware eeprom access - 2/10/25 */
/*   from "epcs_3.c" - for V2, V3 F1 TDC; firmware eeprom access - 7/25/13 */

#include <stdioLib.h>
#include <usrLib.h>
#include <stdlib.h>
#include <taskLib.h>
#include <semLib.h>
#include <intLib.h>
#include <cacheLib.h>
#include <iv.h>
#include <math.h>

#define HD_REG_ADDR  0xED0000	  /*  default base address of HD registers   (A24) */

//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v2_110.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v3_117.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v3x.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v3_120.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v3_121.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v3_122.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v2_122.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v2_120.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v2_c.rbf"
//#define CFG_FILENAME  "/u/home/jastrzem/c/f1tdc_v2/f1tdc_v2.rbf"
#define CFG_FILENAME  "/daqfs/home/jastrzem/c/helicity_decoder/hd_111_auto.rpd"

struct hd_struct {
  unsigned long version;				/* 0x00 - 0xFC */
  unsigned long csr;					/* 0x04 */
  unsigned long ctrl1;				/* 0x08 */
  unsigned long ctrl2;				/* 0x0C */
  unsigned long adr32;				/* 0x10 */
  unsigned long intr;					/* 0x14 */
  unsigned long blk_size;				/* 0x18 */
  unsigned long delay;				/* 0x1C */
  unsigned long gen_config1;			/* 0x20 */
  unsigned long gen_config2;			/* 0x24 */
  unsigned long gen_config3;			/* 0x28 */
  unsigned long test;					/* 0x2C */
  unsigned long trig1_scaler;			/* 0x30 */
  unsigned long trig2_scaler;			/* 0x34 */
  unsigned long sync_scaler;			/* 0x38 */
  unsigned long evt_count;			/* 0x3C */
  unsigned long blk_count;			/* 0x40 */

  unsigned long helicity_scaler[4];	/* 0x44 - 0x50 */
  unsigned long clk125_test;			/* 0x54 */
  unsigned long recovered_shift_reg;	/* 0x58 */
  unsigned long generator_shift_reg;	/* 0x5C */
  unsigned long latency_confirm;		/* 0x60 */
  unsigned long delay_confirm;		/* 0x64 */

  unsigned long helicity_history1;	/* 0x68 */
  unsigned long helicity_history2;	/* 0x6C */
  unsigned long helicity_history3;	/* 0x70 */
  unsigned long helicity_history4;	/* 0x74 */

  unsigned long spare1;				/* 0x78 */
  unsigned long spare2;				/* 0x7C */

  unsigned long pair_delay_setup;		/* 0x80 */
  unsigned long pair_error_count;		/* 0x84 */

  unsigned long spare3;				/* 0x88 */
  unsigned long spare4;				/* 0x8C */

  unsigned long epcq_csr;				/* 0x90 */
  unsigned long epcq_adr_data;		/* 0x94 */
};


unsigned char reverse(unsigned char b);

void epcq_3(void)
{
  volatile struct hd_struct *hd;
  unsigned long hd_reg_addr;
  unsigned long laddr;
  unsigned long addr, data, data_word;
  unsigned long data_file, data_epcq;
  unsigned long ii, jj, num_addr;
  unsigned long value, busy, error;
  long key_value;
  int c;
  unsigned char c_rev;
  FILE *fd_1;
  char *filename = CFG_FILENAME;
  char next[18];
  char *gets();

  hd_reg_addr = (unsigned long)HD_REG_ADDR;
  sysBusToLocalAdrs(0x39,hd_reg_addr,&laddr);
  /*	printf("HD local address = %x\n",laddr);  */
  hd =  (struct hd_struct *)laddr;

  printf("\nHD Version = %LX\n",hd->version);

  printf("\n--- Reset HD board ---\n");
  hd->csr = 0x80000000;  		/* reset board */
  taskDelay(60);

  while (1)
    {
      printf("\nEnter: 's <CR>' status read,          'w <CR>' write location,\n");
      printf(  "       'r <CR>' read location(s),     'R <CR>' read 4096 locations,\n");
      printf(  "       'e <CR>' erase sector,         'E <CR>' ERASE ALL,\n");
      printf(  "       'F <CR>' file load,            'C <CR>' compare to file\n");
      printf(  "       'q <CR>' quit\n\n");


      gets(next);
      if( *next == 'q' )
	{
	  break;
	}
      else if( *next == 's' )
	{
	  printf("\n--- CSR = %X\n", hd->epcq_csr);
	}
      else if( *next == 'w' )
	{
	  hd->epcq_csr = 0x80000000;		// set up for byte write

	  printf("\nEnter (24-bit hex) eprom address - ");
	  scanf("%lx",&key_value);
	  addr = (unsigned long)(key_value & 0xFFFFFF);
	  printf("\nEnter (8-bit hex) eprom data - ");
	  scanf("%lx",&key_value);
	  data = (unsigned long)(key_value & 0xFF);
	  data_word = (addr << 8) | data;
	  printf("address = %LX   data = %X\n   (data word = %LX)", addr, data, data_word);

	  hd->epcq_adr_data = data_word;	// write
	  printf("\n--- CSR = %X\n", hd->epcq_csr);

	  do {
	    value = hd->epcq_csr;		// test for busy
	    busy = (value & 0x100) >> 8;
	  } while(busy);

	  printf("\n--- CSR = %X\n", hd->epcq_csr);
	  hd->epcq_csr = 0;			// default state is read
	}
      else if( *next == 'r' )
	{
	  hd->epcq_csr = 0;			// set up for read

	  printf("\nEnter (24-bit hex) starting address - ");
	  scanf("%lx",&key_value);
	  addr = (unsigned long)(key_value & 0xFFFFFF);
	  printf("\nEnter number of locations to read - ");
	  scanf("%ld",&key_value);
	  num_addr = key_value;

	  for(ii = 1; ii <= num_addr; ii++)
	    {
	      data_word = (addr << 8);
	      hd->epcq_adr_data = data_word;

	      do {
		value = hd->epcq_csr;	// test for busy
		busy = (value & 0x100) >> 8;
	      } while(busy);

	      printf("addr = %LX   data = %X\n", addr, (hd->epcq_csr) & 0x1FF);
	      addr++;
	    }
	}
      else if( *next == 'R' )			// read and print 4096 values
	{
	  hd->epcq_csr = 0;			// set up for read

	  printf("\nEnter (24-bit hex) starting address - ");
	  scanf("%lx",&key_value);
	  addr = (unsigned long)(key_value & 0xFFFFFF);
	  printf("\n");

	  for(ii = 0; ii < 256; ii++)
	    {
	      printf("a %6X   ",addr);
	      for(jj = 0; jj < 16; jj++)
		{
		  data_word = (addr << 8);
		  hd->epcq_adr_data = data_word;

		  do {
		    value = hd->epcq_csr;	// test for busy
		    busy = (value & 0x100) >> 8;
		  } while(busy);

		  printf("%2X  ", (hd->epcq_csr) & 0xFF);
		  addr++;
		}
	      printf("\n");
	    }
	}
      else if( *next == 'e' )
	{
	  hd->epcq_csr = 0xA0000000;		// set up for sector erase

	  printf("\nEnter (24-bit hex) eprom address in sector - ");
	  scanf("%lx",&key_value);
	  addr = (unsigned long)(key_value & 0xFFFFFF);
	  data_word = (addr << 8) | 0;

	  hd->epcq_adr_data = data_word;
	  printf("\n--- CSR = %X\n", hd->epcq_csr);

	  do {
	    taskDelay(1);
	    value = hd->epcq_csr;		// test for busy
	    busy = (value & 0x100) >> 8;
	  } while(busy);

	  printf("\n--- CSR = %X\n", hd->epcq_csr);
	  hd->epcq_csr = 0;	// set up for read
	}
      else if( *next == 'E' )
	{
	  printf("\nBULK ERASE - are you SURE? (1 = yes, 0 = no): ");
	  scanf("%lx",&key_value);
	  if( key_value == 1 )
	    {
	      hd->epcq_csr = 0xC0000000;	// set up for bulk erase
	      printf("\n--- CSR = %X\n", hd->epcq_csr);

	      hd->epcq_adr_data = 0;		// write triggers erase
	      printf("\n--- CSR = %X\n", hd->epcq_csr);

	      do {
		taskDelay(1);
		value = hd->epcq_csr;	// test for busy
		busy = (value & 0x100) >> 8;
	      } while(busy);

	      printf("\n--- CSR = %X\n", hd->epcq_csr);
	      hd->epcq_csr = 0;		// set up for read
	    }
	  else
	    printf("\nNO BULK ERASE\n\n");

	}
      else if( *next == 'F' )			// write cfg from file
	{
	  printf("\nWRITE CFG DATA - are you SURE? (EPROM ERASED?) (1 = yes, 0 = no): ");
	  scanf("%lx",&key_value);
	  if( key_value == 1 )
	    {
	      hd->epcq_csr = 0x80000000;	// set up for byte writes
	      fd_1 = fopen(filename,"r");

	      addr = 0;				// starting address

	      while( (c = getc(fd_1)) != EOF )	// read byte from file and write it
		{
		  c_rev = reverse(c);		// reverse bits in byte
		  data = (unsigned long)(c_rev & 0xFF);
		  data_word = (addr << 8) | data;
		  hd->epcq_adr_data = data_word;

		  do {
		    value = hd->epcq_csr;	// test for busy
		    busy = (value & 0x100) >> 8;
		  } while(busy);

		  addr++;
		}

	      fclose(fd_1);
	    }
	  hd->epcq_csr = 0;			// default state is read
	}
      else if( *next == 'C' )			// compare to file
	{
	  error = 0;
	  hd->epcq_csr = 0;			// set up for read
	  fd_1 = fopen(filename,"r");

	  addr = 0;				// starting address

	  while( (c = getc(fd_1)) != EOF )	// read byte from file
	    {
	      c_rev = reverse(c);			// reverse bits in byte
	      data_file = (unsigned long)(c_rev & 0xFF);

	      data_word = (addr << 8);		// trigger read from location
	      hd->epcq_adr_data = data_word;

	      do {
		value = hd->epcq_csr;	// test for busy
		busy = (value & 0x100) >> 8;
	      } while(busy);

	      value = hd->epcq_csr;		// get read value
	      data_epcq = value & 0xFF;

	      if( (data_file != data_epcq) )
		{
		  printf("\n*** data error: addr = %6X  data = %2X  data(file) = %2X\n",
			 addr, data_epcq, data_file);
		  error = 1;
		  break;
		}

	      addr++;
	    }

	  fclose(fd_1);

	  if( !error )
	    printf("\n!!!!! DATA COMPARE OK !!!!!\n\n");
	}

    }

}

unsigned char reverse(unsigned char b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}
