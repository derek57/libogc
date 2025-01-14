#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include <gctypes.h>
#include "asm.h"
#include "lwp_config.h"

#define __stringify(rn)								#rn
#define ATTRIBUTE_ALIGN(v)							__attribute__((aligned(v)))
// courtesy of Marcan
#define STACK_ALIGN(type, name, cnt, alignment)		unsigned8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + (((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - ((sizeof(type)*(cnt))%(alignment))) : 0))]; \
													type *name = (type*)(((unsigned32)(_al__##name)) + ((alignment) - (((unsigned32)(_al__##name))&((alignment)-1))))

#define _sync() asm volatile("sync")
#define _nop() asm volatile("nop")
#define ppcsync() asm volatile("sc")
#define ppchalt() ({					\
	asm volatile("sync");				\
	while(1) {							\
		asm volatile("nop");			\
		asm volatile("li 3,0");			\
		asm volatile("nop");			\
	}									\
})

#define mfpvr() ({register unsigned32 _rval; \
		asm volatile("mfpvr %0" : "=r"(_rval)); _rval;})

#define mfdcr(_rn) ({register unsigned32 _rval; \
		asm volatile("mfdcr %0," __stringify(_rn) \
             : "=r" (_rval)); _rval;})
#define mtdcr(rn, val)  asm volatile("mtdcr " __stringify(rn) ",%0" : : "r" (val))

#define mfmsr()   ({register unsigned32 _rval; \
		asm volatile("mfmsr %0" : "=r" (_rval)); _rval;})
#define mtmsr(val)  asm volatile("mtmsr %0" : : "r" (val))

#define mfdec()   ({register unsigned32 _rval; \
		asm volatile("mfdec %0" : "=r" (_rval)); _rval;})

/*
 *  Routines to access the decrementer register
 */

#define PPC_Set_decrementer( _clicks ) \
  do { \
    asm volatile( "mtdec %0" : : "r" ((_clicks)) ); \
  } while (0)

#define mfspr(_rn) \
({	register unsigned32 _rval = 0; \
	asm volatile("mfspr %0," __stringify(_rn) \
	: "=r" (_rval));\
	_rval; \
})

#define mtspr(_rn, _val) asm volatile("mtspr " __stringify(_rn) ",%0" : : "r" (_val))

#define mfwpar()		mfspr(WPAR)
#define mtwpar(_val)	mtspr(WPAR,_val)

#define mfmmcr0()		mfspr(MMCR0)
#define mtmmcr0(_val)	mtspr(MMCR0,_val)
#define mfmmcr1()		mfspr(MMCR1)
#define mtmmcr1(_val)	mtspr(MMCR1,_val)

#define mfpmc1()		mfspr(PMC1)
#define mtpmc1(_val)	mtspr(PMC1,_val)
#define mfpmc2()		mfspr(PMC2)
#define mtpmc2(_val)	mtspr(PMC2,_val)
#define mfpmc3()		mfspr(PMC3)
#define mtpmc3(_val)	mtspr(PMC3,_val)
#define mfpmc4()		mfspr(PMC4)
#define mtpmc4(_val)	mtspr(PMC4,_val)

#define mfhid0()		mfspr(HID0)
#define mthid0(_val)	mtspr(HID0,_val)
#define mfhid1()		mfspr(HID1)
#define mthid1(_val)	mtspr(HID1,_val)
#define mfhid2()		mfspr(HID2)
#define mthid2(_val)	mtspr(HID2,_val)
#define mfhid4()		mfspr(HID4)
#define mthid4(_val)	mtspr(HID4,_val)

