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
public:
    MyServerCBack(): connections(0)
    {
        BLEServerCallbacks();
    }

    void onConnect(BLEServer *pServer)
    {
        Log::trace("[BLE] Connected to client {%d}\n", connections+1);
        Log::trace("[BLE] Readvertising\n");
        pServer->getAdvertising()->start();
        connections++;
    };

    void onDisconnect(BLEServer *pServer)
    {
        Log::trace("[BLE] Disconneted from client {%d}\n", connections-1);
        connections--;
    }

    bool is_connected()
    {
        return connections;
    }

    int n_connections()
    {
        return connections;
    }

private:
    int connections;
};

class MyCInCBack : public BLECharacteristicCallbacks
{
public:
    MyCInCBack(ABBLEWriteCallback** _c, std::vector<ABBLESetting> &_s): c(_c), settings(_s) {}

    virtual void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        static char v[16];
        strcpy(v, pCharacteristic->getValue().c_str());
        Log::trace("[BLE] Client sent value {%s}\n", v);

        int i = 0;
        for (i = 0; i<settings.size(); i++) if (settings[i].uuid.equals(pCharacteristic->getUUID())) break;
        if (i<settings.size() && *c)
        {
            (*c)->on_write(i, v);
        }
    }
private:
    ABBLEWriteCallback** c;
    std::vector<ABBLESetting> &settings;
};

class WriteCallbackWrapper: public ABBLEWriteCallback
{
public:
    WriteCallbackWrapper(setting_write_callback callback): c(callback) {}

    virtual void on_write(int h, const char* v)
    {
        if (c)
        {
            c(h, v);
        }
    }

private:
    setting_write_callback c;
};

BTInterface::BTInterface(const char* uuid, const char *name): serviceUUID(uuid), callback(NULL), callback_wrapper(NULL)
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
    for (int i = 0; i<settings.size(); i++) delete characteristicsSettings[i];
    for (int i = 0; i<fields.size(); i++) delete characteristicsFields[i];
    delete pServer;
    delete pService;
    delete listener;
    delete serverCBack;
    if (callback_wrapper) delete callback_wrapper;
}

void BTInterface::set_write_callback(setting_write_callback cback)
{
    if (callback_wrapper) delete callback_wrapper;
    callback_wrapper = cback?new WriteCallbackWrapper(cback):NULL;
    callback = callback_wrapper;
}

void BTInterface::add_setting(const char* name, const char* uuid)
{
    ABBLESetting s(name, uuid);
    settings.push_back(s);
}

void BTInterface::add_field(const char* name, const char* uuid)
{
    ABBLEField f(name, uuid);
    fields.push_back(f);
}

void BTInterface::set_field_value(int handle, const char* value)
{
    if (handle>=0 && handle<characteristicsFields.size())
    {
        BLECharacteristic* c = characteristicsFields[handle];
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

void BTInterface::set_field_value(int handle, void* value, int len)
{
    if (handle>=0 && handle<characteristicsFields.size())
    {
        BLECharacteristic* c = characteristicsFields[handle];
        c->setValue((uint8_t*)value, len);
        c->indicate();
    }
}

void BTInterface::set_setting_value(int handle, const char* value)
{
    if (handle>=0 && handle<characteristicsSettings.size())
    {
        BLECharacteristic* c = characteristicsSettings[handle];
        c->setValue(value);
    }
}

void BTInterface::set_setting_value(int handle, int value)
{
    if (handle>=0 && handle<characteristicsSettings.size())
    {
        static char temp[16]; itoa(value, temp, 10);
        BLECharacteristic* c = characteristicsSettings[handle];
        c->setValue(temp);
    }
}

void createSettingCharacteristics(BLEService* pService, BLEUUID uuid, BLECharacteristic** c, BLECharacteristicCallbacks* cback)
{
    Log::trace("[BLE] Creating bool characteristic {%s} for {%s}\n", uuid.toString().c_str(), pService->getUUID().toString().c_str());
    *c = pService->createCharacteristic( uuid,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
    (*c)->setReadProperty(true);
    (*c)->setWriteProperty(true);
    (*c)->setCallbacks(cback);
}

void createFieldCharacteristic(BLEService* pService, BLEUUID uuid, BLECharacteristic** c)
{
    Log::trace("[BLE] Creating numeric characteristic {%s} for {%s}\n", uuid.toString().c_str(), pService->getUUID().toString().c_str());
    *c = pService->createCharacteristic( uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_INDICATE);
    (*c)->setIndicateProperty(true);
    (*c)->setReadProperty(true);
    (*c)->addDescriptor(new BLE2902());
}

void BTInterface::setup()
{
    Log::trace("[BLE] Setting up BLE device {%s}\n", device_name);
    BLEDevice::init(device_name);
    BLEDevice::setMTU(128);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(serverCBack);
    pService = pServer->createService(serviceUUID);
    Log::trace("[BLE] Loading characteristics\n");
    for (int i = 0; i<settings.size(); i++)
    {
        ABBLESetting &s = settings[i];
        BLECharacteristic* c;
        createSettingCharacteristics(pService, s.uuid, &c, listener);
        characteristicsSettings.push_back(c);
    }
    for (int i = 0; i<fields.size(); i++)
    {
        ABBLEField &s = fields[i];
        BLECharacteristic* c;
        createFieldCharacteristic(pService, s.uuid, &c);
        characteristicsFields.push_back(c);
    }
    Log::trace("[BLE] Loaded {Settings %d / Fieds %d} characteristics\n", settings.size(), fields.size());
}

void BTInterface::begin()
{
    Log::trace("[BLE] Starting BLE device {%s}\n", device_name);
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

bool BTInterface::is_connected()
{
    return ((MyServerCBack*)serverCBack)->is_connected();
}

int BTInterface::clients()
{
    return ((MyServerCBack*)serverCBack)->n_connections();
}