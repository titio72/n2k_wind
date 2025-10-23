#include <Arduino.h>
#include <Log.h>
#include "BTInterface.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLECharacteristic.h>

class MyServerCBack : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        Log::trace("[BLE] Connected to client\n");
        Log::trace("[BLE] Readvertising\n");
        pServer->getAdvertising()->start();
    };

    void onDisconnect(BLEServer *pServer)
    {
        Log::trace("[BLE] Disconneted from client\n");
    }
};

class MyCInCBack : public BLECharacteristicCallbacks
{
public:
    MyCInCBack(ABBLEWriteCallback **_c, std::vector<ABBLESetting> &_s) : c(_c), settings(_s) {}

    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        static char v[16];
        strcpy(v, pCharacteristic->getValue().c_str());
        //Log::tracex("BLE", "Characteristic write", "UUID {%s} value {%s}", pCharacteristic->getUUID().toString().c_str(), v);

        int i = 0;
        for (i = 0; i < settings.size(); i++)
            if (settings[i].uuid.equals(pCharacteristic->getUUID()))
                break;
        if (i < settings.size() && *c)
        {
            (*c)->on_write(i, v);
        }
    }

private:
    ABBLEWriteCallback **c;
    std::vector<ABBLESetting> &settings;
};

BTInterface::BTInterface(const char *uuid, const char *name) : serviceUUID(uuid), callback(NULL), pServer(NULL), pService(NULL)
{
    if (name)
        strcpy(device_name, name);
    else
        strcpy(device_name, "AB");

    listener = new MyCInCBack(&callback, settings);
    serverCBack = new MyServerCBack();
}

BTInterface::~BTInterface()
{
    BLEDevice::deinit();
    for (int i = 0; i < settings.size(); i++)
        delete characteristicsSettings[i];
    for (int i = 0; i < fields.size(); i++)
        delete characteristicsFields[i];
    delete pServer;
    delete pService;
    delete listener;
    delete serverCBack;
}

int BTInterface::add_setting(const char *name, const char *uuid)
{
    ABBLESetting s(name, uuid);
    settings.push_back(s);
    return settings.size() - 1;
}

int BTInterface::add_field(const char *name, const char *uuid)
{
    ABBLEField f(name, uuid);
    fields.push_back(f);
    return fields.size() - 1;
}

void BTInterface::set_field_value(int handle, const char *value)
{
    if (handle >= 0 && handle < characteristicsFields.size())
    {
        BLECharacteristic *c = characteristicsFields[handle];
        c->setValue(value);
        c->indicate();
    }
}

void BTInterface::set_field_value(int handle, uint16_t value)
{
    if (handle>=0 && handle<characteristicsFields.size())
    {
        BLECharacteristic* c = characteristicsFields[handle];
        c->setValue(value);
        c->indicate();
    }
}

void BTInterface::set_field_value(int handle, void *value, int len)
{
    if (handle >= 0 && handle < characteristicsFields.size())
    {
        BLECharacteristic *c = characteristicsFields[handle];
        c->setValue((uint8_t *)value, len);
        c->indicate();
    }
}

void BTInterface::set_setting_value(int handle, const char *value)
{
    if (handle >= 0 && handle < characteristicsSettings.size())
    {
        BLECharacteristic *c = characteristicsSettings[handle];
        c->setValue(value);
    }
}

void BTInterface::set_setting_value(int handle, int value)
{
    if (handle >= 0 && handle < characteristicsSettings.size())
    {
        static char temp[16];
        itoa(value, temp, 10);
        BLECharacteristic *c = characteristicsSettings[handle];
        c->setValue(temp);
    }
}

void createSettingCharacteristics(BLEService *pService, BLEUUID uuid, BLECharacteristic **c, BLECharacteristicCallbacks *cback)
{
    Log::tracex("BLE", "Creating bool characteristic", "UUID {%s} service {%s}", uuid.toString().c_str(), pService->getUUID().toString().c_str());
    *c = pService->createCharacteristic(uuid,
                                        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
    (*c)->setReadProperty(true);
    (*c)->setWriteProperty(true);
    (*c)->setCallbacks(cback);
}

void createFieldCharacteristic(BLEService *pService, BLEUUID uuid, BLECharacteristic **c)
{
    Log::tracex("BLE", "Creating numeric characteristic", "UUID {%s} service {%s}", uuid.toString().c_str(), pService->getUUID().toString().c_str());
    *c = pService->createCharacteristic(uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_INDICATE);
    (*c)->setIndicateProperty(true);
    (*c)->setReadProperty(true);
    (*c)->addDescriptor(new BLE2902());
}

void BTInterface::setup()
{
    Log::tracex("BLE", "Setup", "device {%s}", device_name);
    BLEDevice::init(device_name);
    BLEDevice::setMTU(128);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(serverCBack);
    pService = pServer->createService(serviceUUID);
    Log::tracex("BLE", "Loading characteristics");
    for (int i = 0; i < settings.size(); i++)
    {
        ABBLESetting &s = settings[i];
        BLECharacteristic *c;
        createSettingCharacteristics(pService, s.uuid, &c, listener);
        characteristicsSettings.push_back(c);
    }
    for (int i = 0; i < fields.size(); i++)
    {
        ABBLEField &s = fields[i];
        BLECharacteristic *c;
        createFieldCharacteristic(pService, s.uuid, &c);
        characteristicsFields.push_back(c);
    }
    Log::tracex("BLE", "Loaded", "Settings {%d} Fields {%d}", settings.size(), fields.size());
}

void BTInterface::begin()
{
    Log::tracex("BLE", "Starting BLE", "device {%s}", device_name);
    pService->start();
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pServer->getAdvertising()->addServiceUUID(serviceUUID);
    pServer->getAdvertising()->start();
}

void BTInterface::loop(unsigned long ms)
{
}

void BTInterface::set_device_name(const char *name)
{
    strcpy(device_name, name);
    if (pServer)
    {
        pServer->getAdvertising()->stop();
        esp_err_t errRc = ::esp_ble_gap_set_device_name(name);
        if (errRc != ESP_OK)
        {
            Log::tracex("BLE", "Change device name", "error {%d} name {%s}", errRc, name);
        }
        else
        {
            Log::tracex("BLE", "Change device name", "name {%s}", name);
        }
        pServer->getAdvertising()->start();
    }
}
