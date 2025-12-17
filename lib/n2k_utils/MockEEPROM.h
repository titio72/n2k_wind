#ifndef _MOCK_EEPROM_H
#define _MOCK_EEPROM_H

#include <cstring>
#include <cmath>
#include <stdint.h>

/**
 * MockEEPROM - Complete EEPROM emulation for unit testing
 * Simulates EEPROM behavior with additional testing utilities
 */
class MockEEPROM
{
public:
    static const size_t DEFAULT_SIZE = 512;
    static const size_t MAX_SIZE = 4096;

    MockEEPROM()
    {
        // Static member initialization
        data_buffer = nullptr;
        initialized = false;
        write_count = 0;
        commit_count = 0;
    }

    /**
     * Initialize EEPROM with specified size
     */
    bool begin(size_t size = DEFAULT_SIZE)
    {
        if (!initialized)
        {
            if (size > MAX_SIZE)
                return false;

            eeprom_size = size;
            if (data_buffer == nullptr)
            {
                data_buffer = new uint8_t[MAX_SIZE];
                if (data_buffer == nullptr)
                    return false;
            }

            reset();
            initialized = true;
        }
        return true;
    }

    /**
     * Commit changes to EEPROM (no-op in mock, always succeeds)
     */
    bool commit()
    {
        if (!initialized)
            return false;

        commit_count++;
        return true;
    }

    /**
     * Read multiple bytes from EEPROM
     */
    size_t readBytes(int address, void *value, size_t len)
    {
        if (!initialized || !validate_range(address, len))
            return 0;

        memcpy(value, data_buffer + address, len);
        return len;
    }

    /**
     * Write multiple bytes to EEPROM
     */
    size_t writeBytes(int address, const void *value, size_t len)
    {
        if (!initialized || !validate_range(address, len) || value == nullptr)
            return 0;

        memcpy(data_buffer + address, value, len);
        write_count++;
        return len;
    }

    /**
     * Read single byte
     */
    uint8_t read(int address)
    {
        if (!initialized || !validate_range(address, 1))
            return 0xFF;

        return data_buffer[address];
    }

    /**
     * Read character
     */
    char readChar(int address)
    {
        if (!initialized || !validate_range(address, 1))
            return 0xFF;

        return data_buffer[address];
    }

    /**
     * Write single byte
     */
    void write(int address, uint8_t value)
    {
        if (initialized && validate_range(address, 1))
        {
            data_buffer[address] = value;
            write_count++;
        }
    }

    /**
     * Read 16-bit integer (little-endian)
     */
    uint16_t readUShort(int address)
    {
        if (!initialized || !validate_range(address, 2))
            return 0xFFFF;

        uint16_t value;
        memcpy(&value, &data_buffer[address], sizeof(uint16_t));
        return value;
    }

    /**
     * Write 16-bit integer (little-endian)
     */
    void writeUShort(int address, uint16_t value)
    {
        if (initialized && validate_range(address, 2))
        {
            memcpy(&data_buffer[address], &value, sizeof(uint16_t));
            write_count++;
        }
    }

    /**
     * Read signed 16-bit integer
     */
    int16_t readShort(int address)
    {
        return (int16_t)readUShort(address);
    }

    /**
     * Write signed 16-bit integer
     */
    void writeShort(int address, int16_t value)
    {
        writeUShort(address, (uint16_t)value);
    }

    /**
     * Read 32-bit integer (little-endian)
     */
    uint32_t readUInt(int address)
    {
        if (!initialized || !validate_range(address, 4))
            return 0xFFFFFFFF;

        uint32_t value;
        memcpy(&value, &data_buffer[address], sizeof(uint32_t));
        return value;
    }

    /**
     * Write 32-bit integer (little-endian)
     */
    void writeUInt(int address, uint32_t value)
    {
        if (initialized && validate_range(address, 4))
        {
            memcpy(&data_buffer[address], &value, sizeof(uint32_t));
            write_count++;
        }
    }

    /**
     * Read signed 32-bit integer
     */
    int32_t readInt(int address)
    {
        return (int32_t)readUInt(address);
    }

    /**
     * Write signed 32-bit integer
     */
    void writeInt(int address, int32_t value)
    {
        writeUInt(address, (uint32_t)value);
    }

    /**
     * Read 64-bit integer (little-endian)
     */
    uint64_t readULong64(int address)
    {
        if (!initialized || !validate_range(address, 8))
            return 0xFFFFFFFFFFFFFFFFULL;

        uint64_t value;
        memcpy(&value, &data_buffer[address], sizeof(uint64_t));
        return value;
    }

    /**
     * Write 64-bit integer (little-endian)
     */
    size_t writeULong64(int address, uint64_t value)
    {
        if (!initialized || !validate_range(address, 8))
            return 0;

        memcpy(&data_buffer[address], &value, sizeof(uint64_t));
        write_count++;
        return sizeof(uint64_t);
    }

    /**
     * Read float (32-bit IEEE 754)
     */
    float readFloat(int address)
    {
        if (!initialized || !validate_range(address, sizeof(float)))
            return NAN;

        float value;
        memcpy(&value, &data_buffer[address], sizeof(float));
        return value;
    }

