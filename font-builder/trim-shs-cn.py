import sys
import json


def NameFont(font, region, weight, version):

	isStdStyle = weight == 'Regular' or weight == 'Bold'

	font['OS_2']['achVendID'] = 'Cyan'
	font['name'] = [
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 0,
			"nameString": "Copyright © 2018—2019 Cyano Hao, with reserved font name “Nowar”, “有爱”, and “有愛”. Portions © 2014, 2015, 2018 Adobe (http://www.adobe.com/)."
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
			"nameString": "Ryoko NISHIZUKA 西塚涼子 (kana, bopomofo & ideographs); Sandoll Communications 산돌커뮤니케이션, Soo-young JANG 장수영 & Joo-yeon KANG 강주연 (hangul elements, letters & syllables); Dr. Ken Lunde (project architect, glyph set definition & overall production); Masataka HATTORI 服部正貴 (production & ideograph elements)"
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
			"nameString": "This Font Software is licensed under the SIL Open Font License, Version 1.1. This Font Software is distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the SIL Open Font License for the specific language, permissions and limitations governing your use of this Font Software."
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 14,
			"nameString": "http://scripts.sil.org/OFL"
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
	region = sys.argv[1]
	weight = sys.argv[2]
	encoding = sys.argv[3]
	version = sys.argv[4]

	with open("src/NotoSans-SemiCondensed{}.otd".format(weight), 'rb') as latinFile:
		latinFont = json.loads(latinFile.read().decode('UTF-8', errors='replace'))

	with open("src/SourceHanSans{}-{}.otd".format(region, weight), 'rb') as baseFile:
		baseFont = json.loads(baseFile.read().decode('UTF-8', errors = 'replace'))

	baseFont['OS_2']['ulCodePageRange1'] = { encoding: True }
	NameFont(baseFont, 'Classic' if region == 'CL' else region, weight, version)

	# quotes, em-dash and ellipsis
	for u in [0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026]:
		if str(u) in latinFont['cmap']:
			del latinFont['cmap'][str(u)]

	for u in latinFont['cmap']:
		if u in baseFont['cmap']:
			del baseFont['cmap'][u]

	del baseFont['vhea']
	del baseFont['cmap_uvs']
	del baseFont['GSUB']
	del baseFont['GPOS']
	del baseFont['GDEF']
	del baseFont['BASE']
	TrimGlyph(baseFont)

	outStr = json.dumps(baseFont, ensure_ascii=False)
	with open("out/Nowar-Sans-CJK-{}-{}.otd".format(region, weight), 'w') as outFile:
		outFile.write(outStr)
