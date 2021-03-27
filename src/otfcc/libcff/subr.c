#include "subr.h"
/**
Type 2 CharString subroutinizer.
This program uses SEQUITUR (Nevill-Manning algorithm) to construct a CFG from the input sequence of
opcodes (in the minimum unit of a operator call. That is, operand* operator special*.)
Kieffer-Yang optimization is unnecessary, given that in almost all payloads, there are no repeating
subroutines.
*/

#ifdef DEBUG
static int nodesCreated = 0;
static int nodesRemoved = 0;
static int rulesCreated = 0;
static int rulesRemoved = 0;
#endif

static cff_SubrNode *cff_new_Node() {
	cff_SubrNode *n;
	NEW(n);
	n->rule = NULL;
	n->terminal = NULL;
	n->guard = false;
	n->hard = false;
	n->prev = NULL;
	n->next = NULL;
#ifdef DEBUG
	nodesCreated += 1;
#endif
	return n;
}

static cff_SubrRule *cff_new_Rule() {
	cff_SubrRule *r;
	NEW(r);
	r->refcount = 0;
	r->guard = cff_new_Node();
	r->guard->prev = r->guard;
	r->guard->next = r->guard;
	r->guard->terminal = 0;
	r->guard->guard = true;
	r->guard->rule = r;
	r->next = NULL;
#ifdef DEBUG
	rulesCreated += 1;
#endif
	return r;
}

static void initSubrGraph(cff_SubrGraph *g) {
	g->root = cff_new_Rule();
	g->last = g->root;
	g->diagramIndex = NULL;
	g->totalRules = 0;
	g->totalCharStrings = 0;
	g->doSubroutinize = false;
}

static void clean_Node(cff_SubrNode *x) {
	if (x->rule) { x->rule->refcount -= 1; }
	x->rule = NULL;
	buffree(x->terminal);
	x->terminal = NULL;
}
static void delete_Node(cff_SubrNode *x) {
	if (!x) return;
	clean_Node(x);
#ifdef DEBUG
	nodesRemoved += 1;
#endif
	FREE(x);
}

static void deleteFullRule(cff_SubrRule *r) {
	if (r->guard) {
		for (cff_SubrNode *e = r->guard->next; e != r->guard;) {
			cff_SubrNode *next = e->next;
			if (e->terminal) buffree(e->terminal);
			FREE(e);
#ifdef DEBUG
			nodesRemoved += 1;
#endif
			e = next;
		}
		{
			FREE(r->guard);
#ifdef DEBUG
			nodesRemoved += 1;
#endif
		}
	}

	FREE(r);
#ifdef DEBUG
	rulesRemoved += 1;
#endif
}

static void disposeSubrGraph(cff_SubrGraph *g) {
	{
		cff_SubrRule *r = g->root;
		while (r) {
			cff_SubrRule *next = r->next;
			deleteFullRule(r);
			r = next;
		}
	}
	cff_SubrDiagramIndex *s, *tmp;
	HASH_ITER(hh, g->diagramIndex, s, tmp) {
		HASH_DEL(g->diagramIndex, s);
		FREE(s->key);
		FREE(s);
	}
#ifdef DEBUG
	fprintf(stderr, "ALLOC: %d >< %d nodes\n", nodesCreated, nodesRemoved);
	fprintf(stderr, "ALLOC: %d >< %d rules\n", rulesCreated, rulesRemoved);
#endif
}

caryll_standardRefType(cff_SubrGraph, cff_iSubrGraph, initSubrGraph, disposeSubrGraph);

// Subroutinizer

static void joinNodes(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n);

static uint8_t *getSingletHashKey(cff_SubrNode *n, size_t *len) {
	size_t l1;
	if (n->rule) {
		l1 = sizeof(n->rule->uniqueIndex);
	} else {
		l1 = buflen(n->terminal) * sizeof(uint8_t);
	}

	*len = 3 + l1 + 1;
	uint8_t *key;
	NEW(key, *len);
	key[0] = '1';
	key[1] = (n->rule ? '1' : '0');
	key[2] = '0';
	key[*len - 1] = 0;

	if (n->rule) {
		memcpy(key + 3, &(n->rule->uniqueIndex), l1);
	} else {
		memcpy(key + 3, n->terminal->data, l1);
	}
	return key;
}

