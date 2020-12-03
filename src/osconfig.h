#ifndef __OSCONFIG_H
#define __OSCONFIG_H

#define CPU_FREQUENCY  16000000
#define OS_FREQUENCY       1000
// task stack size in bytes
// 1024/64 = memory for 16 tasks in theory
#define OS_STACK_SIZE        64
#define OS_TIMER_SIZE        16

#endif//__OSCONFIG_H
