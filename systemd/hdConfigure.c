/*
 * File:
 *    hdConfigure
 *
 * Description:
 *    Configure helicity decoder at specified address
 *
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "jvme.h"
#include "hdLib.h"

int hd_trigger_src = HD_INIT_FP;
int hd_helicity_src = HD_INIT_EXTERNAL_FIBER;
int hd_init_flag = 0;

int
main(int argc, char *argv[])
{

  int stat = 0, rval = 0;
  uint32_t address=0;

  if (argc > 1)
    {
      address = (unsigned int) strtoll(argv[1],NULL,16)&0xffffffff;
    }
  else
    {
      address = 0x00ed0000; // my test module
    }

  vmeSetQuietFlag(1);
  stat = vmeOpen();
  if(stat != OK)
    {
      fprintf(stderr, "ERROR: vmeOpen failed\n");
      rval = ERROR;
      goto CLOSE;
    }

  vmeCheckMutexHealth(1);
  vmeBusLock();

  stat = hdInit(address, hd_trigger_src, hd_helicity_src, hd_init_flag);
  if (stat != OK)
    {
      fprintf(stderr, "ERROR: Helicity Decoder initialization failed\n");
      rval = ERROR;
    }

  hdStatus(1);

 CLOSE:

  vmeBusUnlock();

  stat = vmeClose();
  if (stat != OK)
    {
      fprintf(stderr, "ERROR: vmeClose failed\n");
      rval = ERROR;
    }

  exit(rval);


}

/*
  Local Variables:
  compile-command: "make -k hdConfigure "
  End:
*/