static uint8_t *getDoubletHashKey(cff_SubrNode *n, size_t *len) {
	size_t l1, l2;
	if (n->rule) {
		l1 = sizeof(n->rule->uniqueIndex);
	} else {
		l1 = buflen(n->terminal) * sizeof(uint8_t);
	}
	if (n->next->rule) {
		l2 = sizeof(n->next->rule->uniqueIndex);
	} else {
		l2 = buflen(n->next->terminal) * sizeof(uint8_t);
	}
	*len = 3 + l1 + l2 + 1;
	uint8_t *key;
	NEW(key, *len);
	key[0] = '2';
	key[1] = (n->rule ? '1' : '0');
	key[2] = (n->next->rule ? '1' : '0');
	key[*len - 1] = 0;
	if (n->rule) {
		memcpy(key + 3, &(n->rule->uniqueIndex), l1);
	} else {
		memcpy(key + 3, n->terminal->data, l1);
	}
	if (n->next->rule) {
		memcpy(key + 3 + l1, &(n->next->rule->uniqueIndex), l2);
	} else {
		memcpy(key + 3 + l1, n->next->terminal->data, l2);
	}
	return key;
}

static cff_SubrNode *lastNodeOf(cff_SubrRule *r) {
	return r->guard->prev;
}

static cff_SubrNode *copyNode(cff_SubrNode *n) {
	cff_SubrNode *m = cff_new_Node();
	if (n->rule) {
		m->rule = n->rule;
		m->rule->refcount += 1;
	} else {
		m->terminal = bufnew();
		bufwrite_buf(m->terminal, n->terminal);
	}
	m->last = n->last;
	return m;
}

// checkNode: check whether node N is shrinkable
static bool checkDoubletMatch(cff_SubrGraph *g, cff_SubrNode *n);

