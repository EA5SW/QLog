#include "TCIDrv.h"
#include "rig/macros.h"

MODULE_IDENTIFICATION("qlog.rig.driver.tcidrv");

// https://github.com/ExpertSDR3/TCI/blob/main/TCI%20Protocol.pdf

QList<QPair<int, QString> > TCIDrv::getModelList()
{
    FCT_IDENTIFICATION;

    QList<QPair<int, QString>> ret;

    ret << QPair<int, QString>(1, tr("Rig 0")) // 0 cause a crash in setting dialog when RigModel combo is expacted, I don't know why
        << QPair<int, QString>(2, tr("Rig 1"))
        << QPair<int, QString>(3, tr("Rig 2"))
        << QPair<int, QString>(4, tr("Rig 3"));

    return ret;
}

RigCaps TCIDrv::getCaps(int)
{
    FCT_IDENTIFICATION;

    RigCaps ret;

    ret.isNetworkOnly = true;

    ret.canGetFreq = true;
    ret.canGetMode = true;
    ret.canGetVFO = true;
    ret.canGetRIT = true;
    ret.canGetXIT = true;
    ret.canGetPTT = true;
    ret.canSendMorse = true;
    ret.canGetKeySpeed = true;
    ret.canGetPWR = true;

    return ret;
}

TCIDrv::TCIDrv(const RigProfile &profile, QObject *parent)
    : GenericDrv(profile, parent),
      ready(false),
      receivedOnly(false),
      currFreq(0.0),
      currRIT(0.0),
      currXIT(0.0),
      RITEnabled(false),
      XITEnabled(false)
{
    FCT_IDENTIFICATION;

    connect(&ws, &QWebSocket::connected, this, &TCIDrv::onConnected);
#if (QT_VERSION < QT_VERSION_CHECK(6, 5, 0))
    connect(&ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &TCIDrv::onSocketError);
#else
    connect(&ws, &QWebSocket::errorOccurred, this, &TCIDrv::onSocketError);
#endif
}

TCIDrv::~TCIDrv()
{
    FCT_IDENTIFICATION;

    TCIDrv::stopTimers();
}

bool TCIDrv::open()
{
    FCT_IDENTIFICATION;

    QUrl url;
    url.setScheme("ws");
    url.setHost(rigProfile.hostname);
    url.setPort(rigProfile.netport);

    ws.open(url);
    return true;
}

bool TCIDrv::isMorseOverCatSupported()
{
    FCT_IDENTIFICATION;

    return true;
}

QStringList TCIDrv::getAvailableModes()
{
    return modeList;
}

void TCIDrv::setFrequency(double newFreq)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << newFreq;

    if ( !rigProfile.getFreqInfo )
        return;

    unsigned long long internalFreq = static_cast<unsigned long long>(newFreq);

    QStringList args = {"0", //VFOID
                        QString::number(internalFreq)
                       };
    sendCmd("vfo", true, args);
}

void TCIDrv::setRawMode(const QString &rawMode)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << rawMode;

    if ( !rigProfile.getModeInfo || rawMode.isEmpty() )
        return;

    QStringList args = {rawMode};
    sendCmd("modulation", true, args);
}

void TCIDrv::setMode(const QString &mode, const QString &subMode)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << mode << subMode;

    setRawMode(mode2RawMode(mode, subMode));
}

void TCIDrv::setPTT(bool newPTTState)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << newPTTState;

    if ( !rigProfile.getPTTInfo )
        return;

    if ( receivedOnly )
    {
        qCDebug(runtime) << "Only receiver - no action";
        return;
    }

    QStringList args = {(newPTTState) ? "true" : "false"};
    sendCmd("trx", true, args);
}

void TCIDrv::setKeySpeed(qint16 wpm)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << wpm;

    if ( !rigProfile.getKeySpeed )
        return;

    if ( receivedOnly )
    {
        qCDebug(runtime) << "Only receiver - no action";
        return;
    }

    QStringList args = {QString::number(wpm)};
    sendCmd("cw_keyer_speed", false, args);
}

void TCIDrv::syncKeySpeed(qint16 wpm)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << wpm;

    if ( !rigProfile.keySpeedSync )
        return;

    setKeySpeed(wpm);
}

