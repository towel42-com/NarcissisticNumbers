#include "NarcissisticNumCalculator.h"
#include "utils.h"

#include <iostream>
#include <cctype>
#include <string>


bool CNarcissisticNumCalculator::parse( int argc, char ** argv )
{
    for ( int ii = 1; ii < argc; ++ii )
    {
        bool aOK = false;
        if ( strncmp( argv[ ii ], "-base", 5 ) == 0 )
        {
            fBase = getInt( ii, argc, argv, "-base", aOK );
            if ( aOK && ((fBase < 2) || (fBase > 36)) )
            {
                std::cerr << "Base must be between 2 and 36" << std::endl;
                aOK = false;
            }
        }
        else if ( strncmp( argv[ ii ], "-min", 4 ) == 0 )
        {
            fRange.first = getInt( ii, argc, argv, "-min", aOK );
        }
        else if ( strncmp( argv[ ii ], "-max", 4 ) == 0 )
        {
            fRange.second = getInt( ii, argc, argv, "-max", aOK );
        }
        else if ( strncmp( argv[ ii ], "-thread_max", 11 ) == 0 )
        {
            fThreadMax = getInt( ii, argc, argv, "-max", aOK );
        }
        else if ( strncmp( argv[ ii ], "-report_seconds", 15 ) == 0 )
        {
            fReportSeconds = getInt( ii, argc, argv, "-report_seconds", aOK );
        }
        else if ( strncmp( argv[ ii ], "-numbers", 8 ) == 0 )
        {
            aOK = (ii + 1) < argc;
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
                    fNumbers.push_back( curr );
                if ( (ii + 1) >= argc )
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

std::chrono::system_clock::duration CNarcissisticNumCalculator::run( const std::function< int64_t(int64_t, int64_t) > & pwrFunction)
{
    fNarcissisticNumbers.clear();
    report();
    fPowerFunction = pwrFunction;
    fRunTime.first = std::chrono::system_clock::now();
    partition();
    auto prev = std::chrono::system_clock::now();
    std::cout << "=============================================\n";
    reportNumThreadsRemaining( prev, true );
    while ( !isFinished( prev ) )
    {
        reportNumThreadsRemaining( prev );
    }

    reportNumThreadsRemaining( prev, true );

    fRunTime.second = std::chrono::system_clock::now();
    reportFindings();
    return fRunTime.second - fRunTime.first;
}

int CNarcissisticNumCalculator::getInt( int & ii, int argc, char ** argv, const char * switchName, bool & aOK )
{
    aOK = false;

    if ( ++ii == argc )
    {
        std::cerr << "-base requires a value";
        return 0;
    }
    const char * str = argv[ ii ];
    int retVal = 0;
    try
    {
        retVal = std::stoi( str );
        aOK = true;
    }
    catch ( std::invalid_argument const & e )
    {
        std::cerr << switchName << " value '" << str << "' is invalid. \n" << e.what() << "\n";
    }
    catch ( std::out_of_range const & e )
    {
        std::cerr << switchName << " value '" << str << "' is out of range. \n" << e.what() << "\n";
    }

    return retVal;
}

void CNarcissisticNumCalculator::dumpNumbers( const std::list< int64_t > & numbers ) const
{
    bool first = true;
    size_t ii = 0;
    for ( auto && currVal : numbers )
    {
        if ( ii && (ii % 5 == 0) )
        {
            std::cout << "\n";
            first = true;
        }
        if ( !first )
            std::cout << ", ";
        else
            std::cout << "    ";
        first = false;
        std::cout << toString( currVal, fBase );
        if ( fBase != 10 )
            std::cout << "(=" << currVal << ")";
        ii++;
    }
    std::cout << "\n";
}

void CNarcissisticNumCalculator::report() const
{
    if ( fNumbers.empty() )
    {
        std::cout << "Finding Narcissistic in the range: [" << fRange.first << ":" << fRange.second << "]." << std::endl;
    }
    else
    {
        std::cout << "Checking if the following numbers are Narcissistic:\n";
        dumpNumbers( fNumbers );
    }
    std::cout << "Maximum Numbers per thread: " << fThreadMax << "\n";
    std::cout << "Base : " << fBase << "\n";
}

void CNarcissisticNumCalculator::reportFindings() const
{
    std::cout << "=============================================\n";
    std::cout << "There are " << fNarcissisticNumbers.size() << " Narcissistic numbers";
    if ( fNumbers.empty() )
        std::cout << " in the range [" << fRange.first << ":" << fRange.second << "]." << std::endl;
    else
        std::cout << " in the requested list." << std::endl;
    fNarcissisticNumbers.sort();
    dumpNumbers( fNarcissisticNumbers );
    std::cout << "=============================================\n";
    std::cout << "Runtime: " << getTimeString( fRunTime, true, true ) << std::endl;
}

std::pair< bool, bool > CNarcissisticNumCalculator::checkAndAddValue( int64_t value )
{
    bool aOK = true;
    bool isNarcissistic = this->isNarcissistic( value, fBase, aOK );
    if ( !aOK )
        return std::make_pair( false, false );
    if ( isNarcissistic )
    {
        //std::cout << curr << " is Narcissistic? " << (isNarcissistic ? "yes" : "no") << std::endl;
        addNarcissisticValue( value );
    }
    return std::make_pair( isNarcissistic, true );
}

void CNarcissisticNumCalculator::findNarcissisticRange( int num, int64_t min, int64_t max )
{
    //std::cout << "\n" << num << ": Computing for (" << min << "," << max-1 << ")" << std::endl;
    int numArm = 0;
    for ( auto ii = min; ii < max; ++ii )
    {
        bool isNarcissistic = false;
        bool aOK = false;
        std::tie( isNarcissistic, aOK ) = checkAndAddValue( ii );
        if ( !aOK )
            return;
        if ( isNarcissistic )
            numArm++;
    }
    //std::cout << "\n" << num << ": ----> Computing for (" << min << "," << max-1 << ")" << " = " << numArm << std::endl;
}

void CNarcissisticNumCalculator::findNarcissisticList( int num, const std::list< int64_t > & values )
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

void CNarcissisticNumCalculator::partition()
{
    //std::cout << "Creating Handles for for (" << min << "," << max - 1 << ")" << std::endl;
    if ( fNumbers.empty() )
    {
        int num = 0;
        for ( auto ii = fRange.first; ii < fRange.second; ii += fThreadMax )
        {
            auto max = std::min( fRange.second, ii + fThreadMax );
            fHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::findNarcissisticRange, this, num++, ii, max ) );
        }
    }
    else
    {
        if ( fNumbers.size() <= fThreadMax )
            fHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::findNarcissisticList, this, 0, fNumbers ) );
        else
        {
            auto tmp = fNumbers;
            int num = 0;
            while ( !tmp.empty() )
            {
                auto start = tmp.begin();
                auto end = tmp.end();
                if ( tmp.size() > fThreadMax )
                {
                    end = tmp.begin();
                    std::advance( end, fThreadMax );
                }
                auto curr = std::list< int64_t >( start, end );
                fHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::findNarcissisticList, this, num++, curr ) );
                tmp.erase( start, end );
            }
        }
    }
    std::cout << "=============================================\n";
    std::cout << "Number of Threads Created: " << fHandles.size() << "\n";
}

