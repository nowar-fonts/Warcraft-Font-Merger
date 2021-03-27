#include "fvar.h"

#include "support/util.h"

// fvar instance tuple
// fvar instance
static INLINE void initFvarInstance(fvar_Instance *inst) {
	memset(inst, 0, sizeof(*inst));
	iVV.init(&inst->coordinates);
}
static INLINE void disposeFvarInstance(fvar_Instance *inst) {
	iVV.dispose(&inst->coordinates);
}
caryll_standardType(fvar_Instance, fvar_iInstance, initFvarInstance, disposeFvarInstance);
caryll_standardVectorImpl(fvar_InstanceList, fvar_Instance, fvar_iInstance, fvar_iInstanceList);
// table fvar

static INLINE void disposeFvarMaster(fvar_Master *m) {
	sdsfree(m->name);
	vq_deleteRegion(m->region);
}

static INLINE void initFvar(table_fvar *fvar) {
	memset(fvar, 0, sizeof(*fvar));
	vf_iAxes.init(&fvar->axes);
	fvar_iInstanceList.init(&fvar->instances);
}
static INLINE void disposeFvar(table_fvar *fvar) {
	vf_iAxes.dispose(&fvar->axes);
	fvar_iInstanceList.dispose(&fvar->instances);

	fvar_Master *current, *tmp;
	HASH_ITER(hh, fvar->masters, current, tmp) {
		HASH_DEL(fvar->masters, current);
		disposeFvarMaster(current);
		FREE(current);
	}
}

static const vq_Region *fvar_registerRegion(table_fvar *fvar, MOVE vq_Region *region) {
	fvar_Master *m = NULL;
	HASH_FIND(hh, fvar->masters, region, VQ_REGION_SIZE(region->dimensions), m);
	if (m) {
		vq_deleteRegion(region);
		return m->region;
	} else {
		NEW_CLEAN_1(m);
		sds sMasterID = sdsfromlonglong(1 + HASH_CNT(hh, fvar->masters));
		m->name = sdscatsds(sdsnew("m"), sMasterID);
		sdsfree(sMasterID);
		m->region = region;
		HASH_ADD_KEYPTR(hh, fvar->masters, m->region, VQ_REGION_SIZE(region->dimensions), m);
		return m->region;
	}
}

static const fvar_Master *fvar_findMasterByRegion(const table_fvar *fvar, const vq_Region *region) {
	fvar_Master *m = NULL;
	HASH_FIND(hh, fvar->masters, region, VQ_REGION_SIZE(region->dimensions), m);
	return m;
}

caryll_standardRefTypeFn(table_fvar, initFvar, disposeFvar);
caryll_ElementInterfaceOf(table_fvar) table_iFvar = {caryll_standardRefTypeMethods(table_fvar),
                                                     .registerRegion = fvar_registerRegion,
                                                     .findMasterByRegion = fvar_findMasterByRegion};

// Local typedefs for parsing axis record
#pragma pack(1)
struct FVARHeader {
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint16_t axesArrayOffset;
	uint16_t reserved1;
	uint16_t axisCount;
	uint16_t axisSize;
	uint16_t instanceCount;
	uint16_t instanceSize;
};

struct VariationAxisRecord {
	uint32_t axisTag;
	f16dot16 minValue;
	f16dot16 defaultValue;
	f16dot16 maxValue;
	uint16_t flags;
	uint16_t axisNameID;
};

struct InstanceRecord {
	uint16_t subfamilyNameID;
	uint16_t flags;
	f16dot16 coordinates[];
};
#pragma pack()

