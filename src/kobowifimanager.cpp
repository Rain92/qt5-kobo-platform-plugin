#include "kobowifimanager.h"

#include <QDebug>
#include <QFile>
#include <QString>

bool KoboWifiManager::testInternetConnection(int timeout)
{
    QString cmd = QString("ping -c 1 -q -W %1 1.1.1.1 2>&1 >/dev/null").arg(timeout);
    int res = system(cmd.toLocal8Bit());
    return res == 0;
}

void KoboWifiManager::enableWiFiConnection()
{
    QFile restoreWifiFile(":/scripts/restore-wifi.sh");
    restoreWifiFile.open(QIODevice::ReadOnly);
    QByteArray restoreWifiScript = restoreWifiFile.readAll();
    restoreWifiFile.close();

    system(restoreWifiScript);
}

void KoboWifiManager::disableWiFiConnection()
{
    QFile disableWifiFile(":/scripts/disable-wifi.sh");
    disableWifiFile.open(QIODevice::ReadOnly);
    QByteArray disableWifiScript = disableWifiFile.readAll();
    disableWifiFile.close();

    system(disableWifiScript);
}