void CNarcissisticNumCalculator::reportNumThreadsRemaining( std::chrono::system_clock::time_point & prev, bool force )
{
    auto now = std::chrono::system_clock::now();
    auto duration = now - prev;
    if ( force || ( std::chrono::duration_cast<std::chrono::seconds>(duration).count() > fReportSeconds) )
    {
        std::cout << "Number of Threads Remaining: " << fHandles.size() << "\n";
        prev = now;
    }
}

bool CNarcissisticNumCalculator::isFinished( std::chrono::system_clock::time_point & prev )
{
    for ( auto ii = fHandles.begin(); ii != fHandles.end(); )
    {
        reportNumThreadsRemaining( prev );

        if ( (*ii).wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready ) // finished
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

bool CNarcissisticNumCalculator::isNarcissistic( int64_t val, int base, bool & aOK )
{
    auto str = toString( val, base );

    int64_t sumOfPowers = 0;
    int64_t value = 0;
    for ( size_t ii = 0; ii < str.length(); ++ii )
    {
        auto currChar = str[ ii ];

        int64_t currVal = fromChar( currChar, base, aOK );
        if ( !aOK )
        {
            std::cerr << "Invalid character: " << currChar << std::endl;
            return false;
        }
        if (fPowerFunction)
            sumOfPowers += fPowerFunction(currVal, str.length());
        else
            sumOfPowers += power(currVal, str.length());

        value = (value * base) + currVal;
    }

    return value == sumOfPowers;
}

