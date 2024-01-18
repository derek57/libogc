#ifndef __EXCONTEXT_H__
#define __EXCONTEXT_H__

#define NUM_EXCEPTIONS		15

#define EX_SYS_RESET		 0
#define EX_MACH_CHECK		 1
#define EX_DSI				 2
#define EX_ISI				 3
#define EX_INT				 4
#define EX_ALIGN			 5
#define EX_PRG				 6
#define EX_FP				 7
#define EX_DEC				 8
#define EX_SYS_CALL			 9
#define EX_TRACE			10
#define EX_PERF				11
#define EX_IABR				12
#define EX_RESV				13
#define EX_THERM			14

#ifndef ASM

#include <gctypes.h>
#include "lwp_config.h"

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef struct {
	unsigned32 EXCPT_Number;
	unsigned32 SRR0,SRR1;
	unsigned32 GPR[32];
	unsigned32 GQR[8];
	unsigned32 CR, LR, CTR, XER, MSR, DAR;

	unsigned16	state;		//used to determine whether to restore the fpu context or not
	unsigned16 mode;		//unused

	double_precision FPR[32];
	unsigned64	FPSCR;
	double_precision PSFPR[32];
} Context_Control;

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif  /* !ASM */

#endif
