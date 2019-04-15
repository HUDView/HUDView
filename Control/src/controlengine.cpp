#include <signal.h>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QTime>

#include <ssd1306.h>

#include "controlengine.h"
#include "ubuntumono.h"
/*--------------------------------------------------------------------------------------------------------------------*/

static void vSignalHandler( int iSignal );
/*--------------------------------------------------------------------------------------------------------------------*/

ControlEngine::ControlEngine( QObject * pParent ) : QObject( pParent )
{
    m_sConfigPath = "";
    m_eDisplayMode = eControlDisplayMode_Time;

    /* Set up the refresh timer for the display. */
    m_DisplayRefreshTimer.setInterval( 500 );
    m_DisplayRefreshTimer.setSingleShot( false );
    connect( &m_DisplayRefreshTimer, SIGNAL( timeout() ), this, SLOT( vUpdateDisplay() ) );

    m_ModeSwitchTimer.setInterval( 5000 );
    m_ModeSwitchTimer.setSingleShot( false );
    connect( &m_ModeSwitchTimer, SIGNAL( timeout() ), this, SLOT( vChangeMode() ) );

    /* Install the Ctrl-C handler. */
    signal( SIGINT, vSignalHandler );
}
/*--------------------------------------------------------------------------------------------------------------------*/

ControlEngine::~ControlEngine()
{
    /* Kill any running processes. */
    for ( const xHUDViewComponent_t & xComponent : m_lstRegisteredComponents )
    {
        qDebug() << "Terminating process for component: " << sEnumValueToComponentName( xComponent.eID );
        xComponent.pProcess->kill();
        xComponent.pProcess->waitForFinished();
        delete xComponent.pProcess;
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

int ControlEngine::iRun( QCoreApplication * pApp )
{
    QString sConfigFile = m_sConfigPath.isEmpty() ? DEFAULT_CONFIG_FILE_PATH : m_sConfigPath;
    int iReturn = 0;

    /* Populate the list of registered components. */
    if ( bParseConfig( sConfigFile ) )
    {
        qDebug() << "Loaded config file: " << sConfigFile;

        /* Start the registered component processes. */
        for ( const xHUDViewComponent_t & xComponent : m_lstRegisteredComponents )
        {
            xComponent.pProcess->start();

            if ( xComponent.pProcess->waitForStarted( PROCESS_START_WAIT_TIMEOUT_MS ) )
            {
                qDebug() << "Started process for component: " << sEnumValueToComponentName( xComponent.eID );
            }
            else
            {
                qDebug() << "Failed to start process for component: " << sEnumValueToComponentName( xComponent.eID );
                iReturn = -1;
            }
        }

        /* Initialize the display. */
        vDisplayInit();

        /* Execute the application loop. */
        if ( 0 == iReturn )
        {
            iReturn = pApp->exec();
        }
    }
    else
    {
        qDebug() << "Failed to parse config file: " << sConfigFile;
        iReturn = -1;
    }

    return iReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void ControlEngine::vSetConfigFile( const QString & sPath )
{
    if ( !sPath.isEmpty() )
    {
        m_sConfigPath = sPath;
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

bool ControlEngine::bIsValidComponent( const xHUDViewComponent_t & xComponent )
{
    return ( eHUDViewComponentID_Unknown == xComponent.eID ) || ( nullptr == xComponent.pProcess );
}
/*--------------------------------------------------------------------------------------------------------------------*/

QString ControlEngine::sEnumValueToComponentName( const eHUDViewComponentID_t & eValue )
{
    QString sReturn = "Unknown";

    switch ( eValue )
    {
    case eHUDViewComponentID_Accelerometer:
        sReturn = "Accelerometer";
        break;

    case eHUDViewComponentID_Camera:
        sReturn = "Camera";
        break;

    case eHUDViewComponentID_CameraDisplay:
        sReturn = "CameraDisplay";
        break;

    case eHUDViewComponentID_Control:
        sReturn = "Control";
        break;

    case eHUDViewComponentID_ControlDisplay:
        sReturn = "ControlDisplay";
        break;

    case eHUDViewComponentID_GPS:
        sReturn = "GPS";
        break;

    case eHUDViewComponentID_HandlebarButtons:
        sReturn = "HandlebarButtons";
        break;

    case eHUDViewComponentID_LightSensor:
        sReturn = "LightSensor";
        break;

    default:
        /* Unrecognized component type. */
        break;
    }

    return sReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

ControlEngine::eHUDViewComponentID_t ControlEngine::eComponentNameToEnumValue( const QString & sName )
{
    eHUDViewComponentID_t eReturn = eHUDViewComponentID_Unknown;

    for ( int iComponent = eHUDViewComponentIDMin; eHUDViewComponentIDMax > iComponent; iComponent++ )
    {
        if ( 0 == QString::compare( sEnumValueToComponentName( static_cast<eHUDViewComponentID_t>( iComponent ) ), sName ) )
        {
            eReturn = static_cast<eHUDViewComponentID_t>( iComponent );
            break;
        }
    }

    return eReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

ControlEngine::eHUDViewComponentID_t ControlEngine::eComponentProgramToEnumValue( const QString & sProgram )
{
    eHUDViewComponentID_t eReturn = eHUDViewComponentID_Unknown;

    for ( xHUDViewComponent_t xComponent : m_lstRegisteredComponents )
    {
        if ( 0 == QString::compare( xComponent.pProcess->program(), sProgram ) )
        {
            eReturn = xComponent.eID;
            break;
        }
    }

    return eReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void ControlEngine::vHandleData()
{
    QObject *pSender = QObject::sender();
    QProcess *pCaller = nullptr;

    if ( nullptr != pSender )
    {
        pCaller = qobject_cast<QProcess *>( pSender );

        /* Ensure the caller is supported. */
        if ( nullptr != pCaller )
        {
            /* Act on the corresponding component. */
            switch ( eComponentProgramToEnumValue( pCaller->program() ) )
            {
            case eHUDViewComponentID_Accelerometer:
            {
                QStringList lstData = QString( pCaller->readAll() ).trimmed().split( ',' );
                qDebug() << "Accelerometer:" << lstData;

                /* Update the data model. */
                m_xAccelerometerData.dX = lstData.at( 0 ).toDouble();
                m_xAccelerometerData.dX = lstData.at( 1 ).toDouble();
                m_xAccelerometerData.dX = lstData.at( 2 ).toDouble();

                break;
            }

            case eHUDViewComponentID_GPS:
            {
                /* Parse out the individual pieces of data. */
                QRegularExpression Regex( "GPRMC,([\\d\\.\\d]+),([A|V]),([\\d\\.\\d]+),N,([\\d\\.\\d]+),W,([\\d\\.\\d]+),([\\d\\.\\d]+),([\\d]+),,,A" );
                QRegularExpressionMatch Matches = Regex.match( pCaller->readAll() );

                if ( Matches.hasMatch() )
                {
                    qDebug() << "GPS:";
                    qDebug() << "    Time: " << Matches.captured( 1 );
                    qDebug() << "    Valid: " << Matches.captured( 2 );
                    qDebug() << "    Latitude: " << Matches.captured( 3 );
                    qDebug() << "    Longitude: " << Matches.captured( 4 );
                    qDebug() << "    Speed: " << Matches.captured( 5 );
                    qDebug() << "    Direction: " << Matches.captured( 6 );
                    qDebug() << "    Date: " << Matches.captured( 7 );

                    /* Update the data model. */
                    m_xGPSData.bHasFix = true;
                    m_xGPSData.dLatitude = Matches.captured( 3 ).toDouble();
                    m_xGPSData.dLongitude = Matches.captured( 4 ).toDouble();
                    m_xGPSData.dSpeed = Matches.captured( 5 ).toDouble();
                    m_xGPSData.dDirection = Matches.captured( 6 ).toDouble();
                }
                else
                {
                    qDebug() << "GPS: No fix!";
                    m_xGPSData.bHasFix = false;
                }

                break;
            }

            case eHUDViewComponentID_HandlebarButtons:
                /* Filter out spurious input. */
#if 0
                if ( "0\n" == pCaller->readAll() )
                {
                    /* Update the display mode. */
                    if ( eControlDisplayMode_Time == m_eDisplayMode )
                    {
                        m_eDisplayMode = eControlDisplayMode_Speed;
                    }
                    else if ( eControlDisplayMode_Speed == m_eDisplayMode )
                    {
                        m_eDisplayMode = eControlDisplayMode_Direction;
                    }
                    else
                    {
                        m_eDisplayMode = eControlDisplayMode_Time;
                    }

                    /* Clear the display and immediately refresh. */
                    ssd1306_clearScreen8();
                    vUpdateDisplay();
                }
#endif

                break;

            case eHUDViewComponentID_LightSensor:
                /* Store the value. */
                m_LightSensorData = pCaller->readAll();
                qDebug() << "Light Sensor:" << m_LightSensorData;
                break;

            default:
                /* Nothing to do. */
                qDebug() << "ControlEngine::vHandleData() received data for process: " << pCaller->program();
                break;
            }
        }
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

bool ControlEngine::bParseConfig( const QString & sConfigPath )
{
    QFile ConfigFile( sConfigPath );
    QString sContents = "";
    QStringList lstLines;
    QRegularExpression Regex;
    QRegularExpressionMatch Match;
    QString sLine = "";
    eHUDViewComponentID_t eComponentID = eHUDViewComponentID_Unknown;
    QString sComponentProgram = "";
    xHUDViewComponent_t xComponent = { eHUDViewComponentID_Unknown, nullptr };
    bool bReturn = false;

    if ( ConfigFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        sContents = ConfigFile.readAll();

        if ( !sContents.isEmpty() )
        {
            lstLines = sContents.split( "\n", QString::SkipEmptyParts );

            /* Store each component name and path. */
            for ( int i = 0; i < lstLines.length(); i++ )
            {
                sLine = lstLines.at( i );
                Regex.setPattern( "^(\\S+):" );
                Match = Regex.match( sLine );

                /* Attempt to extract the component parts. */
                if ( Match.hasMatch() )
                {
                    eComponentID = eComponentNameToEnumValue( Match.captured( 1 ) );

                    /* Check whether the extracted component ID is valid. */
                    if ( eHUDViewComponentID_Unknown != eComponentID )
                    {
                        xComponent.eID = eComponentID;
                    }
                    else
                    {
                        /* Unsupported component. */
                        qDebug() << "Got unsupported component when parsing config file.";
                        bReturn = false;
                        break;
                    }

                    Regex.setPattern( ":(\\S+)$" );
                    Match = Regex.match( sLine );

                    if ( Match.hasMatch() )
                    {
                        sComponentProgram = Match.captured( 1 );

                        /* Ensure the program is valid. */
                        if ( !QFile::exists( sComponentProgram ) )
                        {
                            qDebug() << "Specified program path could not be found for component: "
                                     << sEnumValueToComponentName( xComponent.eID );
                            bReturn = false;
                            break;
                        }

                        /* Set up the component process. */
                        xComponent.pProcess = new QProcess();
                        xComponent.pProcess->setProgram( sComponentProgram );
                        xComponent.pProcess->setReadChannel( QProcess::StandardOutput );
                        connect( xComponent.pProcess, SIGNAL( readyReadStandardOutput() ), this, SLOT( vHandleData() ) );

                        /* Register the component. */
                        m_lstRegisteredComponents.append( xComponent );
                        bReturn = true;
                    }
                    else
                    {
                        /* Improper formatting. */
                        qDebug() << "Invalid or missing program path for component: "
                                 << sEnumValueToComponentName( xComponent.eID );
                        bReturn = false;
                        break;
                    }
                }
                else
                {
                    /* Improper formatting. */
                    bReturn = false;
                    break;
                }

                /* Check for failure before continuing. */
                if ( !bReturn )
                {
                    break;
                }
            }
        }
        else
        {
            /* Empty config file. */
            bReturn = false;
        }
    }
    else
    {
        /* Failed to open the file. */
        bReturn = false;
    }

    return bReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

void ControlEngine::vDisplayInit()
{
    st7735_128x160_spi_init( 22, 1, 23 );
    ssd1306_setMode( LCD_MODE_NORMAL );
    st7735_setRotation( 1 );
    ssd1306_fillScreen8( 0x00 );
    ssd1306_setFixedFont( UbuntuMono25x34 );
    ssd1306_clearScreen8();
    m_DisplayRefreshTimer.start();
    m_ModeSwitchTimer.start();
}
/*--------------------------------------------------------------------------------------------------------------------*/

void ControlEngine::vUpdateDisplay()
{
    /* Set the font color depending on the light sensor value. */
    if ( LIGHT_SENSOR_DARK_THRESHOLD > m_LightSensorData.left( m_LightSensorData.length() - 1 ).toInt() )
    {
        /* Red font for nighttime. */
        ssd1306_setColor( RGB_COLOR8( 255, 0, 0 ) );
    }
    else
    {
        /* White font for daytimne. */
        ssd1306_setColor( RGB_COLOR8( 255, 255, 255 ) );
    }

    /* Determine what to display. */
    switch ( m_eDisplayMode )
    {
    case eControlDisplayMode_Time:
        ssd1306_printFixed8( 16, 48, QTime::currentTime().toString( "hh:mm" ).toStdString().c_str(), STYLE_NORMAL );
        break;

    case eControlDisplayMode_Speed:
        /* Only attempt to display something if there is valid data to process. */
        if ( m_xGPSData.bHasFix )
        {
            ssd1306_printFixed8( 16, 48, QString::number( qRound( m_xGPSData.dSpeed * 1.15078 ) )
                                 .toStdString().c_str(), STYLE_NORMAL );
        }
        else
        {
            ssd1306_printFixed8( 16, 48, "---", STYLE_NORMAL );
        }

        break;

    case eControlDisplayMode_Direction:
        /* Only attempt to display something if there is valid data to process. */
        if ( m_xGPSData.bHasFix )
        {
            ssd1306_printFixed8( 16, 48, sGPSDirectionToString( m_xGPSData.dDirection ).toStdString().c_str(),
                                 STYLE_NORMAL );
        }
        else
        {
            ssd1306_printFixed8( 16, 48, "-- ", STYLE_NORMAL );
        }

        break;

    default:
        /* Nothing to do. */
        break;
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

void ControlEngine::vChangeMode()
{
    /* Update the display mode. */
    if ( eControlDisplayMode_Time == m_eDisplayMode )
    {
        m_eDisplayMode = eControlDisplayMode_Speed;
    }
    else if ( eControlDisplayMode_Speed == m_eDisplayMode )
    {
        m_eDisplayMode = eControlDisplayMode_Direction;
    }
    else
    {
        m_eDisplayMode = eControlDisplayMode_Time;
    }

    /* Clear the display and immediately refresh. */
    ssd1306_clearScreen8();
    vUpdateDisplay();
}
/*--------------------------------------------------------------------------------------------------------------------*/

QString ControlEngine::sGPSDirectionToString( const double & dDirection )
{
    QString sReturn = "-- ";

    /* Ensure a sane measurement. */
    if ( ( 0.0 <= dDirection ) && ( 360.0 >= dDirection ) )
    {
        /* Determine the direction. */
        if ( ( ( 0.0 <= dDirection ) && ( 11.25 > dDirection ) )
             || ( ( 348.75 <= dDirection ) && ( 360.0 >= dDirection ) ) )
        {
            sReturn = "N  ";
        }
        else if ( ( 11.25 <= dDirection ) && ( 33.75 > dDirection ) )
        {
            sReturn = "NNE";
        }
        else if ( ( 33.75 <= dDirection ) && ( 56.25 > dDirection ) )
        {
            sReturn = "NE ";
        }
        else if ( ( 56.25 <= dDirection ) && ( 78.75 > dDirection ) )
        {
            sReturn = "ENE";
        }
        else if ( ( 78.75 <= dDirection ) && ( 101.25 > dDirection ) )
        {
            sReturn = "E  ";
        }
        else if ( ( 101.25 <= dDirection ) && ( 123.75 > dDirection ) )
        {
            sReturn = "ESE";
        }
        else if ( ( 123.75 <= dDirection ) && ( 146.25 > dDirection ) )
        {
            sReturn = "SE ";
        }
        else if ( ( 146.25 <= dDirection ) && ( 168.75 > dDirection ) )
        {
            sReturn = "SSE";
        }
        else if ( ( 168.75 <= dDirection ) && ( 191.25 > dDirection ) )
        {
            sReturn = "S  ";
        }
        else if ( ( 191.25 <= dDirection ) && ( 213.75 > dDirection ) )
        {
            sReturn = "SSW";
        }
        else if ( ( 213.75 <= dDirection ) && ( 236.25 > dDirection ) )
        {
            sReturn = "SW ";
        }
        else if ( ( 236.25 <= dDirection ) && ( 258.75 > dDirection ) )
        {
            sReturn = "WSW";
        }
        else if ( ( 258.75 <= dDirection ) && ( 281.25 > dDirection ) )
        {
            sReturn = "W  ";
        }
        else if ( ( 281.225 <= dDirection ) && ( 303.75 > dDirection ) )
        {
            sReturn = "WNW";
        }
        else if ( ( 303.75 <= dDirection ) && ( 326.25 > dDirection ) )
        {
            sReturn = "NW ";
        }
        else if ( ( 326.25 <= dDirection ) && ( 348.75 > dDirection ) )
        {
            sReturn = "NNW";
        }
    }
    else
    {
        qDebug() << "WARNING: ControlEngine::sGPSDirectionToString got invalid direction.";
    }

    return sReturn;
}
/*--------------------------------------------------------------------------------------------------------------------*/

static void vSignalHandler( int iSignal )
{
    /* Check for a signal to quit. */
    if ( SIGINT == iSignal )
    {
        QCoreApplication::instance()->quit();
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