void TCIDrv::sendMorse(const QString &text)
{
    FCT_IDENTIFICATION;

    if ( text.isEmpty() )
        return;

    if ( receivedOnly )
    {
        qCDebug(runtime) << "Only receiver - no action";
        return;
    }

    QStringList args {text};
    sendCmd("cw_macros", false, args);
}

void TCIDrv::stopMorse()
{
    FCT_IDENTIFICATION;

    if ( receivedOnly )
    {
        qCDebug(runtime) << "Only receiver - no action";
        return;
    }

    sendCmd("cw_macros_stop", false);
}

void TCIDrv::sendState()
{
    FCT_IDENTIFICATION;

    // it seems that Rig sends its state at the begining of the session
    // it needs to be tested
#if 0
    sendCmd("vfo", true, QStringList("0")); //Freq
    sendCmd("trx", true); // PTT
    sendCmd("modulation", true); //Mode
    //sendCmd(); // Current VFO ????
    sendCmd("tune_drive", true); //PWR
    sendCmd("rit_offset", true);
    sendCmd("xit_offset", true);
    sendCmd("cw_macros_speed", false);
#endif
}

void TCIDrv::stopTimers()
{
    FCT_IDENTIFICATION;

    // no timer

    // close the WebSocket here becuase this method is called
    // via queue signals. What caused that there is not warning
    // that Websocket is destroyed from another thread
    ws.close();
    return;
}

void TCIDrv::onConnected()
{
    FCT_IDENTIFICATION;

    qCDebug(runtime) << "Rig is connected";

    connect(&ws, &QWebSocket::textMessageReceived,
            this, &TCIDrv::onTextMessageReceived);

    // QLog has to wait for READY message to complete connection = to emit RigReady
}

void TCIDrv::onSocketError(QAbstractSocket::SocketError socker_error)
{
    FCT_IDENTIFICATION;

    QString error_msg;

    qCDebug(runtime) << socker_error;

    switch (socker_error)
    {
    case QAbstractSocket::ConnectionRefusedError:
        error_msg.append(QObject::tr("Connection Refused"));
        break;

    case QAbstractSocket::RemoteHostClosedError:
        error_msg.append(QObject::tr("Host closed the connection"));
        //reconectRequested = true;
        break;

    case QAbstractSocket::HostNotFoundError:
        error_msg.append(QObject::tr("Host not found"));
        break;
    case QAbstractSocket::SocketTimeoutError:
        error_msg.append(QObject::tr("Timeout"));
        //reconectRequested = true;
        break;
    case QAbstractSocket::NetworkError:
        error_msg.append(QObject::tr("Network Error"));
        break;
    default:
        error_msg.append(QObject::tr("Internal Error"));
    }

   emit errorOccured(tr("Error Occured"),
                     error_msg);
}

void TCIDrv::onTextMessageReceived(const QString &message)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << message;

    if ( message.isEmpty() )
        return;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    const QStringList &commands = message.split(";", Qt::SkipEmptyParts);
#else /* Due to ubuntu 20.04 where qt5.12 is present */
    const QStringList &commands = message.split(";", QString::SkipEmptyParts);
#endif

    for ( const QString &command : commands )
    {
        // message structure CMD1;CMD2;CMD3....
        // CMD structure cmdName:arg0,arg1,arg2
        qCDebug(runtime) << command;

        const QString &trimmedCommand = command.trimmed();

        if ( trimmedCommand.isEmpty() )
            continue;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
        const QStringList &cmdElemets = trimmedCommand.split(":", Qt::SkipEmptyParts);
#else /* Due to ubuntu 20.04 where qt5.12 is present */
        const QStringList &cmdElemets = trimmedCommand.split(":", QString::SkipEmptyParts);
#endif

        const QString &cmdName = cmdElemets.at(0).toLower().trimmed();
        QStringList cmdArgs;
        TCIDrv::parseFce parser = responseParsers.value(cmdName);

        if ( !parser )
        {
            qCDebug(runtime) << "Parser function is not defined for" << cmdName;
            continue;
        }

        if ( cmdElemets.size() > 1 )
        {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
            cmdArgs = cmdElemets.at(1).split(",", Qt::SkipEmptyParts);
#else /* Due to ubuntu 20.04 where qt5.12 is present */
            cmdArgs = cmdElemets.at(1).split(",", QString::SkipEmptyParts);
#endif
        }

        // Trim argument - is a space possible?
        for( QString &arg : cmdArgs )
            arg = arg.trimmed();

        (this->*(parser))(cmdArgs);
    }
}

