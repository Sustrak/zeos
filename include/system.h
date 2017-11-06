/*
 * system.h - Capçalera del mòdul principal del sistema operatiu
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <types.h>
#include <list.h>

extern TSS         tss;
extern Descriptor* gdt;
extern int zeos_clock;
extern int nextPID;

#endif  /* __SYSTEM_H__ */
