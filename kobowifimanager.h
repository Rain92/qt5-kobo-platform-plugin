#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QObject>

class KoboWifiManager
{
public:
    static bool testInternetConnection(int timeout);

    static void enableWiFiConnection();
    static void disableWiFiConnection();
};

#endif  // WIFIMANAGER_H
