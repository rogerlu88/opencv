/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Copyright (C) 2015, Itseez Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/*============================================================================

This C header file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3c, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2015, 2016, 2017 The Regents of the
University of California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#pragma once
#ifndef softfloat_h
#define softfloat_h 1

#include "cvdef.h"

namespace cv
{

/*----------------------------------------------------------------------------
| Software floating-point underflow tininess-detection mode.
*----------------------------------------------------------------------------*/
enum {
    tininess_beforeRounding = 0,
    tininess_afterRounding  = 1
};
//fixed to make softfloat code stateless
const uint_fast8_t globalDetectTininess = tininess_afterRounding;

/*----------------------------------------------------------------------------
| Software floating-point rounding mode.
*----------------------------------------------------------------------------*/
enum {
    round_near_even   = 0,
    round_minMag      = 1,
    round_min         = 2,
    round_max         = 3,
    round_near_maxMag = 4,
    round_odd         = 5
};
//fixed to make softfloat code stateless
const uint_fast8_t globalRoundingMode = round_near_even;

/*----------------------------------------------------------------------------
| Software floating-point exception flags.
*----------------------------------------------------------------------------*/
enum {
    flag_inexact   =  1,
    flag_underflow =  2,
    flag_overflow  =  4,
    flag_infinite  =  8,
    flag_invalid   = 16
};

// Disabled to make softfloat code stateless
// The user has to check values manually with *_isSignalingNaN() functions
CV_INLINE void raiseFlags( uint_fast8_t /* flags */)
{
    //exceptionFlags |= flags;
}

/*----------------------------------------------------------------------------
*----------------------------------------------------------------------------*/

#define signF32UI( a ) (((uint32_t) (a)>>31) != 0)
#define expF32UI( a ) ((int_fast16_t) ((a)>>23) & 0xFF)
#define fracF32UI( a ) ((a) & 0x007FFFFF)
#define packToF32UI( sign, exp, sig ) (((uint32_t) (sign)<<31) + ((uint32_t) (exp)<<23) + (sig))

#define isNaNF32UI( a ) (((~(a) & 0x7F800000) == 0) && ((a) & 0x007FFFFF))

/*----------------------------------------------------------------------------
*----------------------------------------------------------------------------*/

#define signF64UI( a ) (((uint64_t) (a)>>63) != 0)
#define expF64UI( a ) ((int_fast16_t) ((a)>>52) & 0x7FF)
#define fracF64UI( a ) ((a) & UINT64_C( 0x000FFFFFFFFFFFFF ))
#define packToF64UI( sign, exp, sig ) ((uint64_t) (((uint_fast64_t) (sign)<<63) + ((uint_fast64_t) (exp)<<52) + (sig)))

#define isNaNF64UI( a ) (((~(a) & UINT64_C( 0x7FF0000000000000 )) == 0) && ((a) & UINT64_C( 0x000FFFFFFFFFFFFF )))

/*----------------------------------------------------------------------------
*----------------------------------------------------------------------------*/

struct softfloat32_t;
struct softfloat64_t;

struct CV_EXPORTS softfloat32_t
{
public:
    softfloat32_t() { v = 0; }
    softfloat32_t( const softfloat32_t& c) { v = c.v; }
    softfloat32_t& operator=( const softfloat32_t& c)
    {
        if(&c != this) v = c.v;
        return *this;
    }
    static softfloat32_t fromRaw( uint32_t a) { softfloat32_t x; x.v = a; return x; }

    softfloat32_t( const uint32_t );
    softfloat32_t( const uint64_t );
    softfloat32_t( const int32_t );
    softfloat32_t( const int64_t );
    softfloat32_t( const float a ) { v = *((uint32_t*) &a); }

