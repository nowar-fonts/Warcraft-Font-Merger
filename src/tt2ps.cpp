#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

#include "point.hpp"
#include "tt2ps.h"

using nlohmann::json;

static void TransformInPlace(json &glyph, double a, double b, double c,
                             double d, double dx, double dy)
{
	if (glyph.find("contours") != glyph.end())
		for (auto &contour : glyph["contours"])
			for (auto &point : contour)
			{
				double x = point["x"];
				double y = point["y"];
				point["x"] = a * x + c * y + dx;
				point["y"] = b * x + d * y + dy;
			}
	// we have dereferenced the glyph.
}

static json Dereference(json glyph, const json &glyf)
{
	if (glyph.find("references") == glyph.end())
		return glyph;
	glyph["contours"] = json::array();

	for (const auto &ref : glyph["references"])
	{
		json target = glyf[std::string(ref["glyph"])];
		if (target.find("references") != target.end())
			target = Dereference(std::move(target), glyf);
		TransformInPlace(target, ref["a"], ref["b"], ref["c"], ref["d"],
		                 ref["x"], ref["y"]);
		std::copy(target["contours"].begin(), target["contours"].end(),
		          std::back_inserter(glyph["contours"]));
	}

	glyph.erase("references");
	return glyph;
}

namespace ConstructCffPath
{
void Move(json &cubicContour, Point p0)
{
	cubicContour.push_back(p0.ToJson(true));
}

void Line(json &cubicContour, Point p1)
{
	size_t length = cubicContour.size();
	if (length >= 2 && cubicContour[length - 2]["on"])
	{
		// 2 lines, merge if the are collinear.
		Point p2 = cubicContour[length - 1];
		Point p3 = cubicContour[length - 2];
		double a = p3.y - p1.y;
		double b = p1.x - p3.x;
		double c = p1.y * p3.x - p1.x * p3.y;
		double distance = abs(a * p2.x + b * p2.y + c) / sqrt(a * a + b * b);
		if (distance < 1)
			cubicContour.erase(length - 1);
	}
	cubicContour.push_back(p1.ToJson(true));
}

void Curve(json &cubicContour, Point p1, Point p2, Point p3)
{
	cubicContour.push_back(p1.ToJson(false));
	cubicContour.push_back(p2.ToJson(false));
	cubicContour.push_back(p3.ToJson(true));
}

// merge the last point and the first point
void Finish(json &cubicContour)
{
	size_t length = cubicContour.size();
	// cubicContour[0] and cubicContour[-1] are implicitly on-curve
	if (length >= 2 &&
	    abs(Point(cubicContour[0]) - cubicContour[length - 1]) < 1)
	{
		cubicContour.erase(length - 1);
		length--;
	}
	if (length >= 3 && cubicContour[1]["on"] && cubicContour[length - 1]["on"])
	{
		// 2 lines, merge if the are collinear.
		Point p1 = cubicContour[1];
		Point p2 = cubicContour[0];
		Point p3 = cubicContour[length - 1];
		double a = p3.y - p1.y;
		double b = p1.x - p3.x;
		double c = p1.y * p3.x - p1.x * p3.y;
		double distance = abs(a * p2.x + b * p2.y + c) / sqrt(a * a + b * b);
		if (distance < 1)
			cubicContour.erase(0);
	}
}
} // namespace ConstructCffPath

static void SimpleCurve(Point *p, json &cubicContour)
{
	ConstructCffPath::Curve(cubicContour, (p[0] + 2 * p[1]) / 3,
	                        (2 * p[1] + p[2]) / 3, p[2]);
}

/* reimplemented afdko’s ttread::combinePair

   test if curve pair should be combined.
   if true, combine curve and save to cubicContour, else save the first segment.
   return 1 if curves combined else 0.
*/
static int CombinePair(Point *p, json &cubicContour)
{
	double a = p[3].y - p[1].y;
	double b = p[1].x - p[3].x;
	if ((a != 0 || p[1].y != p[2].y) && (b != 0 || p[1].x != p[2].x))
	{
		// Not a vertical or horizontal join...
		double absq = a * a + b * b;
		if (absq != 0)
		{
			double sr = a * (p[2].x - p[1].x) + b * (p[2].y - p[1].y);
			if ((sr * sr) / absq < 1)
			{
				// ...that is straight...
				if ((a * (p[0].x - p[1].x) + b * (p[0].y - p[1].y) < 0) ==
				    (a * (p[4].x - p[1].x) + b * (p[4].y - p[1].y) < 0))
				{
					// ...and without inflexion...
					double d0 = (p[2].x - p[0].x) * (p[2].x - p[0].x) +
					            (p[2].y - p[0].y) * (p[2].y - p[0].y);
					double d1 = (p[4].x - p[2].x) * (p[4].x - p[2].x) +
					            (p[4].y - p[2].y) * (p[4].y - p[2].y);
					if (d0 <= 3 * d1 && d1 <= 3 * d0)
					{
						// ...and small segment length ratio; combine curve
						ConstructCffPath::Curve(cubicContour,
						                        (4 * p[1] - p[0]) / 3,
						                        (4 * p[3] - p[4]) / 3, p[4]);
						p[0] = p[4];
						return 1;
					}
				}
			}
		}
	}

	// save first curve then replace it by second curve
	SimpleCurve(p, cubicContour);
	p[0] = p[2];
	p[1] = p[3];
	p[2] = p[4];

	return 0;
}

