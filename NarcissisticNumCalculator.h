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

namespace CNarcissisticNumCalculatorDefaults
{
    int base();
    void setBase( int value );

    int numThreads();
    void setNumThreads( int value );

    int numPerThread();
    void setNumPerThread( int value );

    bool byRange();
    void setByRange( bool value );

    std::pair< int64_t, int64_t > range();
    void setRange( const std::pair< int64_t, int64_t >& value );

    std::list< int64_t > numbersList();
    void setNumbersList( const std::list< int64_t >& values );
}
    
class CNarcissisticNumCalculator
{
public:
    CNarcissisticNumCalculator( bool saveSettings=true );
    ~CNarcissisticNumCalculator();
    bool parse( int argc, char** argv );
    std::chrono::system_clock::duration run( const std::function< int64_t( int64_t, int64_t ) >& pwrFunction );
    std::chrono::system_clock::duration run();

    void init();
    void partition( bool report = true );
    void launch( bool report = true );

    void setBase( int value ) { fBase = value; }
    void setNumThreads( int value ){ fNumThreads = value; }
    void setNumPerThread( int value ) { fNumPerThread = value; }
    void setByRange( bool value ){ std::get< 0 >( fNumbers ) = value; }
    void setRange( const std::pair< int64_t, int64_t >& value ) { std::get< 1 >( fNumbers ) = value; }
    void setNumbersList( const std::list< int64_t >& values ) { std::get< 2 >( fNumbers ) = values; }

    std::list< int64_t > results() const{ return fNarcissisticNumbers; }
    std::chrono::system_clock::time_point startTime() const{ return fRunTime.first; }
    size_t numPartitions() const{ return fPartitions.size(); }
    size_t numThreads() const { return fHandles.size(); }
    bool isFinished( std::chrono::system_clock::time_point * prev );
private:
    void loadSettings();

    void saveSettings() const;
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
    void reportNumRangesRemaining( std::chrono::system_clock::time_point& prev, bool force = false );

    void addNarcissisticValue( int64_t value );

    void addRange( const std::pair< int64_t, int64_t >& range );
    void addRange( const std::list< int64_t >& list );
    void analyzeNextRange();

    // setup
    int fBase{ 10 };
    std::tuple< bool, std::pair< int64_t, int64_t >, std::list< int64_t > > fNumbers = std::make_tuple< bool, std::pair< int64_t, int64_t >, std::list< int64_t > >( true, { 0, kDefaultMaxNum }, std::list< int64_t >() );
    int fNumPerThread{ 100 };
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
    std::list< TRangeSet > fPartitions;
    bool fSaveSettings{ true };
    bool fFinishedPartition{ false };

};
#endif