void TCIDrv::sendCmd(const QString &cmd,
                     bool addRigID,
                     const QStringList& args)
{
    FCT_IDENTIFICATION;

    if ( !ready )
    {
        qCDebug(runtime) << "Rig is not ready";
        return;
    }

    QStringList modifiedArgs(args);

    if ( addRigID )
        modifiedArgs.prepend(QString::number(rigProfile.model - 1));

    //Mandatory part of the command
    QString cmdText = cmd.toUpper();

    if ( modifiedArgs.size() > 0 )
        cmdText.append(":" + modifiedArgs.join(","));

    cmdText.append(";");

    qCDebug(runtime) << "Sending command:" << cmdText;
    ws.sendTextMessage(cmdText);
}

const QString TCIDrv::getModeNormalizedText(const QString &rawMode, QString &submode)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << rawMode;

    submode = QString();

    if ( rawMode.contains("CW") )
        return "CW";

    if ( rawMode == "USB" )
    {
        submode = "USB";
        return "SSB";
    }

    if ( rawMode == "LSB" )
    {
        submode = "LSB";
        return "SSB";
    }

    if ( rawMode == "AM" )
        return "AM";

    if ( rawMode == "NFM" )
        return "FM";

    if ( rawMode == "WFM" )
        return "FM";

    if ( rawMode == "DIGL" )
    {
        submode = "LSB";
        return "SSB";
    }

    if ( rawMode == "DIGU" )
    {
        submode = "USB";
        return "SSB";
    }

    return QString();
}

const QString TCIDrv::mode2RawMode(const QString &mode, const QString &submode)
{
    FCT_IDENTIFICATION;


    if ( mode == "SSB" )
        return submode;

    if ( mode == "CW" )
        return mode;

    if ( mode == "FM" )
        return "NFM";

    if ( mode == "AM" )
        return mode;

    return QString();
}

#define CHECK_PARAMS_COUNT( size, min ) \
    if ( size < min ) \
    { qCWarning(runtime) << "Incorrect number of arguments" << size << min; \
      return; }

void TCIDrv::rspPROTOCOL(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 — program name.
    // аrg1 — version of the protocol.
    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);

    qCDebug(runtime) << "Program name:" << cmdArgs.at(0)
                     << "Procotol version:" << cmdArgs.at(1);
}

void TCIDrv::rspREADY(const QStringList &)
{
    FCT_IDENTIFICATION;

    ready = true;
    emit rigIsReady();
}

void TCIDrv::rspSTART(const QStringList &)
{
    FCT_IDENTIFICATION;

    ready = true;
}

void TCIDrv::rspSTOP(const QStringList &)
{
    FCT_IDENTIFICATION;

    ready = false;

    emit errorOccured(tr("Rig status changed"),
                      tr("Rig is not connected"));
}

void TCIDrv::rspRECEIVE_ONLY(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - received only (true), transceiver (false)
    CHECK_PARAMS_COUNT(cmdArgs.size(), 1);

    receivedOnly = ( cmdArgs.at(0).toLower() == "true" );
}

void TCIDrv::rspMODULATIONS_LIST(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 ... N
    for ( const QString &arg : cmdArgs )
        modeList << arg.toUpper();

    qCDebug(runtime) << modeList;
}

