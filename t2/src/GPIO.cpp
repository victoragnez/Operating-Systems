#include "GPIO.h"

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

GPIO::GPIO(std::string const& _key, Direction _d)
: key(_key), pin(-1), direction(_d)
{
	for (unsigned int i = 0; i < pin_table_len; ++i) {
		auto pinfo = pins_table[i];
		if (pinfo.key == key) {
			pin = pinfo.pin;
			break;
		}
	}

	if (pin < 0) {
		throw std::invalid_argument("invalid GPIO key");
	}

	setup();
}

GPIO::GPIO(int _pin, Direction _d)
: key(""), pin(_pin), direction(_d)
{
	if (pin != 0) {
		for (unsigned int i = 0; i < pin_table_len; ++i) {
			auto pinfo = pins_table[i];
			if (pinfo.pin == pin) {
				key = pinfo.key;
				break;
			}
		}
	}

	if (pin != 0 && key.empty()) {
		throw std::invalid_argument("invalid GPIO pin");
	}

	setup();
}

GPIO::~GPIO()
{
	//set_direction(Input);

	std::string unexport_path(gpio_path);
	unexport_path += "unexport";
	std::ofstream unexport_file(unexport_path);

	unexport_file << this->pin;
	unexport_file.close();
}

void GPIO::setup() const
{
	std::string export_path(gpio_path);
	export_path += "export";
	std::ofstream export_file(export_path);
	if (!export_file) {
		auto err_msg = "failure writing to gpio export file";
		throw std::ofstream::failure(err_msg);
	}

	export_file << this->pin;
	export_file.close();

	// wait for export completion
	sleep_for(milliseconds(500));
	write_direction();
}

void GPIO::set_direction(Direction _d) 
{
	this->direction = _d;

	write_direction();
}

void GPIO::write_direction() const
{
	std::string direction_path(gpio_path);
	direction_path += "gpio" + std::to_string(this->pin) + "/direction";
	std::ofstream direction_file(direction_path);

	if (!direction_file) {
		auto err_msg = "failure writing to gpio"+std::to_string(pin)+" direction file";
		throw std::ofstream::failure(err_msg);
	}

	if (this->direction == Output)
		direction_file << "out";
	else
		direction_file << "in";
	direction_file.close();
}

void GPIO::set_value(Value _v)
{
	std::string value_path(gpio_path);
	value_path += "gpio" + std::to_string(this->pin) + "/value";
	std::ofstream value_file(value_path);
	if (!value_file) {
		auto err_msg = "failure writing to gpio"+std::to_string(pin)+" value file";
		throw std::ofstream::failure(err_msg);
	}

	value_file << _v;
	value_file.close();
}

GPIO::Value GPIO::get_value() const
{
	std::string value_path(gpio_path);
	value_path += "gpio" + std::to_string(this->pin) + "/value";
	std::ifstream value_file(value_path);
	if (!value_file) {
		auto err_msg = "failure reading from gpio"+std::to_string(pin)+" value file";
		throw std::ifstream::failure(err_msg);
	}

	int value;
	value_file >> value;
	value_file.close();

	if (value > 0)
		return High;
	return Low;
}
