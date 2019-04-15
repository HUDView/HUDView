/** @file main.c
 *  @brief HUDView light sensor control application.
 *
 *  This program initializes and periodically reads the Adafruit TSL2561 light sensor lux value and prints it to stdout
 *  for downstream consumption by the control application.
 *
 *  @author Ben Prisby (BenPrisby)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tsl2561.h"
/*--------------------------------------------------------------------------------------------------------------------*/

#define WAIT_TIME_SECONDS ( 10 )
/*--------------------------------------------------------------------------------------------------------------------*/

static void *pvSensor = NULL;
/*--------------------------------------------------------------------------------------------------------------------*/

static void vSignalHandler( int iSignal );
/*--------------------------------------------------------------------------------------------------------------------*/

int main( int argc, char ** argv )
{
    const int iSensorAddress = 0x39;
    const char * const pcBus = "/dev/i2c-1";
    long lLuxReading = 0;
    int iReturn = -1;

    /* Install the Ctrl-C handler. */
    signal( SIGINT, vSignalHandler );

    /* Disable buffering on standard output. */
    setbuf( stdout, NULL );

    /* Attempt initialize the sensor. */
    pvSensor = tsl2561_init( iSensorAddress, pcBus );

    if ( NULL != pvSensor )
    {
        /* Configure initial settings. */
        tsl2561_enable_autogain( pvSensor );
        tsl2561_set_integration_time( pvSensor, TSL2561_INTEGRATION_TIME_13MS );

     /* Periodically collect the lux reading and append it to the output file. */
        for ( ;; )
        {
            lLuxReading = tsl2561_lux( pvSensor );
            printf( "%lu\n", lLuxReading );
            sleep( WAIT_TIME_SECONDS );
        }

        /* Should never get here. */
        tsl2561_close( pvSensor );
        iReturn = -1;
    }
    else
    {
        /* Failed to initialize sensor. */
        iReturn = -1;
    }

    return iReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

static void vSignalHandler( int iSignal )
{
    /* Check for a signal to quit. */
    if ( SIGINT == iSignal )
    {
        tsl2561_close( pvSensor );
        exit( 0 );
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

