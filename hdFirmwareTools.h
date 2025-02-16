#pragma once

#include <stdint.h>
#include "jvme.h"

int hdFirmwareReadFile(char *filename);
int hdFirmwareEraseEPROM();
int hdFirmwareDownloadConfigData(int print_header);
int hdFirmwareVerifyDownload(int print_header);
