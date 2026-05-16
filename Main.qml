import QtQuick
import QtQuick.Controls

// 1. IMPORTIAMO IL NOSTRO PACCHETTO C++
import com.esp32.ble 1.0

Window {
    width: 360
    height: 640
    visible: true
    title: qsTr("ESP32 BLE Controller")
    color: "#1e1e24" // Sfondo scuro ed elegante

    // DICHIARIAMO L'OGGETTO C++ DENTRO IL QML
    // Da adesso, l'oggetto si chiama "ble" e possiamo usare le sue funzioni e segnali
    BleController {
        id: ble

        // Catturiamo i segnali inviati dal C++ per aggiornare l'interfaccia
        onInfoMessage: function(msg) {
            statusText.text = msg
        }
        onConnectedToDevice: {
            btConnectButton.enabled = false
            playButton.enabled = true
            stopButton.enabled = true
        }
        onDisconnectedFromDevice: {
            btConnectButton.enabled = true
            playButton.enabled = false
            stopButton.enabled = false
        }
    }

    // Struttura della pagina, usiamo un layout verticale
    Column {
        anchors.centerIn: parent
        spacing: 30
        width: parent.width * 0.85

        // Titolo dell'applicazione
        Text {
            text: "ESP32 Stereo Player"
            color: "#ffffff"
            font.pixelSize: 26
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
        }

        // Display di Stato: dove mostriamo i messaggi che arrivano dal C++
        Rectangle {
            width: parent.width
            height: 80
            color: "#2a2a35"
            radius: 10
            border.color: "#3f3f50"
            border.width: 1

            Text {
                id: statusText
                text: "Disconnesso. Premi Connetti."
                color: "#a0a0b0"
                font.pixelSize: 16
                anchors.centerIn: parent
                horizontalAlignment: Text.AlignHCenter
                width: parent.width * 0.9
                wrapMode: Text.Wrap
            }
        }

        // Pulsante di Connessione/Scansione
        Button {
            id: btConnectButton
            width: parent.width
            height: 55
            text: " CONNETTI ALL'ESP32"

            background: Rectangle {
                color: btConnectButton.enabled ? "#007fff" : "#404050"
                radius: 8
            }
            contentItem: Text {
                text: btConnectButton.text
                color: "white"
                font.pixelSize: 16
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // Al click chiamiamo la funzione C++ startScan()
            onClicked: {
                ble.startScan()
            }
        }

        // Separatore visivo
        Rectangle {
            width: parent.width
            height: 1
            color: "#3f3f50"
        }

        // Pulsante Play e Stop sono disabilitati finché la connessione non è avvenuta

        // Pulsante  PLAY
        Button {
            id: playButton
            width: parent.width
            height: 70
            text: " PLAY"
            enabled: false

            background: Rectangle {
                color: playButton.enabled ? "#2ecc71" : "#204030"
                radius: 12
                opacity: playButton.enabled ? 1.0 : 0.4
            }
            contentItem: Text {
                text: playButton.text
                color: "white"
                font.pixelSize: 22
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // Al click, chiamiamo la funzione C++ sendPlay() per avviare la riproduzione
            onClicked: {
                ble.sendPlay()
            }
        }

        // Pulsante  STOP
        Button {
            id: stopButton
            width: parent.width
            height: 70
            text: " STOP"
            enabled: false

            background: Rectangle {
                color: stopButton.enabled ? "#e74c3c" : "#502020"
                radius: 12
                opacity: stopButton.enabled ? 1.0 : 0.4
            }
            contentItem: Text {
                text: stopButton.text
                color: "white"
                font.pixelSize: 22
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // Al click, chiamiamo la funzione C++ sendStop()
            // per stoppare la riproduzione
            onClicked: {
                ble.sendStop()
            }
        }
    }
}