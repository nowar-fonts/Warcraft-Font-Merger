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

# x' = a x + b y + dx
# y' = c x + d y + dy
def Transform(glyph, a, b, c, d, dx, dy, roundToInt = False):
	r = round if roundToInt else lambda x: x
	glyph['advanceWidth'] = r(glyph['advanceWidth'] * a)
	if 'contours' in glyph:
		for contour in glyph['contours']:
			for point in contour:
				x = point['x']
				y = point['y']
				point['x'] = r(a * x + b * y + dx)
				point['y'] = r(c * x + d * y + dy)
	if 'references' in glyph:
		for reference in glyph['references']:
			x = reference['x']
			y = reference['y']
			reference['x'] = r(a * x + b * y + dx)
			reference['y'] = r(c * x + d * y + dy)

RegionSubfamilyMap = {
	'XS': "Compressed",
	'CN': "GB18030",
	'SC': "CN",
	'TC': "TW",
	'CL': "Classic",
}
