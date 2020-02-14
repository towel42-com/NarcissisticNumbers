#include "NarcissisticNumCalculator.h"
#include "utils.h"

#include <unordered_map>
#include <iostream>

int main( int argc, char** argv )
{
    CNarcissisticNumCalculator values;
    if ( !values.parse( argc, argv ) )
        return 1;


    using TRunTime = std::tuple< std::function< int64_t( int64_t, int64_t ) >, std::chrono::system_clock::duration, std::string >;

    std::vector< TRunTime > runTimes;
    runTimes.push_back( std::make_tuple( []( int64_t x, int64_t y )->int64_t { return NUtils::power( x, y ); }, std::chrono::system_clock::duration(), "Loop" ) );
    runTimes.push_back( std::make_tuple( []( int64_t x, int64_t y )->int64_t { return static_cast<int64_t>( std::pow( x, y ) ); }, std::chrono::system_clock::duration(), "std::pow" ) );

    for ( auto&& curr : runTimes )
    {
        std::get< 1 >( curr ) = values.run( std::get< 0 >( curr ) );
    }

    auto minMax = std::minmax_element( runTimes.begin(), runTimes.end(), []( const TRunTime& lhs, const TRunTime& rhs ) { return std::get< 1 >( lhs ) < std::get< 1 >( rhs ); } );

    std::cout << "=============================================\n";
    std::cout << "Fastest: " << std::get< 2 >( *( minMax.first ) ) << " - " << NUtils::getTimeString( std::get< 1 >( *( minMax.first ) ), true, true ) << std::endl;
    std::cout << "Slowest: " << std::get< 2 >( *( minMax.second ) ) << " - " << NUtils::getTimeString( std::get< 1 >( *( minMax.second ) ), true, true ) << std::endl;
    std::cout << "=============================================\n";
    for ( auto&& curr : runTimes )
    {
        std::cout << "Fastest: " << std::get< 2 >( curr ) << " - " << NUtils::getTimeString( std::get< 1 >( curr ), true, true ) << std::endl;
    }

    return 0;
}

// for 1000000
//0, 1, 2, 3, 4
//5, 6, 7, 8, 9
//153, 370, 371, 407, 1634
//8208, 9474, 54748, 92727, 93084
//548834