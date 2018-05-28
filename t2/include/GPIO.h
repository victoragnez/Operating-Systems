#ifndef __GPIO_H__
#define __GPIO_H__

#include <chrono>
#include <thread>
#include <string>
#include <fstream>

class GPIO
{
public:
    enum Direction
    {
        Input = 0,
        Output = 1
    };

    enum Value
    {
        Low = 0,
        High = 1
    };

    GPIO(std::string const& _key, Direction _d);
    GPIO(int _pin, Direction _d);
    ~GPIO();

    void set_value(Value _v);
    Value get_value() const;

    void set_direction(Direction _d);
    Direction get_direction() const { return direction; }
    int get_pin() const { return pin; }
    std::string get_key() const { return key; }


private:
    std::string key;
    int pin;
    Direction direction;    

    static constexpr const char* gpio_path = "/sys/class/gpio/";

    typedef struct pins_t { 
		const char *key;
		int pin;
	} pins_t;

    static const pins_t pins_table[];
    static const unsigned int pin_table_len;

    void write_direction() const;
    void setup() const;
};

#endif
