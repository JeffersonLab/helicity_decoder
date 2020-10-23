/******************************************************************************
 *
 *  hdLib.c -  Driver library for configuration and readout of the
 *                  JLab Helicity Decoder using VxWorks 5.4+ (PPC), or
 *                  Linux (i686, x86_64) VME single board computer.
 *
 *  Authors: Bryan Moffit
 *           Jefferson Lab FEDAQ
 *           October 2020
 *
 *
 */

#ifdef VXWORKS
#include <vxWorks.h>
#include <logLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <semLib.h>
#include <vxLib.h>
#else
#include <unistd.h>
#include <stddef.h>
#endif
#include <pthread.h>
#include <stdio.h>
#include "jvme.h"

#include "hdLib.h"

static volatile HD *hdp;	/* pointer to HD memory map */
static uintptr_t vmeLocalOffset = 0;	/* Offset between VME and Local address space */

/* Mutex for thread safe read/writes */
pthread_mutex_t hdMutex = PTHREAD_MUTEX_INITIALIZER;
#define HLOCK   if(pthread_mutex_lock(&hdMutex)<0) perror("pthread_mutex_lock");
#define HUNLOCK if(pthread_mutex_unlock(&hdMutex)<0) perror("pthread_mutex_unlock");
