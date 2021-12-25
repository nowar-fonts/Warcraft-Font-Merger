#ifdef _MSC_VER
#pragma comment(lib, "shell32")
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <streambuf>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <nowide/args.hpp>
#include <nowide/cstdio.hpp>
#include <nowide/fstream.hpp>
#include <nowide/iostream.hpp>

#include <clipp/clipp.h>

#include "invisible.hpp"
#include "merge-name.h"
#include "ps2tt.h"
#include "tt2ps.h"

using json = nlohmann::json;

std::string LoadFile(const std::string &u8filename) {
	static char u8buffer[4096];
	nowide::ifstream file(u8filename);
	if (!file) {
		nowide::cerr << u8"读取文件 " << u8filename << u8" 失败\n" << std::endl;
		throw std::runtime_error("failed to load file");
	}
	std::string result{std::istreambuf_iterator<char>(file),
	                   std::istreambuf_iterator<char>()};
	return result;
}

bool IsPostScriptOutline(json &font) {
	return font.find("CFF_") != font.end() || font.find("CFF2") != font.end();
}

// x' = a x + b y + dx
// y' = c x + d y + dy
void Transform(json &glyph, double a, double b, double c, double d, double dx,
               double dy) {
	glyph["advanceWidth"] = round(a * double(glyph["advanceWidth"]));
	if (glyph.find("advanceHeight") != glyph.end()) {
		glyph["advanceHeight"] = round(d * double(glyph["advanceHeight"]));
		glyph["verticalOrigin"] = round(d * double(glyph["verticalOrigin"]));
	}
	if (glyph.find("contours") != glyph.end())
		for (auto &contour : glyph["contours"])
			for (auto &point : contour) {
				double x = point["x"];
				double y = point["y"];
				point["x"] = int(a * x + b * y + dx);
				point["y"] = int(c * x + d * y + dy);
			}
	if (glyph.find("references") != glyph.end())
		for (auto &reference : glyph["references"]) {
			double x = reference["x"];
			double y = reference["y"];
			reference["x"] = int(a * x + b * y + dx);
			reference["y"] = int(c * x + d * y + dy);
		}
}

// move referenced glyphs recursively
void MoveRef(json &glyph, json &base, json &ext) {
	if (glyph.find("references") != glyph.end())
		for (auto &r : glyph["references"]) {
			std::string name = r["glyph"];
			if (base["glyf"].find(name) == base["glyf"].end()) {
				base["glyf"][name] = std::move(ext["glyf"][name]);
				MoveRef(base["glyf"][name], base, ext);
			}
		}
}

bool IsGidOrCid(const std::string &name) {
	return (name.length() >= 6 && name.substr(0, 5) == "glyph") ||
	       (name.length() >= 4 && name.substr(0, 3) == "cid");
}

void FixGlyphName(json &font, const std::string &prefix) {
	for (auto &[u, n] : font["cmap"].items()) {
		std::string name = n;
		if (IsGidOrCid(name))
			n = prefix + name;
	}
	std::vector<std::string> mod;
	for (auto &[n, g] : font["glyf"].items()) {
		if (IsGidOrCid(n))
			mod.push_back(n);
		if (g.find("references") != g.end()) {
			auto &ref = g["references"];
			for (auto &d : ref)
				if (IsGidOrCid(d["glyph"]))
					d["glyph"] = prefix + std::string(d["glyph"]);
		}
	}
	auto &glyf = font["glyf"];
	for (auto &n : mod) {
		glyf[prefix + n] = glyf[n];
		glyf.erase(n);
	}
}

void MergeFont(json &base, json &ext) {
	double baseUpm = base["head"]["unitsPerEm"];
	double extUpm = ext["head"]["unitsPerEm"];

	if (baseUpm != extUpm) {
		for (auto &glyph : ext["glyf"])
			Transform(glyph, baseUpm / extUpm, 0, 0, baseUpm / extUpm, 0, 0);
	}

	for (json::iterator it = ext["cmap"].begin(); it != ext["cmap"].end();
	     ++it) {
		if (base["cmap"].find(it.key()) == base["cmap"].end()) {
			std::string name = *it;
			base["cmap"][it.key()] = ext["cmap"][it.key()];
			if (base["glyf"].find(name) == base["glyf"].end()) {
				base["glyf"][name] = std::move(ext["glyf"][name]);
				MoveRef(base["glyf"][name], base, ext);
			}
		}
	}
}

