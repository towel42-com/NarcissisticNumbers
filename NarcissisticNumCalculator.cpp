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

#include "NarcissisticNumCalculator.h"
#include "utils.h"

#include <QSettings>

#include <iostream>
#include <cctype>
#include <string>
#include <cstring>
#include <sstream>

namespace CNarcissisticNumCalculatorDefaults
{
    int base()
    {
        QSettings settings;
        return settings.value( "Base", 10 ).toInt();
    }

    void setBase( int value )
    {
        QSettings settings;
        return settings.setValue( "Base", value );
    }

    int numThreads()
    {
        QSettings settings;
        return settings.value( "NumThreads", std::thread::hardware_concurrency() ).toInt();
    }

    void setNumThreads( int value )
    {
        QSettings settings;
        return settings.setValue( "NumThreads", value );
    }

    int numPerThread()
    {
        QSettings settings;
        return settings.value( "NumPerThread", 100 ).toInt();
    }

    void setNumPerThread( int value )
    {
        QSettings settings;
        return settings.setValue( "NumPerThread", value );
    }

    bool byRange()
    {
        QSettings settings;
        return settings.value( "ByRange", true ).toBool();
    }

    void setByRange( bool value )
    {
        QSettings settings;
        return settings.setValue( "ByRange", value );
    }

    std::list< uint64_t > toIntList( const QList< QVariant > & inList )
    {
        std::list< uint64_t > retVal;
        for( auto && ii : inList )
            retVal.push_back( ii.toULongLong() );
        return retVal;
    }

    std::pair< uint64_t, uint64_t > range()
    {
        QSettings settings;
        auto retVal = toIntList( settings.value( "Range", QVariant::fromValue( QList< QVariant >() << 0 << kDefaultMaxNum ) ).value< QList< QVariant > >() );
        return std::make_pair( retVal.front(), retVal.back() );
    }

    void setRange( const std::pair< uint64_t, uint64_t >& value )
    {
        QSettings settings;
        auto values = QList< QVariant >() << value.first << value.second;
        return settings.setValue( "Range", QVariant::fromValue( values ) );
    }

    std::list< uint64_t > numbersList()
    {
        QSettings settings;
        auto retVal = toIntList( settings.value( "NumbersList", QVariant::fromValue( QList< QVariant >() ) ).value< QList< QVariant > >() );
        return retVal;
    }

    void setNumbersList( const std::list< uint64_t >& value )
    {
        QSettings settings;
        QList< QVariant > values;
        for( auto && ii : value )
            values << ii;
        return settings.setValue( "NumbersList", QVariant::fromValue( values ) );
    }

    bool useStringBasedAnalysis()
    {
        QSettings settings;
        return settings.value( "UseStringBasedAnalysis", false ).toBool();
    }

    void setUseStringBasedAnalysis( bool value )
    {
        QSettings settings;
        return settings.setValue( "UseStringBasedAnalysis", value );
    }

    void reset()
    {
        QSettings settings;
        settings.remove( "Base" );
        settings.remove( "NumThreads" );
        settings.remove( "NumPerThread" );
        settings.remove( "ByRange" );
        settings.remove( "Range" );
        settings.remove( "NumbersList" );
        settings.remove( "UseStringBasedAnalysis" );
    }
}

CNarcissisticNumCalculator::CNarcissisticNumCalculator( bool saveSettings )
{
    fSaveSettings = saveSettings;
    fNumThreads = std::thread::hardware_concurrency();
    loadSettings();
    init();
}

CNarcissisticNumCalculator::~CNarcissisticNumCalculator()
{
    if ( fSaveSettings )
        saveSettings();
}

