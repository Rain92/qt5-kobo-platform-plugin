#include "kobobuttonintegration.h"

#define KEY_LIGHT 90
#define KEY_HOME 102
#define KEY_POWER 116
#define KEY_SLEEP_COVER 59
#define KEY_PAGE_UP 193
#define KEY_PAGE_DOWN 194

#define EVENT_PRESS 1
#define EVENT_RELEASE 0

#define OFF 0
#define ON 1

KoboButtonIntegration::KoboButtonIntegration(QObject *parent, const QString &inputDevice, bool debug)
    : QObject(parent), debug(debug), isInputCaptured(false)
{
    inputHandle = open(inputDevice.toStdString().c_str(), O_RDONLY);

    socketNotifier = new QSocketNotifier(inputHandle, QSocketNotifier::Read);

    connect(socketNotifier, &QSocketNotifier::activated, this, &KoboButtonIntegration::activity);

    socketNotifier->setEnabled(true);

    captureInput();
}

KoboButtonIntegration::~KoboButtonIntegration()
{
    releaseInput();
    delete socketNotifier;
    close(inputHandle);
}

void KoboButtonIntegration::captureInput()
{
    if (isInputCaptured || inputHandle == -1)
        return;

    if (debug)
        qDebug("KoboKb: Attempting to capture input...");

    int res = ioctl(inputHandle, EVIOCGRAB, ON);

    if (res == 0)
        isInputCaptured = true;
    else if (debug)
        qDebug() << "KoboKb: Capture keyboard input error:" << res;
}

void KoboButtonIntegration::releaseInput()
{
    if (!isInputCaptured || inputHandle == -1)
        return;

    if (debug)
        qDebug("KoboKb: attempting to release input...");

    if (ioctl(inputHandle, EVIOCGRAB, OFF) == 0)
        isInputCaptured = false;
    else if (debug)
        qDebug("KoboKb: release keyboard input: error");
}

void KoboButtonIntegration::activity(int)
{
    socketNotifier->setEnabled(false);

    input_event in;
    read(inputHandle, &in, sizeof(input_event));

    KoboKey code;

    if (KoboPhysicalKeyMap.contains(in.code))
    {
        code = KoboPhysicalKeyMap[in.code];

        QEvent::Type eventType = in.value == EVENT_PRESS ? QEvent::KeyPress : QEvent::KeyRelease;

        QKeyEvent keyEvent(eventType, code, Qt::NoModifier);

        QObject *focusObject = qGuiApp->focusObject();
        if (focusObject)
            QGuiApplication::sendEvent(focusObject, &keyEvent);

        if (debug)
            qDebug() << "found focusobject:" << (focusObject != nullptr) << "in.type:" << in.type
                     << " | in.code: " << in.code << " | code:" << code << " | "
                     << (in.value ? "pressed" : "released");
    }

    socketNotifier->setEnabled(true);
}
