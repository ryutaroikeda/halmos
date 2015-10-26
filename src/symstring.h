#ifndef _HALMOSSYMSTRING_H_
#define _HALMOSSYMSTRING_H_

#include "array.h"

typedef struct size_tArray symstring;
DECLARE_ARRAY(symstring)

/* we do this to be able to write 'struct symstring' */
#define symstring size_tArray

void
symstringInit(struct symstring* symstr);

void
symstringClean(struct symstring* symstr);

void
symstringAdd(struct symstring* symstr, size_t symId);

void
symstringAppend(struct symstring* a, const struct symstring* b);

void
symstringInsert(struct symstring* a, size_t idx, const struct symstring* b);

void
symstringDelete(struct symstring* a, size_t idx);

/* substitute every occurence of s in a by b */
/* use substitutionSubstitute instead, for doing simultaneous substitutions */
void
symstringSubstitute(struct symstring* a, size_t s, const struct symstring* b);

struct substitution {
  struct size_tArray vars;
  struct symstringArray subs;
/* used for simultaneous substitution */
  struct symstring isMarked;
};

void
substitutionInit(struct substitution* sub);

void
substitutionClean(struct substitution* sub);

void
substitutionAdd(struct substitution* sub, size_t var, struct symstring* str);

void
substitutionUnmark(struct substitution* sub, size_t len);

/* do the substitution for one variable. varId is the index to sub->vars. */
/* Mark positions in the string where a substituted string occurs. */
void
substitutionSubstitute(struct substitution* sub, size_t varId,
  struct symstring* str);

/* do a simultaneous substitution */
void
substitutionApply(struct substitution* sub, struct symstring* str);

#endif