bool CNarcissisticNumCalculator::parse( int argc, char** argv )
{
    for ( int ii = 1; ii < argc; ++ii )
    {
        bool aOK = false;
        if ( strncmp( argv[ ii ], "-base", 5 ) == 0 )
        {
            fBase = getInt( ii, argc, argv, "-base", aOK );
            if ( aOK && ( ( fBase < 2 ) || ( fBase > 36 ) ) )
            {
                std::cerr << "Base must be between 2 and 36" << std::endl;
                aOK = false;
            }
        }
        else if ( strncmp( argv[ ii ], "-min", 4 ) == 0 )
        {
            std::get< 0 >( fNumbers ) = true;
            std::get< 1 >( fNumbers ).first = getInt( ii, argc, argv, "-min", aOK );
        }
        else if ( strncmp( argv[ ii ], "-max", 4 ) == 0 )
        {
            std::get< 0 >( fNumbers ) = true;
            std::get< 1 >( fNumbers ).second = getInt( ii, argc, argv, "-max", aOK );
        }
        else if ( strncmp( argv[ ii ], "-num_threads", 12 ) == 0 )
        {
            fNumThreads = getInt( ii, argc, argv, "-num_threads", aOK );
        }
        else if ( strncmp( argv[ ii ], "-range_max", 11 ) == 0 )
        {
            fNumPerThread = getInt( ii, argc, argv, "-range_max", aOK );
        }
        else if ( strncmp( argv[ ii ], "-report_seconds", 15 ) == 0 )
        {
            fReportSeconds = getInt( ii, argc, argv, "-report_seconds", aOK );
        }
        else if ( strncmp( argv[ ii ], "-numbers", 8 ) == 0 )
        {
            std::get< 0 >( fNumbers ) = false;
            aOK = ( ii + 1 ) < argc;
            if ( !aOK )
            {
                std::cerr << "-numbers requires a list of integers\n";
            }
            while ( aOK )
            {
                if ( argv[ ii + 1 ][ 0 ] == '-' )
                    break;

                auto curr = getInt( ii, argc, argv, "-numbers", aOK );
                if ( aOK )
                    std::get< 2 >( fNumbers ).push_back( curr );
                if ( ( ii + 1 ) >= argc )
                    break;
            }
        }
        else
        {
            std::cerr << "unknown switch: '" << argv[ ii ] << "'\n";
            aOK = false;
        }

        if ( !aOK )
            return false;

    }
    return true;
}

void CNarcissisticNumCalculator::launch( const TReportFunctionType & reportFunction, bool callInLoop )
{
    for ( unsigned int ii = 0; ii < fNumThreads; ++ii )
    {
        fHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::analyzeNextPartition, this ) );
        if ( callInLoop && reportFunction )
        {
            if ( !reportFunction( 0, fNumThreads, ii ) )
                break;
        }
    }
    if ( reportFunction )
    {
        reportFunction( 0, fNumThreads, fNumThreads);
    }
}

std::chrono::system_clock::duration CNarcissisticNumCalculator::run()
{
    return run( std::function< int64_t( int64_t, int64_t ) >() );
}

void CNarcissisticNumCalculator::init()
{
    fNarcissisticNumbers.clear();
    fHandles.clear();
    fFinishedPartition = false;
    fRunTime.first = std::chrono::system_clock::now();
}

std::chrono::system_clock::duration CNarcissisticNumCalculator::run( const std::function< int64_t( int64_t, int64_t ) >& pwrFunction )
{
    init();
    report();
    if ( pwrFunction )
        fPowerFunction = pwrFunction;

    TReportFunctionType launchReport = 
        [this]( int /*min*/, int /*max*/, int /*curr*/ )
    {
        std::cout << "=============================================\n";
        std::unique_lock< std::mutex > lock( fMutex );
        std::cout << "Number of Threads Created: " << fHandles.size() << "\n";
        return true;
    };
    launch( launchReport, false );
    std::cout << "=============================================\n";

    TReportFunctionType partitionReport =
        [ this ]( int /*min*/, int /*max*/, int /*curr*/ )
    {
        std::unique_lock< std::mutex > lock( fMutex );
        std::cout << "Number of Ranges Created: " << fPartitions.size() << "\n";
        std::cout << "=============================================\n";
        return true;
    };
    partition( partitionReport, false );

    auto prev = std::chrono::system_clock::now();
    reportNumPartitionsRemaining( prev, true );
    while ( !isFinished( &prev ) )
    {
    }
    reportNumPartitionsRemaining( prev, true );

    fRunTime.second = std::chrono::system_clock::now();
    reportFindings();
    return fRunTime.second - fRunTime.first;
}

void CNarcissisticNumCalculator::loadSettings()
{
    QSettings settings;
    fBase = CNarcissisticNumCalculatorDefaults::base();
    fNumThreads = CNarcissisticNumCalculatorDefaults::numThreads();
    fNumPerThread = CNarcissisticNumCalculatorDefaults::numPerThread();
    std::get< 0 >( fNumbers ) = CNarcissisticNumCalculatorDefaults::byRange();
    std::get< 1 >( fNumbers ) = CNarcissisticNumCalculatorDefaults::range();
    std::get< 2 >( fNumbers ) = CNarcissisticNumCalculatorDefaults::numbersList();
}

