#define _USE_MATH_DEFINES

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <utility>
#include <vector>

#include "point.hpp"
#include "ps2tt.h"

using nlohmann::json;

using Coeff2 = std::array<Point, 4>;
using Coeff1 = std::array<double, 4>;
using Segment = std::array<Point, 4>;
using SegmentQ = std::array<Point, 3>;
using Solution = std::vector<double>;

namespace ConstructTtPath
{
void Move(json &quadContour, Point p0)
{
	quadContour.push_back(p0.ToJson(true));
}

void Line(json &quadContour, Point p1)
{
	size_t length = quadContour.size();
	if (length >= 2 && quadContour[length - 2]["on"])
	{
		// 2 lines, merge if the are collinear.
		Point p2 = quadContour[length - 1];
		Point p3 = quadContour[length - 2];
		double a = p3.y - p1.y;
		double b = p1.x - p3.x;
		double c = p1.y * p3.x - p1.x * p3.y;
		double distance = abs(a * p2.x + b * p2.y + c) / sqrt(a * a + b * b);
		if (distance < 1)
			quadContour.erase(length - 1);
	}
	quadContour.push_back(p1.ToJson(true));
}

void Curve(json &quadContour, Point p1, Point p2)
{
	size_t length = quadContour.size();
	Point p0 = quadContour[length - 1];
	if (abs((p0 + p2) / 2 - p1) < 1)
		return Line(quadContour, p2);
	if (length >= 2 && !quadContour[length - 2]["on"])
	{
		// 2 curves, remove on-curve point if it is the center point
		Point off = quadContour[length - 2];
		if (abs((p1 + off) / 2 - p0) < 1)
			quadContour.erase(length - 1);
	}
	quadContour.push_back(p1.ToJson(false));
	quadContour.push_back(p2.ToJson(true));
}

// merge the last point and the first point
void Finish(json &quadContour)
{
	size_t length = quadContour.size();
	// quadContour[0] and quadContour[-1] are implicitly on-curve
	if (length >= 2 && abs(Point(quadContour[0]) - quadContour[length - 1]) < 1)
	{
		quadContour.erase(length - 1);
		length--;
	}
	if (length <= 2)
		return;
	if (quadContour[1]["on"] != quadContour[length - 1]["on"])
		// first point is tangent point, do nothing
		return;
	Point p1 = quadContour[1];
	Point p2 = quadContour[0];
	Point p3 = quadContour[length - 1];
	if (quadContour[1]["on"])
	{
		// 2 lines, merge if the are collinear.
		double a = p3.y - p1.y;
		double b = p1.x - p3.x;
		double c = p1.y * p3.x - p1.x * p3.y;
		double distance = abs(a * p2.x + b * p2.y + c) / sqrt(a * a + b * b);
		if (distance < 1)
			quadContour.erase(0);
	}
	else if (abs((p1 + p3) / 2 - p2) < 1)
		// 2 curves, remove on-curve point if it is the center point
		quadContour.erase(0);
}
} // namespace ConstructTtPath

/* point(t) = p1 (1-t)³ + c1 t (1-t)² + c2 t² (1-t) + p2 t³
            = a t³ + b t² + c t + d
*/
inline Coeff2 CalcPowerCoefficients(Segment s)
{
	auto [p1, c1, c2, p2] = s;
	Point a = (p2 - p1) + 3 * (c1 - c2);
	Point b = 3 * (p1 + c2) - 6 * c1;
	Point c = 3 * (c1 - p1);
	Point d = p1;
	return {d, c, b, a};
}

inline Point CalcPoint(double t, Coeff2 coeff)
{
	auto [d, c, b, a] = coeff;
	return ((a * t + b) * t + c) * t + d;
}

inline Point CalcPointQuad(double t, Coeff2 coeff)
{
	[[maybe_unused]] auto [c, b, a, _] = coeff;
	return (a * t + b) * t + c;
}

inline Point CalcPointDerivative(double t, Coeff2 coeff)
{
	[[maybe_unused]] auto [_, c, b, a] = coeff;
	return (3 * a * t + 2 * b) * t + c;
}

// a x² + b x + c
inline Solution QuadSolve(Coeff1 coeff)
{
	using T = Solution;
	[[maybe_unused]] auto [c, b, a, _] = coeff;
	if (!a)
		return b ? T{-c / b} : T{};
	double delta = b * b - 4 * a * c;
	return delta > 0
	           ? T{(-b - sqrt(delta)) / (2 * a), (-b + sqrt(delta)) / (2 * a)}
	           : (delta ? T{-b / (2 * a)} : T{});
}

inline double curt(double x)
{
	return x < 0 ? -pow(-x, 1.0 / 3) : pow(x, 1.0 / 3);
}

