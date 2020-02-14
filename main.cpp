#include "NarcissisticNumCalculator.h"
#include "utils.h"

#include <unordered_map>
#include <iostream>

using TRunTime = std::tuple< std::function< int64_t( int64_t, int64_t ) >, std::chrono::system_clock::duration, int, std::string >;

void report( const std::string& prefix, const TRunTime& curr )
{
    if ( !prefix.empty() )
        std::cout << prefix << ": ";

    std::cout << std::get< 3 >( curr ) << " - NumThreads : " << std::get< 2 >( curr ) << " - " << NUtils::getTimeString( std::get< 1 >( curr ), true, true ) << std::endl;
}

void reportMinMax( const std::vector<TRunTime>& runTimes, size_t max )
{
    auto minMax = std::minmax_element( runTimes.begin(), runTimes.begin() + max, []( const TRunTime& lhs, const TRunTime& rhs ) { return std::get< 1 >( lhs ) < std::get< 1 >( rhs ); } );

    std::cout << "=============================================\n";
    report( "Fastest", ( *minMax.first ) );
    report( "Slowest", ( *minMax.second ) );
    std::cout << "=============================================\n";
}

int main( int argc, char** argv )
{
    CNarcissisticNumCalculator values;
    if ( !values.parse( argc, argv ) )
        return 1;


    std::vector< TRunTime > runTimes;
    for( int ii = 6; ii <= 500; ++ii )
    {
        runTimes.push_back( std::make_tuple( []( int64_t x, int64_t y )->int64_t { return NUtils::power( x, y ); }, std::chrono::system_clock::duration(), ii, "Loop" ) );
    }

    try
    {
        for ( size_t ii = 0; ii < runTimes.size(); ++ii )
        {
            auto && curr = runTimes[ ii ];
            values.setNumThreads( std::get< 2 >( curr ) );
            std::get< 1 >( curr ) = values.run( std::get< 0 >( curr ) );

            reportMinMax( runTimes, ii );
        }
    }
    catch ( ... )
    {
    }

    for ( auto&& curr : runTimes )
    {
        report( "", curr );
    }

    return 0;
}

// for 1000000
//0, 1, 2, 3, 4
//5, 6, 7, 8, 9
//153, 370, 371, 407, 1634
//8208, 9474, 54748, 92727, 93084
//548834