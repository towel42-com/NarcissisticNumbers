// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
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
#include <string>
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

    std::pair< uint64_t, uint64_t > range();
    void setRange( const std::pair< uint64_t, uint64_t >& value );

    std::list< uint64_t > numbersList();
    void setNumbersList( const std::list< uint64_t >& values );

    bool useStringBasedAnalysis();
    void setUseStringBasedAnalysis( bool value );

    void reset();
}
    
class CNarcissisticNumCalculator
{
public:
    CNarcissisticNumCalculator( bool saveSettings=true );
    ~CNarcissisticNumCalculator();
    bool parse( int argc, char** argv );
    std::chrono::system_clock::duration run( const std::function< uint64_t( uint64_t, uint64_t ) >& pwrFunction );
    std::chrono::system_clock::duration run();

    void init();
    using TReportFunctionType = std::function< bool( uint64_t min, uint64_t max, uint64_t curr ) >;

    void launch( const TReportFunctionType& reportFunction, bool callInLoop );
    uint64_t partition( const TReportFunctionType & reportFunction, bool callInLoop );

    void setBase( int value ) { if ( ( value < 2 ) || ( value > 36 ) ) return; fBase = value; }
    void setNumThreads( int value ){ fNumThreads = value; }
    void setNumPerThread( uint64_t value ) { fNumPerThread = value; }
    void setByRange( bool value ){ std::get< 0 >( fNumbers ) = value; }
    void setRange( const std::pair< uint64_t, uint64_t >& value ) { std::get< 1 >( fNumbers ) = value; }
    void setNumbersList( const std::list< uint64_t >& values ) { std::get< 2 >( fNumbers ) = values; }

    const std::list< uint64_t > & results() const{ return fNarcissisticNumbers; }
    std::chrono::system_clock::time_point startTime() const{ return fRunTime.first; }
    size_t numPartitions() const{ return fPartitions.size(); }
    size_t numThreads() const { return fHandles.size(); }
    bool isFinished( std::chrono::system_clock::time_point * prev );
    std::pair< std::string, bool > currentResults();

    std::string getRunningResults() const;

    void setStopped( bool stopped ){ fStopped = stopped; }
    std::pair< std::chrono::system_clock::duration, std::chrono::system_clock::duration > computeETA() const;
private:
    void loadSettings();
    void saveSettings() const;

    static int getInt( int& ii, int argc, char** argv, const char* switchName, bool& aOK );
    void dumpNumbers( const std::list< uint64_t >& numbers ) const;
    void report();
    void reportFindings();
    std::pair< bool, bool > checkAndAddValue( uint64_t value );

    using TPartitionSet = std::tuple< bool, std::list< uint64_t >, std::pair< uint64_t, uint64_t > >;

    void findNarcissistic( size_t threadNum, const TPartitionSet& currRange );
    void findNarcissisticRange( size_t threadNum, uint64_t min, uint64_t max );
    void findNarcissisticRange( size_t threadNum, const std::pair< uint64_t, uint64_t >& range );
    void findNarcissisticList( size_t threadNum, const std::list< uint64_t >& values );
    void reportNumPartitionsRemaining( std::chrono::system_clock::time_point& prev, bool force = false );

    void addNarcissisticValue( uint64_t value );

    void addPartition( const std::pair< uint64_t, uint64_t >& range );
    void addPartition( const std::list< uint64_t >& list );
    void analyzeNextPartition( size_t threadNum );

    // setup
    int fBase{ 10 };
    std::tuple< bool, std::pair< uint64_t, uint64_t >, std::list< uint64_t > > fNumbers = std::make_tuple< bool, std::pair< uint64_t, uint64_t >, std::list< uint64_t > >( true, { 0, kDefaultMaxNum }, std::list< uint64_t >() );
    uint64_t fNumPerThread{ 100 };
    int32_t fReportSeconds{ 5 };
    uint32_t fNumThreads;
    std::function< uint64_t( uint64_t, uint64_t ) > fPowerFunction = []( uint64_t x, uint64_t y )->uint64_t { return NUtils::power( x, y ); };


    // results
    std::list< uint64_t > fNarcissisticNumbers;
    std::list< std::chrono::system_clock::duration > fPartitionTimes;
    std::pair< std::chrono::system_clock::time_point, std::chrono::system_clock::time_point > fRunTime;

    // used to do the thread pool
    std::mutex fMutex;
    std::condition_variable fConditionVariable;
    std::vector< std::pair< std::future< void >, std::tuple< uint64_t, uint64_t, uint64_t > > > fHandles;

    // computational values
    std::list< TPartitionSet > fPartitions;
    bool fSaveSettings{ true };
    bool fFinishedPartition{ false };
    bool fStopped{ false };
};
#endif
