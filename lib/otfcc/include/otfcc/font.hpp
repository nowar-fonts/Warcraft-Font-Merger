#pragma once

#include <optional>

#include <nlohmann/json.hpp>

#include "sfnt.h"

namespace otfcc {
struct font;
};

#include "glyph-order.hpp"

#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
#include "table/fvar.h"
#endif

#include "table/cff_.hpp"
#include "table/glyf.hpp"
#include "table/hdmx.hpp"
#include "table/head.hpp"
#include "table/hhea.hpp"
#include "table/hmtx.hpp"
#include "table/maxp.hpp"
#include "table/meta.hpp"
#include "table/name.hpp"
#include "table/os_2.hpp"
#include "table/post.hpp"
#include "table/vhea.hpp"
#include "table/vmtx.hpp"

#include "table/cmap.hpp"
#include "table/cvt_.hpp"
#include "table/fpgm-prep.hpp"
#include "table/gasp.hpp"
#include "table/vdmx.hpp"

#include "table/ltsh.hpp"
#include "table/vorg.hpp"

#include "table/base.hpp"
#include "table/gdef.hpp"
#include "table/otl.hpp"

#include "table/colr.hpp"
#include "table/cpal.hpp"
#include "table/svg_.hpp"

namespace otfcc {

struct font {

	enum class outline { ttf, cff };

	outline subtype;

#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
	std::optional<table::fvar> fvar;
#endif

	std::optional<table::head> head;
	std::optional<table::hhea> hhea;
	std::optional<table::maxp> maxp;
	std::optional<table::os_2> OS_2;
	std::optional<table::hmtx> hmtx;
	std::optional<table::post> post;
	std::optional<table::hdmx> hdmx;

	std::optional<table::vhea> vhea;
	std::optional<table::vmtx> vmtx;
	std::optional<table::vorg> VORG;

	std::optional<table::cff_> CFF_;
	table::glyf::table glyf;
	std::optional<table::cmap> cmap;
	table::name::table name;
	std::optional<table::meta> meta;

	std::optional<table::fpgm_prep> fpgm;
	std::optional<table::fpgm_prep> prep;
	std::optional<table::cvt_> cvt_;
	std::optional<table::gasp> gasp;
	std::optional<table::vdmx> VDMX;

	std::optional<table::ltsh> LTSH;

	std::optional<table::otl> GSUB;
	std::optional<table::otl> GPOS;
	std::optional<table::gdef> GDEF;
	std::optional<table::base> BASE;

	std::optional<table::cpal> CPAL;
	table::colr::table COLR;
	table::svg::table SVG_;

	glyph_order glyph_order;

	void consolidate(const options options);

	font(void *source, uint32_t index, const options &options);

	nlohmann::json serialize(const options &options);
};

} // namespace otfcc
