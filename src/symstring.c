#include "dbg.h"
#include "symstring.h"

void
symstringInit(struct symstring* symstr)
{
  size_tArrayInit(&symstr->syms, 1);
}

void
symstringClean(struct symstring* symstr)
{
  size_tArrayClean(&symstr->syms);
}

void
symstringAdd(struct symstring* symstr, size_t symId)
{
  size_tArrayAdd(&symstr->syms, symId);
}

void
symstringAppend(struct symstring* a, const struct symstring* b)
{
  size_tArrayAppend(&a->syms, b->syms.vals, b->syms.size);
}

void
symstringInsert(struct symstring* a, size_t idx, const struct symstring* b)
{
  DEBUG_ASSERT(a->syms.size >= idx,
    "invalid index: size is %lu but index was %lu", a->syms.size, idx);
  if (b->syms.size == 0) {
    return;
  }
  struct size_tArray tmp;
  size_tArrayInit(&tmp, a->syms.size - idx);
  size_tArrayAppend(&tmp, &a->syms.vals[idx], a->syms.size - idx);
  a->syms.size = idx;
  symstringAppend(a, b);
  size_tArrayAppend(&a->syms, tmp.vals, tmp.size);
}

/* delete the item at idx */
void
symstringDelete(struct symstring* a, size_t idx)
{
  DEBUG_ASSERT(a->syms.size > idx,
    "invalid index: size is %lu but index was %lu", a->syms.size, idx);
  size_t i;
  for (i = idx; i < a->syms.size - 1; i++) {
    a->syms.vals[i] = a->syms.vals[i + 1];
  }
  a->syms.size--;
}

/* replace each occurrence of s by b */
void
symstringSubstitute(struct symstring* a, size_t s, const struct symstring* b)
{
  size_t i = 0;
  const size_t len = b->syms.size;
  while (i < a->syms.size) {
    if (a->syms.vals[i] == s) {
      symstringDelete(a, i);
      symstringInsert(a, i, b);
      i += len;
    } else {
      i++;
    }
  }
}
