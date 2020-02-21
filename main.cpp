#include "NarcissisticNumCalculator.h"
#include "utils.h"

#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

using TPowerFunc = std::function< int64_t( int64_t, int64_t ) >;
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
    CNarcissisticNumCalculator values;
    if ( !values.parse( argc, argv ) )
        return 1;


    TPowerFunc powerFunc = []( int64_t x, int64_t y )->int64_t { return NUtils::power( x, y ); };
    auto zeroDuration = std::chrono::system_clock::duration();
    std::vector< TRunTime > runTimes;
    int numCores = std::thread::hardware_concurrency();
    for( int ii = numCores; ii <= 50; ii += numCores )
    {
        for( int jj = 100; jj < 1000; jj += 100 )
            runTimes.push_back( std::make_tuple( powerFunc, zeroDuration, ii, "Loop", jj ) );
    }

    for ( size_t ii = 0; ii < runTimes.size(); ++ii )
    {
        auto && curr = runTimes[ ii ];
        values.setNumThreads( std::get< 2 >( curr ) );
        values.setNumPerRange( std::get< 4 >( curr ) );
        std::get< 1 >( curr ) = values.run( std::get< 0 >( curr ) );

        reportTimes( runTimes, ii );
    }

    for ( auto&& curr : runTimes )
    {
        report( "", curr );
    }

    reportTimes( runTimes );

    return 0;
}

// for 1000000
//0, 1, 2, 3, 4
//5, 6, 7, 8, 9
//153, 370, 371, 407, 1634
//8208, 9474, 54748, 92727, 93084
//548834
