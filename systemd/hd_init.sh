#!/bin/bash

export LINUXVME_LIB=/home/idaq/linuxvme/Linux-x86_64/lib
export LD_LIBRARY_PATH=$LINUXVME_LIB

# Run the program with option arguments
# ./hdConfigure -a <A24 address> -t <Trigger Source> -h <Helicity Source>
# -a <A24 address>               A24 of helicity decoder
#                                default: -a 0xed0000
# -t <Trigger Source>            0: Internal
#                                1: Front Panel - default
#                                2: VXS
# -h <Helicity Source>           0: Internal
#                                1: Fiber - default
#                                2: Copper

/home/idaq/linuxvme/helicity_decoder/systemd/hdConfigure
