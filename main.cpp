#include <QGuiApplication>
#include <QQmlApplicationEngine>


#include "blecontroller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    qmlRegisterType<BleController>("com.esp32.ble", 1, 0, "BleController");


    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("ESP32BLEStereo", "Main");

    return QGuiApplication::exec();
}
