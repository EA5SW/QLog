#include "HostsPortString.h"
#include "debug.h"

MODULE_IDENTIFICATION("qlog.core.hostsportstring");

HostsPortString::HostsPortString(const QString &addressesString, QObject *parent) :
    QObject(parent)
{
    FCT_IDENTIFICATION;

    qDebug(function_parameters) << addressesString;

    if ( addressesString.isEmpty() )
    {
        return;
    }

    QStringList addressTokens = addressesString.split(" ");

    for ( const QString &addrToken : qAsConst(addressTokens) )
    {
        qCDebug(runtime) << "Processing address " << addrToken;

        QStringList addressPair = addrToken.split(":");

        if ( addressPair.size() == 2 )
        {
            bool isPortOK = false;
            uint port = addressPair.at(1).toUInt(&isPortOK);

            if ( isPortOK && port < 65536 )
            {
                qCDebug(runtime) << "Adding address" << addrToken;
                addressList << HostPortAddress(addressPair.at(0), port);
            }
            else
            {
                qCInfo(runtime) << "Malformed port "<< addressPair.at(1) << "for address " << addrToken << " - skipping";
            }
        }
        else
        {
            qCInfo(runtime) << "Malformed address " << addrToken << " - skipping";
        }
    }
}

QList<HostPortAddress> HostsPortString::getAddrList()
{
    FCT_IDENTIFICATION;

    return addressList;
}

HostPortAddress::HostPortAddress(const QString &hostAddress, quint16 hostPort) :
    QHostAddress(hostAddress),
    port(hostPort)
{
    FCT_IDENTIFICATION;

}

void HostPortAddress::setPort(quint16 hostPort)
{
    FCT_IDENTIFICATION;

    qCDebug(function_parameters) << hostPort;

    port = hostPort;
}

quint16 HostPortAddress::getPort() const
{
    FCT_IDENTIFICATION;

    return port;
}
