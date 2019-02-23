#ifndef CONTROLENGINE_H
#define CONTROLENGINE_H

#include <QCoreApplication>
#include <QObject>
#include <QProcess>

class ControlEngine : public QObject
{
    Q_OBJECT

public:
    const QString DEFAULT_CONFIG_FILE_PATH = "/opt/HUDView/Control/default.conf";
    const int PROCESS_START_WAIT_TIMEOUT_MS = 5000;

    enum eHUDViewComponentID_t {
        eHUDViewComponentIDMin = 0,

        eHUDViewComponentID_Accelerometer,
        eHUDViewComponentID_Camera,
        eHUDViewComponentID_CameraDisplay,
        eHUDViewComponentID_Control,
        eHUDViewComponentID_ControlDisplay,
        eHUDViewComponentID_GPS,
        eHUDViewComponentID_HandlebarButtons,
        eHUDViewComponentID_LightSensor,
        eHUDViewComponentID_Unknown,

        eHUDViewComponentIDMax
    };

    struct xHUDViewComponent_t {
        eHUDViewComponentID_t eID;
        QProcess *pProcess;
    };

    explicit ControlEngine( QObject * pParent = nullptr );
    ~ControlEngine();

    int iRun( QCoreApplication * pApp );
    void vSetConfigFile( const QString & sPath );

    static bool bIsValidComponent( const xHUDViewComponent_t & xComponent );

    static QString sEnumValueToComponentName( const eHUDViewComponentID_t & eValue );
    static eHUDViewComponentID_t eComponentNameToEnumValue( const QString & sName );
    eHUDViewComponentID_t eComponentProgramToEnumValue( const QString & sProgram );

private slots:
    void vHandleData();

private:
    QString m_sConfigPath;
    QList<xHUDViewComponent_t> m_lstRegisteredComponents;

    bool bParseConfig( const QString & sConfigPath );
};

#endif // CONTROLENGINE_H

