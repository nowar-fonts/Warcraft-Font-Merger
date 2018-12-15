#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

#include "invisible.hpp"
#include "json.hpp"

const wchar_t *usage = L"用法：\n\t%ls 1.otd 2.otd [n.otd ...]\n";
const wchar_t *loadfilefail = L"读取文件 %ls 失败\n";

using json = nlohmann::json;

std::string LoadFile(wchar_t *filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		fwprintf(stderr, loadfilefail, filename);
		throw std::runtime_error("failed to load file");
	}
	std::string result{std::istreambuf_iterator<char>(file),
	                   std::istreambuf_iterator<char>()};
	return result;
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

int main(void) {
	int argc;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	setlocale(LC_ALL, "");

	if (argc < 3) {
		wprintf(usage, argv[0]);
	}

	json base;
	try {
		auto s = LoadFile(argv[1]);
		base = json::parse(s);
	} catch (std::runtime_error) {
		return EXIT_FAILURE;
	}
	RemoveBlankGlyph(base);

	for (int argi = 2; argi < argc; argi++) {
		json ext;
		try {
			auto s = LoadFile(argv[argi]);
			ext = json::parse(s);
		} catch (std::runtime_error) {
			return EXIT_FAILURE;
		}
		RemoveBlankGlyph(ext);
		MergeFont(base, ext);
	}

	std::ofstream outfile(argv[1]);
	outfile << base.dump() << std::endl;
	return 0;
}