void TCIDrv::rspVFO(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - VFO
    // arg2 - freq
    CHECK_PARAMS_COUNT(cmdArgs.size(), 3);

    if ( !rigProfile.getFreqInfo )
        return;

    if ( cmdArgs.at(0) != QString::number(rigProfile.model - 1))
    {
        qCDebug(runtime) << "Command is not for QLog";
        return;
    }

    if ( cmdArgs.at(1) != "0" )
    {
        qCDebug(runtime) << "Skipping info from VFO" << cmdArgs.at(1);
        return;
    }

    bool ok;
    currFreq = cmdArgs.at(2).toDouble(&ok);
    if ( ok )
    {
        qCDebug(runtime) << "Rig Freq" << QSTRING_FREQ(Hz2MHz(currFreq));
        qCDebug(runtime) << "emitting FREQ changed";
        emit frequencyChanged(Hz2MHz(currFreq),
                              Hz2MHz(getRITFreq()),
                              Hz2MHz(getXITFreq()));
    }
    else
    {
        qCDebug(runtime) << "Received Freq is not double" << cmdArgs.at(2);
    }
}

void TCIDrv::rspTRX(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - status
    // arg2 - signal source (optional)
    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);

    if ( !rigProfile.getPTTInfo )
        return;

    if ( cmdArgs.at(0) != QString::number(rigProfile.model - 1))
    {
        qCDebug(runtime) << "Command is not for QLog";
        return;
    }

    bool ptt = ( cmdArgs.at(1).compare("true", Qt::CaseInsensitive) == 0 );

    qCDebug(runtime) << "Rig PTT:"<< ptt;
    qCDebug(runtime) << "emitting PTT changed" << ptt;
    emit pttChanged(ptt);
}

void TCIDrv::rspMODULATION(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - mode
    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);

    if ( !rigProfile.getModeInfo )
        return;

    if ( cmdArgs.at(0) != QString::number(rigProfile.model - 1))
    {
        qCDebug(runtime) << "Command is not for QLog";
        return;
    }

    const QString &rawModeText = cmdArgs.at(1);
    QString submode;
    const QString mode = getModeNormalizedText(rawModeText, submode);


    qCDebug(runtime) << "emitting MODE changed" << rawModeText << mode << submode;
    emit modeChanged(rawModeText,
                     mode, submode,
                     0);  // TODO: bandwidth should also be emited
                          // when RX_FILTER_BAND is processed
}

void TCIDrv::rspTUNE_DRIVE(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - power output (in watts????)
    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);


    // NO ACTION NOW
}

void TCIDrv::rspDRIVE(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - power output (in watts)
    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);

    if ( !rigProfile.getPWRInfo )
        return;

    if ( cmdArgs.at(0) != QString::number(rigProfile.model - 1))
    {
        qCDebug(runtime) << "Command is not for QLog";
        return;
    }

    bool ok;
    unsigned int rigPower = cmdArgs.at(1).toUInt(&ok);

    if ( ok )
    {
        qCDebug(runtime) << "Rig PWR" << rigPower;
        qCDebug(runtime) << "emitting PWR changed";
        emit powerChanged(rigPower);
    }
    else
    {
        qCDebug(runtime) << "Received PWR is not a number" << cmdArgs.at(1);
    }
}

void TCIDrv::rspRIT_OFFSET(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - rit (+- in HZ)
    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);

    if ( !rigProfile.getRITInfo )
        return;

    if ( cmdArgs.at(0) != QString::number(rigProfile.model - 1))
    {
        qCDebug(runtime) << "Command is not for QLog";
        return;
    }

    bool ok;
    currRIT = cmdArgs.at(1).toDouble(&ok);

    if ( ok )
    {
        qCDebug(runtime) << "Rig RIT" << RITEnabled << currRIT;
        qCDebug(runtime) << "emitting RIT changed" << QSTRING_FREQ(Hz2MHz(getRawRIT()));
        qCDebug(runtime) << "emitting FREQ changed " << QSTRING_FREQ(Hz2MHz(currFreq))
                                                     << QSTRING_FREQ(Hz2MHz(getRITFreq()))
                                                     << QSTRING_FREQ(Hz2MHz(getXITFreq()));

        emit ritChanged(Hz2MHz(getRawRIT()));
        emit frequencyChanged(Hz2MHz(currFreq),
                              Hz2MHz(getRITFreq()),
                              Hz2MHz(getXITFreq()));
    }
    else
    {
        qCDebug(runtime) << "Received RIT is not a double" << cmdArgs.at(1);
    }
}

