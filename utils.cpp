#include "utils.h"
#include <iostream>
#include <sstream>

namespace NUtils
{

    int fromChar( char ch, int base, bool& aOK )
    {
        if ( ch == '-' )
        {
            aOK = true;
            return 1;
        }
        aOK = false;
        if ( ( ch >= '0' ) && ch <= ( '0' + ( base - 1 ) ) )
        {
            aOK = true;
            return ( ch - '0' );
        }

        if ( base <= 10 )
            return 0;

        ch = std::tolower( ch );
        auto maxChar = 'a' + base;

        if ( ( ch >= 'a' ) && ( ch <= maxChar ) )
        {
            aOK = true;
            return 10 + ch - 'a';
        }
        return 0;
    }

    char toChar( int value )
    {
        if ( ( value >= 0 ) && ( value < 10 ) )
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
        } while ( val != 0 );
        return retVal;
    }

    int64_t fromString( const std::string& str, int base )
    {
        int64_t retVal = 0;
        bool aOK = false;
        for ( size_t ii = 0; ii < str.length(); ++ii )
        {
            auto currChar = str[ ii ];
            int64_t currVal = fromChar( currChar, base, aOK );
            if ( !aOK )
            {
                std::cerr << "Invalid character: " << currChar << std::endl;
                return 0;
            }
            retVal = ( retVal * base ) + currVal;
        }
        return retVal;
    }

    std::string getTimeString( const std::pair< std::chrono::system_clock::time_point, std::chrono::system_clock::time_point >& startEndTime, bool reportTotalSeconds, bool highPrecision )
    {
        auto duration = startEndTime.second - startEndTime.first;
        return getTimeString( duration, reportTotalSeconds, highPrecision );
    }

    double getSeconds( const std::chrono::system_clock::duration& duration, bool highPrecision )
    {
        double totalSeconds = 1.0 * std::chrono::duration_cast<std::chrono::seconds>( duration ).count();
        if ( highPrecision )
            totalSeconds = std::chrono::duration_cast<std::chrono::duration< double, std::micro >>( duration ).count() / 1000000.0;
        return totalSeconds;
    }

    std::string getTimeString( const std::chrono::system_clock::duration& duration, bool reportTotalSeconds, bool highPrecision )
    {
        auto totalSeconds = getSeconds( duration, highPrecision );
        auto hrs = std::chrono::duration_cast<std::chrono::hours>( duration ).count();
        auto mins = std::chrono::duration_cast<std::chrono::minutes>( duration ).count() - ( hrs * 60 );
        double secs = 1.0 * std::chrono::duration_cast<std::chrono::seconds>( duration ).count();
        if ( highPrecision )
            secs = ( std::chrono::duration_cast<std::chrono::duration< double, std::micro >>( duration ).count() ) / 1000000.0;
        secs -= ( ( mins * 60 ) + ( hrs * 3600 ) );

        std::ostringstream oss;
        if ( hrs > 0 )
        {
            oss << hrs << " hour";
            if ( hrs != 1 )
                oss << "s";
            oss << ", ";
        }

        if ( mins > 0 )
        {
            oss << mins << " minute";
            if ( mins != 1 )
                oss << "s";
            oss << ", ";
        }

        if ( highPrecision )
        {
            oss.setf( std::ios::fixed, std::ios::floatfield );
            oss.precision( 6 );
        }

        oss << secs << " second";

        if ( secs != 1 )
            oss << "s";
        if ( reportTotalSeconds && ( totalSeconds > 60 ) )
        {
            oss << ", (" << totalSeconds << " second";
            if ( totalSeconds != 1 )
                oss << "s";
            oss << ")";
        }
        return oss.str();
    }
}