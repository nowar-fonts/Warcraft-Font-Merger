#include "private.h"

const char SCRIPT_LANGUAGE_SEPARATOR = '_';
const char *lookupFlagsLabels[] = {"rightToLeft", "ignoreBases", "ignoreLigatures", "ignoreMarks", NULL};
const char *tableNames[] = {[otl_type_unknown] = "unknown",
                            [otl_type_gsub_unknown] = "gsub_unknown",
                            [otl_type_gsub_single] = "gsub_single",
                            [otl_type_gsub_multiple] = "gsub_multiple",
                            [otl_type_gsub_alternate] = "gsub_alternate",
                            [otl_type_gsub_ligature] = "gsub_ligature",
                            [otl_type_gsub_context] = "gsub_context",
                            [otl_type_gsub_chaining] = "gsub_chaining",
                            [otl_type_gsub_extend] = "gsub_extend",
                            [otl_type_gsub_reverse] = "gsub_reverse",
                            [otl_type_gpos_unknown] = "gpos_unknown",
                            [otl_type_gpos_single] = "gpos_single",
                            [otl_type_gpos_pair] = "gpos_pair",
                            [otl_type_gpos_cursive] = "gpos_cursive",
                            [otl_type_gpos_markToBase] = "gpos_mark_to_base",
                            [otl_type_gpos_markToLigature] = "gpos_mark_to_ligature",
                            [otl_type_gpos_markToMark] = "gpos_mark_to_mark",
                            [otl_type_gpos_context] = "gpos_context",
                            [otl_type_gpos_chaining] = "gpos_chaining",
                            [otl_type_gpos_extend] = "gpos_extend"};
