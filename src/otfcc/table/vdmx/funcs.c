#include "../VDMX.h"

#include "support/util.h"
#include "bk/bkgraph.h"

table_VDMX *otfcc_readVDMX(const otfcc_Packet packet, const otfcc_Options *options) {
	table_VDMX *vdmx = NULL;
	FOR_TABLE('VDMX', table) {
		if (table.length < 6) goto FAIL;
		uint16_t version = read_16u(table.data + 0);
		uint16_t numRatios = read_16u(table.data + 4);
		if (table.length < 6 + 6 * numRatios) goto FAIL;

		vdmx = table_iVDMX.create();
		vdmx->version = version;
		for (shapeid_t g = 0; g < numRatios; g++) {
			const size_t ratioRangeOffset = 6 + 4 * g;
			const size_t offsetOffset = 6 + 4 * numRatios + 2 * g;
			vdmx_RatioRange r;
			vdmx_iRatioRange.init(&r);
			r.bCharset = read_8u(table.data + ratioRangeOffset + 0);
			r.xRatio = read_8u(table.data + ratioRangeOffset + 1);
			r.yStartRatio = read_8u(table.data + ratioRangeOffset + 2);
			r.yEndRatio = read_8u(table.data + ratioRangeOffset + 3);

			uint16_t groupOffset = read_16u(table.data + offsetOffset);
			uint16_t recs = read_16u(table.data + groupOffset + 0);
			for (uint16_t j = 0; j < recs; j++) {
				uint16_t yPelHeight = read_16u(table.data + groupOffset + 4 + j * 6 + 0);
				int16_t yMax = read_16s(table.data + groupOffset + 4 + j * 6 + 2);
				int16_t yMin = read_16s(table.data + groupOffset + 4 + j * 6 + 4);
				vdmx_iGroup.push(&r.records, (vdmx_Record){
				                                 .yPelHeight = yPelHeight,
				                                 .yMax = yMax,
				                                 .yMin = yMin,
				                             });
			}
			vdmx_iRatioRangeList.push(&vdmx->ratios, r);
		}
		return vdmx;
	FAIL:
		logWarning("Table 'VDMX' corrupted.\n");
		table_iVDMX.free(vdmx);
		vdmx = NULL;
	}
	return vdmx;
}

void otfcc_dumpVDMX(const table_VDMX *vdmx, json_value *root, const otfcc_Options *options) {
	if (!vdmx) return;
	loggedStep("VDMX") {
		json_value *_vdmx = json_object_new(2);
		json_object_push(_vdmx, "version", json_integer_new(vdmx->version));

		json_value *_ratios = json_array_new(vdmx->ratios.length);
		json_object_push(_vdmx, "ratios", _ratios);
		foreach (vdmx_RatioRange *rr, vdmx->ratios) {
			json_value *_rr = json_object_new(5);
			json_array_push(_ratios, _rr);
			json_object_push(_rr, "bCharset", json_integer_new(rr->bCharset));
			json_object_push(_rr, "xRatio", json_integer_new(rr->xRatio));
			json_object_push(_rr, "yStartRatio", json_integer_new(rr->yStartRatio));
			json_object_push(_rr, "yEndRatio", json_integer_new(rr->yEndRatio));

			json_value *_records = json_array_new(rr->records.length);
			json_object_push(_rr, "records", _records);
			foreach (vdmx_Record *r, rr->records) {
				json_value *_r = json_object_new(3);
				json_array_push(_records, _r);
				json_object_push(_r, "yPelHeight", json_integer_new(r->yPelHeight));
				json_object_push(_r, "yMax", json_integer_new(r->yMax));
				json_object_push(_r, "yMin", json_integer_new(r->yMin));
			}
		}
		json_object_push(root, "VDMX", _vdmx);
	}
}

table_VDMX *otfcc_parseVDMX(const json_value *root, const otfcc_Options *options) {
	json_value *_vdmx = NULL;
	if (!(_vdmx = json_obj_get_type(root, "VDMX", json_object))) return NULL;
	table_VDMX *vdmx = table_iVDMX.create();
	loggedStep("VDMX") {
		vdmx->version = json_obj_getnum(_vdmx, "version");
		json_value *_ratios = json_obj_get_type(_vdmx, "ratios", json_array);
		for (size_t j = 0; j < _ratios->u.array.length; j++) {
			json_value *_rr = _ratios->u.array.values[j];
			if (!_rr || _rr->type != json_object) continue;
			vdmx_RatioRange r;
			vdmx_iRatioRange.init(&r);
			r.bCharset = json_obj_getnum(_rr, "bCharset");
			r.xRatio = json_obj_getnum(_rr, "xRatio");
			r.yStartRatio = json_obj_getnum(_rr, "yStartRatio");
			r.yEndRatio = json_obj_getnum(_rr, "yEndRatio");
			json_value *_records = json_obj_get_type(_rr, "records", json_array);
			if (!_records) {
				vdmx_iRatioRange.dispose(&r);
				continue;
			}
			for (size_t j = 0; j < _records->u.array.length; j++) {
				json_value *_r = _records->u.array.values[j];
				if (!_r || _r->type != json_object) continue;
				vdmx_iGroup.push(&r.records, (vdmx_Record){
				                                 .yPelHeight = json_obj_getnum(_r, "yPelHeight"),
				                                 .yMax = json_obj_getnum(_r, "yMax"),
				                                 .yMin = json_obj_getnum(_r, "yMin"),
				                             });
			}
			vdmx_iRatioRangeList.push(&vdmx->ratios, r);
		}
	}
	return vdmx;
}

caryll_Buffer *otfcc_buildVDMX(const table_VDMX *vdmx, const otfcc_Options *options) {
	if (!vdmx || !vdmx->ratios.length) return NULL;
	bk_Block *root = bk_new_Block(b16, vdmx->version,       // Version
	                              b16, vdmx->ratios.length, // numRecs
	                              b16, vdmx->ratios.length, // numRatios
	                              bkover);
	foreach (vdmx_RatioRange *rr, vdmx->ratios) {
		bk_push(root,                // RatioRange
		        b8, rr->bCharset,    // bCharset
		        b8, rr->xRatio,      // bCharset
		        b8, rr->yStartRatio, // bCharset
		        b8, rr->yEndRatio,   // bCharset
		        bkover);
	}
	foreach (vdmx_RatioRange *rr, vdmx->ratios) {
		uint16_t startsz = 0xFFFF;
		uint16_t endsz = 0;
		foreach (vdmx_Record *r, rr->records) {
			if (startsz > r->yPelHeight) startsz = r->yPelHeight;
			if (endsz < r->yPelHeight) endsz = r->yPelHeight;
		}
		bk_Block *group = bk_new_Block(b16, rr->records.length, // recs
		                               b8, startsz,             //
		                               b8, endsz,               //
		                               bkover);
		foreach (vdmx_Record *r, rr->records) {
			bk_push(group,              //
			        b16, r->yPelHeight, //
			        b16, r->yMax,       //
			        b16, r->yMin,       //
			        bkover);
		}
		bk_push(root, p16, group, bkover);
	}
	return bk_build_Block_noMinimize(root);
}
