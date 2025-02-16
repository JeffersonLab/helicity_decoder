#pragma once
/******************************************************************************
 *
 *  hdLib.h -  Header for Driver library for JLab helicity decoder
 *
 */

#include <stdint.h>

typedef struct hd_struct
{
  /* 0x0000 */ volatile uint32_t version;
  /* 0x0004 */ volatile uint32_t csr;
  /* 0x0008 */ volatile uint32_t ctrl1;
  /* 0x000C */ volatile uint32_t ctrl2;
  /* 0x0010 */ volatile uint32_t adr32;
  /* 0x0014 */ volatile uint32_t intr;
  /* 0x0018 */ volatile uint32_t blk_size;
  /* 0x001C */ volatile uint32_t delay;
  /* 0x0020 */ volatile uint32_t gen_config1;
  /* 0x0024 */ volatile uint32_t gen_config2;
  /* 0x0028 */ volatile uint32_t gen_config3;
  /* 0x002C */ volatile uint32_t int_testtrig_delay;
  /* 0x0030 */ volatile uint32_t trig1_scaler;
  /* 0x0034 */ volatile uint32_t trig2_scaler;
  /* 0x0038 */ volatile uint32_t sync_scaler;
  /* 0x003C */ volatile uint32_t evt_count;
  /* 0x0040 */ volatile uint32_t blk_count;

  /* 0x0044 */ volatile uint32_t helicity_scaler[4];
  /* 0x0054 */ volatile uint32_t clk125_test;
  /* 0x0058 */ volatile uint32_t recovered_shift_reg;
  /* 0x005C */ volatile uint32_t generator_shift_reg;
  /* 0x0060 */ volatile uint32_t latency_confirm;
  /* 0x0064 */ volatile uint32_t delay_confirm;

  /* 0x0068 */ volatile uint32_t helicity_history1;
  /* 0x006C */ volatile uint32_t helicity_history2;
  /* 0x0070 */ volatile uint32_t helicity_history3;
  /* 0x0074 */ volatile uint32_t helicity_history4;

  /* 0x0078          */ uint32_t spare[(0x80-0x78)>>2];

  /* 0x0080 */ volatile uint32_t delay_setup;
  /* 0x0084 */ volatile uint32_t delay_count;

  /* 0x0088          */ uint32_t spare2[(0x90-0x88)>>2];
  /* 0x0090 */ volatile uint32_t config_csr;
  /* 0x0094 */ volatile uint32_t config_data;

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
#define HD_CTRL1_CLK_SRC_MASK            0x00000003
#define HD_CTRL1_CLK_SRC_P0              (0 << 0)
#define HD_CTRL1_CLK_SRC_FP              (1 << 0)
#define HD_CTRL1_CLK_SRC_FP2             (2 << 0)
#define HD_CTRL1_CLK_SRC_INT             (3 << 0)
#define HD_CTRL1_INT_CLK_ENABLE          (1 << 2)
#define HD_CTRL1_TRIG_SRC_MASK           0x00000018
#define HD_CTRL1_TRIG_SRC_P0             (0 << 3)
#define HD_CTRL1_TRIG_SRC_FP             (1 << 3)
#define HD_CTRL1_TRIG_SRC_FP2            (2 << 3)
#define HD_CTRL1_TRIG_SRC_SOFT           (3 << 3)
#define HD_CTRL1_SYNC_RESET_SRC_MASK     0x00000060
#define HD_CTRL1_SYNC_RESET_SRC_P0       (0 << 5)
#define HD_CTRL1_SYNC_RESET_SRC_FP       (1 << 5)
#define HD_CTRL1_SYNC_RESET_SRC_FP2      (2 << 5)
#define HD_CTRL1_SYNC_RESET_SRC_SOFT     (3 << 5)
#define HD_CTRL1_SOFT_CONTROL_ENABLE     (1 << 7)
#define HD_CTRL1_INT_TESTTRIG_ENABLE     (1 << 8)
#define HD_CTRL1_DEBUG_BUSY_OUT_ENABLE   (1 << 12)
#define HD_CTRL1_TSETTLE_FILTER_MASK     0x0000e000
#define HD_CTRL1_TSETTLE_FILTER_DISABLED (0 << 13)
#define HD_CTRL1_TSETTLE_FILTER_4        (1 << 13)
#define HD_CTRL1_TSETTLE_FILTER_8        (2 << 13)
#define HD_CTRL1_TSETTLE_FILTER_16       (3 << 13)
#define HD_CTRL1_TSETTLE_FILTER_32       (4 << 13)
#define HD_CTRL1_TSETTLE_FILTER_64       (5 << 13)
#define HD_CTRL1_TSETTLE_FILTER_128      (6 << 13)
#define HD_CTRL1_TSETTLE_FILTER_256      (7 << 13)
#define HD_CTRL1_INT_ENABLE              (1 << 16)
#define HD_CTRL1_BERR_ENABLE             (1 << 17)
#define HD_CTRL1_HEL_SRC_MASK            0x001c0000
#define HD_CTRL1_USE_INT_HELICITY        (1 << 18)
#define HD_CTRL1_USE_EXT_CU_IN           (1 << 19)
#define HD_CTRL1_INT_HELICITY_TO_FP      (1 << 20)
#define HD_CTRL1_INVERT_FIBER_INPUT      (1 << 21)
#define HD_CTRL1_INVERT_CU_INPUT         (1 << 22)
#define HD_CTRL1_INVERT_CU_OUTPUT        (1 << 23)
#define HD_CTRL1_PROCESSED_TO_FP         (1 << 24)
#define HD_CTRL1_INVERT_MASK             0x00e00000

/* 0xC ctrl2 bits and masks */
#define HD_CTRL2_DECODER_ENABLE      (1 << 0)
#define HD_CTRL2_GO                  (1 << 1)
#define HD_CTRL2_EVENT_BUILD_ENABLE  (1 << 2)
#define HD_CTRL2_INT_HELICITY_ENABLE (1 << 8)
#define HD_CTRL2_FORCE_BUSY          (1 << 9)

/* 0x10 adr32 bits and masks */
#define HD_ADR32_ENABLE    (1 << 0)
#define HD_ADR32_BASE_MASK 0x0000FF80

/* 0x14 interrupt bits and masks */
#define HD_INT_VEC_MASK         0x000000FF
#define HD_INT_LEVEL_MASK       0x00000700
#define HD_INT_GEO_MASK         0x001F0000
#define HD_INT_GEO_PARITY_ERROR (1 << 23)

/* 0x18 blocklevel bits and masks */
#define HD_BLOCKLEVEL_MASK 0x0000FFFF

/* 0x1C delay bits and masks */
#define HD_DELAY_TRIGGER_MASK       0x000003FF
#define HD_DELAY_TRIGGER_CONFIGURED (1 << 15)
#define HD_DELAY_DATA_MASK          0x0FFF0000
#define HD_DELAY_DATA_CONFIGURED    (1 << 31)


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
#define HD_HELICITY_CONFIG3_PSEUDO_SEED_MASK 0x3FFFFFFF

/* 0x2C int_testtrig_delay */
#define HD_INT_TESTTRIG_DELAY_MASK 0x0003FFFF

/* 0x30-0x38 control scaler Index definitions */
#define HD_CONTROL_SCALER_TRIG1     0
#define HD_CONTROL_SCALER_TRIG2     1
#define HD_CONTROL_SCALER_SYNCRESET 2

/* 0x3c events_on_board mask */
#define HD_EVENTS_ON_BOARD_MASK 0x00FFFFFF

/* 0x40 blocks_on_board mask */
#define HD_BLOCKS_ON_BOARD_MASK 0x000FFFFF

/* 0x44-0x54 scaler Index definitions */
#define HD_HELICITY_SCALER_TSTABLE_FALLING   0
#define HD_HELICITY_SCALER_TSTABLE_RISING    1
#define HD_HELICITY_SCALER_PATTERN_SYNC      2
#define HD_HELICITY_SCALER_PAIR_SYNC         3
#define HD_HELICITY_SCALER_HELICITY_WINDOWS  4

/* 0x58 recovered_shift_reg masks */
#define HD_RECOVERED_SHIFT_REG_MASK 0x3FFFFFFF

/* 0x58 generator_shift_reg masks */
#define HD_GENERATOR_SHIFT_REG_MASK 0x3FFFFFFF

/* 0x60-64 delay processing confirmation masks */
#define HD_CONFIRM_READ_ADDR_MASK   0x00000FFF
#define HD_CONFIRM_WRITE_ADDR_MASK  0x0FFF0000

/* 0x80 delay_setup bits and masks */
#define HD_DELAY_SETUP_SELECTION_MASK 0x0000000F
#define HD_DELAY_SETUP_ENABLE          (1 << 31)


/* 0x90 config_csr bits and masks */
#define HD_CONFIG_CSR_LAST_VALID_READ_MASK 0x000000FF
#define HD_CONFIG_CSR_BUSY                 (1 << 8)
#define HD_CONFIG_CSR_SECTOR_ERASE         (1 << 29)
#define HD_CONFIG_CSR_BULK_ERASE           (1 << 30)
#define HD_CONFIG_CSR_WRITE_ENABLE         (1 << 31)

/* 0x94 firmware_data bits and masks */
#define HD_CONFIG_DATA_WRITE_MASK   0x000000FF
#define HD_CONFIG_DATA_ADDRESS_MASK 0xFFFFFF00



/* Data Format words and masks */
#define HD_DUMMY_WORD             0xF8000000
#define HD_DATA_TYPE_DEFINE       0x80000000
#define HD_DATA_TYPE_MASK         0x78000000
#define HD_DATA_BLOCK_HEADER      0x00000000
#define HD_DATA_BLOCK_TRAILER     0x08000000

/* Supported Firmware Version */
#define HD_SUPPORTED_FIRMWARE  0x09

/* hdInit initialization flags */
#define HD_INIT_IGNORE_FIRMWARE (1 << 0)
#define HD_INIT_NO_INIT         (1 << 1)
#define HD_INIT_INTERNAL        0
#define HD_INIT_FP              1
#define HD_INIT_VXS             2
#define HD_INIT_INTERNAL_HELICITY  0
#define HD_INIT_EXTERNAL_FIBER     1
#define HD_INIT_EXTERNAL_COPPER    2

/* function prototypes */

int32_t hdCheckAddresses();
int32_t hdInit(uint32_t vAddr, uint8_t source, uint8_t helSignalSrc, uint32_t iFlag);
uint32_t hdFind();
int32_t hdStatus(int pflag);
int32_t hdGetFirmwareVersion();
int32_t hdReset(uint8_t type, uint8_t clearA32);
int32_t hdSetA32(uint32_t a32base);
uint32_t hdGetA32();
int32_t hdSetSignalSources(uint8_t clkSrc, uint8_t trigSrc, uint8_t srSrc);
int32_t hdGetSignalSources(uint8_t *clkSrc, uint8_t *trigSrc, uint8_t *srSrc);
int32_t hdSetHelicitySource(uint8_t helSrc, uint8_t input, uint8_t output);
int32_t hdGetHelicitySource(uint8_t *helSrc, uint8_t *input, uint8_t *output);

int32_t hdSetBlocklevel(uint8_t blklevel);
int32_t hdGetBlocklevel();
int32_t hdSetProcDelay(uint16_t dataInputDelay, uint16_t triggerLatencyDelay);
int32_t hdGetProcDelay(uint16_t *dataInputDelay, uint16_t *triggerLatencyDelay);
int32_t hdConfirmProcDelay(uint8_t pflag);

int32_t hdSetBERR(uint8_t enable);
int32_t hdGetBERR();

int32_t hdEnableDecoder();
int32_t hdEnable();
int32_t hdDisable();

int32_t hdTrig(int pflag);
int32_t hdSync(int pflag);
int32_t hdBusy(int enable);

int32_t hdBReady();
int32_t hdBERRStatus();
int32_t hdBusyStatus(uint8_t *latched);

int32_t hdReadBlock(volatile unsigned int *data, int nwrds, int rflag);
int32_t hdReadScalers(volatile unsigned int *data, int rflag);
int32_t hdPrintScalers();

int32_t hdReadHelicityHistory(volatile unsigned int *data);

int32_t hdGetRecoveredShiftRegisterValue(uint32_t *recovered, uint32_t *internalGenerator);

int32_t hdEnableHelicityGenerator();
int32_t hdDisableHelicityGenerator();
int32_t hdHelicityGeneratorConfig(uint8_t pattern, uint8_t windowDelay,
				  uint16_t settleTime, uint32_t stableTime,
				  uint32_t seed);

int32_t hdGetHelicityGeneratorConfig(uint8_t *pattern, uint8_t *windowDelay,
				     uint16_t *settleTime, uint32_t *stableTime,
				     uint32_t *seed);
int32_t hdPrintHelicityGeneratorConfig();
void hdDecodeData(uint32_t data);

int32_t hdSetInternalTestTriggerDelay(uint32_t delay);
int32_t hdGetInternalTestTriggerDelay(uint32_t *delay);
int32_t hdEnableInternalTestTrigger(int32_t pflag);
int32_t hdDisableInternalTestTrigger(int32_t pflag);
int32_t hdGetClockPLLStatus(int32_t *system, int32_t *local);
int32_t hdGetSlotNumber(uint32_t *slotnumber);

int32_t hdSetHelicityInversion(uint8_t fiber_input, uint8_t cu_input, uint8_t cu_output);
int32_t hdGetHelicityInversion(uint8_t *fiber_input, uint8_t *cu_input, uint8_t *cu_output);

int32_t hdSetTSettleFilter(uint8_t clock);
int32_t hdGetTSettleFilter(uint8_t *clock);
int32_t hdSetProcessedOutput(int8_t enable);
int32_t hdGetProcessedOutput();
int32_t hdDelayTestSetup(uint8_t pair_delay_selection, int8_t enable);
int32_t hdGetDelayTestSetup(uint8_t *pair_delay_selection, int8_t *enable);
