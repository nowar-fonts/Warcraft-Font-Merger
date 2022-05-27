#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iterator>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include "merge-name.h"

using namespace std;
using json = nlohmann::json;

namespace LicenseString {
const char *Apache =
    "This Font Software is licensed under the Apache License, Version 2.0. "
    "This Font Software is distributed on an \"AS IS\" BASIS, WITHOUT "
    "WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the "
    "Apache License for the specific language, permissions and limitations "
    "governing your use of this Font Software.";
const char *GPL =
    "This Font Software is licensed under the GNU General Public License, "
    "either version 3 of the License, or (at your option) any later version. "
    "This font is distributed in the hope that it will be useful, but WITHOUT "
    "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or "
    "FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for "
    "more details.";
const char *LGPL =
    "This Font Software is licensed under the GNU Lesser General Public "
    "License, either version 3 of the License, or (at your option) any later "
    "version. This font is distributed in the hope that it will be useful, but "
    "WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser "
    "General Public License for more details.";
const char *OFL =
    "This Font Software is licensed under the SIL Open Font License, Version "
    "1.1. This Font Software is distributed on an \"AS IS\" BASIS, WITHOUT "
    "WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the "
    "SIL Open Font License for the specific language, permissions and "
    "limitations governing your use of this Font Software.";
} // namespace LicenseString

namespace LicenseUrlString {
const char *Apache = "https://www.apache.org/licenses/LICENSE-2.0";
const char *GPL = "https://www.gnu.org/copyleft/gpl.html";
const char *LGPL = "https://www.gnu.org/copyleft/lesser.html";
const char *OFL = "https://scripts.sil.org/OFL";
} // namespace LicenseUrlString

struct License {
	// share alike
	bool GPL;
	bool LGPL;
	bool OFL;
	// bool UFL;

	// non-share-alike
	bool Apache;
	// bool BSD;
	// bool MIT;

	explicit operator bool() const { return GPL || LGPL || OFL || Apache; }
};

enum class Platform {
	Unicode = 0,
	Macintosh = 1,
	Windows = 3,
};

enum class Encoding {
	UnicodeBMP = 1,
	Unicode = 10,

	MacRoman = 1,
};

enum class Language {
	en_US = 0x0409, // English (United States)
	en_GB = 0x0809, // English (United Kingdom)
	zh_CN = 0x0804, // Chinese (People’s Republic of China)
	zh_HK = 0x0C04, // Chinese (Hong Kong S.A.R.)
	zh_MO = 0x1404, // Chinese (Macao S.A.R.)
	zh_SG = 0x1004, // Chinese (Singapore)
	zh_TW = 0x0404, // Chinese (Taiwan)

	MacEnglish = 0,
};

map<Language, Language> LanguageFallback{
    {Language::en_GB, Language::en_US}, {Language::zh_CN, Language::en_US},
    {Language::zh_SG, Language::zh_CN}, {Language::zh_TW, Language::en_US},
    {Language::zh_HK, Language::zh_TW}, {Language::zh_MO, Language::zh_TW},
};

enum class NameId {
	Copyright = 0,
	Family = 1,
	Subfamily = 2,
	UniqueIdentifier = 3,
	FullName = 4,
	Version = 5,
	PostScript = 6,
	Trademark = 7,
	Manufacturer = 8,
	Designer = 9,
	Description = 10,
	VendorUrl = 11,
	DesignerUrl = 12,
	LicenseDescription = 13,
	LicenseUrl = 14,
	PreferredFamily = 16,
	PreferredSubfamily = 17,
};

constexpr char NormalizeToPostScriptName(char ch) {
	switch (ch) {
	case ' ':
		return '-';
	case '%':
	case '(':
	case ')':
	case '/':
	case '<':
	case '>':
	case '[':
	case ']':
	case '{':
	case '}':
		return '_';
	case '!':
	case '"':
	case '#':
	case '$':
	case '&':
	case '\'':
	case '*':
	case '+':
	case ',':
	case '-':
	case '.':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case ':':
	case ';':
	case '=':
	case '?':
	case '@':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case '\\':
	case '^':
	case '_':
	case '`':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
	case '|':
	case '~':
		return ch;
	default:
		return '_';
	}
}