void RemoveBlankGlyph(json &font) {
	static UnicodeInvisible invisible;
	std::vector<std::string> eraseList;

	for (json::iterator it = font["cmap"].begin(); it != font["cmap"].end();
	     ++it) {
		if (!invisible.CanBeInvisible(std::stoi(it.key()))) {
			std::string name = it.value();
			auto &glyph = font["glyf"][name];
			if (glyph.find("contours") == glyph.end() &&
			    glyph.find("references") == glyph.end())
				eraseList.push_back(it.key());
		}
	}

	for (auto g : eraseList) {
		std::string name = font["cmap"][g];
		font["cmap"].erase(g);
		if (std::find_if(font["cmap"].begin(), font["cmap"].end(),
		                 [name](auto v) { return v == name; }) ==
		    font["cmap"].end())
			font["glyf"].erase(name);
	}
}

json MergeCodePage(std::vector<json> cpranges) {
	json result = json::object();

	for (auto &cprange : cpranges)
		for (auto &[k, v] : cprange.items()) {
			// std::cerr << cprange << ' ' << k << std::endl;
			if (result.find(k) != result.end())
				result[k] = result[k] || v;
			else
				result[k] = v;
		}

	return result;
}

int main(int argc, char *u8argv[]) {
	nowide::args _{argc, u8argv};

	std::string outputPath;

	std::string baseFileName;
	std::vector<std::string> appendFileNames;

	auto cli = (clipp::option("-o", "--output") &
	                clipp::value("output otd path", outputPath),
	            clipp::value("base.otd", baseFileName),
	            clipp::repeatable(clipp::value("append.otd", appendFileNames)));
	if (!clipp::parse(argc, u8argv, cli) || appendFileNames.empty()) {
		nowide::cout << "用法：" << std::endl
		             << clipp::usage_lines(cli, "merge-otd") << std::endl;
		return EXIT_FAILURE;
	}

	std::vector<json> ulCodePageRanges1, ulCodePageRanges2;
	std::vector<json> nametables;

	json base;
	bool basecff;
	try {
		auto s = LoadFile(baseFileName);
		base = json::parse(s);
	} catch (const std::runtime_error &) {
		return EXIT_FAILURE;
	}
	basecff = IsPostScriptOutline(base);
	RemoveBlankGlyph(base);
	nametables.push_back(base["name"]);

	for (const std::string &name : appendFileNames) {
		json ext;
		try {
			auto s = LoadFile(name);
			ext = json::parse(s);
		} catch (std::runtime_error &) {
			return EXIT_FAILURE;
		}
		bool extcff = IsPostScriptOutline(ext);
		if (basecff && !extcff) {
			ext["glyf"] = Tt2Ps(ext["glyf"]);
		} else if (!basecff && extcff) {
			ext["glyf"] = Ps2Tt(ext["glyf"]);
		}
		RemoveBlankGlyph(ext);
		nametables.push_back(ext["name"]);
		FixGlyphName(ext, name + std::string(":"));
		MergeFont(base, ext);
		if (ext.find("OS_2") != ext.end()) {
			auto &OS_2 = ext["OS_2"];
			if (OS_2.find("ulCodePageRange1") != OS_2.end())
				ulCodePageRanges1.push_back(OS_2["ulCodePageRange1"]);
			if (OS_2.find("ulCodePageRange2") != OS_2.end())
				ulCodePageRanges2.push_back(OS_2["ulCodePageRange2"]);
		}
	}

	if (base.find("OS_2") != base.end()) {
		auto &OS_2 = base["OS_2"];
		if (OS_2.find("ulCodePageRange1") != OS_2.end())
			ulCodePageRanges1.push_back(OS_2["ulCodePageRange1"]);
		if (OS_2.find("ulCodePageRange2") != OS_2.end())
			ulCodePageRanges2.push_back(OS_2["ulCodePageRange2"]);

		OS_2["ulCodePageRange1"] = MergeCodePage(ulCodePageRanges1);
		OS_2["ulCodePageRange2"] = MergeCodePage(ulCodePageRanges2);
	}

	base["name"] = MergeNameTable(nametables);

	std::string out = base.dump();
	FILE *outfile = nowide::fopen(
	    outputPath.empty() ? baseFileName.c_str() : outputPath.c_str(), "wb");
	fwrite(out.c_str(), 1, out.size(), outfile);
	return 0;
}
