#ifndef __GPIO_H__
#define __GPIO_H__

#include <chrono>
#include <thread>
#include <string>
#include <sstream>
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

    GPIO(std::string const& _key, Direction _d=Input);
    GPIO(int _pin, Direction _d=Input);
    ~GPIO();

    void set(Value _v);
    Value get() const;
    double get_value() const;

    void set_direction(Direction _d);
    Direction get_direction() const { return direction; }
    int get_pin() const { return pin; }
    std::string get_key() const { return key; }


private:
    std::string key;
    int pin;
    bool AIN;
    Direction direction;    

    static constexpr const char* GPIO_path = "/sys/class/gpio/";
    static constexpr const char* AIN_path = "/sys/bus/iio/devices/iio:device0/";
    static constexpr const double max_AIN_value = 4095;

    typedef struct pins_t { 
		const char *key;
		int pin;
	} pins_t;

    static const pins_t GPIO_pins[];
    static const size_t GPIO_pins_len;
    static const pins_t AIN_pins[];
    static const size_t AIN_pins_len;

    void write_direction() const;
    void setup() const;
};

#endif
