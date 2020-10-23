#ifndef __HDLIBH__
#define __HDLIBH__
/******************************************************************************
 *
 *  hdLib.h -  Header for Driver library for JLab helicity decoder
 *
 */

typedef struct hd_struct
{
  /* 0x0000 */ volatile uint32_t version;
  /* 0x0004 */ volatile uint32_t csr;
  /* 0x0008 */ volatile uint32_t ctrl1;
  /* 0x000C */ volatile uint32_t ctrl2;
  /* 0x0010 */ volatile uint32_t adr32;
  /* 0x0014 */ volatile uint32_t interrupt;
  /* 0x0018 */ volatile uint32_t blocklevel;
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
  /* 0x0060 */ volatile uint32_t spare[(0x78 - 0x60) >> 2];
  /* 0x0078 */ volatile uint32_t firmware_csr;
  /* 0x007C */ volatile uint32_t firmware_data;
} HD;

/* 0x0 version bits and masks */
#define HD_VERSION_FIRMWARE_MASK    0x000000FF
#define HD_VERSION_BOARD_REV_MASK   0x0000FF00
#define HD_VERSION_BOARD_TYPE_MASK  0xFFFF0000
#define HD_VERSION_BOARD_TYPE       0xDEC0

/* 0x4 csr bits and masks */
#define HD_CSR_SYSTEM_CLK_PLL_LOCKED       (1 << 0)
#define HD_CSR_LOCAL_CLK_PLL_LOCKED        (1 << 1)
#define HD_CSR_BLOCK_ACCEPTED              (1 << 2)
#define HD_CSR_BLOCK_READY                 (1 << 3)
#define HD_CSR_EMPTY                       (1 << 4)
#define HD_CSR_BERR_ASSERTED               (1 << 5)
#define HD_CSR_BUSY                        (1 << 6)
#define HD_CSR_BUSY_LATCHED                (1 << 7)
#define HD_CSR_INTERNAL_BUF0               (1 << 8)
#define HD_CSR_INTERNAL_BUF1               (1 << 9)
#define HD_CSR_HELICITY_SEQ_ERROR          (1 << 10)
#define HD_CSR_TRIGTIME_WORD_ERROR         (1 << 11)
#define HD_CSR_FORCE_BLOCK_TRAILER         (1 << 16)
#define HD_CSR_FORCE_BLOCK_TRAILER_SUCCESS (1 << 17)
#define HD_CSR_FORCE_BLOCK_TRAILER_FAILED  (1 << 18)
#define HD_CSR_SYNC_RESET_PULSE            (1 << 28)
#define HD_CSR_TRIGGER_PULSE               (1 << 29)
#define HD_CSR_SOFT_RESET                  (1 << 30)
#define HD_CSR_HARD_RESET                  (1 << 31)

/* 0x8 ctrl1 bits and masks */
#define HD_CTRL1_CLK_SRC_MASK        0x00000003
#define HD_CTRL1_CLK_SRC_P0          0
#define HD_CTRL1_CLK_SRC_FP          1
#define HD_CTRL1_CLK_SRC_FP2         2
#define HD_CTRL1_CLK_SRC_INT         3
#define HD_CTRL1_INT_CLK_ENABLE      (1 << 2)
#define HD_CTRL1_TRIG_SRC_MASK       0x00000018
#define HD_CTRL1_TRIG_SRC_P0         0
#define HD_CTRL1_TRIG_SRC_FP         1
#define HD_CTRL1_TRIG_SRC_FP2        2
#define HD_CTRL1_TRIG_SRC_SOFT       3
#define HD_CTRL1_SYNC_RESET_SRC_MASK 0x00000060
#define HD_CTRL1_SYNC_RESET_SRC_P0   0
#define HD_CTRL1_SYNC_RESET_SRC_FP   1
#define HD_CTRL1_SYNC_RESET_SRC_FP2  2
#define HD_CTRL1_SYNC_RESET_SRC_SOFT 3
#define HD_CTRL1_SOFT_CONTROL_ENABLE (1 << 7)
#define HD_CTRL1_INT_ENABLE          (1 << 16)
#define HD_CTRL1_BERR_ENABLE         (1 << 17)
#define HD_CTRL1_USE_INT_HELICITY    (1 << 18)

