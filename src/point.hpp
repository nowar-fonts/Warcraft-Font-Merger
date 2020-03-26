#pragma once

#include <cmath>
#include <nlohmann/json.hpp>

struct Point
{
	double x;
	double y;

	Point() : x(0), y(0)
	{
	}
	Point(double x, double y) : x(x), y(y)
	{
	}
	Point(const nlohmann::json &p) : x(p["x"]), y(p["y"])
	{
	}
	nlohmann::json ToJson(bool on)
	{
		return {{"x", x}, {"y", y}, {"on", on}};
	}
};

inline Point operator+(Point a, Point b)
{
	return {a.x + b.x, a.y + b.y};
}

inline Point operator-(Point a, Point b)
{
	return {a.x - b.x, a.y - b.y};
}

inline Point operator-(Point a)
{
	return {-a.x, -a.y};
}

inline Point operator/(Point a, double b)
{
	return {a.x / b, a.y / b};
}

inline Point operator*(Point a, double b)
{
	return {a.x * b, a.y * b};
}

inline Point operator*(double a, Point b)
{
	return {a * b.x, a * b.y};
}

// dot
inline double operator*(Point a, Point b)
{
	return a.x * b.x + a.y * b.y;
}

inline double abs(Point a)
{
	return sqrt(a * a);
}

inline void RoundInPlace(nlohmann::json &glyph)
{
	for (auto &contour : glyph["contours"])
		for (auto &point : contour)
		{
			point["x"] = int(round(double(point["x"])));
			point["y"] = int(round(double(point["y"])));
		}
}