// a x³ + b x² + c x + d
inline Solution CubicSolve(Coeff1 coeff)
{
	auto [d, c, b, a] = coeff;
	if (!a)
		return QuadSolve(coeff);
	// solve using Cardan's method
	// http://www.nickalls.org/dick/papers/maths/cubic1993.pdf
	// (doi:10.2307/3619777)
	double xn = -b / (3 * a); // point of symmetry x coordinate
	double yn =
	    ((a * xn + b) * xn + c) * xn + d; // point of symmetry y coordinate
	double deltaSq = (b * b - 3 * a * c) / (9 * a * a); // delta^2
	double hSq = 4 * a * a * pow(deltaSq, 3);           // h^2
	double d3 = yn * yn - hSq;
	if (d3 > 0)
		// 1 real root
		return {xn + curt((-yn + sqrt(d3)) / (2 * a)) +
		        curt((-yn - sqrt(d3)) / (2 * a))};
	else if (d3 == 0)
		// 2 real roots
		return {xn - 2 * curt(yn / (2 * a)), xn + curt(yn / (2 * a))};
	else
	{
		// 3 real roots
		double theta = acos(-yn / sqrt(hSq)) / 3;
		double delta = sqrt(deltaSq);
		return {xn + 2 * delta * cos(theta),
		        xn + 2 * delta * cos(theta + M_PI * 2 / 3),
		        xn + 2 * delta * cos(theta + M_PI * 4 / 3)};
	}
}

/* f(t) = p1 (1-t)² + 2 c1 t (1-t) + p2 t²
        = a t^2 + b t + c, t in [0, 1],
   a = p1 + p2 - 2 * c1
   b = 2 * (c1 - p1)
   c = p1

   The distance between given point and quadratic curve is equal to
   sqrt((f(t) - p)²), so these expression has zero derivative by t at
   points where (f'(t), (f(t) - point)) = 0.

   Substituting quadratic curve as f(t) one could obtain a cubic equation
   e3 t³ + e2 t² + e1 t + e0 = 0 with following coefficients:
   e3 = 2 a²
   e2 = 3 a b
   e1 = b² + 2 a (c - p)
   e0 = (c - p) b

   One of the roots of the equation from [0, 1], or t = 0 or t = 1 is a value of
   t at which the distance between given point and quadratic Bezier curve has
minimum.
   So to find the minimal distance one have to just pick the minimum value
of
   the distance on set {t = 0, t = 1, t is root of the equation from [0, 1] }.
*/
double MinDistanceToQuad(Point p, SegmentQ s)
{
	auto [p1, c1, p2] = s;
	Point a = p1 + p2 - 2 * c1;
	Point b = 2 * (c1 - p1);
	Point c = p1;
	Coeff1 e = {(c - p) * b, b * b + 2 * a * (c - p), 3 * a * b, 2 * a * a};
	Solution _candidates = CubicSolve(e);
	Solution candidates = {0, 1};
	std::copy_if(_candidates.begin(), _candidates.end(),
	             std::back_inserter(candidates),
	             [](double t) { return t > 0 && t < 1; });

	double minDistance = 1e9;
	for (auto t : candidates)
	{
		double distance = abs(CalcPointQuad(t, {c, b, a, {}}) - p);
		if (distance < minDistance)
			minDistance = distance;
	}
	return minDistance;
}

SegmentQ ProcessSegment(double t1, double t2, Coeff2 coeff)
{
	Point f1 = CalcPoint(t1, coeff);
	Point f2 = CalcPoint(t2, coeff);
	Point f1d = CalcPointDerivative(t1, coeff);
	Point f2d = CalcPointDerivative(t2, coeff);

	// normal vector: p -- tangent vector
	auto normal = [](Point p) { return Point{-p.y, p.x}; };

	double d = f1d * normal(f2d);
	if (abs(d) < 1e-6)
		return {f1, (f1 + f2) / 2, f2};
	else
		return {f1, (f1d * (f2 * normal(f2d)) + f2d * (f1 * -normal(f1d))) / d,
		        f2};
}

bool IsSegmentApproximationClose(double tmin, double tmax, Coeff2 coeff,
                                 SegmentQ s, double error)
{
	int n = 4;
	double dt = (tmax - tmin) / n;
	for (double t = tmin + dt; t < tmax - 1e-6; t += dt)
	{
		Point p = CalcPoint(t, coeff);
		if (MinDistanceToQuad(p, s) > error)
			return false;
	}
	return true;
}

/* Split cubic bézier curve into two cubic curves, see details here:
   https://math.stackexchange.com/questions/877725
*/
static std::pair<Segment, Segment> SubdivideCubic(double t, Segment s)
{
	auto [p1, c1, c2, p2] = s;
	Point b = (1 - t) * p1 + t * c1;
	Point _ = (1 - t) * c1 + t * c2;
	Point f = (1 - t) * c2 + t * p2;
	Point c = (1 - t) * b + t * _;
	Point e = (1 - t) * _ + t * f;
	Point d = (1 - t) * c + t * e;
	return {{p1, b, c, d}, {d, e, f, p2}};
}