/* reimplemented afdko’s ttread::callbackApproxPath

   state    sequence        points
            0=off,1=on      accumulated
   0        1               0
   1        1 0             0-1
   2        1 0 0           0-3 (p[2] is mid-point of p[1] and p[3])
   3        1 0 1           0-2
   4        1 0 1 0         0-3
*/
static json ConvertApprox(json glyph, const json &glyf)
{
	glyph = Dereference(std::move(glyph), glyf);
	glyph.erase("instructions");
	glyph.erase("LTSH_yPel");

	for (json &contour : glyph["contours"])
	{
		if (contour.size() <= 1)
			continue;

		json cubicContour = json::array();
		Point p[6];             // points: 0,2,4-on, 1,3-off, 5-tmp
		json::const_iterator q; // current point
		auto beg = contour.cbegin();
		auto end = --contour.cend();
		size_t cnt = contour.size();
		int state = 0;

		// save initial on-curve point
		if ((*beg)["on"])
		{
			q = beg;
			p[0] = *q;
		}
		else if ((*end)["on"])
		{
			q = end;
			p[0] = *q;
		}
		else
		{
			// start at mid-point
			q = beg;
			cnt++;
			p[0] = (Point(*beg) + *end) / 2;
		}
		ConstructCffPath::Move(cubicContour, p[0]);

		while (cnt--)
		{
			// advance to next point, in reversed direction
			q = (q == beg) ? end : q - 1;

			if ((*q)["on"])
			{
				// on-curve
				switch (state)
				{
				case 0:
					if (cnt > 0)
					{
						p[0] = *q;
						ConstructCffPath::Line(cubicContour, p[0]);
						// stay in state 0
					}
					break;
				case 1:
					p[2] = *q;
					state = 3;
					break;
				case 2:
					p[4] = *q;
					state = CombinePair(p, cubicContour) ? 0 : 3;
					break;
				case 3:
					SimpleCurve(p, cubicContour);
					if (cnt > 0)
					{
						p[0] = *q;
						ConstructCffPath::Line(cubicContour, p[0]);
					}
					state = 0;
					break;
				case 4:
					p[4] = *q;
					state = CombinePair(p, cubicContour) ? 0 : 3;
					break;
				}
			}
			else
			{
				// off-curve
				switch (state)
				{
				case 0:
					p[1] = *q;
					state = 1;
					break;
				case 1:
					p[3] = *q;
					p[2] = (p[1] + p[3]) / 2;
					state = 2;
					break;
				case 2:
					p[5] = *q;
					p[4] = (p[3] + p[5]) / 2;
					if (CombinePair(p, cubicContour))
					{
						p[1] = p[5];
						state = 1;
					}
					else
					{
						p[3] = p[5];
						state = 4;
					}
					break;
				case 3:
					p[3] = *q;
					state = 4;
					break;
				case 4:
					p[5] = *q;
					p[4] = (p[3] + p[5]) / 2;
					if (CombinePair(p, cubicContour))
					{
						p[1] = p[5];
						state = 1;
					}
					else
					{
						p[3] = p[5];
						state = 2;
					}
					break;
				}
			}
		}

		// finish up
		switch (state)
		{
		case 2:
			p[3] = *q;
			p[2] = (p[1] + p[3]) / 2;
			[[fallthrough]];
		case 3:
		case 4:
			SimpleCurve(p, cubicContour);
			break;
		}

		ConstructCffPath::Finish(cubicContour);
		contour = cubicContour;
	}

	return glyph;
}

json Tt2Ps(const json &glyf, bool roundToInt)
{
	json glyfCubic;
	for (const auto &[name, glyph] : glyf.items())
	{
		glyfCubic[name] = ConvertApprox(glyph, glyf);
		if (roundToInt)
			RoundInPlace(glyfCubic[name]);
	}
	return glyfCubic;
}
