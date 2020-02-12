#include "NarcissisticNumCalculator.h"

int main( int argc, char ** argv )
{
    CNarcissisticNumCalculator values;
    if ( !values.parse( argc, argv ) )
        return 1;

    values.run();
    return 0;
}

