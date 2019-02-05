import json
import sys

def NameFont(font, weight, version):

	isStdStyle = weight == 'Regular' or weight == 'Bold'

	font['OS_2']['achVendID'] = 'Cyan'
	font['name'] = [
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 0,
			"nameString": "Copyright © 2018—2019 Cyano Hao, with reserved font name “Nowar”, “有爱”, and “有愛”. Portions Copyright 2011 Google Inc."
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 1,
			"nameString": "Nowar Neo Sans (Latin, Кириллица and Ελληνικό)" if isStdStyle else "Nowar Neo Sans (Latin, Кириллица and Ελληνικό) " + weight
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
			"nameString": "Nowar Neo Sans (Latin, Кириллица and Ελληνικό) " + weight + ' ' + str(version)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 4,
			"nameString": "Nowar Neo Sans (Latin, Кириллица and Ελληνικό) " + weight
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
			"nameString": "Nowar-Neo-Sans-LCG-" + weight.replace(' ', '-')
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
			"nameString": "Christian Robertson"
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
			"nameString": "Nowar Neo Sans (Latin, Кириллица and Ελληνικό)"
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 17,
			"nameString": weight
		},
	]

def AddRef(n, font, ref):
	if n in ref:
		return
	glyph = font['glyf'][n]
	if 'references' in glyph:
		for r in glyph['references']:
			ref.append(r['glyph'])
			AddRef(r['glyph'], font, ref)

def TrimGlyph(font):
	needed = [ font['glyph_order'][0] ]
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
	weight = sys.argv[1]
	version = sys.argv[2]

	with open("src/Roboto-{}.otd".format(weight), 'rb') as baseFile:
		baseFont = json.loads(baseFile.read().decode('UTF-8', errors='replace'))

	NameFont(baseFont, weight, version)

	# quotes, em-dash and ellipsis
	for u in [0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026]:
		if str(u) in baseFont['cmap']:
			del baseFont['cmap'][str(u)]

	del baseFont['GSUB']
	del baseFont['GPOS']
	del baseFont['GDEF']
	TrimGlyph(baseFont)

	outStr = json.dumps(baseFont, ensure_ascii=False)
	with open("out/Nowar-Neo-Sans-LCG-{}.otd".format(weight), 'w') as outFile:
		outFile.write(outStr)
