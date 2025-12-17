#ifndef MOCK_PORT_H
#define MOCK_PORT_H

#include "Ports.h"
#include <string.h>
#include <queue>

/**
 * Mock implementation of Port for testing
 * Allows simulating serial port behavior without actual hardware
 */
class MockPort : public Port
{
public:
    // Constructor with port name
    MockPort(const char* name = "MOCK_PORT") 
        : Port(name),
          is_open_flag(false),
          open_count(0),
          close_count(0),
          read_count(0),
          listen_count(0),
          total_bytes_simulated(0)
    {
        memset(last_read_data, 0, sizeof(last_read_data));
    }
    
    virtual ~MockPort() {}
    
    // Simulate receiving data
    void simulate_data(const char* data)
    {
        if (!data) return;
        
        // Queue the data for reading
        for (size_t i = 0; i < strlen(data); i++)
        {
            input_queue.push(data[i]);
        }
        total_bytes_simulated += strlen(data);
    }
    
    // Simulate receiving a complete line
    void simulate_line(const char* line)
    {
        if (!line) return;
        
        simulate_data(line);
        simulate_data("\r\n");
    }
    
    // Simulate multiple lines
    void simulate_lines(const char** lines, int count)
    {
        for (int i = 0; i < count; i++)
        {
            simulate_line(lines[i]);
        }
    }
    
    void set_error_on_read(bool error)
    {
        should_error_on_read = error;
    }
    
    // Query mock state
    int get_open_count() const { return open_count; }
    int get_close_count() const { return close_count; }
    int get_read_count() const { return read_count; }
    int get_listen_count() const { return listen_count; }
    unsigned long get_total_bytes_simulated() const { return total_bytes_simulated; }
    
    const char* get_last_read_data() const { return last_read_data; }
    
    bool is_input_queue_empty() const { return input_queue.empty(); }
    
    void reset_counters()
    {
        open_count = 0;
        close_count = 0;
        read_count = 0;
        listen_count = 0;
        total_bytes_simulated = 0;
        memset(last_read_data, 0, sizeof(last_read_data));
    }
    
    void clear_input_queue()
    {
        while (!input_queue.empty())
        {
            input_queue.pop();
        }
    }
    
    // public for test purposes
    virtual bool _is_open() override
    {
        return is_open_flag;
    }
    
    // public for test purposes
    virtual int _read(bool &nothing_to_read, bool &error) override
    {
        read_count++;

        // Simulate error condition
        if (should_error_on_read)
        {
            error = true;
            return -1;
        }
        
        // Simulate nothing to read
        if (input_queue.empty())
        {
            nothing_to_read = true;
            return -1;
        }
        
        // Return next character from queue
        char c = input_queue.front();
        input_queue.pop();
        
        // Store for inspection
        strncat(last_read_data, &c, 1);
        if (strlen(last_read_data) >= sizeof(last_read_data) - 1)
        {
            memset(last_read_data, 0, sizeof(last_read_data));
        }
        
        nothing_to_read = false;
        error = false;
        return (int)c;
    }
    
protected:
    // Implementation of pure virtual methods from Port
    virtual void _open() override
    {
        open_count++;
        is_open_flag = true;
    }
    
    virtual void _close() override
    {
        close_count++;
        is_open_flag = false;
    }

private:
    bool is_open_flag;
    bool should_error_on_read = false;
    
    int open_count;
    int close_count;
    int read_count;
    int listen_count;
    unsigned long total_bytes_simulated;
    
    char last_read_data[256];
    
    std::queue<char> input_queue;
};

#endif // MOCK_PORT_H