    /**
     * Write float (32-bit IEEE 754)
     */
    void writeFloat(int address, float value)
    {
        if (initialized && validate_range(address, sizeof(float)))
        {
            memcpy(&data_buffer[address], &value, sizeof(float));
            write_count++;
        }
    }

    /**
     * Read double (64-bit IEEE 754)
     */
    double readDouble(int address)
    {
        if (!initialized || !validate_range(address, sizeof(double)))
            return NAN;

        double value;
        memcpy(&value, &data_buffer[address], sizeof(double));
        return value;
    }

    /**
     * Write double (64-bit IEEE 754)
     */
    void writeDouble(int address, double value)
    {
        if (initialized && validate_range(address, sizeof(double)))
        {
            memcpy(&data_buffer[address], &value, sizeof(double));
            write_count++;
        }
    }

    /**
     * Read string (null-terminated)
     */
    size_t readString(int address, char *buffer, size_t max_len)
    {
        if (!initialized || buffer == nullptr || max_len == 0)
            return 0;

        if (!validate_range(address, max_len))
            return 0;

        size_t len = 0;
        while (len < max_len - 1 && data_buffer[address + len] != '\0')
        {
            buffer[len] = data_buffer[address + len];
            len++;
        }
        buffer[len] = '\0';

        return len;
    }

    /**
     * Write string (null-terminated)
     */
    size_t writeString(int address, const char *str)
    {
        if (!initialized || str == nullptr)
            return 0;

        size_t len = strlen(str) + 1; // Include null terminator

        if (!validate_range(address, len))
            return 0;

        memcpy(&data_buffer[address], str, len);
        write_count++;
        return len;
    }

    template <typename T>
    T &get(int address, T &t)
    {
        if (validate_range(address, sizeof(T)) == false)
            return t;

        memcpy((uint8_t *)&t, data_buffer + address, sizeof(T));
        return t;
    }

    template <typename T>
    const T &put(int address, const T &t)
    {
        if (validate_range(address, sizeof(T)) == false)
            return t;

        memcpy(data_buffer + address, (const uint8_t *)&t, sizeof(T));
        return t;
    }

    // ============ Testing Utilities ============

    /**
     * Reset EEPROM to initial state (all 0xFF)
     */
    void reset()
    {
        if (data_buffer != nullptr)
        {
            memset(data_buffer, 0xFF, eeprom_size);
        }
        write_count = 0;
        commit_count = 0;
    }

    /**
     * Get current EEPROM size
     */
    size_t size()
    {
        return eeprom_size;
    }

    /**
     * Check if EEPROM is initialized
     */
    bool is_initialized()
    {
        return initialized;
    }

    /**
     * Get number of write operations performed
     */
    uint32_t get_write_count()
    {
        return write_count;
    }

    /**
     * Get number of commit operations performed
     */
    uint32_t get_commit_count()
    {
        return commit_count;
    }

    /**
     * Reset counters
     */
    void reset_counters()
    {
        write_count = 0;
        commit_count = 0;
    }

    /**
     * Dump raw EEPROM contents to buffer (for debugging)
     */
    void dump(uint8_t *output, size_t len)
    {
        if (!initialized || output == nullptr)
            return;

        size_t copy_len = (len < eeprom_size) ? len : eeprom_size;
        memcpy(output, data_buffer, copy_len);
    }

    /**
     * Get raw data pointer (for direct inspection)
     */
    const uint8_t *get_data()
    {
        return data_buffer;
    }

    /**
     * Set raw data (for test setup)
     */
    void set_data(int address, const uint8_t *data, size_t len)
    {
        if (initialized && validate_range(address, len))
        {
            memcpy(&data_buffer[address], data, len);
        }
    }

    /**
     * Verify EEPROM checksum (simple XOR of all bytes)
     */
    uint8_t compute_checksum()
    {
        if (!initialized)
            return 0;

        uint8_t checksum = 0;
        for (size_t i = 0; i < eeprom_size; i++)
        {
            checksum ^= data_buffer[i];
        }
        return checksum;
    }

    /**
     * Clear entire EEPROM to zero
     */
    void clear_to_zero()
    {
        if (data_buffer != nullptr)
        {
            memset(data_buffer, 0x00, eeprom_size);
        }
    }

    /**
     * Fill EEPROM with pattern for testing
     */
    void fill_pattern(uint8_t pattern)
    {
        if (data_buffer != nullptr)
        {
            memset(data_buffer, pattern, eeprom_size);
        }
    }

    /**
     * Cleanup (call at end of tests)
     */
    void end()
    {
        if (data_buffer != nullptr)
        {
            delete[] data_buffer;
            data_buffer = nullptr;
        }
        initialized = false;
        eeprom_size = 0;
    }

private:
    uint8_t *data_buffer;
    size_t eeprom_size;
    bool initialized;
    uint32_t write_count;
    uint32_t commit_count;

    /**
     * Validate address range
     */
    bool validate_range(int address, size_t len)
    {
        if (address < 0 || static_cast<size_t>(address) >= eeprom_size)
            return false;

        if (address + static_cast<int>(len) > static_cast<int>(eeprom_size))
            return false;

        return true;
    }
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
extern MockEEPROM mockEEPROM;
#endif

#endif // _MOCK_EEPROM_H