string GeneratePostScriptName(string name, string style) {
	transform(name.begin(), name.end(), name.begin(),
	          NormalizeToPostScriptName);
	transform(style.begin(), style.end(), style.begin(),
	          NormalizeToPostScriptName);

	if (style.length() > 31)
		style = style.substr(0, 31);
	if (name.length() > 63 - 1 - style.length())
		name = name.substr(0, 63 - 1 - style.length());
	return name + "-" + style;
}

string GetNameEntry(const json &name, NameId nameId,
                    Platform platform = Platform::Windows,
                    Encoding encoding = Encoding::UnicodeBMP,
                    Language language = Language::en_US,
                    bool enableLanguageFallback = false) {
	for (const auto &entry : name)
		if (nameId == entry["nameID"] && platform == entry["platformID"] &&
		    encoding == entry["encodingID"] && language == entry["languageID"])
			return entry["nameString"];
	if (enableLanguageFallback) {
		auto pFallback = LanguageFallback.find(language);
		if (pFallback != LanguageFallback.end())
			return GetNameEntry(name, nameId, platform, encoding,
			                    pFallback->second, true);
	}
	return {};
}

License GuessLicense(const string &license, const string &url,
                     const string &copyright) {
#define FindURL(str) (url.find(str) != string::npos)
#define FindLiteral(str)                                                       \
	(license.find(str) != string::npos || copyright.find(str) != string::npos)
	License result{};

	if (FindURL("www.apache.org/licenses") || FindLiteral("Apache License"))
		result.Apache = true;

	/* LGPL will be treated as GPL, but that's okay.
	 *
	 * We cannot add a condition "not find a specific string" to avoid it,
	 * because a font can be dual licensed.
	 *
	 * `FindLiteral("GPL")` is a trick for 文泉驿正黑 (WenQuanYi Zen Hei).
	 */
	if (FindURL("www.gnu.org/copyleft/gpl") ||
	    FindLiteral("General Public License") || FindLiteral("GPL"))
		result.GPL = true;

	if (FindURL("www.gnu.org/copyleft/lesser") ||
	    FindLiteral("Lesser General Public License"))
		result.LGPL = true;

	if (FindURL("scripts.sil.org/OFL") || FindLiteral("Open Font License"))
		result.OFL = true;

	return result;
#undef FindLiteral
#undef FindURL
}

pair<string, string> MergeLicense(const vector<json> &nametables) {
	vector<License> licenses;
	vector<string> licenseStrings;

	for (const json &name : nametables) {
		const string &license = GetNameEntry(name, NameId::LicenseDescription);
		const string &url = GetNameEntry(name, NameId::LicenseUrl);
		const string &copyright = GetNameEntry(name, NameId::Copyright);
		licenses.push_back(GuessLicense(license, url, copyright));
		string fullname = GetNameEntry(name, NameId::FullName);
		if (!fullname.length())
			fullname = GetNameEntry(name, NameId::Family) + " " +
			           GetNameEntry(name, NameId::Subfamily);
		licenseStrings.push_back(fullname + ": " + license);
	}

	bool Apache = false;
	bool GPL = false;
	bool LGPL = false;
	bool OFL = false;
	for (License l : licenses) {
		// one of them are not licensed under Apache, GPL, LGPL or OFL.
		if (!l)
			goto cannot_decide;
		Apache = Apache || l.Apache;
		GPL = GPL || l.GPL;
		LGPL = LGPL || l.LGPL;
		OFL = OFL || l.OFL;

		// printf("Apache: %d, GPL: %d, LGPL: %d, OFL: %d\n", l.Apache, l.GPL,
		// l.LGPL, l.OFL);
	}

	// try promoting to Apache
	{
		bool noShareAlike = true;
		for (License l : licenses) {
			if ((l.OFL || l.GPL) && !l.Apache) {
				noShareAlike = false;
				break;
			}
		}
		if (noShareAlike)
			return {LicenseString::Apache, LicenseUrlString::Apache};
	}

	// try promoting to OFL
	if (OFL) {
		bool noGPL = true;
		for (License l : licenses) {
			if (l.GPL && !l.OFL) {
				noGPL = false;
				break;
			}
		}
		if (noGPL)
			return {LicenseString::OFL, LicenseUrlString::OFL};
	}

	// try promoting to (L)GPL
	if (GPL) {
		bool noOFL = true;
		bool canBeLGPL = true;
		for (License l : licenses) {
			if (l.OFL && !l.GPL)
				noOFL = false;
			if (l.GPL && !l.LGPL)
				canBeLGPL = false;
		}
		if (noOFL) {
			if (canBeLGPL)
				return {LicenseString::LGPL, LicenseUrlString::LGPL};
			else
				return {LicenseString::GPL, LicenseUrlString::GPL};
		}
	}

cannot_decide:
	string license = "WFM cannot decide the license for merged font. "
	                 "Please check again before distribute.";
	for (const auto &l : licenseStrings)
		license += "\n" + l;
	return {license, {}};
}

