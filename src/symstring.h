#ifndef _HALMOSSYMSTRING_H_
#define _HALMOSSYMSTRING_H_

#include "array.h"

struct symstring;
typedef struct symstring symstring;
DECLARE_ARRAY(symstring)

struct symstring {
  struct size_tArray syms;
};

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
void
symstringSubstitute(struct symstring* a, size_t s, const struct symstring* b);

#endif
