cmake_minimum_required(VERSION 3.10)

project(helicity_decoder)

add_library(hd
  SHARED
  hdLib.c
)

add_compile_options(-Wall -Wno-unused -g)

set(linuxvme_lib "$ENV{LINUXVME_LIB}")
set(linuxvme_inc "$ENV{LINUXVME_INC}")
set(linuxvme_bin "$ENV{LINUXVME_BIN}")

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${linuxvme_inc}
)

install(TARGETS hd
  DESTINATION ${linuxvme_lib}
)

install(FILES hdLib.h
  DESTINATION ${linuxvme_inc}
)
