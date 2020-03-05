#include "NarcissisticNumCalculator.h"
#include "utils.h"

#include <QSettings>

#include <iostream>
#include <cctype>
#include <string>
#include <cstring>

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

    std::list< int64_t > toIntList( const QList< QVariant > & inList )
    {
        std::list< int64_t > retVal;
        for( auto && ii : inList )
            retVal.push_back( ii.toLongLong() );
        return retVal;
    }
    std::pair< int64_t, int64_t > range()
    {
        QSettings settings;
        auto retVal = toIntList( settings.value( "Range", QVariant::fromValue( QList< QVariant >() << 0 << kDefaultMaxNum ) ).value< QList< QVariant > >() );
        return std::make_pair( retVal.front(), retVal.back() );
    }

    void setRange( const std::pair< int64_t, int64_t >& value )
    {
        QSettings settings;
        auto values = QList< QVariant >() << value.first << value.second;
        return settings.setValue( "Range", QVariant::fromValue( values ) );
    }

    std::list< int64_t > numbersList()
    {
        QSettings settings;
        auto retVal = toIntList( settings.value( "NumbersList", QVariant::fromValue( QList< QVariant >() ) ).value< QList< QVariant > >() );
        return retVal;
    }

    void setNumbersList( const std::list< int64_t >& value )
    {
        QSettings settings;
        QList< QVariant > values;
        for( auto && ii : value )
            values << ii;
        return settings.setValue( "NumbersList", QVariant::fromValue( values ) );
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

void CNarcissisticNumCalculator::launch( bool report )
{
    for ( unsigned int ii = 0; ii < fNumThreads; ++ii )
    {
        fHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::analyzeNextRange, this ) );
    }
    std::unique_lock< std::mutex > lock( fMutex );
    if ( report )
    {
        std::cout << "=============================================\n";
        std::cout << "Number of Threads Created: " << fHandles.size() << "\n";
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
    launch();
    std::cout << "=============================================\n";
    partition();

    auto prev = std::chrono::system_clock::now();
    reportNumRangesRemaining( prev, true );
    while ( !isFinished( &prev ) )
    {
    }
    reportNumRangesRemaining( prev, true );

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

void CNarcissisticNumCalculator::dumpNumbers( const std::list< int64_t >& numbers ) const
{
    bool first = true;
    size_t ii = 0;
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

void CNarcissisticNumCalculator::analyzeNextRange()
{
    while ( true ) // infinite loop 
    {
        TRangeSet currRange;
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
    }
}

void CNarcissisticNumCalculator::findNarcissistic( const TRangeSet& range )
{
    if ( std::get< 0 >( range ) )
        findNarcissisticRange( std::get< 2 >( range ) );
    else
        findNarcissisticList( std::get< 1 >( range ) );
}

void CNarcissisticNumCalculator::findNarcissisticRange( int64_t min, int64_t max )
{
    return findNarcissisticRange( std::make_pair( min, max ) );
}

void CNarcissisticNumCalculator::findNarcissisticRange( const std::pair< int64_t, int64_t >& range )
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
    }
    {
        //std::unique_lock< std::mutex > lock(fMutex);
        //std::cout << "Locked: FindNarcissisticRange - Footer\n";
        //std::cout << "\n" << std::this_thread::get_id() << ": ----> Computing for (" << range.first << "," << range.second - 1 << ")" << " = " << numArm << std::endl;
        //std::cout << "UnLocked: FindNarcissisticRange - Footer\n";
    }
}

void CNarcissisticNumCalculator::findNarcissisticList( const std::list< int64_t >& values )
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

void CNarcissisticNumCalculator::partition( bool report )
{
    if ( std::get< 0 >( fNumbers ) )
    {
        int num = 0;
        for ( auto ii = std::get< 1 >( fNumbers ).first; ii < std::get< 1 >( fNumbers ).second; ii += fNumPerThread )
        {
            auto max = std::min( std::get< 1 >( fNumbers ).second, ii + fNumPerThread );
            addRange( std::make_pair( ii, max ) );
            //fHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::findNarcissisticRange, this, num++, ii, max ) );
        }
    }
    else
    {
        if ( std::get< 2 >( fNumbers ).size() <= fNumPerThread )
        {
            addRange( std::get< 2 >( fNumbers ) );
            //   fHandles.push_back(std::async(std::launch::async, &CNarcissisticNumCalculator::findNarcissisticList, this, 0, std::get< 2 >( fNumbers ) ) );
        }
        else
        {
            auto tmp = std::get< 2 >( fNumbers );
            int num = 0;
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
                addRange( std::list< int64_t >( start, end ) );
                // fHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::findNarcissisticList, this, num++, curr ) );
                tmp.erase( start, end );
            }
        }
    }
    {
        std::unique_lock< std::mutex > lock( fMutex );
        if ( report )
        {
            std::cout << "Number of Ranges Created: " << fPartitions.size() << "\n";
            std::cout << "=============================================\n";
        }
    }
    fFinishedPartition = true;
    fConditionVariable.notify_all();
}

void CNarcissisticNumCalculator::reportNumRangesRemaining( std::chrono::system_clock::time_point& prev, bool force )
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
            reportNumRangesRemaining( *prev );

        if ( ( *ii ).wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready ) // finished
            ii = fHandles.erase( ii );
        else
            break;
    }
    return fHandles.empty();
}

void CNarcissisticNumCalculator::addNarcissisticValue( int64_t value )
{
    std::lock_guard< std::mutex > lock( fMutex );
    fNarcissisticNumbers.push_back( value );
}

void CNarcissisticNumCalculator::addRange( const std::pair< int64_t, int64_t >& range )
{
    auto&& tmp = std::make_tuple( true, std::list< int64_t >(), range );
    std::unique_lock< std::mutex > lock( fMutex );
    fPartitions.push_back( tmp );
}

void CNarcissisticNumCalculator::addRange( const std::list< int64_t >& list )
{
    auto&& tmp = std::make_tuple( false, list, std::make_pair< int64_t, int64_t >( 0, 0 ) );
    std::unique_lock< std::mutex > lock( fMutex );
    fPartitions.push_back( tmp );
}

