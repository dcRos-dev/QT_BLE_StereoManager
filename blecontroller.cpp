#include "blecontroller.h"
#include <QDebug>
#include <QCoreApplication>
#include <QBluetoothPermission>

BleController::BleController(QObject *parent) : QObject(parent)
{
    // Prepariamo il radar per scansionare i dispositivi
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(5000);

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BleController::deviceFound);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BleController::scanFinished);
}

void BleController::startScan()
{
    // Controlliamo il permesso per accedere al bluetooth
    QBluetoothPermission perm;
    perm.setCommunicationModes(QBluetoothPermission::Access);

    Qt::PermissionStatus status = qApp->checkPermission(perm);

    if (status == Qt::PermissionStatus::Granted) {
        // Se il permesso c'è già
        emit infoMessage("Scansione in corso...");
        m_esp32Device = QBluetoothDeviceInfo();
        m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    }
    else if (status == Qt::PermissionStatus::Undetermined) {
        // Se non è ancora stato chiesto, mostriamo il pop-up.
        // Al termine della scelta dell'utente, Android richiamerà questa stessa funzione startScan()
        emit infoMessage("Richiesta permessi Bluetooth...");
        qApp->requestPermission(perm, this, &BleController::startScan);
    }
    else {
        // Se l'utente ha negato
        emit infoMessage("Errore: Permessi Bluetooth negati.");
    }
}

void BleController::deviceFound(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {

        QList<QBluetoothUuid> advertisedServices = device.serviceUuids();

        for (const QBluetoothUuid &serviceUuid : advertisedServices) {
            if (serviceUuid.toString().contains(SERVICE_UUID, Qt::CaseInsensitive)) {


                // Disconnettiamo subito il segnale per evitare che questa funzione
                // venga richiamata mille volte se l'ESP32 invia altri pacchetti adesso
                disconnect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
                           this, &BleController::deviceFound);

                emit infoMessage("Target identificato! Fermo il radar e connessione");
                qDebug() << "MATCH TROVATO E ISOLATO ";

                m_esp32Device = device;
                // Fermiamo il radar hardware
                m_discoveryAgent->stop();

                // Chiamiamo direttamente la funzione di connessione
                scanFinished();
                return;
            }
        }
    }
}


void BleController::scanFinished()
{
    if (m_esp32Device.isValid()) {
        emit infoMessage("Dispositivo valido. Configurazione controller...");

        if (m_controller) {
            m_controller->disconnectFromDevice();
            delete m_controller;
            m_controller = nullptr;
        }

        m_controller = QLowEnergyController::createCentral(m_esp32Device, this);
        m_controller->setRemoteAddressType(QLowEnergyController::PublicAddress);

        connect(m_controller, &QLowEnergyController::connected, this, &BleController::deviceConnected);
        connect(m_controller, &QLowEnergyController::disconnected, this, &BleController::deviceDisconnected);
        connect(m_controller, &QLowEnergyController::serviceDiscovered, this, &BleController::serviceDiscovered);

        connect(m_controller, &QLowEnergyController::errorOccurred, this, [this](QLowEnergyController::Error error){
            emit infoMessage("Errore di connessione codice: " + QString::number(error));

            // Tentativo di fallback se l'indirizzo era marcato male
            if (error == QLowEnergyController::ConnectionError) {
                static bool retried = false;
                if (!retried && m_controller) {
                    retried = true;
                    emit infoMessage("Tentativo di riconnessione con Fallback Address...");
                    m_controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
                    m_controller->connectToDevice();
                }
            }
        });

        emit infoMessage("Tentativo di connessione a " + TARGET_NAME + "...");
        m_controller->connectToDevice();
    } else {
        emit infoMessage("ESP32 non trovato. Avvicinati o resetta la scheda.");
    }
}

void BleController::serviceDiscovered(const QBluetoothUuid &newService)
{
    // Stampa di debug nei log per vedere quali servizi l'ESP32 sta esponendo
    qDebug() << "Servizio rilevato dall'ESP32:" << newService.toString();

    // Confronto degli UUID ignorando maiuscole/minuscole e parentesi
    if (newService.toString().contains(SERVICE_UUID, Qt::CaseInsensitive)) {
        emit infoMessage("Servizio Audio abbinato! Connessione al menu comandi...");
        m_audioService = m_controller->createServiceObject(newService, this);

        if (m_audioService) {
            connect(m_audioService, &QLowEnergyService::stateChanged, this, &BleController::serviceStateChanged);

            connect(m_audioService, &QLowEnergyService::characteristicChanged, this, [](const QLowEnergyCharacteristic &c, const QByteArray &value){
                qDebug() << "Caratteristica cambiata:" << c.uuid().toString() << "Valore:" << value;
            });

            m_audioService->discoverDetails();
            emit connectedToDevice();
        }
    }
}

void BleController::deviceConnected()
{
    emit infoMessage("Connesso! Cerco il servizio Audio...");
    m_controller->discoverServices();
}

void BleController::deviceDisconnected()
{
    emit infoMessage("Disconnesso dall'ESP32.");
    emit disconnectedFromDevice();
}

void BleController::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    if (s == QLowEnergyService::RemoteServiceDiscovered) {
        qDebug() << "Dettagli del servizio completamente letti.";
    }
}

void BleController::writeCharacteristic(const QString &uuidStr, const QByteArray &data)
{
    if (!m_audioService || m_audioService->state() != QLowEnergyService::RemoteServiceDiscovered) {
        emit infoMessage("Errore: Servizio non pronto!");
        return;
    }

    QLowEnergyCharacteristic characteristic = m_audioService->characteristic(QBluetoothUuid(uuidStr));
    if (characteristic.isValid()) {
        // Inviamo il dato (Write senza risposta è solitamente più veloce per il BLE)
        m_audioService->writeCharacteristic(characteristic, data, QLowEnergyService::WriteWithoutResponse);
        qDebug() << "Inviato:" << data << "all'UUID:" << uuidStr;
    } else {
        emit infoMessage("Errore: Caratteristica non trovata.");
    }
}

void BleController::sendPlay()
{
    // Inviamo la stringa "PLAY"
    writeCharacteristic(CHAR_PLAY_UUID, "PLAY");
}

void BleController::sendStop()
{
    // Inviamo la stringa "STOP"
    writeCharacteristic(CHAR_STOP_UUID, "STOP");
}