    uint_fast32_t toUI32_minMag( bool exact = false );
    uint_fast64_t toUI64_minMag( bool exact = false );
    int_fast32_t   toI32_minMag( bool exact = false );
    int_fast64_t   toI64_minMag( bool exact = false );
    uint_fast32_t toUI32( uint_fast8_t roundingMode = round_near_even, bool exact = false );
    uint_fast64_t toUI64( uint_fast8_t roundingMode = round_near_even, bool exact = false );
    int_fast32_t   toI32( uint_fast8_t roundingMode = round_near_even, bool exact = false );
    int_fast64_t   toI64( uint_fast8_t roundingMode = round_near_even, bool exact = false );

    softfloat32_t  round( uint_fast8_t roundingMode = round_near_even, bool exact = false);
    softfloat64_t toF64();
    float toFloat();

    softfloat32_t operator + (const softfloat32_t&) const;
    softfloat32_t operator - (const softfloat32_t&) const;
    softfloat32_t operator * (const softfloat32_t&) const;
    softfloat32_t operator / (const softfloat32_t&) const;
    softfloat32_t operator % (const softfloat32_t&) const;
    softfloat32_t operator - () const { softfloat32_t x; x.v = v ^ (1U << 31); return x; }

    softfloat32_t& operator += (const softfloat32_t& a) { *this = *this + a; return *this; }
    softfloat32_t& operator -= (const softfloat32_t& a) { *this = *this - a; return *this; }
    softfloat32_t& operator *= (const softfloat32_t& a) { *this = *this * a; return *this; }
    softfloat32_t& operator /= (const softfloat32_t& a) { *this = *this / a; return *this; }
    softfloat32_t& operator %= (const softfloat32_t& a) { *this = *this % a; return *this; }

    bool operator == ( const softfloat32_t& ) const;
    bool operator != ( const softfloat32_t& ) const;
    bool operator >  ( const softfloat32_t& ) const;
    bool operator >= ( const softfloat32_t& ) const;
    bool operator <  ( const softfloat32_t& ) const;
    bool operator <= ( const softfloat32_t& ) const;

    bool isNaN() { return (v & 0x7fffffff)  > 0x7f800000; }
    bool isInf() { return (v & 0x7fffffff) == 0x7f800000; }

    static const softfloat32_t zero;
    static const softfloat32_t  inf;
    static const softfloat32_t  nan;
    static const softfloat32_t  one;

    uint32_t v;
};

/*----------------------------------------------------------------------------
*----------------------------------------------------------------------------*/


struct CV_EXPORTS softfloat64_t
{
    softfloat64_t() { }
    softfloat64_t( const softfloat64_t& c) { v = c.v; }
    softfloat64_t& operator=( const softfloat64_t& c )
    {
        if(&c != this) v = c.v;
        return *this;
    }
    static softfloat64_t fromRaw(uint64_t a) { softfloat64_t x; x.v = a; return x; }

    softfloat64_t( const uint32_t );
    softfloat64_t( const uint64_t );
    softfloat64_t( const  int32_t );
    softfloat64_t( const  int64_t );
    softfloat64_t( const double a ) { v = *((uint64_t*) &a); }

    uint_fast32_t toUI32_minMag( bool exact = false );
    uint_fast64_t toUI64_minMag( bool exact = false );
    int_fast32_t   toI32_minMag( bool exact = false );
    int_fast64_t   toI64_minMag( bool exact = false );
    uint_fast32_t toUI32( uint_fast8_t roundingMode = round_near_even, bool exact = false );
    uint_fast64_t toUI64( uint_fast8_t roundingMode = round_near_even, bool exact = false );
    int_fast32_t   toI32( uint_fast8_t roundingMode = round_near_even, bool exact = false );
    int_fast64_t   toI64( uint_fast8_t roundingMode = round_near_even, bool exact = false );

    softfloat64_t  round( uint_fast8_t roundingMode = round_near_even, bool exact = false);
    softfloat32_t toF32();
    double toDouble();

    softfloat64_t operator + (const softfloat64_t&) const;
    softfloat64_t operator - (const softfloat64_t&) const;
    softfloat64_t operator * (const softfloat64_t&) const;
    softfloat64_t operator / (const softfloat64_t&) const;
    softfloat64_t operator % (const softfloat64_t&) const;
    softfloat64_t operator - () const { softfloat64_t x; x.v = v ^ (1ULL << 63); return x; }

