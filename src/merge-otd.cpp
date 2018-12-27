#ifdef _MSC_VER
#pragma comment(lib, "shell32")
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <streambuf>
#include <string>

#include <nowide/args.hpp>
#include <nowide/cstdio.hpp>
#include <nowide/fstream.hpp>
#include <nowide/iostream.hpp>

#include "invisible.hpp"
#include "json.hpp"

const char *usage = u8"用法：\n\t%s 1.otd 2.otd [n.otd ...]\n";
const char *loadfilefail = u8"读取文件 %s 失败\n";
const char *mixedpostscript = u8"暂不支持混用 TrueType 和 PostScript 轮廓字体";

using json = nlohmann::json;

std::string LoadFile(char *u8filename) {
	static char u8buffer[4096];
	nowide::ifstream file(u8filename);
	if (!file) {
		snprintf(u8buffer, sizeof u8buffer, loadfilefail, u8filename);
		nowide::cerr << u8buffer << std::endl;
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
				point["x"] =
				    a * double(point["x"]) + b * double(point["y"]) + dx;
				point["y"] =
				    c * double(point["x"]) + d * double(point["y"]) + dy;
			}
	if (glyph.find("references") != glyph.end())
		for (auto &reference : glyph["references"]) {
			reference["x"] =
			    a * double(reference["x"]) + b * double(reference["y"]) + dx;
			reference["y"] =
			    c * double(reference["x"]) + d * double(reference["y"]) + dy;
		}
}

// copy referenced glyphs recursively
void CopyRef(json &glyph, json &base, json &ext) {
	if (glyph.find("references") != glyph.end())
		for (auto &r : glyph["references"]) {
			std::string name = r["glyph"];
			if (base["glyf"].find(name) == base["glyf"].end()) {
				base["glyf"][name] = ext["glyf"][name];
				CopyRef(base["glyf"][name], base, ext);
			}
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
				base["glyf"][name] = ext["glyf"][name];
				CopyRef(base["glyf"][name], base, ext);
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

int main(int argc, char *u8argv[]) {
	static char u8buffer[4096];
	nowide::args _{argc, u8argv};

	if (argc < 3) {
		snprintf(u8buffer, sizeof u8buffer, usage, u8argv[0]);
		nowide::cout << u8buffer << std::endl;
		return EXIT_FAILURE;
	}

	json base;
	bool basecff;
	try {
		auto s = LoadFile(u8argv[1]);
		base = json::parse(s);
	} catch (std::runtime_error) {
		return EXIT_FAILURE;
	}
	basecff = IsPostScriptOutline(base);
	RemoveBlankGlyph(base);

	for (int argi = 2; argi < argc; argi++) {
		json ext;
		try {
			auto s = LoadFile(u8argv[argi]);
			ext = json::parse(s);
		} catch (std::runtime_error) {
			return EXIT_FAILURE;
		}
		if (IsPostScriptOutline(ext) != basecff) {
			nowide::cerr << mixedpostscript << std::endl;
			return EXIT_FAILURE;
		}
		RemoveBlankGlyph(ext);
		MergeFont(base, ext);
	}

	std::string out = base.dump();
	FILE *outfile = nowide::fopen(u8argv[1], "wb");
	fwrite(out.c_str(), 1, out.size(), outfile);
	return 0;
}
