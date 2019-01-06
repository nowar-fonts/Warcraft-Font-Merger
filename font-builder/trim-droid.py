import json
import sys

def NameFont(font, region, weight, version):

	isStdStyle = weight == 'Regular' or weight == 'Bold'

	font['OS_2']['achVendID'] = 'Cyan'

	font['name'] = [
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 0,
			"nameString": "Copyright © 2018—2019 Cyano Hao, with reserved font name “Nowar”, “有爱”, and “有愛”. Portions (c) Google Corporation 2006."
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 1,
			"nameString": "Nowar Sans (漢字 and 仮名) {}".format(region) if isStdStyle else "Nowar Sans (漢字 and 仮名) {} {}".format(region, weight)
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
			"nameString": "Nowar Sans (漢字 and 仮名) {} {} {}".format(region, weight, version)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 4,
			"nameString": "Nowar Sans (漢字 and 仮名) {} {}".format(region, weight)
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
			"nameString": "Nowar-Sans-CJK-{}-{}".format(region, weight.replace(' ', '-'))
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 8,
			"nameString": "Cyano Hao"
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
			"nameString": "https://github.com/CyanoHao"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 13,
			"nameString": "Licensed under the Apache License, Version 2.0"
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
			"nameString": "Nowar Sans (漢字 and 仮名) {}".format(region)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 17,
			"nameString": weight
		},
	]

def CopyRef(glyph, a, b):
	if 'references' in glyph:
		for r in glyph['references']:
			if r['glyph'] not in a['glyf']:
				a['glyf'][r['glyph']] = b['glyf'][r['glyph']]
				CopyRef(a['glyf'][r['glyph']], a, b)

def AddRef(n, font, ref):
	if n in ref:
		return
	glyph = font['glyf'][n]
	if 'references' in glyph:
		for r in glyph['references']:
			ref.append(r['glyph'])
			AddRef(r['glyph'], font, ref)

def TrimGlyph(font):
	needed = []
	for (_, n) in font['cmap'].items():
		needed.append(n)
	ref = []
	for n in needed:
		AddRef(n, font, ref)

	unneeded = []
	for n in font['glyf']:
		if not (n in needed or n in ref):
			unneeded.append(n)
	
	for n in unneeded:
		del font['glyf'][n]

if __name__ == '__main__':

	with open("src/NotoSans-SemiCondensedMedium.otd", 'rb') as latinFile:
		latinFont = json.loads(latinFile.read().decode('UTF-8', errors='replace'))

	# quotes, em-dash and ellipsis
	for u in [0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026]:
		if str(u) in latinFont['cmap']:
			del latinFont['cmap'][str(u)]

	with open("src/DroidSansFallbackFull.otd", 'rb') as asianFile:
		baseFont = json.loads(asianFile.read().decode('UTF-8', errors='replace'))

	with open("src/DroidSansFallbackLegacy.otd", 'rb') as latinExtFile:
		latinExtFont = json.loads(latinExtFile.read().decode('UTF-8', errors='replace'))

	baseFont['OS_2']['ulCodePageRange1'] = { "gbk": True }
	baseFont['OS_2']['ulCodePageRange2'] = {}
	NameFont(baseFont, "Compressed", "Regular", "0.4.0")

	for (u, n) in latinExtFont['cmap'].items():
		if int(u) < 0x3000 and u not in baseFont['cmap']:
			baseFont['cmap'][u] = latinExtFont['cmap'][u]
			if n not in baseFont['glyf']:
				baseFont['glyf'][n] = latinExtFont['glyf'][n]
				CopyRef(baseFont['glyf'][n], baseFont, latinExtFont)

	for u in latinFont['cmap']:
		if u in baseFont['cmap']:
			del baseFont['cmap'][u]

	del baseFont['vhea']
	del baseFont['GSUB']
	del baseFont['GDEF']
	TrimGlyph(baseFont)

	asianOutStr = json.dumps(baseFont, ensure_ascii=False)
	with open("out/Nowar-Sans-CJK-XS-Regular.otd", 'w') as asianOutFile:
		asianOutFile.write(asianOutStr)