enum Slope : int {
	Upright = 0,
	Italic = 1,
	Oblique = 2,
};

namespace wwsNameToValue {

const static map<string, int> weight{
    {"thin", 100},       {"ultralight", 100},

    {"extralight", 200},

    {"light", 300},

    {"semilight", 350},  {"demilight", 350},

    {"normal", 372},

    {"regular", 400},    {"roman", 400},      {"", 400},

    {"book", 450},

    {"medium", 500},

    {"semibold", 600},   {"demi", 600},       {"demibold", 600},

    {"bold", 700},

    {"extrabold", 800},

    {"black", 900},      {"heavy", 900},      {"ultrabold", 900},

    {"extrablack", 950},
};

const static map<string, int> width{
    {"ultracondensed", 1},

    {"extracondensed", 2},

    {"condensed", 3},

    {"semicondensed", 4},

    {"normal", 5},         {"", 5},

    {"semiextended", 6},   {"semiexpanded", 6},

    {"extended", 7},       {"expanded", 7},

    {"extraextended", 8},  {"extraexpanded", 8},

    {"ultraextended", 9},  {"ultraexpanded", 9},
};

const static map<string, Slope> slope{
    {"upright", Slope::Upright}, {"normal", Slope::Upright},
    {"roman", Slope::Upright},   {"unslanted", Slope::Upright},
    {"", Slope::Upright},

    {"italic", Slope::Italic},   {"italized", Slope::Italic},

    {"oblique", Slope::Oblique}, {"slant", Slope::Oblique},
};

} // namespace wwsNameToValue

inline vector<string> split(string const &str, char delim) {
	vector<string> result;
	stringstream s(str);
	string s2;
	while (getline(s, s2, delim))
		result.push_back(s2);
	return result;
}

inline string replace_all(string const &source, string from, string to) {
	return regex_replace(source, std::regex(from), to);
}

inline string NormalizeStyle(string style) {
	style = replace_all(style, "[ -]", "");
	std::transform(style.begin(), style.end(), style.begin(),
	               [](auto c) { return tolower(c); });
	return style;
}

tuple<string, int, int, Slope> ParseWws(string overrideNameStyle) {

	overrideNameStyle = replace_all(overrideNameStyle, "；", ";");
	overrideNameStyle += ';'; // in case that last field is empty
	auto fields = split(overrideNameStyle, ';');

	if (fields.size() != 4)
		throw std::invalid_argument("--name 参数格式错误");

	string family = fields[0];
	string weight_ = NormalizeStyle(fields[1]),
	       width_ = NormalizeStyle(fields[2]),
	       slope_ = NormalizeStyle(fields[3]);
	int weight;
	int width;
	Slope slope;

	try {
		weight = stoi(weight_);
		if (weight < 100 || weight > 950)
			throw std::out_of_range("字重超出范围");
	} catch (const std::invalid_argument &) {
		try {
			weight = wwsNameToValue::weight.at(weight_);
		} catch (const std::out_of_range &) {
			throw std::invalid_argument("字重参数无效");
		}
	}

	try {
		width = stoi(width_);
		if (width < 1 || width > 9)
			throw std::out_of_range("宽度超出范围");
	} catch (const std::invalid_argument &) {
		try {
			width = wwsNameToValue::width.at(width_);
		} catch (const std::out_of_range &) {
			throw std::invalid_argument("宽度参数无效");
		}
	}

	try {
		slope = wwsNameToValue::slope.at(slope_);
	} catch (const std::out_of_range &) {
		throw std::invalid_argument("倾斜参数无效");
	}

	return {family, weight, width, slope};
}

