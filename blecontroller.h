#ifndef BLECONTROLLER_H
#define BLECONTROLLER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>

class BleController : public QObject
{
    Q_OBJECT
public:
    explicit BleController(QObject *parent = nullptr);

    // Creaimo funzioni Q_INVOKABLE così da poterle chiamare dai pulsanti QML

    // QML è un linguaggio simole a Javascript/CSS perciò non può vedere
    // funzioni C++ puro, perciò è necessario renderle Q_INVOKABLE pure
    Q_INVOKABLE void startScan();
    Q_INVOKABLE void sendPlay();
    Q_INVOKABLE void sendStop();



// Signals servono al C++ per comunicare cambiamento a QML o altre classi C++
signals:

    //Segnali che mangiamo a QML per aggiornare l'interfaccia
    void infoMessage(QString msg);
    void connectedToDevice();
    void disconnectedFromDevice();

private slots:
    // Funzioni automatiche che scattano agli eventi del bluetooth
    void deviceFound(const QBluetoothDeviceInfo &device);
    void scanFinished();
    void deviceConnected();
    void deviceDisconnected();
    void serviceDiscovered(const QBluetoothUuid &newService);
    void serviceStateChanged(QLowEnergyService::ServiceState s);

private:
    void writeCharacteristic(const QString &uuidStr, const QByteArray &data);

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QBluetoothDeviceInfo m_esp32Device;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_audioService = nullptr;


    const QString TARGET_NAME = "ESP32_Stereo_Player";
    const QString SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
    const QString CHAR_PLAY_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
    const QString CHAR_STOP_UUID = "c2a8b94f-1234-459e-8fcc-fa07361b26bc";
};

#endif // BLECONTROLLER_H
