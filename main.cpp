// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "NarcissisticNumbers.h"
#include "NarcissisticNumCalculator.h"
#include "utils.h"

#include <QApplication>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

using TPowerFunc = std::function< uint64_t( uint64_t, uint64_t ) >;
using TRunTime = std::tuple< TPowerFunc, std::chrono::system_clock::duration, int, std::string, int >;

void report( const std::string& prefix, const TRunTime& curr )
{
    if ( !prefix.empty() )
        std::cout << prefix << ": ";

    std::cout << std::get< 3 >( curr ) << " - Num Threads : " << std::get< 2 >( curr ) << " - Num Per Thread: " << std::get< 4 >( curr ) << " - " << NUtils::getTimeString( std::get< 1 >( curr ), true, true ) << std::endl;
}

void reportTimes( const std::vector<TRunTime>& runTimes, size_t max )
{
    if ( runTimes.empty() )
        return;

    double total = 0;
    auto sorted = std::vector<TRunTime>( runTimes.begin(), runTimes.begin() + max + 1 );
    std::sort( sorted.begin(), sorted.end(), []( const TRunTime& lhs, const TRunTime& rhs ) { return std::get< 1 >( lhs ) < std::get< 1 >( rhs ); }  );
    for( auto && ii : sorted )
    {
        total += NUtils::getSeconds( std::get< 1 >( ii ), true );
    }

    auto mean = total / ( max + 1 );

    double stdDev = 0;
    for( auto && ii : sorted )
    {
        auto currVal = NUtils::getSeconds( std::get< 1 >( ii ), true ) - mean;
        stdDev += currVal * currVal;
    }

    stdDev = std::sqrt( stdDev / ( max + 1 ) );

    std::cout << "=============================================\n";
    report( "Min/Fastest", *sorted.begin() );
    if ( sorted.size() > 4 )
        report( "Second Fastest", *(sorted.begin()+1) );
    auto medianPos = sorted.begin() + ( sorted.size() / 2 );
    report( "Median", *medianPos );
    std::cout << "Mean: " << mean << " seconds" << std::endl;
    report( "Max/Slowest", *sorted.rbegin() );
    if ( sorted.size() > 4 )
        report( "Second Slowest", *( sorted.rbegin() + 1 ) );

    auto prev = std::cout.flags();
    std::cout << "StdDev: " << stdDev << " seconds" << "(" << std::fixed << std::setprecision( 2 ) << 100 * stdDev/mean << "%)" << std::endl;

    std::cout.flags( prev );
    std::cout << "=============================================\n";
}

void reportTimes( const std::vector<TRunTime>& runTimes )
{
    reportTimes( runTimes, runTimes.size() - 1 );
}

int main( int argc, char** argv )
{
    //CNarcissisticNumCalculator values;
    //if ( !values.parse( argc, argv ) )
    //    return 1;


    //TPowerFunc powerFunc = []( uint64_t x, uint64_t y )->uint64_t { return NUtils::power( x, y ); };
    //auto zeroDuration = std::chrono::system_clock::duration();
    //std::vector< TRunTime > runTimes;
    //int numCores = std::thread::hardware_concurrency();
    //for( int ii = numCores; ii <= 50; ii += numCores )
    //{
    //    for( int jj = 100; jj < 1000; jj += 100 )
    //        runTimes.push_back( std::make_tuple( powerFunc, zeroDuration, ii, "Loop", jj ) );
    //}

    //for ( size_t ii = 0; ii < runTimes.size(); ++ii )
    //{
    //    auto && curr = runTimes[ ii ];
    //    values.setNumThreads( std::get< 2 >( curr ) );
    //    values.setNumPerThread( std::get< 4 >( curr ) );
    //    std::get< 1 >( curr ) = values.run( std::get< 0 >( curr ) );

    //    reportTimes( runTimes, ii );
    //}

    //for ( auto&& curr : runTimes )
    //{
    //    report( "", curr );
    //}

    //reportTimes( runTimes );

    QApplication appl( argc, argv );
    appl.setOrganizationDomain( "http://towel42.com" );
    appl.setOrganizationName( "Scott Aron Bloom" );
    appl.setApplicationName( "Narcissistic Number Calculator" );
    appl.setApplicationVersion( "1.0.0" );
    Q_INIT_RESOURCE( application );
    CNarcissisticNumbers calc;
    return calc.exec();
}