void CNarcissisticNumCalculator::saveSettings() const
{
    QSettings settings;
    CNarcissisticNumCalculatorDefaults::setBase( fBase );
    CNarcissisticNumCalculatorDefaults::setNumThreads( fNumThreads );
    CNarcissisticNumCalculatorDefaults::setNumPerThread( fNumPerThread );

    CNarcissisticNumCalculatorDefaults::setByRange( std::get< 0 >( fNumbers ) );
    CNarcissisticNumCalculatorDefaults::setRange( std::get< 1 >( fNumbers ) );
    CNarcissisticNumCalculatorDefaults::setNumbersList( std::get< 2 >( fNumbers ) );
}

int CNarcissisticNumCalculator::getInt( int& ii, int argc, char** argv, const char* switchName, bool& aOK )
{
    aOK = false;

    if ( ++ii == argc )
    {
        std::cerr << "-base requires a value";
        return 0;
    }
    const char* str = argv[ ii ];
    int retVal = 0;
    try
    {
        retVal = std::stoi( str );
        aOK = true;
    }
    catch ( std::invalid_argument const& e )
    {
        std::cerr << switchName << " value '" << str << "' is invalid. \n" << e.what() << "\n";
    }
    catch ( std::out_of_range const& e )
    {
        std::cerr << switchName << " value '" << str << "' is out of range. \n" << e.what() << "\n";
    }

    return retVal;
}

void CNarcissisticNumCalculator::dumpNumbers( const std::list< uint64_t >& numbers ) const
{
    bool first = true;
    size_t ii = 0;
    std::string str;
    for ( auto&& currVal : numbers )
    {
        if ( ii && ( ii % 5 == 0 ) )
        {
            std::cout << "\n";
            first = true;
        }
        if ( !first )
            std::cout << ", ";
        else
            std::cout << "    ";
        first = false;
        
        std::cout << NUtils::toString( currVal, fBase );
        if ( fBase != 10 )
            std::cout << "(=" << currVal << ")";
        ii++;
    }
    std::cout << "\n";
}

void CNarcissisticNumCalculator::report()
{
    if ( std::get< 0 >( fNumbers ) )
    {
        std::cout << "Finding Narcissistic in the range: [" << std::get< 1 >( fNumbers ).first << ":" << std::get< 1 >( fNumbers ).second << "]." << std::endl;
    }
    else
    {
        std::cout << "Checking if the following numbers are Narcissistic:\n";
        dumpNumbers( std::get< 2 >( fNumbers ) );
    }
    std::cout << "Maximum Numbers per thread: " << fNumPerThread << "\n";
    std::cout << "Base : " << fBase << "\n";
    std::cout << "HW Concurrency : " << std::thread::hardware_concurrency() << "\n";
}

void CNarcissisticNumCalculator::reportFindings()
{
    std::cout << "=============================================\n";
    std::cout << "There are " << fNarcissisticNumbers.size() << " Narcissistic numbers";
    if ( std::get< 0 >( fNumbers ) )
        std::cout << " in the range [" << std::get< 1 >( fNumbers ).first << ":" << std::get< 1 >( fNumbers ).second << "]." << std::endl;
    else
        std::cout << " in the requested list." << std::endl;
    fNarcissisticNumbers.sort();
    dumpNumbers( fNarcissisticNumbers );
    std::cout << "=============================================\n";
    std::cout << "Runtime: " << NUtils::getTimeString( fRunTime, true, true ) << std::endl;
    std::cout << "=============================================\n";
}

std::pair< bool, bool > CNarcissisticNumCalculator::checkAndAddValue( int64_t value )
{
    bool aOK = true;
    bool isNarcissistic = NUtils::isNarcissistic( value, fBase, aOK );
    if ( !aOK )
        return std::make_pair( false, false );
    if ( isNarcissistic )
    {
        addNarcissisticValue( value );
    }
    return std::make_pair( isNarcissistic, true );
}

