// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __UTILS_H
#define __UTILS_H

#include <cinttypes>
#include <string>
#include <chrono>
#include <cmath>

namespace NUtils
{

template< typename T1, typename T2 >
using TLargestType = typename std::conditional< ( sizeof( T1 ) >= sizeof( T2 ) ), T1, T2 >::type;

template < typename T1, typename T2 >
// FLOATING POINT just use std::pow 
// the return type is the larger of the two T1 and T2 types
auto power( T1 x, T2 y )
-> typename std::enable_if< std::is_floating_point<T1>::value || std::is_floating_point<T2>::value, TLargestType< T1, T2 > >::type
{
    return std::pow( x, y );
}

template < typename T1, typename T2>
// Integral types
// the return type is the larger of the two T1 and T2 types
auto power( T1 x, T2 y )
-> typename std::enable_if< std::is_integral<T1>::value && std::is_integral<T2>::value, TLargestType< T1, T2 > >::type
{
    if ( y == 0 )
        return 1;
    if ( y == 1 )
        return x;
    if ( x == 0 )
        return 0;
    if ( x == 1 )
        return 1;

    TLargestType< T1, T2 > retVal = 1;
    for ( T2 ii = 0; ii < y; ++ii )
        retVal *= x;
    return retVal;
}

int fromChar( char ch, int base, bool& aOK );
char toChar( int value );

std::string toString( int64_t val, int base );
int64_t fromString( const std::string& str, int base );
std::string getTimeString( const std::pair< std::chrono::system_clock::time_point, std::chrono::system_clock::time_point >& startEndTime, bool reportTotalSeconds, bool highPrecision );
std::string getTimeString( const std::chrono::system_clock::duration& duration, bool reportTotalSeconds, bool highPrecision );
double getSeconds( const std::chrono::system_clock::duration& duration, bool highPrecision );


bool isNarcissistic( int64_t val, int base, bool& aOK );

}

#endif