pair<string, string> GetLagacyFamilyAndStyle(const string &family,
                                             const string &style) {
	// 4 standard styles
	if (style == "Regular" || style == "Italic" || style == "Bold" ||
	    style == "Bold Italic")
		return {family, style};

	size_t pos;
	if ((pos = style.find(" Bold Italic")) != std::string::npos ||
	    (pos = style.find(" Italic")) != std::string::npos ||
	    (pos = style.find(" Bold")) != std::string::npos) {
		return {family + " " + style.substr(0, pos), style.substr(pos + 1)};
	}

	return {family + " " + style, "Regular"};
}

tuple<string, string, string> AutoMergeName(const vector<json> &nameTables) {
	const json &first = nameTables[0];
	string style = GetNameEntry(first, NameId::PreferredSubfamily);
	if (style.empty())
		style = GetNameEntry(nameTables[0], NameId::PreferredSubfamily);
	if (style.empty())
		style = GetNameEntry(nameTables[0], NameId::Subfamily);
	if (style.empty())
		style = "Regular";

	string family = GetNameEntry(first, NameId::PreferredFamily);
	if (!family.length())
		family = GetNameEntry(first, NameId::Family);

	for (size_t i = 1; i < nameTables.size(); i++) {
		const json &second = nameTables[i];
		string family2 = GetNameEntry(second, NameId::PreferredFamily);
		if (!family2.length())
			family2 = GetNameEntry(second, NameId::Family);
		family += " + " + family2;
	}

	string psname = GeneratePostScriptName(replace_all(family, " ", ""),
	                                       replace_all(style, " ", ""));
	return {family, style, psname};
}

string MergeCopyright(const vector<json> &nameTables) {
	string result = GetNameEntry(nameTables[0], NameId::Copyright);
	for (size_t i = 1; i < nameTables.size(); i++)
		result += "\n" + GetNameEntry(nameTables[i], NameId::Copyright);
	return result;
}

void RemoveRedundantTable(vector<json> &nameTables) {
	vector<string> names;
	size_t nowarlcg = -1;
	for (size_t i = 0; i < nameTables.size();) {
		const json &table = nameTables[i];
		string family = GetNameEntry(table, NameId::PreferredFamily);
		if (!family.length())
			family = GetNameEntry(table, NameId::Family);
		if (find(names.begin(), names.end(), family) != names.end()) {
			nameTables.erase(nameTables.begin() + i);
		} else {
			names.push_back(family);
			if (family == "Nowar Sans LCG")
				nowarlcg = i;
			i++;
		}
	}

	// Nowar Sans LCG + Nowar Sans CJK
	if (nowarlcg != size_t(-1)) {
		bool hasCJK = false;
		for (auto &table : nameTables) {
			string family = GetNameEntry(table, NameId::PreferredFamily);
			if (family.length() >= 14 &&
			    family.substr(0, 14) == "Nowar Sans CJK") {
				hasCJK = true;
				for (auto &entry : table)
					switch (NameId(entry["nameID"])) {
					case NameId::Family:
					case NameId::PreferredFamily:
						entry["nameString"] =
						    string(entry["nameString"]).replace(10, 4, "");
						break;
					default:
						break;
					}
			}
		}
		if (hasCJK)
			nameTables.erase(nameTables.begin() + nowarlcg);
	}
}

