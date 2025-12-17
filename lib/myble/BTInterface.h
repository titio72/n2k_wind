#ifndef _BTINTERFACE_H
#define _BTINTERFACE_H

#include <vector>
#include <string>
#include <stdint.h>
#include <Utils.h>

class Configuration;

class ABBLEWriteCallback {
public:
    virtual void on_write(int handle, const char* value) = 0;
};

class ABBLESetting {
public:
    ABBLESetting(const char* n, const char* id): name(n), c_uuid(id) {}

    std::string name;
    std::string c_uuid;
};

class ABBLEField {
public:
    ABBLEField(const char* n, const char* id): name(n), c_uuid(id) {}

    std::string name;
    std::string c_uuid;
};

class InternalBLEState
{
public:
    virtual void init(const char* name, const char* uuid, ABBLEWriteCallback* c) = 0;
    virtual void setup(const std::vector<ABBLEField> &fields, const std::vector<ABBLESetting> &settings) = 0;
    virtual void begin() = 0;
    virtual void change_device_name(const char *n) = 0;
    virtual const char* get_device_name() = 0;
    virtual void end() = 0;

    virtual void set_field_value(int handle, const char *value) = 0;
    virtual void set_field_value(int handle, uint16_t value) = 0;
    virtual void set_field_value(int handle, void *value, int len) = 0;
    virtual ByteBuffer get_field_value(int handle) = 0;
    virtual void set_setting_value(int handle, const char *value) = 0;
    virtual void set_setting_value(int handle, int value) = 0;
 };

class BTInterface {
    public:
        BTInterface(const char* uuid, const char* device_name, ABBLEWriteCallback* cmd_cback, InternalBLEState* internalState = nullptr);
        ~BTInterface();
        void setup();
        void begin();
        void loop(unsigned long ms);

        int add_setting(const char* name, const char* uuid);
        int add_field(const char* name, const char* uuid);

        void set_setting_value(int handle, const char* value);
        void set_setting_value(int handle, int value);
        void set_field_value(int handle, uint16_t value);
        void set_field_value(int handle, const char* value);
        void set_field_value(int handle, void* value, int len);
        ByteBuffer get_field_value(int handle);

        void set_device_name(const char* name);
        const char* get_device_name();

    private:
        bool internalStateOwned;
        InternalBLEState* state;
        ABBLEWriteCallback* writeCallback;

        std::vector<ABBLEField> fields;
        std::vector<ABBLESetting> settings;

        bool init;
};

#endif
