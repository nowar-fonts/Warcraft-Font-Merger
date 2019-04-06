import json
import sys

from common import TrimGlyph, CopyRef

def NameFont(font, weight, version):

	isStdStyle = weight == 'Regular' or weight == 'Bold'

	font['OS_2']['achVendID'] = 'Cyan'
	font['name'] = [
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 0,
			"nameString": "Copyright © 2018—2019 Cyano Hao, with reserved font name “Nowar”, “有爱”, and “有愛”. Portions Copyright 2015 Google Inc."
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 1,
			"nameString": "Nowar Sans LCG" if isStdStyle else "Nowar Sans LCG " + weight
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
			"nameString": "Nowar Sans LCG " + weight + ' ' + str(version)
		},
		{
			"platformID": 3,
			"encodingID": 1,
			"languageID": 1033,
			"nameID": 4,
			"nameString": "Nowar Sans LCG " + weight
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
			"nameString": "Nowar-Sans-LCG-" + weight.replace(' ', '-')
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
			"nameString": "Monotype Design Team"
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
			"nameString": "Nowar Sans LCG"
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
	weight = sys.argv[1]
	version = sys.argv[2]

	with open("src/NotoSans-SemiCondensed{}.otd".format(weight), 'rb') as baseFile:
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
	with open("out/Nowar-Sans-LCG-{}.otd".format(weight), 'w') as outFile:
		outFile.write(outStr)