/* 0xC ctrl2 bits and masks */
#define HD_CTRL2_DECODER_ENABLE      (1 << 0)
#define HD_CTRL2_GO                  (1 << 1)
#define HD_CTRL2_EVENT_BUILD_ENABLE  (1 << 2)
#define HD_CTRL2_INT_HELICITY_ENABLE (1 << 8)
#define HD_CTRL2_FORCE_BUSY          (1 << 9)

/* 0x10 adr32 bits and masks */
#define HD_ADR32_ENABLE    (1 << 0)
#define HD_ADR32_BASE_MASK 0x0000FF00

/* 0x14 interrupt bits and masks */
#define HD_INT_VEC_MASK         0x000000FF
#define HD_INT_LEVEL_MASK       0x00000700
#define HD_INT_GEO_MASK         0x001F0000
#define HD_INT_GEO_PARITY_ERROR (1 << 23)

/* 0x18 blocklevel bits and masks */
#define HD_BLOCKLEVEL_MASK 0x0000FFFF

/* 0x1C trigger_latency bits and masks */
#define HD_TRIGGER_LATENCY_MASK   0x000003FF
#define HD_TRIGGER_LATENCY_STATUS (1 << 31)


/* 0x20 helicity_config1 */
#define HD_HELICITY_CONFIG1_PATTERN_MASK         0x00000003
#define HD_HELICITY_CONFIG1_PATTERN_PAIR         0
#define HD_HELICITY_CONFIG1_PATTERN_QUARTET      1
#define HD_HELICITY_CONFIG1_PATTERN_OCTET        2
#define HD_HELICITY_CONFIG1_PATTERN_TOGGLE       3
#define HD_HELICITY_CONFIG1_HELICITY_DELAY_MASK  0x0000FF00
#define HD_HELICITY_CONFIG1_HELICITY_SETTLE_MASK 0xFFFF0000

/* 0x24 helicity_config2 */
#define HD_HELICITY_CONFIG2_STABLE_TIME_MASK 0x00FFFFFF

/* 0x28 helicity_config3 */
#define HD_HELICITY_CONFIG2_PSEUDO_SEED_MASK 0x3FFFFFFF

/* 0x30-0x38 control scaler Index definitions */
#define HD_CONTROL_SCALER_TRIG1     0
#define HD_CONTROL_SCALER_TRIG2     1
#define HD_CONTROL_SCALER_SYNCRESET 2

/* 0x3c events_on_board mask */
#define HD_EVENTS_ON_BOARD_MASK 0x00FFFFFF

/* 0x40 blocks_on_board mask */
#define HD_BLOCKS_ON_BOARD_MASK 0x000FFFFF

/* 0x44-0x54 scaler Index definitions */
#define HD_HELICITY_SCALER_TSTABLE_RISING    0
#define HD_HELICITY_SCALER_TSTABLE_FALLING   1
#define HD_HELICITY_SCALER_PATTERN_SYNC      2
#define HD_HELICITY_SCALER_PAIR_SYNC         3
#define HD_HELICITY_SCALER_HELICITY_WINDOWS  4

/* 0x58 recovered_shift_reg masks */
#define HD_RECOVERED_SHIFT_REG_MASK 0x3FFFFFFF

/* 0x58 generator_shift_reg masks */
#define HD_GENERATOR_SHIFT_REG_MASK 0x3FFFFFFF

/* 0x78 firmware_csr bits and masks */
#define HD_FIRMWARE_CSR_LAST_VALID_READ_MASK 0x000000FF
#define HD_FIRMWARE_CSR_BUSY                 (1 << 8)
#define HD_FIRMWARE_CSR_SECTOR_ERASE         (1 << 29)
#define HD_FIRMWARE_CSR_BULK_ERASE           (1 << 30)
#define HD_FIRMWARE_CSR_WRITE_ENABLE         (1 << 31)

/* 0x80 firmware_data bits and masks */
#define HD_FIRMWARE_DATA_WRITE_MASK   0x000000FF
#define HD_FIRMWARE_DATA_ADDRESS_MASK 0xFFFFFF00

/* Supported Firmware Version */
#define HD_SUPPORTED_FIRMWARE  0x0000

/* hdInit initialization flags */
#define HD_INIT_IGNORE_FIRMWARE (1 << 0)
#define HD_INIT_NO_INIT         (1 << 1)

/* function prototypes */

int32_t hdCheckAddresses();
int32_t hdInit(uint32_t vAddr, uint32_t iFlag);
uint32_t hdFind();
#endif /* __HDLIBH__ */
