/** @file main.c
 *  @brief HUDView light sensor control application.
 *
 *  This program initializes and periodically reads the Adafruit TSL2561 light sensor lux value and appends it to the
 *  the specified output pipe for downstream consumption by the control application.
 *
 *  @author Ben Prisby (BenPrisby)
 */

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "tsl2561.h"
/*--------------------------------------------------------------------------------------------------------------------*/

#define WAIT_TIME_SECONDS ( 30 )
/*--------------------------------------------------------------------------------------------------------------------*/

static void *pvSensor = NULL;
static const char *pcPipe = "/tmp/hudview_light_sensor_output";
static FILE *pOutput = NULL;
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

    /* Attempt to establish a named pipe for output. */
    if ( 0 <= mkfifo( pcPipe, 0666 ) )
    {
        /* Attempt to open the pipe for writing. */
        pOutput = fopen( pcPipe, "r+" );

        if ( NULL != pOutput )
        {
            /* Disable buffering on the output file. */
            setbuf( pOutput, NULL );

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
                    fprintf( pOutput, "%lu\n", lLuxReading );
                    sleep( WAIT_TIME_SECONDS );
                }

                /* Should never get here. */
                fclose( pOutput );
                unlink( pcPipe );
                tsl2561_close( pvSensor );
                iReturn = -1;
            }
            else
            {
                /* Failed to initialize sensor. */
                fclose( pOutput );
                unlink( pcPipe );
                iReturn = -1;
            }
        }
        else
        {
            /* Failed to open the pipe for writing. */
            unlink( pcPipe );
            iReturn = -1;
        }
    }
    else
    {
        /* Failed to create the pipe. */
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
        fclose( pOutput );
        unlink( pcPipe );
        tsl2561_close( pvSensor );
        exit( 0 );
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