/* Find inflection points on a cubic curve, algorithm is similar to this one:
   http://www.caffeineowl.com/graphics/2d/vectorial/cubic-inflexion.html
*/
static Solution SolveInflections(Segment s)
{
	auto [p1, c1, c2, p2] = s;
	double p = -(p2.x * (p1.y - 2 * c1.y + c2.y)) +
	           c2.x * (2 * p1.y - 3 * c1.y + p2.y) +
	           p1.x * (c1.y - 2 * c2.y + p2.y) -
	           c1.x * (p1.y - 3 * c2.y + 2 * p2.y);
	double q = p2.x * (p1.y - c1.y) + 3 * c2.x * (-p1.y + c1.y) +
	           c1.x * (2 * p1.y - 3 * c2.y + p2.y) -
	           p1.x * (2 * c1.y - 3 * c2.y + p2.y);
	double r =
	    c2.x * (p1.y - c1.y) + p1.x * (c1.y - c2.y) + c1.x * (-p1.y + c2.y);

	Solution result_ = QuadSolve({r, q, p, 0});
	Solution result;
	std::copy_if(result_.begin(), result_.end(), std::back_inserter(result),
	             [](double t) { return t > 1e-6 && t < 1 - 1e-6; });
	std::sort(result.begin(), result.end());
	return result;
}

// approximate cubic segment w/o inflections
static void ApproximateSimpleSegment(Segment s, json &quadContour, double error)
{
	auto [p1, c1, c2, p2] = s;
	Coeff2 pc = CalcPowerCoefficients(s);

	std::vector<SegmentQ> apprx;
	for (int segCount = 1; segCount <= 4; segCount++)
	{
		apprx = {};
		double dt = 1.0 / segCount;
		bool isClose = true;
		for (int i = 0; i < segCount; i++)
		{
			SegmentQ seg = ProcessSegment(i * dt, (i + 1) * dt, pc);
			isClose = isClose && IsSegmentApproximationClose(
			                         i * dt, (i + 1) * dt, pc, seg, error);
			apprx.push_back(seg);
		}
		if (segCount == 1 && ((apprx[0][1] - p1) * (c1 - p1) < -1e-6 ||
		                      (apprx[0][1] - p2) * (c2 - p2) < -1e-6))
			// approximation concave, while the curve is convex (or vice versa)
			continue;
		if (isClose)
			break;
	}

	for (auto seg : apprx)
		ConstructTtPath::Curve(quadContour, seg[1], seg[2]);
}

static void ApproximateCurve(Segment s, json &quadContour, double error)
{
	Solution inflections = SolveInflections(s);
	if (!inflections.size())
		return ApproximateSimpleSegment(s, quadContour, error);
	Segment curve = s;
	double prev = 0;
	for (double i : inflections)
	{
		auto split = SubdivideCubic(1 - (1 - i) / (1 - prev), curve);
		ApproximateSimpleSegment(split.first, quadContour, error);
		curve = split.second;
		prev = i;
	}
	ApproximateSimpleSegment(curve, quadContour, error);
}

static json Convert(json glyph, double error)
{
	glyph.erase("stemH");
	glyph.erase("stemV");
	glyph.erase("hintMasks");
	glyph.erase("contourMasks");

	for (json &contour : glyph["contours"])
	{
		if (contour.size() <= 1)
			continue;

		json quadContour = json::array();
		Segment s;
		auto beg = contour.cbegin();
		auto end = --contour.cend();
		size_t cnt = contour.size();

		// save initial on-curve point
		json::const_iterator q = beg;
		s[0] = *q;
		ConstructTtPath::Move(quadContour, s[0]);

		// advance to next point, in reversed direction
		auto advance = [beg, end](auto q) { return (q == beg) ? end : q - 1; };

		while (cnt > 0)
		{
			q = advance(q);
			if ((*q)["on"])
			{
				s[0] = *q;
				ConstructTtPath::Line(quadContour, s[0]);
				cnt--;
			}
			else
			{
				s[1] = *q;
				q--; // it’s safe here
				s[2] = *q;
				q = advance(q);
				s[3] = *q;
				ApproximateCurve(s, quadContour, error);
				s[0] = s[3];
				cnt -= 3;
			}
		}

		ConstructTtPath::Finish(quadContour);
		contour = quadContour;
	}

	return glyph;
}

json Ps2Tt(const json &glyf, double errorBound)
{
	json glyfQuad;
	for (const auto &[name, glyph] : glyf.items())
	{
		glyfQuad[name] = Convert(glyph, errorBound);
		RoundInPlace(glyfQuad[name]);
	}
	return glyfQuad;
}
