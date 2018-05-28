#include "GPIO.h"

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

GPIO::GPIO(std::string const& _key, Direction _d)
: key(_key), pin(-1), AIN(false), direction(_d)
{
	for (size_t i = 0u; i < AIN_pins_len; ++i) {
		auto pinfo = AIN_pins[i];
		if (pinfo.key == key) {
			AIN = true;
			pin = pinfo.pin;
			break;
		}
	}

	if (!AIN) {
		for (size_t i = 0u; i < GPIO_pins_len; ++i) {
			auto pinfo = GPIO_pins[i];
			if (pinfo.key == key) {
				pin = pinfo.pin;
				setup();
				break;
			}
		}
	}

	if (pin < 0)
		throw std::invalid_argument("invalid GPIO key");

	if (AIN && direction == Output)
		throw std::logic_error("Analogic Input GPIO cannot be used as Output");
}

GPIO::GPIO(int _pin, Direction _d)
: key(""), pin(_pin), direction(_d)
{
	if (pin > 0) {
		for (size_t i = 0u; i < GPIO_pins_len; ++i) {
			auto pinfo = GPIO_pins[i];
			if (pinfo.pin == pin) {
				key = pinfo.key;
				break;
			}
		}

		if (key.empty())
			throw std::invalid_argument("invalid GPIO pin");
		setup();
	} else if (pin < 0) {
		throw std::invalid_argument("invalid GPIO key");
	} else if (direction == Output) {
		throw std::logic_error("Analogic Input GPIO cannot be used as Output");
	}
}

GPIO::~GPIO()
{
	//set_direction(Input);

	std::string unexport_path(GPIO_path);
	unexport_path += "unexport";
	std::ofstream unexport_file(unexport_path);

	unexport_file << this->pin;
	unexport_file.close();
}

void GPIO::setup() const
{
	std::string export_path(GPIO_path);
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
	std::string direction_path(GPIO_path);
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

void GPIO::set(Value _v)
{
	if (this->direction == Direction::Input)
		throw std::logic_error("cannot write to GPIO with direction set as input");

	std::string value_path(GPIO_path);
	value_path += "gpio" + std::to_string(this->pin) + "/value";
	std::ofstream value_file(value_path);
	if (!value_file) {
		auto err_msg = "failure writing to gpio"+std::to_string(pin)+" value file";
		throw std::ofstream::failure(err_msg);
	}

	value_file << int(_v);
	value_file.close();
}

GPIO::Value GPIO::get() const
{
	int value = get_value();

	if (value > 0)
		return High;
	return Low;
}

double GPIO::get_value() const
{
	std::string value_path;
	if (!AIN) {
		value_path = GPIO_path;
		value_path += "gpio" + std::to_string(this->pin) + "/value";

	} else {
		value_path = AIN_path;
		value_path += "in_voltage" + std::to_string(this->pin) + "_raw";
	}

	std::ifstream value_file(value_path);
	if (!value_file) {
		auto err_msg = "failure reading from GPIO "+key+" value file";
		throw std::ifstream::failure(err_msg);
	}

	double value;
	value_file >> value;
	value_file.close();

	return (AIN ? (value/max_AIN_value) : value);
}
