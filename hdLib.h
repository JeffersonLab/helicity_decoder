#ifndef __HDLIBH__
#define __HDLIBH__
/******************************************************************************
 *
 *  hdLib.h -  Header for Driver library for JLab helicity decoder
 *
 */

typdef struct hd_struct
{
  /* 0x0000 */ volatile uint32_t version;
  /* 0x0004 */ volatile uint32_t csr;
  /* 0x0008 */ volatile uint32_t ctrl1;
  /* 0x000C */ volatile uint32_t ctrl2;
  /* 0x0010 */ volatile uint32_t adr32;
  /* 0x0014 */ volatile uint32_t interrupt;
  /* 0x0018 */ volatile uint32_t block_size;
  /* 0x001C */ volatile uint32_t trigger_latency;
  /* 0x0020 */ volatile uint32_t helicity_config1;
  /* 0x0024 */ volatile uint32_t helicity_config2;
  /* 0x0028 */ volatile uint32_t helicity_config3;
  /* 0x002C          */ uint32_t _blank0;
  /* 0x0030 */ volatile uint32_t control_scaler[3];
  /* 0x003C */ volatile uint32_t events_on_board;
  /* 0x0040 */ volatile uint32_t blocks_on_board;
  /* 0x0044 */ volatile uint32_t helicity_scaler[5];
  /* 0x0058 */ volatile uint32_t recovered_shift_reg;
  /* 0x005C */ volatile uint32_t generator_shift_reg;
  /* 0x0060 */ volatile uint32_t spare[() >> 2];
  /* 0x0078 */ volatile uint32_t firmware_csr;
  /* 0x007C */ volatile uint32_t firmware_data;


}

/* 0x30-0x3C control scaler Index definitions */
#define HD_CONTROL_SCALER_TRIG1     0
#define HD_CONTROL_SCALER_TRIG2     1
#define HD_CONTROL_SCALER_SYNCRESET 2

/* 0x44-0x54 scaler Index definitions */
#define HD_HELICITY_SCALER_TSTABLE_RISING    0
#define HD_HELICITY_SCALER_TSTABLE_FALLING   1
#define HD_HELICITY_SCALER_PATTERN_SYNC      2
#define HD_HELICITY_SCALER_PAIR_SYNC         3
#define HD_HELICITY_SCALER_HELICITY_WINDOWS  4



#endif /* __HDLIBH__ */
