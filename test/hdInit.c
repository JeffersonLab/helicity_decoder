/*
 * File:
 *    hdInit
 *
 * Description:
 *    Initialize helicity decoder at specified address with specified arguments
 *
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include "jvme.h"
#include "hdLib.h"


char *progName;

void
usage()
{
  printf("\n");
  printf("%s [options] <A24 address> \n", progName);
  printf("\n");
  printf("\n");
  printf(" options:\n");
  printf("     -s [SIGNAL SOURCE]     Set the clock, syncreset, trigger source\n");
  printf("                               0 Internal (DEFAULT)\n");
  printf("                               1 Front Panel (1)\n");
  printf("                               2 VXS\n");
  printf("     -h [HELICITY SOURCE]   Set the input helicity signal source\n");
  printf("                               0 Internal\n");
  printf("                               1 External Fiber (DEFAULT)\n");
  printf("                               2 External Copper\n");
  printf("     -i                     Invert input polarity\n");
  printf("     -o                     Invert output polarity\n");
  printf("     -f                     'force', ignore firmware version\n");
  printf("\n");

}

int
main(int argc, char *argv[])
{
  progName = argv[0];
  int32_t signal_source = 0, helicity_source = 1,
    invert_input = 0, invert_output = 0, force = 0,
    opt = -1;

  while ((opt = getopt(argc, argv, "s:h:iof")) != -1) {
    switch (opt) {
    case 's':
      signal_source = atoi(optarg);
      break;
    case 'h':
      helicity_source = atoi(optarg);
      break;
    case 'i':
      invert_input = 1;
      break;
    case 'o':
      invert_output = 1;
      break;
    case 'f':
      force = 1;
      break;
    default: /* '?' */
      usage();
      exit(EXIT_FAILURE);
    }
  }

  if ((optind + 1) != argc) {
    usage();
    exit(EXIT_FAILURE);
  }

  uint32_t a24_address = (uint32_t) strtoll(argv[optind++], NULL, 16) & 0xffffffff;

  printf("\n %s: a24 address = 0x%08x\n", argv[0], a24_address);
  printf("----------------------------\n");

  int stat = vmeOpenDefaultWindows();
  if(stat != OK)
    goto CLOSE;

  vmeCheckMutexHealth(1);
  vmeBusLock();

  hdInit(a24_address, signal_source, helicity_source, force ? HD_INIT_IGNORE_FIRMWARE : 0);
  int invert_fiber_input = (helicity_source == 1) && (invert_input == 1);
  int invert_copper_input = (helicity_source == 2) && (invert_input == 1);
  hdSetHelicityInversion(invert_fiber_input, invert_copper_input, invert_output);

  hdStatus(1);

 CLOSE:

  vmeBusUnlock();

  stat = vmeCloseDefaultWindows();
  if (stat != OK)
    {
      printf("vmeCloseDefaultWindows failed: code 0x%08x\n",stat);
      return -1;
    }

  exit(0);
}

/*
  Local Variables:
  compile-command: "make -k hdInit"
  End:
 */
