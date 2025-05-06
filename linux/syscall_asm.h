#ifndef SYSCALL_H_INCLUDED
#define SYSCALL_H_INCLUDED

#if defined(__aarch64__)
#   include "arch/aarch64/syscall_arch.h"
#endif /* defined(__aarch64__) */

#if defined(__arm__)
#   include "arch/arm/syscall_arch.h"
#endif /* defined(__arm__) */

#if defined(__i386__)
#   include "arch/i386/syscall_arch.h"
#endif /* defined(__i386__) */

#if defined(__loongarch__) && defined(_LP64)
#   include "arch/loongarch64/syscall_arch.h"
#endif /* defined(__loongarch__) && defined(_LP64) */

#if defined(__m68k__)
#   include "arch/m68k/syscall_arch.h"
#endif /* defined(__m68k__) */

#if defined(__microblaze__)
#   include "arch/microblaze/syscall_arch.h"
#endif /* defined(__microblaze__) */

#if defined(__mips__) && (_MIPS_SIM == _ABIO32)
#   include "arch/mips64/syscall_arch.h"
#endif /* defined(__mips__) && (_MIPS_SIM == _ABIO32) */

#if defined(__mips__) && (_MIPS_SIM == _ABI64)
#   include "arch/mipsn32/syscall_arch.h"
#endif /* defined(__mips__) && (_MIPS_SIM == _ABI64) */

#if defined(__mips__) && (_MIPS_SIM == _ABIN32)
#   include "arch/mips/syscall_arch.h"
#endif /* defined(__mips__) && (_MIPS_SIM == _ABIN32) */

#if defined(__or1k__)
#   include "arch/or1k/syscall_arch.h"
#endif /* defined(__or1k__) */

#if defined(__powerpc__)
#   include "arch/powerpc64/syscall_arch.h"
#endif /* defined(__powerpc__) */

#if defined(__powerpc64__)
#   include "arch/powerpc/syscall_arch.h"
#endif /* defined(__powerpc64__) */

#if defined(__riscv) && !defined(_LP64)
#   include "arch/riscv32/syscall_arch.h"
#endif /* defined(__riscv) && !defined(_LP64) */

#if defined(__riscv) && defined(_LP64)
#   include "arch/riscv64/syscall_arch.h"
#endif /* defined(__riscv) && defined(_LP64) */

#if defined(__s390x__)
#   include "arch/s390x/syscall_arch.h"
#endif /* defined(__s390x__) */

#if defined(__sh__)
#   include "arch/sh/syscall_arch.h"
#endif /* defined(__sh__) */

#if defined(__x86_64__) && defined(__ILP32__)
#   include "arch/x32/syscall_arch.h"
#endif /* defined(__x86_64__) && defined(__ILP32__) */

#if defined(__x86_64__)
#   include "arch/x86_64/syscall_arch.h"
#endif /* defined(__x86_64__) */

#ifndef __SYSCALL_LL_E
#   error "Unsupported CPU architecture"
#endif /* __SYSCALL_LL_E */

#endif /* SYSCALL_H_INCLUDED */
