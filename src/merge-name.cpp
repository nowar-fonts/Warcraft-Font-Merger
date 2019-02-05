#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include "merge-name.h"

using namespace std;

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

using json = nlohmann::json;

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

	operator bool() { return GPL || LGPL || OFL || Apache; }
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

string GetNameEntry(const json &name, NameId nameid,
                    Platform platform = Platform::Windows,
                    Encoding encoding = Encoding::UnicodeBMP,
                    Language language = Language::en_US,
                    bool enableLanguageFallback = false) {
	for (const auto &entry : name)
		if (nameid == entry["nameID"] && platform == entry["platformID"] &&
		    encoding == entry["encodingID"] && language == entry["languageID"])
			return entry["nameString"];
	if (enableLanguageFallback) {
		auto pFallback = LanguageFallback.find(language);
		if (pFallback != LanguageFallback.end())
			return GetNameEntry(name, nameid, platform, encoding,
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

pair<string, string> GetLagacyFamilyAndStyle(string family, string style) {
	// 4 standard styles
	if (style == "Regular" || style == "Italic" || style == "Bold" ||
	    style == "Bold Italic")
		return {family, style};

	size_t pos;
	if ((pos = style.find(" Bold Italic")) != style.npos ||
	    (pos = style.find(" Italic")) != style.npos ||
	    (pos = style.find(" Bold")) != style.npos) {
		return {family + " " + style.substr(0, pos), style.substr(pos + 1)};
	}

	return {family + " " + style, "Regular"};
}

tuple<string, string, string> MergeName(const vector<json> &nametables) {
	const json &first = nametables[0];
	string style = GetNameEntry(first, NameId::PreferredSubfamily);
	if (!style.length())
		style = GetNameEntry(first, NameId::Subfamily);
	if (!style.length())
		style = "Regular";

	string family = GetNameEntry(first, NameId::PreferredFamily);
	if (!family.length())
		family = GetNameEntry(first, NameId::Family);

	string psname = family;

	for (size_t i = 1; i < nametables.size(); i++) {
		const json &second = nametables[i];
		string family2 = GetNameEntry(second, NameId::PreferredFamily);
		if (!family2.length())
			family2 = GetNameEntry(second, NameId::Family);
		family += " + " + family2;
		psname += "+" + family2;
	}

	psname = GeneratePostScriptName(psname, style);
	return {family, style, psname};
}

string MergeCopyright(const vector<json> &nametables) {
	string result = GetNameEntry(nametables[0], NameId::Copyright);
	for (size_t i = 1; i < nametables.size(); i++)
		result += "\n" + GetNameEntry(nametables[i], NameId::Copyright);
	return result;
}

json MergeNameTable(const vector<json> &nametables) {
	auto [family, style, psname] = MergeName(nametables);
	auto [legacyFamily, legacyStyle] = GetLagacyFamilyAndStyle(family, style);
	auto [license, licenseUrl] = MergeLicense(nametables);
	auto copyright = MergeCopyright(nametables);
	auto version = GetNameEntry(nametables[0], NameId::Version);

	json result = nametables[0];
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
			entry["nameString"] = psname;
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

	if (GetNameEntry(result, NameId::LicenseDescription) == "")
		result.push_back({
		    {"platformID", Platform::Windows},
			{"encodingID", Encoding::Unicode},
			{"languageID", Language::en_US},
			{"nameID", NameId::LicenseDescription},
			{"nameString", license},
		});

	if (GetNameEntry(result, NameId::Copyright) == "")
		result.push_back({
		    {"platformID", Platform::Windows},
			{"encodingID", Encoding::Unicode},
			{"languageID", Language::en_US},
			{"nameID", NameId::Copyright},
			{"nameString", copyright},
		});

	return result;
}