void CNarcissisticNumCalculator::analyzeNextPartition()
{
    while ( true ) // infinite loop 
    {
        TPartitionSet currRange;
        {
            //std::cout << std::this_thread::get_id() << " - Trying next\n";
            std::unique_lock< std::mutex > lock( fMutex );
            //std::cout << "Locked: " << std::this_thread::get_id() << " - Analyze Next Range\n";

            fConditionVariable.wait( lock, [ this ]() { return ( fFinishedPartition || !fPartitions.empty() ); } );
            if ( fPartitions.empty() )
            {
                //std::cout << std::this_thread::get_id() << " - Finished with all ranges\n";
                //std::cout << "UnLocked - Analyze Next Range (finished)\n";
                return;
            }

            currRange = std::move( fPartitions.front() );
            fPartitions.pop_front();
            //std::cout << "UnLocked: Analyze Next Range (still going)\n";
        }
        findNarcissistic( currRange );
        if ( fStopped )
            break;
    }
}

void CNarcissisticNumCalculator::findNarcissistic( const TPartitionSet& range )
{
    auto start = std::chrono::system_clock::now();

    if ( std::get< 0 >( range ) )
        findNarcissisticRange( std::get< 2 >( range ) );
    else
        findNarcissisticList( std::get< 1 >( range ) );

    auto end = std::chrono::system_clock::now();

    std::lock_guard< std::mutex > lock( fMutex );
    fPartitionTimes.push_back( end - start );
}

void CNarcissisticNumCalculator::findNarcissisticRange( uint64_t min, uint64_t max )
{
    return findNarcissisticRange( std::make_pair( min, max ) );
}

void CNarcissisticNumCalculator::findNarcissisticRange( const std::pair< uint64_t, uint64_t >& range )
{
    {
        //std::unique_lock< std::mutex > lock(fMutex);
        //std::cout << "Locked: FindNarcissisticRange - Header\n";
        //std::cout << "\n" << std::this_thread::get_id() << ": Computing for (" << range.first << "," << range.second - 1 << ")" << std::endl;
        //std::cout << "UnLocked: FindNarcissisticRange - Header\n";
    }
    int numArm = 0;
    for ( auto ii = range.first; ii < range.second; ++ii )
    {
        bool isNarcissistic = false;
        bool aOK = false;
        std::tie( isNarcissistic, aOK ) = checkAndAddValue( ii );
        if ( !aOK )
            return;
        if ( isNarcissistic )
            numArm++;
        if ( fStopped )
            break;
    }
    {
        //std::unique_lock< std::mutex > lock(fMutex);
        //std::cout << "Locked: FindNarcissisticRange - Footer\n";
        //std::cout << "\n" << std::this_thread::get_id() << ": ----> Computing for (" << range.first << "," << range.second - 1 << ")" << " = " << numArm << std::endl;
        //std::cout << "UnLocked: FindNarcissisticRange - Footer\n";
    }
}

void CNarcissisticNumCalculator::findNarcissisticList( const std::list< uint64_t >& values )
{
    int numArm = 0;
    for ( auto ii : values )
    {
        bool isNarcissistic = false;
        bool aOK = false;
        std::tie( isNarcissistic, aOK ) = checkAndAddValue( ii );
        if ( !aOK )
            return;
        if ( isNarcissistic )
            numArm++;
    }
}

int64_t CNarcissisticNumCalculator::partition( const TReportFunctionType & reportFunction, bool callInLoop )
{
    int64_t min = 0;
    int64_t max = 0;
    int64_t numPartitions = 0;
    if ( std::get< 0 >( fNumbers ) )
    {
        min = std::get< 1 >( fNumbers ).first;
        max = std::get< 1 >( fNumbers ).second;
        for ( auto ii = min; ii < max; ii += fNumPerThread, ++numPartitions )
        {
            auto lclMax = std::min( std::get< 1 >( fNumbers ).second, static_cast< uint64_t >( ii + fNumPerThread ) );
            addPartition( std::make_pair( ii, lclMax ) );
            if ( callInLoop && reportFunction && ( ( numPartitions % 100 ) == 0 ) )
            {
                if ( !reportFunction( min, max, numPartitions ) )
                    break;
            }
        }
    }
    else
    {
        if ( std::get< 2 >( fNumbers ).size() <= fNumPerThread )
        {
            addPartition( std::get< 2 >( fNumbers ) );
            numPartitions++;
        }
        else
        {
            auto && tmp = std::get< 2 >( fNumbers );
            min = 0;
            max = tmp.size(); 
            while ( !tmp.empty() )
            {
                auto start = tmp.begin();
                auto end = tmp.end();
                if ( tmp.size() > fNumPerThread )
                {
                    end = tmp.begin();
                    std::advance( end, fNumPerThread );
                }
                //auto curr = std::list< int64_t >( start, end );
                addPartition( std::list< uint64_t >( start, end ) );
                tmp.erase( start, end );
                numPartitions++;
                if ( callInLoop && reportFunction && ( ( numPartitions % 100 ) == 0 ) )
                {
                    if ( !reportFunction( 0, max, numPartitions ) )
                        break;
                }
            }
        }
    }
    if ( reportFunction )
    {
        reportFunction( min, max, max );
    }
    fFinishedPartition = true;
    fConditionVariable.notify_all();
    return numPartitions;
}