    softfloat64_t& operator += (const softfloat64_t& a) { *this = *this + a; return *this; }
    softfloat64_t& operator -= (const softfloat64_t& a) { *this = *this - a; return *this; }
    softfloat64_t& operator *= (const softfloat64_t& a) { *this = *this * a; return *this; }
    softfloat64_t& operator /= (const softfloat64_t& a) { *this = *this / a; return *this; }
    softfloat64_t& operator %= (const softfloat64_t& a) { *this = *this % a; return *this; }

    bool operator == ( const softfloat64_t& ) const;
    bool operator != ( const softfloat64_t& ) const;
    bool operator >  ( const softfloat64_t& ) const;
    bool operator >= ( const softfloat64_t& ) const;
    bool operator <  ( const softfloat64_t& ) const;
    bool operator <= ( const softfloat64_t& ) const;

    bool isNaN() { return (v & 0x7fffffffffffffff)  > 0x7ff0000000000000; }
    bool isInf() { return (v & 0x7fffffffffffffff) == 0x7ff0000000000000; }

    static const softfloat64_t zero;
    static const softfloat64_t  inf;
    static const softfloat64_t  nan;
    static const softfloat64_t  one;

    uint64_t v;
};

/*----------------------------------------------------------------------------
*----------------------------------------------------------------------------*/

CV_EXPORTS softfloat32_t f32_mulAdd( softfloat32_t, softfloat32_t, softfloat32_t );
CV_EXPORTS softfloat32_t f32_sqrt( softfloat32_t );
CV_EXPORTS softfloat64_t f64_mulAdd( softfloat64_t, softfloat64_t, softfloat64_t );
CV_EXPORTS softfloat64_t f64_sqrt( softfloat64_t );

/*----------------------------------------------------------------------------
| Ported from OpenCV and added for usability
*----------------------------------------------------------------------------*/

CV_INLINE softfloat32_t min(const softfloat32_t a, const softfloat32_t b);
CV_INLINE softfloat64_t min(const softfloat64_t a, const softfloat64_t b);

CV_INLINE softfloat32_t max(const softfloat32_t a, const softfloat32_t b);
CV_INLINE softfloat64_t max(const softfloat64_t a, const softfloat64_t b);

CV_INLINE softfloat32_t min(const softfloat32_t a, const softfloat32_t b) { return (a > b) ? b : a; }
CV_INLINE softfloat64_t min(const softfloat64_t a, const softfloat64_t b) { return (a > b) ? b : a; }

CV_INLINE softfloat32_t max(const softfloat32_t a, const softfloat32_t b) { return (a > b) ? a : b; }
CV_INLINE softfloat64_t max(const softfloat64_t a, const softfloat64_t b) { return (a > b) ? a : b; }

CV_INLINE softfloat32_t f32_abs( softfloat32_t a) { softfloat32_t x; x.v = a.v & ((1U   << 31) - 1); return x; }
CV_INLINE softfloat64_t f64_abs( softfloat64_t a) { softfloat64_t x; x.v = a.v & ((1ULL << 63) - 1); return x; }

CV_EXPORTS softfloat32_t f32_exp( softfloat32_t );
CV_EXPORTS softfloat32_t f32_log( softfloat32_t );
CV_EXPORTS softfloat32_t f32_pow( softfloat32_t, softfloat32_t );
CV_EXPORTS softfloat32_t f32_pow( softfloat32_t, int );

CV_EXPORTS softfloat64_t f64_exp( softfloat64_t );
CV_EXPORTS softfloat64_t f64_log( softfloat64_t );
CV_EXPORTS softfloat64_t f64_pow( softfloat64_t, softfloat64_t );
CV_EXPORTS softfloat64_t f64_pow( softfloat64_t, int) ;

CV_EXPORTS softfloat32_t f32_cbrt( softfloat32_t );

}

#endif