void TCIDrv::rspXIT_OFFSET(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - rit (+- in HZ)
    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);

    if ( !rigProfile.getXITInfo )
        return;

    if ( cmdArgs.at(0) != QString::number(rigProfile.model - 1))
    {
        qCDebug(runtime) << "Command is not for QLog";
        return;
    }

    bool ok;
    currXIT = cmdArgs.at(1).toDouble(&ok);

    if ( ok )
    {
        qCDebug(runtime) << "Rig XIT" << XITEnabled << currXIT;
        qCDebug(runtime) << "emitting XIT changed" << QSTRING_FREQ(Hz2MHz(getRawXIT()));
        qCDebug(runtime) << "emitting FREQ changed " << QSTRING_FREQ(Hz2MHz(currFreq))
                                                     << QSTRING_FREQ(Hz2MHz(getRITFreq()))
                                                     << QSTRING_FREQ(Hz2MHz(getXITFreq()));

        emit xitChanged(Hz2MHz(getRawXIT()));
        emit frequencyChanged(Hz2MHz(currFreq),
                              Hz2MHz(getRITFreq()),
                              Hz2MHz(getXITFreq()));
    }
    else
    {
        qCDebug(runtime) << "Received XIT is not a double" << cmdArgs.at(1);
    }
}

void TCIDrv::rspCW_MACROS_SPEED(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - wpm
    CHECK_PARAMS_COUNT(cmdArgs.size(), 1);

    bool ok;
    unsigned int wpm = cmdArgs.at(0).toInt(&ok);

    if ( ok )
    {
        qCDebug(runtime) << "Rig Key Speed:" << wpm;
        emit keySpeedChanged(wpm);
    }
    else
    {
        qCDebug(runtime) << "Received RIT is not a double" << cmdArgs.at(1);
    }
}

void TCIDrv::rspRIT_ENABLE(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - status indicator.

    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);

    if ( !rigProfile.getRITInfo )
        return;

    if ( cmdArgs.at(0) != QString::number(rigProfile.model - 1))
    {
        qCDebug(runtime) << "Command is not for QLog";
        return;
    }

    RITEnabled = ( cmdArgs.at(1).toLower() == "true" );

    qCDebug(runtime) << "Rig RIT status changed" << RITEnabled << currRIT;
    qCDebug(runtime) << "emitting RIT changed" << QSTRING_FREQ(Hz2MHz(getRawRIT()));
    emit ritChanged(Hz2MHz(getRawRIT()));
    emit frequencyChanged(Hz2MHz(currFreq),
                          Hz2MHz(getRITFreq()),
                          Hz2MHz(getXITFreq()));
}

void TCIDrv::rspXIT_ENABLE(const QStringList &cmdArgs)
{
    FCT_IDENTIFICATION;

    // arg0 - rigid
    // arg1 - status indicator.

    CHECK_PARAMS_COUNT(cmdArgs.size(), 2);

    if ( !rigProfile.getXITInfo )
        return;

    if ( cmdArgs.at(0) != QString::number(rigProfile.model - 1))
    {
        qCDebug(runtime) << "Command is not for QLog";
        return;
    }

    XITEnabled = ( cmdArgs.at(1).toLower() == "true" );

    qCDebug(runtime) << "Rig XIT status changed" << XITEnabled << currXIT;
    qCDebug(runtime) << "emitting XIT changed" << QSTRING_FREQ(Hz2MHz(getRawXIT()));
    emit xitChanged(Hz2MHz(getRawXIT()));
    emit frequencyChanged(Hz2MHz(currFreq),
                          Hz2MHz(getRITFreq()),
                          Hz2MHz(getXITFreq()));
}

#undef CHECK_PARAMS_COUNT

double TCIDrv::getRITFreq()
{
    FCT_IDENTIFICATION;

    return currFreq + getRawRIT();
}

void TCIDrv::setRITFreq(double rit)
{
    currRIT = rit;
}

double TCIDrv::getXITFreq()
{
    return currFreq + currXIT;
}

void TCIDrv::setXITFreq(double xit)
{
    currXIT = xit;
}

double TCIDrv::getRawRIT()
{
    return ( ( RITEnabled ) ? currRIT : 0.0 );
}

double TCIDrv::getRawXIT()
{
    return ( ( XITEnabled ) ? currXIT : 0.0 );
}
