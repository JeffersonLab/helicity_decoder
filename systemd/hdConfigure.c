/*
 * File:
 *    hdConfigure
 *
 * Description:
 *    Configure helicity decoder at specified address
 *
 *
 */


#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include "jvme.h"
#include "hdLib.h"


char *program_name;

void
usage()
{
  printf("\n\n");
  printf("%s -a <A24 address> -t <Trigger Source> -h <Helicity Source>\n",
	 program_name);
  printf("\n");
  printf("     -a <A24 address>               A24 of helicity decoder\n");
  printf("                                    default: -a 0xed0000\n");
  printf("\n");
  printf("     -t <Trigger Source>            0: Internal\n");
  printf("                                    1: Front Panel - default\n");
  printf("                                    2: VXS\n");
  printf("\n");
  printf("     -h <Helicity Source>           0: Internal\n");
  printf("                                    1: Fiber - default\n");
  printf("                                    2: Copper\n");
  printf("\n\n");

}

int
main(int argc, char *argv[])
{

  int stat = 0, rval = 0;
  uint32_t address=0x00ed0000;
  int hd_trigger_src = HD_INIT_FP;
  int hd_helicity_src = HD_INIT_EXTERNAL_FIBER;
  int hd_init_flag = 0;
  int option;

  program_name = argv[0];

  while((option = getopt(argc, argv, "a:t:h:")) != -1)
    {
      switch (option)
	{
	case 'a':
	  address = (uint32_t) strtoll(optarg,NULL,16)&0xffffffff;
	  break;
	case 't':
	  hd_trigger_src = atoi(optarg);
	  break;
	case 'h':
	  hd_helicity_src = atoi(optarg);
	  break;
	default:
	  usage();
	  exit(-1);
	}
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