void CNarcissisticNumCalculator::reportNumPartitionsRemaining( std::chrono::system_clock::time_point& prev, bool force )
{
    auto now = std::chrono::system_clock::now();
    auto duration = now - prev;
    if ( force || ( std::chrono::duration_cast<std::chrono::seconds>( duration ).count() > fReportSeconds ) )
    {
        {
            std::unique_lock< std::mutex > lock( fMutex );
            //std::cout << "Locked: reportNumRangesRemaining\n";
            std::cout << "Number of Ranges Remaining: " << fPartitions.size() << "\n";
            std::cout << "Number of Threads Remaining: " << fHandles.size() << "\n";
            //std::cout << "UnLocked: reportNumRangesRemaining\n";
        }
        prev = now;
    }
}

bool CNarcissisticNumCalculator::isFinished( std::chrono::system_clock::time_point * prev )
{
    for ( auto ii = fHandles.begin(); ii != fHandles.end(); )
    {
        if ( prev )
            reportNumPartitionsRemaining( *prev );

        if ( ( *ii ).wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready ) // finished
            ii = fHandles.erase( ii );
        else
            break;
    }
    return fHandles.empty();
}

std::pair< std::chrono::system_clock::duration, std::chrono::system_clock::duration > CNarcissisticNumCalculator::computeETA() const
{
    std::chrono::system_clock::duration totalTime( 0 );
    for ( auto&& ii : fPartitionTimes )
        totalTime += ii;
    auto averageTime = fPartitionTimes.size() ? ( totalTime / fPartitionTimes.size() ) : std::chrono::system_clock::duration( 0 );
    auto eta = averageTime * numPartitions();
    return std::make_pair( averageTime, eta );
}

std::pair< std::string, bool > CNarcissisticNumCalculator::currentResults()
{
    bool finished = isFinished( nullptr );

    std::ostringstream oss;
    oss << "Run Time: " << NUtils::getTimeString( std::chrono::system_clock::now() - startTime(), true, true ) << "\n";
    if ( !finished )
    {
        oss << getRunningResults();
    }

    auto numbers = results();
    oss
        << "Number of Narcissistic Numbers Found: " << numbers.size() << "\n"
        << NUtils::getNumberListString( numbers, fBase );

    return std::make_pair( oss.str(), finished );
}

std::string CNarcissisticNumCalculator::getRunningResults() const
{
    std::chrono::system_clock::duration avg;
    std::chrono::system_clock::duration eta;
    std::tie( avg, eta ) = computeETA();

    std::ostringstream oss;
    oss
        << "Number of Partitions Remaining: " << numPartitions() << "\n"
        << "Number of Threads Remaining: " << numThreads() << "\n"
        << "Average Time/Partition: " << NUtils::getTimeString( avg, false, true ) << "\n"
        << "ETA: " << NUtils::getTimeString( eta, false, false ) << "\n"
        ;
    return oss.str();
}

void CNarcissisticNumCalculator::addNarcissisticValue( int64_t value )
{
    std::lock_guard< std::mutex > lock( fMutex );
    fNarcissisticNumbers.push_back( value );
}

void CNarcissisticNumCalculator::addPartition( const std::pair< uint64_t, uint64_t >& range )
{
    auto&& tmp = std::make_tuple( true, std::list< uint64_t >(), range );
    std::unique_lock< std::mutex > lock( fMutex );
    fPartitions.push_back( tmp );
}

void CNarcissisticNumCalculator::addPartition( const std::list< uint64_t >& list )
{
    auto&& tmp = std::make_tuple( false, list, std::make_pair< uint64_t, uint64_t >( 0, 0 ) );
    std::unique_lock< std::mutex > lock( fMutex );
    fPartitions.push_back( tmp );
}
