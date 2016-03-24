/// @file preprocessor.hpp
/// UNDOCUMENTED
#pragma once

/// ï∂éöóÒâª.
#define PP_QUOTE(text)		PP_QUOTE_1((text))
#define PP_QUOTE_1(par)		PP_QUOTE_0 ## par
#define PP_QUOTE_0(text)	#text

/// ê√ìIîzóÒÇÃí∑Ç≥
#define lengthof(ar)		(sizeof(ar)/sizeof(ar[0]))

/// òAåã.
#define PP_CAT(a, b)	PP_CAT_OO((a, b))
#define PP_CAT_OO(par)	PP_CAT_I ## par
#define PP_CAT_I(a, b)	PP_CAT_II(a ## b)
#define PP_CAT_II(res)	res

///
#define STATIC_TRACE(msg)		msg(__FILE__ "(" PP_QUOTE(__LINE__) "): " msg)

/// CSV0
#define PP_CSV0_0(fn)	
#define PP_CSV0_1(fn)		fn
#define PP_CSV0_2(fn)		PP_CSV0_1(fn), fn
#define PP_CSV0_3(fn)		PP_CSV0_2(fn), fn
#define PP_CSV0_4(fn)		PP_CSV0_3(fn), fn
#define PP_CSV0_5(fn)		PP_CSV0_4(fn), fn
#define PP_CSV0_6(fn)		PP_CSV0_5(fn), fn
#define PP_CSV0_7(fn)		PP_CSV0_6(fn), fn
#define PP_CSV0_8(fn)		PP_CSV0_7(fn), fn
#define PP_CSV0_9(fn)		PP_CSV0_8(fn), fn
#define PP_CSV0_10(fn)		PP_CSV0_9(fn), fn
#define PP_CSV0(n, fn)		PP_CAT(PP_CSV0_, n)(fn)

/// CSV1
#define PP_CSV1_0(fn)	
#define PP_CSV1_1(fn)		fn(0)
#define PP_CSV1_2(fn)		PP_CSV1_1(fn), fn(1)
#define PP_CSV1_3(fn)		PP_CSV1_2(fn), fn(2)
#define PP_CSV1_4(fn)		PP_CSV1_3(fn), fn(3)
#define PP_CSV1_5(fn)		PP_CSV1_4(fn), fn(4)
#define PP_CSV1_6(fn)		PP_CSV1_5(fn), fn(5)
#define PP_CSV1_7(fn)		PP_CSV1_6(fn), fn(6)
#define PP_CSV1_8(fn)		PP_CSV1_7(fn), fn(7)
#define PP_CSV1_9(fn)		PP_CSV1_8(fn), fn(8)
#define PP_CSV1_10(fn)		PP_CSV1_9(fn), fn(9)
#define PP_CSV1(n, fn)		PP_CAT(PP_CSV1_, n)(fn)

/// CSV1 cat
#define PP_CAT_CSV1_0(fn)	
#define PP_CAT_CSV1_1(fn)		PP_CAT_CSV1_0(fn), fn(0)
#define PP_CAT_CSV1_2(fn)		PP_CAT_CSV1_1(fn), fn(1)
#define PP_CAT_CSV1_3(fn)		PP_CAT_CSV1_2(fn), fn(2)
#define PP_CAT_CSV1_4(fn)		PP_CAT_CSV1_3(fn), fn(3)
#define PP_CAT_CSV1_5(fn)		PP_CAT_CSV1_4(fn), fn(4)
#define PP_CAT_CSV1_6(fn)		PP_CAT_CSV1_5(fn), fn(5)
#define PP_CAT_CSV1_7(fn)		PP_CAT_CSV1_6(fn), fn(6)
#define PP_CAT_CSV1_8(fn)		PP_CAT_CSV1_7(fn), fn(7)
#define PP_CAT_CSV1_9(fn)		PP_CAT_CSV1_8(fn), fn(8)
#define PP_CAT_CSV1_10(fn)		PP_CAT_CSV1_9(fn), fn(9)
#define PP_CAT_CSV1(n, fn)		PP_CAT(PP_CAT_CSV1_, n)(fn)

/// repeat
#define PP_REPEAT_0(fn)		
#define PP_REPEAT_1(fn)		PP_REPEAT_0(fn) fn(0)
#define PP_REPEAT_2(fn)		PP_REPEAT_1(fn) fn(1)
#define PP_REPEAT_3(fn)		PP_REPEAT_2(fn) fn(2)
#define PP_REPEAT_4(fn)		PP_REPEAT_3(fn) fn(3)
#define PP_REPEAT_5(fn)		PP_REPEAT_4(fn) fn(4)
#define PP_REPEAT_6(fn)		PP_REPEAT_5(fn) fn(5)
#define PP_REPEAT_7(fn)		PP_REPEAT_6(fn) fn(6)
#define PP_REPEAT_8(fn)		PP_REPEAT_7(fn) fn(7)
#define PP_REPEAT_9(fn)		PP_REPEAT_8(fn) fn(8)
#define PP_REPEAT_10(fn)	PP_REPEAT_9(fn) fn(9)
#define PP_REPEAT(n, fn)	PP_CAT(PP_REPEAT_, n)(fn)

#define PP_REPEAT_FROM_1_0(fn)		
#define PP_REPEAT_FROM_1_1(fn)
#define PP_REPEAT_FROM_1_2(fn)		PP_REPEAT_FROM_1_1(fn) fn(1)
#define PP_REPEAT_FROM_1_3(fn)		PP_REPEAT_FROM_1_2(fn) fn(2)
#define PP_REPEAT_FROM_1_4(fn)		PP_REPEAT_FROM_1_3(fn) fn(3)
#define PP_REPEAT_FROM_1_5(fn)		PP_REPEAT_FROM_1_4(fn) fn(4)
#define PP_REPEAT_FROM_1_6(fn)		PP_REPEAT_FROM_1_5(fn) fn(5)
#define PP_REPEAT_FROM_1_7(fn)		PP_REPEAT_FROM_1_6(fn) fn(6)
#define PP_REPEAT_FROM_1_8(fn)		PP_REPEAT_FROM_1_7(fn) fn(7)
#define PP_REPEAT_FROM_1_9(fn)		PP_REPEAT_FROM_1_8(fn) fn(8)
#define PP_REPEAT_FROM_1_10(fn)		PP_REPEAT_FROM_1_9(fn) fn(9)
#define PP_REPEAT_FROM_1(n, fn)		PP_CAT(PP_REPEAT_FROM_1_, n)(fn)

// arguments verbs
#define PP_ARG_TYPENAME(n)		typename TArg##n
#define PP_ARG_TYPE(n)			TArg##n
#define PP_ARG_VALUE(n)			arg##n
#define PP_ARG(n)				TArg##n arg##n
#define PP_ARG_CONST(n)			const TArg##n & arg##n

// simple arguments
#define PP_TYPENAMES(n)			PP_CSV1(n, PP_ARG_TYPENAME)
#define PP_TYPENAMES_CAT(n)		PP_CAT_CSV1(n, PP_ARG_TYPENAME)
#define PP_ARG_TYPES(n)			PP_CSV1(n, PP_ARG_TYPE)
#define PP_ARG_VALUES(n)		PP_CSV1(n, PP_ARG_VALUE)
#define PP_ARGS(n)				PP_CSV1(n, PP_ARG)
#define PP_ARGS_CONST(n)		PP_CSV1(n, PP_ARG_CONST)
