#include <QCommandLineParser>
#include <QCoreApplication>

#include "controlengine.h"
/*--------------------------------------------------------------------------------------------------------------------*/

int main( int argc, char * argv[] )
{
    QCoreApplication App( argc, argv );
    QCoreApplication::setApplicationName( "HUDView Control" );
    QCoreApplication::setApplicationVersion( "1.0.0" );

    ControlEngine Engine;

    /* Parse the command line arguments. */
    QCommandLineParser Parser;
    QCommandLineOption ConfigFileOption( QStringList() << "c" << "config",
                                         QCoreApplication::translate( "main", "Use the specified configuration file." ),
                                         QCoreApplication::translate( "main", "path" ) );
    Parser.setApplicationDescription( "HUDView Control Application" );
    Parser.addHelpOption();
    Parser.addVersionOption();
    Parser.addOption( ConfigFileOption );
    Parser.process( App );

    if ( Parser.isSet( "config" ) )
    {
        Engine.vSetConfigFile( Parser.value( "config" ) );
    }

    /* Release control to the engine. */
    return Engine.iRun( &App );
}
/*--------------------------------------------------------------------------------------------------------------------*/