json AutoMergeNameTable(vector<json> &nameTables) {

	auto [family, style, psName] = AutoMergeName(nameTables);
	auto [legacyFamily, legacyStyle] = GetLagacyFamilyAndStyle(family, style);
	auto [license, licenseUrl] = MergeLicense(nameTables);
	auto copyright = MergeCopyright(nameTables);
	auto version = GetNameEntry(nameTables[0], NameId::Version);

	json result = nameTables[0];
	for (auto &entry : result) {
		switch (NameId(entry["nameID"])) {
		case NameId::Copyright:
			entry["nameString"] = copyright;
			break;
		case NameId::Family:
			entry["nameString"] = legacyFamily;
			break;
		case NameId::Subfamily:
			entry["nameString"] = legacyStyle;
			break;
		case NameId::UniqueIdentifier:
			entry["nameString"] = family + " " + style + " " + version;
			break;
		case NameId::FullName:
			entry["nameString"] = family + " " + style;
			break;
		case NameId::PostScript:
			entry["nameString"] = psName;
			break;
		case NameId::LicenseDescription:
			entry["nameString"] = license;
			break;
		case NameId::LicenseUrl:
			entry["nameString"] = licenseUrl;
			break;
		case NameId::PreferredFamily:
			entry["nameString"] = family;
			break;
		case NameId::PreferredSubfamily:
			entry["nameString"] = style;
			break;
		default:
			break;
		}
	}

	if (GetNameEntry(result, NameId::LicenseDescription).empty())
		result.push_back({
		    {"platformID", Platform::Windows},
		    {"encodingID", Encoding::Unicode},
		    {"languageID", Language::en_US},
		    {"nameID", NameId::LicenseDescription},
		    {"nameString", license},
		});

	if (GetNameEntry(result, NameId::Copyright).empty())
		result.push_back({
		    {"platformID", Platform::Windows},
		    {"encodingID", Encoding::Unicode},
		    {"languageID", Language::en_US},
		    {"nameID", NameId::Copyright},
		    {"nameString", copyright},
		});

	return result;
}

namespace wwsValueToName {

const static map<int, string> weight{
    {100, "Thin"},       {200, "ExtraLight"}, {300, "Light"},
    {350, "SemiLight"},  {372, "Normal"},     {400, ""},
    {450, "Book"},       {500, "Medium"},     {600, "SemiBold"},
    {700, "Bold"},       {800, "ExtraBold"},  {900, "Black"},
    {950, "ExtraBlack"},
};

const static map<int, string> width{
    {1, "UltraCondensed"},
    {2, "ExtraCondensed"},
    {3, "Condensed"},
    {4, "SemiCondensed"},
    {5, ""},
    {6, "SemiExtended"},
    {7, "Extended"},
    {8, "ExtraExtended"},
    {9, "UltraExtended"},
};

const static map<Slope, string> slope{
    {Slope::Upright, ""},
    {Slope::Italic, "Italic"},
    {Slope::Oblique, "Oblique"},
};

} // namespace wwsValueToName

string join(const vector<string> &strs, const string &delim) {
	string result;
	for (auto it = strs.begin(); it != strs.end(); it++) {
		result += *it;
		if (it != strs.end() - 1)
			result += delim;
	}
	return result;
}

tuple<string, string, string, string> GenerateWwsName(string family, int weight,
                                                      int width, Slope slope) {

	vector<string> typoSubfamily;
	vector<string> legacyFamilyAppend;
	vector<string> legacySubfamily;

	{
		string width_ = wwsValueToName::width.at(width);
		if (width_.length()) {
			typoSubfamily.push_back(width_);
			legacyFamilyAppend.push_back(width_);
		}
	}

	switch (weight) {
	case 400:
		break;
	case 700:
		typoSubfamily.emplace_back("Bold");
		legacySubfamily.emplace_back("Bold");
		break;
	default:
		string weight_;
		try {
			weight_ = wwsValueToName::weight.at(weight);
		} catch (const out_of_range &) {
			char buffer[16];
			snprintf(buffer, sizeof buffer, "W%d", weight);
			weight_ = buffer;
		}
		typoSubfamily.push_back(weight_);
		legacyFamilyAppend.push_back(weight_);
	}

	switch (slope) {
	case Slope::Upright:
		break;
	case Slope::Italic:
		typoSubfamily.emplace_back("Italic");
		legacySubfamily.emplace_back("Italic");
		break;
	case Slope::Oblique:
		typoSubfamily.emplace_back("Oblique");
		legacyFamilyAppend.emplace_back("Oblique");
		break;
	}

	return {
	    typoSubfamily.size() ? join(typoSubfamily, " ") : "Regular",
	    legacyFamilyAppend.size() ? family + ' ' + join(legacyFamilyAppend, " ")
	                              : family,
	    legacySubfamily.size() ? join(legacySubfamily, " ") : "Regular",
	    GeneratePostScriptName(replace_all(family, " ", ""),
	                           typoSubfamily.size() ? join(typoSubfamily, "")
	                                                : "Regular"),
	};
}

