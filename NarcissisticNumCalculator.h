#ifndef __NARCISSISTICNUMCALCULATOR_H
#define __NARCISSISTICNUMCALCULATOR_H

#include "utils.h"

#include <algorithm>
#include <list>
#include <chrono>
#include <mutex>
#include <future>
#include <functional>
#include <condition_variable>

#ifdef _DEBUG
static uint32_t kDefaultMaxNum{ 100000 };
#else
static uint32_t kDefaultMaxNum{ 50000000 };
#endif

class CNarcissisticNumCalculator
{
public:
    CNarcissisticNumCalculator();
    bool parse( int argc, char** argv );
    std::chrono::system_clock::duration run( const std::function< int64_t( int64_t, int64_t ) >& pwrFunction );
    std::chrono::system_clock::duration run();

    void setNumThreads( int ii ){ fNumThreads = ii; }
    void setNumPerRange( int ii ) { fNumPerRange = ii; }
private:
    static int getInt( int& ii, int argc, char** argv, const char* switchName, bool& aOK );
    void dumpNumbers( const std::list< int64_t >& numbers ) const;
    void report();
    void reportFindings();
    std::pair< bool, bool > checkAndAddValue( int64_t value );

    using TRangeSet = std::tuple< bool, std::list< int64_t >, std::pair< int64_t, int64_t > >;

    void findNarcissistic( const TRangeSet& currRange );
    void findNarcissisticRange( int64_t min, int64_t max );
    void findNarcissisticRange( const std::pair< int64_t, int64_t >& range );
    void findNarcissisticList( const std::list< int64_t >& values );
    void partition();
    void launch();
    void reportNumRangesRemaining( std::chrono::system_clock::time_point& prev, bool force = false );

    bool isFinished( std::chrono::system_clock::time_point& prev );
    void addNarcissisticValue( int64_t value );

    bool isNarcissistic( int64_t val, int base, bool& aOK );

    void addRange( const std::pair< int64_t, int64_t >& range );
    void addRange( const std::list< int64_t >& list );
    void analyzeNextRange();

    // setup
    int fBase{ 10 };
    std::pair< std::pair< int64_t, int64_t >, std::list< int64_t > > fNumbers = std::make_pair< std::pair< int64_t, int64_t >, std::list< int64_t > >( { 0, kDefaultMaxNum }, std::list< int64_t >() );
    int fNumPerRange{ 100 };
    int fReportSeconds{ 5 };
    uint32_t fNumThreads;
    std::function< int64_t( int64_t, int64_t ) > fPowerFunction = []( int64_t x, int64_t y )->int64_t { return NUtils::power( x, y ); };


    // results
    std::list< int64_t > fNarcissisticNumbers;
    std::pair< std::chrono::system_clock::time_point, std::chrono::system_clock::time_point > fRunTime;

    // used to do the thread pool
    std::mutex fMutex;
    std::condition_variable fConditionVariable;
    std::list< std::future< void > > fHandles;

    // computational values
    std::list< TRangeSet > fRanges;
    bool fFinished{ false };

};
#endif
