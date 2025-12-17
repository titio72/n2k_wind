
#include <Log.h>
#include "BTInterface.h"
#include <string>
#include <stdint.h>
#include <Utils.h>

#ifndef NATIVE
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLECharacteristic.h>
#include <BLEUUID.h>

class InternalBLEStateImpl
    : public InternalBLEState,
      public BLECharacteristicCallbacks,
      public BLEServerCallbacks
{
private:
    BLEServer *pServer = nullptr;
    BLEService *pService = nullptr;
    std::vector<BLECharacteristic *> characteristicsSettings;
    std::vector<BLECharacteristic *> characteristicsFields;
    ABBLEWriteCallback *clientWriteCallback = nullptr;

    std::string name = "";
    std::string uuid = "";

public:
    InternalBLEStateImpl()
    {
    }

    ~InternalBLEStateImpl()
    {
    }

    
    void init(const char* name, const char* uuid, ABBLEWriteCallback* c)
    {
        this->name = name;
        this->uuid = uuid;
        this->clientWriteCallback = c;
    }

    // implements the BLECharacteristicCallbacks interface
    void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        if (clientWriteCallback == nullptr)
            return;

        static char v[256];
        // bounded copy of incoming value to local buffer
        strncpy(v, pCharacteristic->getValue().c_str(), sizeof(v) - 1);
        v[sizeof(v) - 1] = '\0';
        Log::tracex("BLE", "Characteristic write", "UUID {%s} value {%s}", pCharacteristic->getUUID().toString().c_str(), v);

        int i = 0;
        for (i = 0; i < characteristicsSettings.size(); i++)
        {
            BLEUUID uuid(characteristicsSettings[i]->getUUID());
            if (uuid.equals(pCharacteristic->getUUID()))
                break;
        }
        if (i < characteristicsSettings.size())
        {
            clientWriteCallback->on_write(i, v);
        }
    }

    // implements the BLEServerCallbacks interface
    void onConnect(BLEServer *pServer)
    {
        Log::trace("[BLE] Connected to client\n");
        Log::trace("[BLE] Readvertising\n");
        pServer->getAdvertising()->start();
    }

    void onDisconnect(BLEServer *pServer)
    {
        Log::trace("[BLE] Disconneted from client\n");
    }
    // end of BLEServerCallbacks interface

    void setup(const std::vector<ABBLEField> &fields, const std::vector<ABBLESetting> &settings)
    {
        Log::tracex("BLE", "Setup", "device {%s}", name.c_str());
        BLEDevice::init(name);
        BLEDevice::setMTU(128);
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(this);
        pService = pServer->createService(uuid.c_str());
        Log::tracex("BLE", "Loading characteristics");
        for (int i = 0; i < settings.size(); i++)
        {
            const ABBLESetting &s = settings.at(i);
            BLECharacteristic *c;
            createSettingCharacteristics(pService, s.c_uuid.c_str(), &c, this);
            characteristicsSettings.push_back(c);
        }
        for (int i = 0; i < fields.size(); i++)
        {
            const ABBLEField &s = fields.at(i);
            BLECharacteristic *c;
            createFieldCharacteristic(pService, s.c_uuid.c_str(), &c);
            characteristicsFields.push_back(c);
        }
        Log::tracex("BLE", "Loaded", "Settings {%d} Fields {%d}", settings.size(), fields.size());
    }

    void end()
    {
        BLEDevice::deinit();
    }

    void begin()
    {
        Log::tracex("BLE", "Starting BLE", "device {%s}", name.c_str());
        pService->start();
        BLESecurity *pSecurity = new BLESecurity();
        pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
        pSecurity->setCapability(ESP_IO_CAP_NONE);
        pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
        pServer->getAdvertising()->addServiceUUID(uuid.c_str());
        pServer->getAdvertising()->start();
    }

    void set_field_value(int handle, const char *value)
    {
        if (handle >= 0 && handle < characteristicsFields.size())
        {
            BLECharacteristic *c = characteristicsFields[handle];
            c->setValue(value);
            c->indicate();
        }
    }

    void set_field_value(int handle, uint16_t value)
    {
        if (handle >= 0 && handle < characteristicsFields.size())
        {
            BLECharacteristic *c = characteristicsFields[handle];
            c->setValue(value);
            c->indicate();
        }
    }

    void set_field_value(int handle, void *value, int len)
    {
        if (handle >= 0 && handle < characteristicsFields.size())
        {
            BLECharacteristic *c = characteristicsFields[handle];
            c->setValue((uint8_t *)value, len);
            c->indicate();
        }
    }

    ByteBuffer get_field_value(int handle)
    {
        if (handle >= 0 && handle < characteristicsFields.size())
        {
            BLECharacteristic *c = characteristicsFields[handle];
            return ByteBuffer(c->getData(), c->getLength());
        }
        return ByteBuffer(0);
    }

    void set_setting_value(int handle, const char *value)
    {
        if (handle >= 0 && handle < characteristicsSettings.size())
        {
            BLECharacteristic *c = characteristicsSettings[handle];
            c->setValue(value);
        }
    }

    void set_setting_value(int handle, int value)
    {
        if (handle >= 0 && handle < characteristicsSettings.size())
        {
            static char temp[16];
            itoa(value, temp, 10);
            BLECharacteristic *c = characteristicsSettings[handle];
            c->setValue(temp);
        }
    }

    void createSettingCharacteristics(BLEService *pService, const char *uuid, BLECharacteristic **c, BLECharacteristicCallbacks *cback)
    {
        Log::tracex("BLE", "Creating bool characteristic", "UUID {%s} service {%s}", uuid, pService->getUUID().toString().c_str());
        *c = pService->createCharacteristic(uuid,
                                            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
        (*c)->setReadProperty(true);
        (*c)->setWriteProperty(true);
        (*c)->setCallbacks(cback);
    }

    void createFieldCharacteristic(BLEService *pService, const char *uuid, BLECharacteristic **c)
    {
        Log::tracex("BLE", "Creating numeric characteristic", "UUID {%s} service {%s}", uuid, pService->getUUID().toString().c_str());
        *c = pService->createCharacteristic(uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_INDICATE);
        (*c)->setIndicateProperty(true);
        (*c)->setReadProperty(true);
        (*c)->addDescriptor(new BLE2902());
    }

    void change_device_name(const char *n)
    {
        // trim to max 15 chars
        name = std::string(n).substr(0, 15);
        if (pServer)
        {
            pServer->getAdvertising()->stop();
            esp_err_t errRc = ::esp_ble_gap_set_device_name(name.c_str());
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

    const char *get_device_name()
    {
        return name.c_str();
    }
};
#define USE_REAL_BLE_IMPLEMENTATION
#endif

BTInterface::BTInterface(const char *uuid, const char *name, ABBLEWriteCallback* cmd_cback, InternalBLEState *internalState) : init(false)
{
    internalStateOwned = false;
    #ifdef USE_REAL_BLE_IMPLEMENTATION
    if (internalState == nullptr)
    {
        internalStateOwned = true;
        internalState = new InternalBLEStateImpl();
    }
    #endif
    state = internalState;
    if (state)
    {
        state->init(name, uuid, cmd_cback);
    }
}

BTInterface::~BTInterface()
{
    if (state)
    {
        state->end();
        #ifdef USE_REAL_BLE_IMPLEMENTATION
        if (internalStateOwned)
            delete state;
        #endif
        state = nullptr;
    }
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
    if (state)
        state->set_field_value(handle, value);
}

void BTInterface::set_field_value(int handle, uint16_t value)
{
    if (state)
        state->set_field_value(handle, value);
}

void BTInterface::set_field_value(int handle, void *value, int len)
{
    if (state)
        state->set_field_value(handle, value, len);
}

void BTInterface::set_setting_value(int handle, const char *value)
{
    if (state)
        state->set_setting_value(handle, value);
}

void BTInterface::set_setting_value(int handle, int value)
{
    if (state)
        state->set_setting_value(handle, value);
}

ByteBuffer BTInterface::get_field_value(int handle)
{
    if (state)
        return state->get_field_value(handle);
    return ByteBuffer(0);
}

void BTInterface::setup()
{
    if (!init)
    {
        init = true;
        if (state)
            state->setup(fields, settings);
    }
}

void BTInterface::begin()
{
    if (init && state)
        state->begin();
}

void BTInterface::loop(unsigned long milli_seconds)
{
}

void BTInterface::set_device_name(const char *name)
{
    if (state)
        state->change_device_name(name);
}

const char *BTInterface::get_device_name()
{
    if (state)
        return state->get_device_name();
    return nullptr;
}