table_fvar *otfcc_readFvar(const otfcc_Packet packet, const otfcc_Options *options) {
	table_fvar *fvar = NULL;
	FOR_TABLE('fvar', table) {
		font_file_pointer data = table.data;
		if (table.length < sizeof(struct FVARHeader)) goto FAIL;

		struct FVARHeader *header = (struct FVARHeader *)data;
		if (be16(header->majorVersion) != 1) goto FAIL;
		if (be16(header->minorVersion) != 0) goto FAIL;
		if (be16(header->axesArrayOffset) == 0) goto FAIL;
		if (be16(header->axisCount) == 0) goto FAIL;
		if (be16(header->axisSize) != sizeof(struct VariationAxisRecord)) goto FAIL;
		uint16_t nAxes = be16(header->axisCount);
		uint16_t instanceSizeWithoutPSNID = 4 + nAxes * sizeof(f16dot16);
		uint16_t instanceSizeWithPSNID = 2 + instanceSizeWithoutPSNID;
		if (be16(header->instanceSize) != instanceSizeWithoutPSNID &&
		    be16(header->instanceSize) != instanceSizeWithPSNID)
			goto FAIL;
		if (table.length < be16(header->axesArrayOffset) +
		                       sizeof(struct VariationAxisRecord) * nAxes +
		                       be16(header->instanceSize) * be16(header->instanceCount))
			goto FAIL;

		fvar = table_iFvar.create();

		// parse axes
		struct VariationAxisRecord *axisRecord =
		    (struct VariationAxisRecord *)(data + be16(header->axesArrayOffset));
		for (uint16_t j = 0; j < nAxes; j++) {
			vf_Axis axis = {.tag = be32(axisRecord->axisTag),
			                .minValue = otfcc_from_fixed(be32(axisRecord->minValue)),
			                .defaultValue = otfcc_from_fixed(be32(axisRecord->defaultValue)),
			                .maxValue = otfcc_from_fixed(be32(axisRecord->maxValue)),
			                .flags = be16(axisRecord->flags),
			                .axisNameID = be16(axisRecord->axisNameID)};
			vf_iAxes.push(&fvar->axes, axis);
			axisRecord++;
		}

		// parse instances
		uint16_t nInstances = be16(header->instanceCount);
		bool hasPostscriptNameID = be16(header->instanceSize) == instanceSizeWithPSNID;
		struct InstanceRecord *instance = (struct InstanceRecord *)axisRecord;
		for (uint16_t j = 0; j < nInstances; j++) {
			fvar_Instance inst;
			fvar_iInstance.init(&inst);
			inst.subfamilyNameID = be16(instance->subfamilyNameID);
			inst.flags = be16(instance->flags);
			for (uint16_t k = 0; k < nAxes; k++) {
				iVV.push(&inst.coordinates, otfcc_from_fixed(be32(instance->coordinates[k])));
			}
			iVV.shrinkToFit(&inst.coordinates);
			if (hasPostscriptNameID) {
				inst.postScriptNameID =
				    be16(*(uint16_t *)(((font_file_pointer)instance) + instanceSizeWithoutPSNID));
			}
			fvar_iInstanceList.push(&fvar->instances, inst);
			instance = (struct InstanceRecord *)(((font_file_pointer)instance) +
			                                     be16(header->instanceSize));
		}
		vf_iAxes.shrinkToFit(&fvar->axes);
		fvar_iInstanceList.shrinkToFit(&fvar->instances);

		return fvar;

	FAIL:
		logWarning("table 'fvar' corrupted.\n");
		table_iFvar.free(fvar);
		fvar = NULL;
	}
	return NULL;
}

void otfcc_dumpFvar(const table_fvar *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("fvar") {
		json_value *t = json_object_new(2);
		// dump axes
		json_value *_axes = json_object_new(table->axes.length);
		foreach (vf_Axis *axis, table->axes) {
			json_value *_axis = json_object_new(5);
			json_object_push(_axis, "minValue", json_double_new(axis->minValue));
			json_object_push(_axis, "defaultValue", json_double_new(axis->defaultValue));
			json_object_push(_axis, "maxValue", json_double_new(axis->maxValue));
			json_object_push(_axis, "flags", json_integer_new(axis->flags));
			json_object_push(_axis, "axisNameID", json_integer_new(axis->axisNameID));
			json_object_push_tag(_axes, axis->tag, _axis);
		}
		json_object_push(t, "axes", _axes);
		// dump instances
		json_value *_instances = json_array_new(table->instances.length);
		foreach (fvar_Instance *instance, table->instances) {
			json_value *_instance = json_object_new(4);
			json_object_push(_instance, "subfamilyNameID",
			                 json_integer_new(instance->subfamilyNameID));
			if (instance->postScriptNameID) {
				json_object_push(_instance, "postScriptNameID",
				                 json_integer_new(instance->postScriptNameID));
			}
			json_object_push(_instance, "flags", json_integer_new(instance->flags));
			json_object_push(_instance, "coordinates", json_new_VVp(&instance->coordinates, table));
			json_array_push(_instances, _instance);
		}
		json_object_push(t, "instances", _instances);
		// dump masters
		json_value *_masters = json_object_new(HASH_COUNT(table->masters));
		fvar_Master *current, *tmp;
		HASH_ITER(hh, table->masters, current, tmp) {
			json_object_push(_masters, current->name,
			                 preserialize(json_new_VQRegion_Explicit(current->region, table)));
		}
		json_object_push(t, "masters", _masters);
		json_object_push(root, "fvar", t);
	}
}

// JSON conversion functions
// dump