void MergeNameTable(vector<json> &nameTables, json &font,
                    const std::string &overrideNameStyle) {

	RemoveRedundantTable(nameTables);

	if (overrideNameStyle.empty()) {
		font["name"] = AutoMergeNameTable(nameTables);
		return;
	}

	auto [family, weight, width, slope] = ParseWws(overrideNameStyle);

	auto [style, legacyFamily, legacyStyle, psName] =
	    GenerateWwsName(family, weight, width, slope);
	auto [license, licenseUrl] = MergeLicense(nameTables);
	auto copyright = MergeCopyright(nameTables);
	auto version = GetNameEntry(nameTables[0], NameId::Version);

	json result = nameTables[0];
	for (auto &entry : result) {
		switch (NameId(entry["nameID"])) {
		case NameId::Copyright:
			entry["nameString"] = copyright;
			break;
		case NameId::Family:
			entry["nameString"] = legacyFamily;
			break;
		case NameId::Subfamily:
			entry["nameString"] = legacyStyle;
			break;
		case NameId::UniqueIdentifier:
			entry["nameString"] = family + " " + style + " " + version;
			break;
		case NameId::FullName:
			entry["nameString"] = family + " " + style;
			break;
		case NameId::PostScript:
			entry["nameString"] = psName;
			break;
		case NameId::LicenseDescription:
			entry["nameString"] = license;
			break;
		case NameId::LicenseUrl:
			entry["nameString"] = licenseUrl;
			break;
		case NameId::PreferredFamily:
			entry["nameString"] = family;
			break;
		case NameId::PreferredSubfamily:
			entry["nameString"] = style;
			break;
		default:
			break;
		}
	}

#define SetNameEntry(entry, value)                                             \
	({                                                                         \
		if (GetNameEntry(result, NameId::entry).empty())                       \
			result.push_back({                                                 \
			    {"platformID", Platform::Windows},                             \
			    {"encodingID", Encoding::Unicode},                             \
			    {"languageID", Language::en_US},                               \
			    {"nameID", NameId::entry},                                     \
			    {"nameString", value},                                         \
			});                                                                \
	})

	SetNameEntry(Family, legacyFamily);
	SetNameEntry(Subfamily, legacyStyle);
	SetNameEntry(PreferredFamily, family);
	SetNameEntry(PreferredSubfamily, style);

	SetNameEntry(LicenseDescription, license);
	SetNameEntry(Copyright, copyright);

#undef SetNameEntry

	font["name"] = result;

	if (font.find("head") != font.end()) {
		auto &head = font["head"];
		auto &macStyle = head["macStyle"];
		macStyle["bold"] = (weight == 700);
		macStyle["italic"] = (slope == Slope::Italic);
	}

	if (font.find("OS_2") != font.end()) {
		auto &os_2 = font["OS_2"];
		os_2["usWeightClass"] = weight;
		os_2["usWidthClass"] = width;

		auto &fsSelection = os_2["fsSelection"];
		fsSelection["regular"] =
		    (weight == 400 && width == 5 && slope == Slope::Upright);
		fsSelection["bold"] = (weight == 700);
		fsSelection["italic"] = (slope == Slope::Italic);
		fsSelection["oblique"] = (slope == Slope::Oblique);
		fsSelection["wws"] = true;
	}
}