#define __lhbrx(base,index)			\
({	register unsigned16 res;				\
	__asm__ volatile ("lhbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })

#define __lwbrx(base,index)			\
({	register unsigned32 res;				\
	__asm__ volatile ("lwbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })

#define __sthbrx(base,index,value)	\
	__asm__ volatile ("sthbrx	%0,%1,%2" : : "r"(value), "b%"(index), "r"(base) : "memory")

#define __stwbrx(base,index,value)	\
	__asm__ volatile ("stwbrx	%0,%1,%2" : : "r"(value), "b%"(index), "r"(base) : "memory")

#define cntlzw(_val) ({register unsigned32 _rval; \
					  asm volatile("cntlzw %0, %1" : "=r"((_rval)) : "r"((_val))); _rval;})

#define _CPU_MSR_GET( _msr_value ) \
  do { \
    _msr_value = 0; \
    asm volatile ("mfmsr %0" : "=&r" ((_msr_value)) : "0" ((_msr_value))); \
  } while (0)

#define _CPU_MSR_SET( _msr_value ) \
{ asm volatile ("mtmsr %0" : "=&r" ((_msr_value)) : "0" ((_msr_value))); }

#define _CPU_ISR_Enable() \
	{ register unsigned32 _val = 0; \
	  __asm__ __volatile__ ( \
		"mfmsr %0\n" \
		"ori %0,%0,0x8000\n" \
		"mtmsr %0" \
		: "=&r" ((_val)) : "0" ((_val)) \
	  ); \
	}

#define _ISR_Disable( _isr_cookie ) \
  { register unsigned32 _disable_mask = 0; \
	_isr_cookie = 0; \
    __asm__ __volatile__ ( \
	  "mfmsr %0\n" \
	  "rlwinm %1,%0,0,17,15\n" \
	  "mtmsr %1\n" \
	  "extrwi %0,%0,1,16" \
	  : "=&r" ((_isr_cookie)), "=&r" ((_disable_mask)) \
	  : "0" ((_isr_cookie)), "1" ((_disable_mask)) \
	); \
  }

#define _ISR_Enable( _isr_cookie )  \
  { register unsigned32 _enable_mask = 0; \
	__asm__ __volatile__ ( \
    "    cmpwi %0,0\n" \
	"    beq 1f\n" \
	"    mfmsr %1\n" \
	"    ori %1,%1,0x8000\n" \
	"    mtmsr %1\n" \
	"1:" \
	: "=r"((_isr_cookie)),"=&r" ((_enable_mask)) \
	: "0"((_isr_cookie)),"1" ((_enable_mask)) \
	); \
  }

#define _ISR_Flash( _isr_cookie ) \
  { register unsigned32 _flash_mask = 0; \
    __asm__ __volatile__ ( \
    "   cmpwi %0,0\n" \
	"   beq 1f\n" \
	"   mfmsr %1\n" \
	"   ori %1,%1,0x8000\n" \
	"   mtmsr %1\n" \
	"   rlwinm %1,%1,0,17,15\n" \
	"   mtmsr %1\n" \
	"1:" \
    : "=r" ((_isr_cookie)), "=&r" ((_flash_mask)) \
    : "0" ((_isr_cookie)), "1" ((_flash_mask)) \
    ); \
  }

#define _CPU_FPR_Enable() \
{ register unsigned32 _val = 0; \
	  asm volatile ("mfmsr %0; ori %0,%0,0x2000; mtmsr %0" : \
					"=&r" (_val) : "0" (_val));\
}

#define _CPU_FPR_Disable() \
{ register unsigned32 _val = 0; \
	  asm volatile ("mfmsr %0; rlwinm %0,%0,0,19,17; mtmsr %0" : \
					"=&r" (_val) : "0" (_val));\
}

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

RTEMS_INLINE_ROUTINE unsigned16 bswap16(unsigned16 val)
{
	unsigned16 tmp = val;
	return __lhbrx(&tmp,0);
}

RTEMS_INLINE_ROUTINE unsigned32 bswap32(unsigned32 val)
{
	unsigned32 tmp = val;
	return __lwbrx(&tmp,0);
}

RTEMS_INLINE_ROUTINE unsigned64 bswap64(unsigned64 val)
{
	union ullc {
		unsigned64 ull;
		unsigned32 ul[2];
	} outv;
	unsigned64 tmp = val;

	outv.ul[0] = __lwbrx(&tmp,4);
	outv.ul[1] = __lwbrx(&tmp,0);

	return outv.ull;
}

// Basic I/O

RTEMS_INLINE_ROUTINE unsigned32 read32(unsigned32 addr)
{
	unsigned32 x;
	asm volatile("lwz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

RTEMS_INLINE_ROUTINE void write32(unsigned32 addr, unsigned32 x)
{
	asm("stw %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

RTEMS_INLINE_ROUTINE void mask32(unsigned32 addr, unsigned32 clear, unsigned32 set)
{
	write32(addr, (read32(addr)&(~clear)) | set);
}

RTEMS_INLINE_ROUTINE unsigned16 read16(unsigned32 addr)
{
	unsigned16 x;
	asm volatile("lhz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

RTEMS_INLINE_ROUTINE void write16(unsigned32 addr, unsigned16 x)
{
	asm("sth %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

RTEMS_INLINE_ROUTINE unsigned8 read8(unsigned32 addr)
{
	unsigned8 x;
	asm volatile("lbz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

RTEMS_INLINE_ROUTINE void write8(unsigned32 addr, unsigned8 x)
{
	asm("stb %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

RTEMS_INLINE_ROUTINE void writef32(unsigned32 addr, single_precision x)
{
	asm("stfs %0,0(%1) ; eieio" : : "f"(x), "b"(0xc0000000 | addr));
}

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
