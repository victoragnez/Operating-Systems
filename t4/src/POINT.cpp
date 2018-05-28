#include "POINT.h"

POINT::POINT(){}

POINT::POINT(double x, double y):x(x), y(y){}

POINT POINT::operator+(const POINT & a) const
{
	return POINT(x + a.x, y + a.y);
}

POINT POINT::operator-(const POINT & a) const
{
	return POINT(x - a.x, y - a.y);
}

POINT POINT::operator*(const double & a) const
{
	return POINT(x * a, y * a);
}

POINT POINT::operator/(const double & a) const
{
	return POINT(x / a, y / a);
}
double POINT::operator*(const POINT & a) const
{
	return x*a.x + y*a.y;
}
double POINT::abs() const
{
	return sqrt((*this) * (*this));
}
double POINT::dist(const POINT & a) const
{
	return ((*this) - a).abs();
}
