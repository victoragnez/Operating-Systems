#ifndef __POINT_H__
#define __POINT_H__

#include <math.h>

class POINT
{
	public:
	double x, y;
	POINT();
	POINT(double x, double y);
	POINT operator+(const POINT & a) const;
	POINT operator-(const POINT & a) const;
	POINT operator*(const double & a) const;
	POINT operator/(const double & a) const;
	double operator*(const POINT & a) const;
	double abs() const;
	double dist(const POINT & a) const;
};

#endif