json_value *json_new_VQSegment(const vq_Segment *s, const table_fvar *fvar) {
	switch (s->type) {
		case VQ_STILL:;
			return json_new_position(s->val.still);
		case VQ_DELTA:;
			json_value *d = json_object_new(3);
			json_object_push(d, "delta", json_new_position(s->val.delta.quantity));
			if (!s->val.delta.touched) {
				json_object_push(d, "implicit", json_boolean_new(!s->val.delta.touched));
			}
			json_object_push(d, "on", json_new_VQRegion(s->val.delta.region, fvar));
			return d;
		default:;
			return json_integer_new(0);
	}
}
json_value *json_new_VQ(const VQ z, const table_fvar *fvar) {

	if (!z.shift.length) {
		return preserialize(json_new_position(iVQ.getStill(z)));
	} else {
		json_value *a = json_array_new(z.shift.length + 1);
		json_array_push(a, json_new_position(z.kernel));
		for (size_t j = 0; j < z.shift.length; j++) {
			json_array_push(a, json_new_VQSegment(&z.shift.items[j], fvar));
		}
		return preserialize(a);
	}
}

json_value *json_new_VV(const VV x, const table_fvar *fvar) {
	const vf_Axes *axes = &fvar->axes;
	if (axes && axes->length == x.length) {
		json_value *_coord = json_object_new(axes->length);
		for (size_t m = 0; m < x.length; m++) {
			vf_Axis *axis = &axes->items[m];
			char tag[4] = {(axis->tag & 0xff000000) >> 24, (axis->tag & 0xff0000) >> 16,
			               (axis->tag & 0xff00) >> 8, (axis->tag & 0xff)};
			json_object_push_length(_coord, 4, tag, json_new_position(x.items[m]));
		}
		return preserialize(_coord);
	} else {
		json_value *_coord = json_array_new(x.length);
		for (size_t m = 0; m < x.length; m++) {
			json_array_push(_coord, json_new_position(x.items[m]));
		}
		return preserialize(_coord);
	}
}
json_value *json_new_VVp(const VV *x, const table_fvar *fvar) {
	const vf_Axes *axes = &fvar->axes;

	if (axes && axes->length == x->length) {
		json_value *_coord = json_object_new(axes->length);
		for (size_t m = 0; m < x->length; m++) {
			vf_Axis *axis = &axes->items[m];
			char tag[4] = {(axis->tag & 0xff000000) >> 24, (axis->tag & 0xff0000) >> 16,
			               (axis->tag & 0xff00) >> 8, (axis->tag & 0xff)};
			json_object_push_length(_coord, 4, tag, json_new_position(x->items[m]));
		}
		return preserialize(_coord);

	} else {
		json_value *_coord = json_array_new(x->length);
		for (size_t m = 0; m < x->length; m++) {
			json_array_push(_coord, json_new_position(x->items[m]));
		}
		return preserialize(_coord);
	}
}
// parse
VQ json_vqOf(const json_value *cv, const table_fvar *fvar) {
	return iVQ.createStill(json_numof(cv));
}

json_value *json_new_VQAxisSpan(const vq_AxisSpan *s) {
	if (vq_AxisSpanIsOne(s)) {
		return json_string_new("*");
	} else {
		json_value *a = json_object_new(3);
		json_object_push(a, "start", json_new_position(s->start));
		json_object_push(a, "peak", json_new_position(s->peak));
		json_object_push(a, "end", json_new_position(s->end));
		return a;
	}
}
json_value *json_new_VQRegion_Explicit(const vq_Region *rs, const table_fvar *fvar) {
	const vf_Axes *axes = &fvar->axes;
	if (axes && axes->length == rs->dimensions) {
		json_value *r = json_object_new(rs->dimensions);
		for (size_t j = 0; j < rs->dimensions; j++) {
			json_object_push_tag(r, axes->items[j].tag, json_new_VQAxisSpan(&rs->spans[j]));
		}
		return r;
	} else {
		json_value *r = json_array_new(rs->dimensions);
		for (size_t j = 0; j < rs->dimensions; j++) {
			json_array_push(r, json_new_VQAxisSpan(&rs->spans[j]));
		}
		return r;
	}
}
json_value *json_new_VQRegion(const vq_Region *rs, const table_fvar *fvar) {
	const fvar_Master *m = table_iFvar.findMasterByRegion(fvar, rs);
	if (m && m->name) {
		return json_string_new_length((unsigned int)sdslen(m->name), m->name);
	} else {
		return json_new_VQRegion_Explicit(rs, fvar);
	}
}
