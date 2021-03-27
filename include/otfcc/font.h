#ifndef CARYLL_FONT_H
#define CARYLL_FONT_H

#include "sfnt.h"

struct _caryll_font;
typedef struct _caryll_font otfcc_Font;

#include "otfcc/glyph-order.h"

#include "otfcc/table/fvar.h"

#include "otfcc/table/head.h"
#include "otfcc/table/glyf.h"
#include "otfcc/table/CFF.h"
#include "otfcc/table/maxp.h"
#include "otfcc/table/hhea.h"
#include "otfcc/table/hmtx.h"
#include "otfcc/table/hdmx.h"
#include "otfcc/table/vhea.h"
#include "otfcc/table/vmtx.h"
#include "otfcc/table/OS_2.h"
#include "otfcc/table/post.h"
#include "otfcc/table/name.h"
#include "otfcc/table/meta.h"

#include "otfcc/table/cmap.h"
#include "otfcc/table/cvt.h"
#include "otfcc/table/fpgm-prep.h"
#include "otfcc/table/gasp.h"
#include "otfcc/table/VDMX.h"

#include "otfcc/table/LTSH.h"
#include "otfcc/table/VORG.h"

#include "otfcc/table/GDEF.h"
#include "otfcc/table/BASE.h"
#include "otfcc/table/otl.h"

#include "otfcc/table/CPAL.h"
#include "otfcc/table/COLR.h"
#include "otfcc/table/SVG.h"

#include "otfcc/table/_TSI.h"
#include "otfcc/table/TSI5.h"

typedef enum { FONTTYPE_TTF, FONTTYPE_CFF } otfcc_font_subtype;

struct _caryll_font {
	otfcc_font_subtype subtype;

	table_fvar *fvar;

	table_head *head;
	table_hhea *hhea;
	table_maxp *maxp;
	table_OS_2 *OS_2;
	table_hmtx *hmtx;
	table_post *post;
	table_hdmx *hdmx;

	table_vhea *vhea;
	table_vmtx *vmtx;
	table_VORG *VORG;

	table_CFF *CFF_;
	table_glyf *glyf;
	table_cmap *cmap;
	table_name *name;
	table_meta *meta;

	table_fpgm_prep *fpgm;
	table_fpgm_prep *prep;
	table_cvt *cvt_;
	table_gasp *gasp;
	table_VDMX *VDMX;

	table_LTSH *LTSH;

	table_OTL *GSUB;
	table_OTL *GPOS;
	table_GDEF *GDEF;
	table_BASE *BASE;

	table_CPAL *CPAL;
	table_COLR *COLR;
	table_SVG *SVG_;

	table_TSI *TSI_01;
	table_TSI *TSI_23;
	table_TSI5 *TSI5;

	otfcc_GlyphOrder *glyph_order;
};

extern caryll_ElementInterfaceOf(otfcc_Font) {
	caryll_RT(otfcc_Font);
	void (*consolidate)(otfcc_Font * font, const otfcc_Options *options);
	void *(*createTable)(otfcc_Font * font, const uint32_t tag);
	void (*deleteTable)(otfcc_Font * font, const uint32_t tag);
}
otfcc_iFont;

// Font builder interfaces
typedef struct otfcc_IFontBuilder {
	otfcc_Font *(*read)(void *source, uint32_t index, const otfcc_Options *options);
	void (*free)(struct otfcc_IFontBuilder *self);
} otfcc_IFontBuilder;
otfcc_IFontBuilder *otfcc_newOTFReader();
otfcc_IFontBuilder *otfcc_newJsonReader();

// Font serializer interface
typedef struct otfcc_IFontSerializer {
	void *(*serialize)(otfcc_Font *font, const otfcc_Options *options);
	void (*free)(struct otfcc_IFontSerializer *self);
} otfcc_IFontSerializer;
otfcc_IFontSerializer *otfcc_newJsonWriter();
otfcc_IFontSerializer *otfcc_newOTFWriter();

#endif
