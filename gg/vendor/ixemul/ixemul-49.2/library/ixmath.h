#ifndef __IXMATH_H__
#define __IXMATH_H__

#ifndef __HAVE_68881__

#ifdef __pos__
extern void *gb_MathIeeeSingBasBase;
extern void *gb_MathIeeeDoubBasBase;
extern void *gb_MathIeeeDoubTransBase;

#include <pInline/pMathIEEE2.h>
#include <pInline/pMathIEEED2.h>
#include <pInline/pMathIEEEDTrs2.h>

#define IEEESPFix   pOS_IEEESPFix
#define IEEESPFlt   pOS_IEEESPFlt
#define IEEESPCmp   pOS_IEEESPCmp
#define IEEESPTst   pOS_IEEESPTst
#define IEEESPAbs   pOS_IEEESPAbs
#define IEEESPNeg   pOS_IEEESPNeg
#define IEEESPAdd   pOS_IEEESPAdd
#define IEEESPSub   pOS_IEEESPSub
#define IEEESPMul   pOS_IEEESPMul
#define IEEESPDiv   pOS_IEEESPDiv
#define IEEESPFloor pOS_IEEESPFloor
#define IEEESPCeil  pOS_IEEESPCeil

#define IEEEDPFix   pOS_IEEEDPFix
#define IEEEDPFlt   pOS_IEEEDPFlt
#define IEEEDPCmp   pOS_IEEEDPCmp
#define IEEEDPTst   pOS_IEEEDPTst
#define IEEEDPAbs   pOS_IEEEDPAbs
#define IEEEDPNeg   pOS_IEEEDPNeg
#define IEEEDPAdd   pOS_IEEEDPAdd
#define IEEEDPSub   pOS_IEEEDPSub
#define IEEEDPMul   pOS_IEEEDPMul
#define IEEEDPDiv   pOS_IEEEDPDiv
#define IEEEDPFloor pOS_IEEEDPFloor
#define IEEEDPCeil  pOS_IEEEDPCeil

#define IEEEDPAtan      pOS_IEEEDPAtan
#define IEEEDPSin       pOS_IEEEDPSin
#define IEEEDPCos       pOS_IEEEDPCos
#define IEEEDPTan       pOS_IEEEDPTan
#define IEEEDPSincos    pOS_IEEEDPSincos
#define IEEEDPSinh      pOS_IEEEDPSinh
#define IEEEDPCosh      pOS_IEEEDPCosh
#define IEEEDPTanh      pOS_IEEEDPTanh
#define IEEEDPExp       pOS_IEEEDPExp
#define IEEEDPLog       pOS_IEEEDPLog
#define IEEEDPPow       pOS_IEEEDPPow
#define IEEEDPSqrt      pOS_IEEEDPSqrt
#define IEEEDPTieee     pOS_IEEEDPTieee
#define IEEEDPFieee     pOS_IEEEDPFieee
#define IEEEDPAsin      pOS_IEEEDPAsin
#define IEEEDPAcos      pOS_IEEEDPAcos
#define IEEEDPLog10     pOS_IEEEDPLog10

#else
#include <proto/mathieeedoubbas.h>
#include <proto/mathieeedoubtrans.h>
#include <proto/mathieeesingbas.h>
#endif

#endif /* __HAVE_68881__ */

#endif /* __IXMATH_H__ */
