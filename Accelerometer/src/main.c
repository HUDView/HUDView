/** @file main.c
 *  @brief HUDView acclerometer control application.
 *
 *  This program initializes and periodically reads the Adafruit MMA8451 accelerometer values and prints them to stdout
 *  for downstream consumption by the control application.
 *
 *  @author Ben Prisby (BenPrisby)
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "mma8451_pi.h"
/*--------------------------------------------------------------------------------------------------------------------*/

#define WAIT_TIME_MILLISECONDS ( 500 )
/*--------------------------------------------------------------------------------------------------------------------*/

static void vSignalHandler( int iSignal );
/*--------------------------------------------------------------------------------------------------------------------*/

int main()
{
    const int iSensorAddress = 0x1D;
    const int iBus = 1;
    mma8451 xSensor;
    mma8451_vector3 xReading;
    int iReturn = -1;

    /* Install the Ctrl-C handler. */
    signal( SIGINT, vSignalHandler );

    /* Disable buffering on standard output. */
    setbuf( stdout, NULL );

    /* Initialize the sensor. */
    xSensor = mma8451_initialise( iBus, iSensorAddress );

    /* Configure initial settings. */
    mma8451_set_range( &xSensor, 4 );

    /* Get an intitial measurement. */
    ( void )mma8451_get_acceleration_vector( &xSensor );

    for ( ;; )
    {
        mma8451_get_acceleration( &xSensor, &xReading );
        printf( "%f,%f,%f\n", xReading.x, xReading.y, xReading.z );
        usleep( WAIT_TIME_MILLISECONDS *  1000 );
    }

    /* Should never get here. */
    return iReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

static void vSignalHandler( int iSignal )
{
    /* Check for a signal to quit. */
    if ( SIGINT == iSignal )
    {
        exit( 0 );
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