static void unlinkNode(cff_SubrGraph *g, cff_SubrNode *a) {
	if (a->hard || a->guard) return;
	size_t len;
	uint8_t *key = getDoubletHashKey(a, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (di && di->start == a) {
		HASH_DEL(g->diagramIndex, di);
		FREE(di->key);
		FREE(di);
	}
	FREE(key);
	key = getSingletHashKey(a, &len);
	di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (di && di->start == a) {
		HASH_DEL(g->diagramIndex, di);
		FREE(di->key);
		FREE(di);
	}
	FREE(key);
	return;
}

static void addDoublet(cff_SubrGraph *g, cff_SubrNode *n) {
	if (!n || !n->next || n->guard || n->hard || n->next->hard || n->next->guard) return;
	size_t len;
	uint8_t *key = getDoubletHashKey(n, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (!di) {
		NEW(di);
		di->arity = 2;
		di->key = key;
		di->start = n;
		HASH_ADD_KEYPTR(hh, g->diagramIndex, key, len, di);
	} else {
		di->start = n;
		FREE(key);
	}
}
static void addSinglet(cff_SubrGraph *g, cff_SubrNode *n) {
	if (!n || n->guard || n->hard) return;
	size_t len;
	uint8_t *key = getSingletHashKey(n, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (!di) {
		NEW(di);
		di->arity = 1;
		di->key = key;
		di->start = n;
		HASH_ADD_KEYPTR(hh, g->diagramIndex, key, len, di);
	} else {
		di->start = n;
		FREE(key);
	}
}

static bool identNode(cff_SubrNode *m, cff_SubrNode *n) {
	if (m->rule)
		return (m->rule == n->rule);
	else if (n->rule)
		return false;
	else
		return (m->terminal->size == n->terminal->size &&
		        strncmp((char *)m->terminal->data, (char *)n->terminal->data, m->terminal->size) == 0);
}
static void joinNodes(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n) {
	if (m->next) {
		unlinkNode(g, m);
		if (n->prev && n->next && identNode(n->prev, n) && identNode(n, n->next)) { addDoublet(g, n); }
		if (m->prev && m->next && identNode(m->prev, m) && identNode(m, m->next)) { addDoublet(g, m->prev); }
	}
	m->next = n;
	n->prev = m;
}
static void xInsertNodeAfter(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n) {
	joinNodes(g, n, m->next);
	joinNodes(g, m, n);
}
static void removeNodeFromGraph(cff_SubrGraph *g, cff_SubrNode *a) {
	joinNodes(g, a->prev, a->next);
	if (!a->guard) {
		unlinkNode(g, a);
		delete_Node(a);
	}
}

static void expandCall(cff_SubrGraph *g, cff_SubrNode *a) {
	cff_SubrNode *aprev = a->prev;
	cff_SubrNode *anext = a->next;
	cff_SubrRule *r = a->rule;
	cff_SubrNode *r1 = r->guard->next;
	cff_SubrNode *r2 = r->guard->prev;

	// We should move out [a, a'] from g's diagramIndex
	unlinkNode(g, a);

	joinNodes(g, aprev, r1);
	joinNodes(g, r2, anext);
	addDoublet(g, r2);
	// make this rule a stub.
	r->guard->prev = r->guard->next = r->guard;
	r->refcount -= 1;
	// remove call node
	delete_Node(a);
}

static void substituteDoubletWithRule(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrRule *r) {
	cff_SubrNode *prev = m->prev;
	removeNodeFromGraph(g, prev->next);
	removeNodeFromGraph(g, prev->next);
	cff_SubrNode *invoke = cff_new_Node();
	invoke->rule = r;
	invoke->rule->refcount += 1;
	xInsertNodeAfter(g, prev, invoke);
	addDoublet(g, prev);
	addDoublet(g, invoke);
	addSinglet(g, invoke);

	if (!checkDoubletMatch(g, prev)) { checkDoubletMatch(g, prev->next); }
}
static void substituteSingletWithRule(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrRule *r) {
	cff_SubrNode *prev = m->prev;
	removeNodeFromGraph(g, prev->next);
	cff_SubrNode *invoke = cff_new_Node();
	invoke->rule = r;
	invoke->rule->refcount += 1;
	xInsertNodeAfter(g, prev, invoke);
	addDoublet(g, prev);
	addDoublet(g, invoke);
	addSinglet(g, invoke);
}

static void processMatchDoublet(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n) {
	cff_SubrRule *rule = NULL;
	if (m->prev->guard && m->next->next->guard) {
		// The match [m, m'] is a rule's full content
		rule = m->prev->rule;
		substituteDoubletWithRule(g, n, rule);
	} else {
		rule = cff_new_Rule();
		rule->uniqueIndex = g->totalRules;
		g->totalRules += 1;
		g->last->next = rule;
		g->last = rule;
		xInsertNodeAfter(g, lastNodeOf(rule), copyNode(m));
		xInsertNodeAfter(g, lastNodeOf(rule), copyNode(m->next));
		substituteDoubletWithRule(g, m, rule);
		substituteDoubletWithRule(g, n, rule);
		addDoublet(g, rule->guard->next);
		addSinglet(g, rule->guard->next);
		addSinglet(g, rule->guard->next->next);
	}

	if (rule->guard->next->rule && rule->guard->next->rule->refcount == 1) {
		// The rule is shrinkable.
		expandCall(g, rule->guard->next);
	}
}
static void processMatchSinglet(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n) {
	cff_SubrRule *rule = NULL;
	if (m->prev->guard && m->next->guard) {
		// The match [m] is a rule's full content
		rule = m->prev->rule;
		substituteSingletWithRule(g, n, rule);
	} else {
		// Create a new rule
		rule = cff_new_Rule();
		rule->uniqueIndex = g->totalRules;
		g->totalRules += 1;
		g->last->next = rule;
		g->last = rule;
		xInsertNodeAfter(g, lastNodeOf(rule), copyNode(m));
		substituteSingletWithRule(g, m, rule);
		substituteSingletWithRule(g, n, rule);
		addSinglet(g, rule->guard->next);
	}
}

static bool checkDoubletMatch(cff_SubrGraph *g, cff_SubrNode *n) {
	if (n->guard || n->next->guard || n->hard || n->next->hard) return false;
	// printf("test "), printNode(n, false), printNode(n->next, true);
	size_t len;
	uint8_t *key = getDoubletHashKey(n, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (!di) {
		NEW(di);
		di->arity = 2;
		di->key = key;
		di->start = n;
		HASH_ADD_KEYPTR(hh, g->diagramIndex, key, len, di);
		return false;
	} else if (di->arity == 2 && di->start != n && !di->start->guard && !di->start->next->guard) {
		FREE(key);
		processMatchDoublet(g, di->start, n);
		return true;
	} else {
		FREE(key);
		return true;
	}
}

static bool checkSingletMatch(cff_SubrGraph *g, cff_SubrNode *n) {
	if (n->guard || n->hard) return false;
	size_t len;
	uint8_t *key = getSingletHashKey(n, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (!di) {
		NEW(di);
		di->arity = 1;
		di->key = key;
		di->start = n;
		HASH_ADD_KEYPTR(hh, g->diagramIndex, key, len, di);
		return false;
	} else if (di->arity == 1 && di->start != n && !di->start->guard) {
		FREE(key);
		processMatchSinglet(g, di->start, n);
		return true;
	} else {
		FREE(key);
		return false;
	}
}

static void appendNodeToGraph(cff_SubrGraph *g, cff_SubrNode *n) {
	cff_SubrNode *last = lastNodeOf(g->root);
	xInsertNodeAfter(g, last, n);
	if (g->doSubroutinize) {
		if (!checkDoubletMatch(g, last)) {
			if (buflen(n->terminal) > 15) checkSingletMatch(g, n);
		}
	}
}

void cff_insertILToGraph(cff_SubrGraph *g, cff_CharstringIL *il) {
	caryll_Buffer *blob = bufnew();
	bool flush = false;
	bool last = false;
	for (uint32_t j = 0; j < il->length; j++) {
		switch (il->instr[j].type) {
			case IL_ITEM_OPERAND: {
				if (flush) {
					cff_SubrNode *n = cff_new_Node();
					n->rule = NULL;
					n->terminal = blob;
					n->last = last;
					appendNodeToGraph(g, n);
					blob = bufnew();
					flush = false;
				}
				cff_mergeCS2Operand(blob, il->instr[j].d);
				break;
			}

			case IL_ITEM_OPERATOR: {
				cff_mergeCS2Operator(blob, il->instr[j].i);
				if (il->instr[j].i == op_endchar) { last = true; }
				flush = true;
				break;
			}
			case IL_ITEM_SPECIAL: {
				cff_mergeCS2Special(blob, il->instr[j].i);
				flush = true;
				break;
			}
			default:
				break;
		}
	}
	if (blob->size) {
		cff_SubrNode *n = cff_new_Node();
		n->rule = NULL;
		n->last = last;
		n->terminal = blob;
		appendNodeToGraph(g, n);
	}
	{
		blob = bufnew();
		cff_SubrNode *n = cff_new_Node();
		n->rule = NULL;
		n->terminal = blob;
		n->hard = true;
		appendNodeToGraph(g, n);
		g->totalCharStrings += 1;
	}
}

static void cff_statHeight(cff_SubrRule *r, uint32_t height) {
	if (height > r->height) r->height = height;
	// Stat the heights bottom-up.
	uint32_t effectiveLength = 0;
	for (cff_SubrNode *e = r->guard->next; e != r->guard; e = e->next) {
		if (e->rule) {
			cff_statHeight(e->rule, height + 1);
			effectiveLength += 4;
		} else {
			effectiveLength += e->terminal->size;
		}
	}
	r->effectiveLength = effectiveLength;
}

static void numberASubroutine(cff_SubrRule *r, uint32_t *current) {
	if (r->numbered) return;
	if (r->height >= type2_subr_nesting) return;
	if ((r->effectiveLength - 4) * (r->refcount - 1) - 4 <= 0) return;
	r->number = *current;
	(*current)++;
	r->numbered = true;
	for (cff_SubrNode *e = r->guard->next; e != r->guard; e = e->next) {
		if (e->rule) { numberASubroutine(e->rule, current); }
	}
}
static uint32_t cff_numberSubroutines(cff_SubrGraph *g) {
	uint32_t current = 0;
	for (cff_SubrNode *e = g->root->guard->next; e != g->root->guard; e = e->next) {
		if (e->rule) numberASubroutine(e->rule, &current);
	}
	return current;
}

static inline int32_t subroutineBias(int32_t cnt) {
	if (cnt < 1240)
		return 107;
	else if (cnt < 33900)
		return 1131;
	else
		return 32768;
}

static bool endsWithEndChar(cff_SubrRule *rule) {
	cff_SubrNode *node = lastNodeOf(rule);
	if (node->terminal) {
		return node->last;
	} else {
		return endsWithEndChar(node->rule);
	}
}

static void serializeNodeToBuffer(cff_SubrNode *node, caryll_Buffer *buf, caryll_Buffer *gsubrs, uint32_t maxGSubrs,
                                  caryll_Buffer *lsubrs, uint32_t maxLSubrs) {
	if (node->rule) {
		if (node->rule->numbered && node->rule->number < maxLSubrs + maxGSubrs &&
		    node->rule->height < type2_subr_nesting) {
			// A call.
			caryll_Buffer *target;
			if (node->rule->number < maxLSubrs) {
				int32_t stacknum = node->rule->number - subroutineBias(maxLSubrs);
				target = lsubrs + node->rule->number;
				cff_mergeCS2Int(buf, stacknum);
				cff_mergeCS2Operator(buf, op_callsubr);
			} else {
				int32_t stacknum = node->rule->number - maxLSubrs - subroutineBias(maxGSubrs);
				target = gsubrs + (node->rule->number - maxLSubrs);
				cff_mergeCS2Int(buf, stacknum);
				cff_mergeCS2Operator(buf, op_callgsubr);
			}
			cff_SubrRule *r = node->rule;
			if (!r->printed) {
				r->printed = true;
				for (cff_SubrNode *e = r->guard->next; e != r->guard; e = e->next) {
					serializeNodeToBuffer(e, target, gsubrs, maxGSubrs, lsubrs, maxLSubrs);
				}
				if (!endsWithEndChar(r)) { cff_mergeCS2Operator(target, op_return); }
			}
		} else {
			// A call, but invalid
			// Inline its code.
			cff_SubrRule *r = node->rule;
			for (cff_SubrNode *e = r->guard->next; e != r->guard; e = e->next) {
				serializeNodeToBuffer(e, buf, gsubrs, maxGSubrs, lsubrs, maxLSubrs);
			}
		}
	} else {
		bufwrite_buf(buf, node->terminal);
	}
}

static caryll_Buffer *from_array(void *_context, uint32_t j) {
	caryll_Buffer *context = (caryll_Buffer *)_context;
	caryll_Buffer *blob = bufnew();
	bufwrite_buf(blob, context + j);
	return blob;
}
void cff_ilGraphToBuffers(cff_SubrGraph *g, caryll_Buffer **s, caryll_Buffer **gs, caryll_Buffer **ls,
                          const otfcc_Options *options) {
	cff_statHeight(g->root, 0);
	uint32_t maxSubroutines = cff_numberSubroutines(g);
	logProgress("[libcff] Total %d subroutines extracted.", maxSubroutines);
	uint32_t maxLSubrs = maxSubroutines;
	uint32_t maxGSubrs = 0;
	{
		// balance
		if (maxLSubrs > type2_max_subrs) {
			maxLSubrs = type2_max_subrs;
			maxGSubrs = maxSubroutines - maxLSubrs;
		}
		if (maxGSubrs > type2_max_subrs) { maxGSubrs = type2_max_subrs; }
		uint32_t total = maxLSubrs + maxGSubrs;
		maxLSubrs = total / 2;
		maxGSubrs = total - maxLSubrs;
	}
	caryll_Buffer *charStrings, *gsubrs, *lsubrs;
	NEW(charStrings, g->totalCharStrings + 1);
	NEW(lsubrs, maxLSubrs + 1);
	NEW(gsubrs, maxGSubrs + 1);
	uint32_t j = 0;
	cff_SubrRule *r = g->root;
	for (cff_SubrNode *e = r->guard->next; e != r->guard; e = e->next) {
		serializeNodeToBuffer(e, charStrings + j, gsubrs, maxGSubrs, lsubrs, maxLSubrs);
		if (!e->rule && e->terminal && e->hard) { j++; }
	}

	cff_Index *is = cff_iIndex.fromCallback(charStrings, g->totalCharStrings, from_array);
	cff_Index *igs = cff_iIndex.fromCallback(gsubrs, maxGSubrs, from_array);
	cff_Index *ils = cff_iIndex.fromCallback(lsubrs, maxLSubrs, from_array);

	for (uint32_t j = 0; j < g->totalCharStrings; j++) {
		FREE((charStrings + j)->data);
	}
	for (uint32_t j = 0; j < maxGSubrs; j++) {
		FREE((gsubrs + j)->data);
	}
	for (uint32_t j = 0; j < maxLSubrs; j++) {
		FREE((lsubrs + j)->data);
	}
	FREE(charStrings), FREE(gsubrs), FREE(lsubrs);

	*s = cff_iIndex.build(is), *gs = cff_iIndex.build(igs), *ls = cff_iIndex.build(ils);
	cff_iIndex.free(is), cff_iIndex.free(igs), cff_iIndex.free(ils);
}
