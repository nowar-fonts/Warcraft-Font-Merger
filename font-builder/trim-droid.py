import json
import sys
import copy

from common import TrimGlyph, CopyRef, Transform

def NameLatinFont(font, weight, version):
	isStdStyle = weight == 'Regular' or weight == 'Bold'

	font['OS_2']['achVendID'] = 'NOWR'

	font['name'] = [
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 0,
			"nameString": "Copyright © 2018—2022 Cyano Hao. Portions copyright © 2007, Google Corporation."
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 1,
			"nameString": "WFM Sans LCG" if isStdStyle else "WFM Sans LCG " + weight
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 2,
			"nameString": weight if isStdStyle else "Regular"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 3,
			"nameString": "WFM Sans LCG " + weight + ' ' + str(version)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 4,
			"nameString": "WFM Sans LCG " + weight
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 5,
			"nameString": str(version)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 6,
			"nameString": "WFM-Sans-LCG-" + weight.replace(' ', '-')
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 8,
			"nameString": "Nowar Typeface"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 9,
			"nameString": "Steve Matteson"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 11,
			"nameString": "https://github.com/nowar-fonts"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 13,
			"nameString": "Licensed under the Apache License, Version 2.0."
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 14,
			"nameString": "http://www.apache.org/licenses/LICENSE-2.0"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 16,
			"nameString": "WFM Sans LCG"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 17,
			"nameString": weight
		},
	]

def NameAsianFont(font, region, weight, version):
	isStdStyle = weight == 'Regular' or weight == 'Bold'

	font['OS_2']['achVendID'] = 'NOWR'

	font['name'] = [
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 0,
			"nameString": "Copyright © 2018—2019 Cyano Hao. Portions (c) Google Corporation 2006."
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 1,
			"nameString": "WFM Sans CJK {}".format(region) if isStdStyle else "WFM Sans CJK {} {}".format(region, weight)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 2,
			"nameString": weight if isStdStyle else "Regular"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 3,
			"nameString": "WFM Sans CJK {} {} {}".format(region, weight, version)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 4,
			"nameString": "WFM Sans CJK {} {}".format(region, weight)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 5,
			"nameString": str(version)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 6,
			"nameString": "WFM-Sans-CJK-{}-{}".format(region, weight.replace(' ', '-'))
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 8,
			"nameString": "Nowar Typeface"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 9,
			"nameString": "Steve Matteson"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 11,
			"nameString": "https://github.com/nowar-fonts"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 13,
			"nameString": "Licensed under the Apache License, Version 2.0."
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 14,
			"nameString": "http://www.apache.org/licenses/LICENSE-2.0"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 16,
			"nameString": "WFM Sans CJK {}".format(region)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 17,
			"nameString": weight
		},
	]

if __name__ == '__main__':
	version = sys.argv[1]

	with open("src/DroidSans.otd", 'rb') as latinFile:
		latinFont = json.loads(latinFile.read().decode('UTF-8', errors='replace'))

	with open("src/NotoSans-SemiCondensedMedium.otd", 'rb') as latinRefFile:
		latinRefFont = json.loads(latinRefFile.read().decode('UTF-8', errors='replace'))

	# quotes, em-dash and ellipsis
	for u in [0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026]:
		if str(u) in latinRefFont['cmap']:
			del latinRefFont['cmap'][str(u)]
		if str(u) in latinFont['cmap']:
			del latinFont['cmap'][str(u)]

	with open("src/DroidSansFallbackWoWG.otd", 'rb') as latinExtFile:
		latinExtFont = json.loads(latinExtFile.read().decode('UTF-8', errors='replace'))

	for n, g in latinExtFont['glyf'].items():
		Transform(g, 8, 0, 0, 8, 0, 0)

	with open("src/DroidSansFallbackWoWG.otd", 'rb') as asianFile:
		asianFont = json.loads(asianFile.read().decode('UTF-8', errors='replace'))

	asianFont['OS_2']['ulCodePageRange1'] = { "gbk": True }
	asianFont['OS_2']['ulCodePageRange2'] = {}
	NameLatinFont(latinFont, "Regular", version)
	NameAsianFont(asianFont, "Compressed", "Regular", version)

	for (u, n) in latinExtFont['cmap'].items():
		# Western symbol
		if u in latinRefFont['cmap']:
			if u not in latinFont['cmap']:
				latinFont['cmap'][u] = latinExtFont['cmap'][u]
				if n not in latinFont['glyf']:
					latinFont['glyf'][n] = latinExtFont['glyf'][n]
					CopyRef(latinFont['glyf'][n], latinFont, latinExtFont)

	for u in latinFont['cmap']:
		if u in asianFont['cmap']:
			del asianFont['cmap'][u]

	del asianFont['vhea']
	del asianFont['GSUB']
	del asianFont['GDEF']
	TrimGlyph(asianFont)

	latinOutStr = json.dumps(latinFont, ensure_ascii=False)
	with open("out/WFM-Sans-LCG-Apache-Regular.otd", 'w') as latinOutFile:
		latinOutFile.write(latinOutStr)

	asianOutStr = json.dumps(asianFont, ensure_ascii=False)
	with open("out/WFM-Sans-CJK-XS-Regular.otd", 'w') as asianOutFile:
		asianOutFile.write(asianOutStr)
