#include <iostream>
#include <typeinfo>
#include <cstdint>
#include <string>
#include <future>
#include <mutex>
#include <list>
#include <cctype>

std::list< std::future< void > > sHandles;
std::mutex sMutex;
std::list< int64_t > sArmstrongNumbers;

int fromChar( char ch, int base, bool & aOK )
{
    if ( ch == '-' )
    {
        aOK = true;
        return 1;
    }
    aOK = false;
    if ( (ch >= '0') && ch <= ('0' + (base - 1)) )
    {
        aOK = true;
        return (ch - '0');
    }

    if ( base <= 10 )
        return 0;

    ch = std::tolower( ch );
    auto maxChar = 'a' + base;

    if ( (ch >= 'a') && (ch <= maxChar) )
    {
        aOK = true;
        return 10 + ch - 'a';
    }
    return 0;
}


char toChar( int value )
{
    if ( (value >= 0) && (value < 10) )
        return '0' + value;
    // over 10, must use chars

    return 'a' + value - 10;
}

std::string toString( int64_t val, int base )
{
    std::string retVal;
    do
    {
        int64_t quotient = val / base;
        int remainder = val % base;
        retVal.insert( retVal.begin(), toChar( remainder ) );
        val = quotient;
    } while( val != 0 );
    return retVal;
}

int64_t fromString( const std::string & str, int base )
{
    int64_t retVal = 0;
    bool isNeg = false;
    bool aOK = false;
    for ( size_t ii = 0; ii < str.length(); ++ii )
    {
        auto currChar = str[ ii ];
        if ( (ii == 0) && (currChar == '-') )
        {
            isNeg = true;
            continue;
        }

        int64_t currVal = (isNeg ? -1 : 1) * fromChar( currChar, base, aOK );
        if ( !aOK )
        {
            std::cerr << "Invalid character: " << currChar << std::endl;
            return 0;
        }
        retVal = (retVal * base) + currVal;
    }
    return retVal;
}

bool isArmstrong( int64_t val, int base, bool & aOK )
{
    auto str = toString( val, base );

    int64_t sumOfCubes = 0;
    int64_t value = 0;
    bool isNeg = false;
    for ( size_t ii = 0; ii < str.length(); ++ii )
    {
        auto currChar = str[ ii ];
        if ( (ii == 0) && (currChar == '-') )
        {
            isNeg = true;
            continue;
        }

        int64_t currVal = (isNeg ? -1 : 1) * fromChar(currChar, base, aOK );
        if ( !aOK )
        {
            std::cerr << "Invalid character: " << currChar << std::endl;
            return false;
        }
        sumOfCubes += static_cast<uint64_t>(std::pow( currVal, 3 ));

        value = (value * base) + currVal;
    }

    return value == sumOfCubes;
}

void addArmstrongValue( int64_t value )
{
    std::lock_guard< std::mutex > lock( sMutex );
    sArmstrongNumbers.push_back( value );
}

void computeCurrentBlock( int num, int base, int64_t min, int64_t max )
{
    //std::cout << "\n" << num << ": Computing for (" << min << "," << max-1 << ")" << std::endl;
    int numArm = 0;
    for ( auto ii = min; ii < max; ++ii )
    {
        bool aOK = true;
        bool isArmstrong = ::isArmstrong( ii, base, aOK );
        if ( !aOK )
            return;
        if ( isArmstrong )
        {
            //std::cout << curr << " is Armstrong? " << (isArmstrong ? "yes" : "no") << std::endl;
            addArmstrongValue( ii );
            numArm++;
        }
    }
    //std::cout << "\n" << num << ": ----> Computing for (" << min << "," << max-1 << ")" << " = " << numArm << std::endl;
}

void createHandles( int64_t min, int64_t max, int base, int computeMax )
{
    //std::cout << "Creating Handles for for (" << min << "," << max - 1 << ")" << std::endl;
    int num = 0;
    for ( auto ii = min; ii < max; ii += computeMax )
    {
        sHandles.push_back( std::async( std::launch::async, computeCurrentBlock, num++, base, ii, ii + computeMax ) );
    }
}

int getInt( int & ii, int argc, char ** argv, const char * switchName, bool & aOK )
{
    aOK = false;

    if ( ii++ == argc )
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

bool parseCommandLine( int argc, char ** argv, int & base, int & max, int & computeMax )
{
    for ( int ii = 1; ii < argc; ++ii )
    {
        bool aOK = false;
        if ( strncmp( argv[ ii ], "-base", 5 ) == 0 )
        {
            base = getInt( ii, argc, argv, "-base", aOK );
            if ( aOK && ((base < 2) || (base > 36)) )
            {
                std::cerr << "Base must be between 2 and 36" << std::endl;
                aOK = false;
            }
        }
        else if ( strncmp( argv[ ii ], "-max", 4 ) == 0 )
        {
            max = getInt( ii, argc, argv, "-max", aOK );
        }
        else if ( strncmp( argv[ ii ], "-compute_max", 12 ) == 0 )
        {
            computeMax = getInt( ii, argc, argv, "-max", aOK );
        }
        if ( !aOK )
            return false;
    }
    return true;
}

bool isFinished()
{
    while ( !sHandles.empty() )
    {
        for ( auto ii = sHandles.begin(); ii != sHandles.end(); )
        {
            if ( (*ii).wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready ) // finished
                ii = sHandles.erase( ii );
            else
                break; // still running
        }
    }
    return sHandles.empty();
}

int main( int argc, char ** argv )
{
    int base = 10;
    int max = 100000;
    int computeMax = 100;
    if ( !parseCommandLine( argc, argv, base, max, computeMax ) )
        return 1;

    std::cout << "Finding Narcissistic numbers up to: " << max << "\n";
    std::cout << "Maximum Numbers per thread: " << computeMax << "\n";
    std::cout << "Base : " << base << "\n";
    createHandles( 0, max, base, computeMax );
    while ( !isFinished() )
        ;

    std::cout << "There are " << sArmstrongNumbers.size() << " armstrong numbers less than or equal to " << max << std::endl;
    sArmstrongNumbers.sort();
    for ( auto && ii : sArmstrongNumbers )
    {
        std::cout << ii << "\n";
    }
}

