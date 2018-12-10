/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    Modified by Lars Baumgaertner to act as bluetooth LoRa modem

*/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "modem.h"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

String test="";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {

        bool fullString = false;
        std::string rxValue = pCharacteristic->getValue();

        String cmd = rxValue.c_str();
        cmd.trim();
        cmd.toUpperCase();

        if(cmd.equals("END")){
            Serial.println("Vergleich erfolgreich");
            fullString = true;
        }
        else {
           test = test + cmd; 
        }

        if (fullString)
        {
            Serial.println("*********");
            Serial.print("Received Value: ");
            Serial.print(test);
            Serial.print("***********");

            handleCommand(test);
            
            test = "";
            delay(10);
        }
    }    
    
};

void setup()
{
    //Serial.begin(115200);
    Serial.begin(9600);

    // Create the BLE Device
    BLEDevice::init("RF95 Modem Service");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);

    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE);

    pRxCharacteristic->setCallbacks(new MyCallbacks());

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");

    modem_setup();
    initRF95();
}

void ble_print(String output)
{
    String outputneu = output;
    while(outputneu.length() != 0){
        if (outputneu.length() > 20)
        {
            String temp = outputneu.substring(0, 20);
            outputneu = outputneu.substring(20);
            pTxCharacteristic->setValue((uint8_t *)temp.c_str(), temp.length());
            pTxCharacteristic->notify();
            delay(10);
        }
        else
        {
            pTxCharacteristic->setValue((uint8_t *)outputneu.c_str(), outputneu.length());
            pTxCharacteristic->notify();
            outputneu = "";
            delay(10);
        }
    }

    
    delay(10);
    
}

void loop()
{
    modem_loop_tick();

    if (deviceConnected)
    {
        /*ble_print("hallo\n");
        txValue++;
        delay(10); // bluetooth stack will go into congestion, if too many packets are sent
        ble_print("hallo 2\n");*/
        delay(10);
    }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}