#include <signal.h>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>

#include "controlengine.h"
/*--------------------------------------------------------------------------------------------------------------------*/

static void vSignalHandler( int iSignal );
/*--------------------------------------------------------------------------------------------------------------------*/

ControlEngine::ControlEngine( QObject * pParent ) : QObject( pParent )
{
    m_sConfigPath = "";

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
            case eHUDViewComponentID_Camera:
            case eHUDViewComponentID_CameraDisplay:
            case eHUDViewComponentID_Control:
            case eHUDViewComponentID_ControlDisplay:
            case eHUDViewComponentID_GPS:
            case eHUDViewComponentID_HandlebarButtons:
            case eHUDViewComponentID_LightSensor:
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

static void vSignalHandler( int iSignal )
{
    /* Check for a signal to quit. */
    if ( SIGINT == iSignal )
    {
        QCoreApplication::instance()->quit();
    }
}
/*--------------------------------------------------------------------------------------------------------------------*/

