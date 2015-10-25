#include "unittest.h"
#include "symstring.h"

static int
test_symstringInsert(void)
{
  const size_t a[4] = {0, 1, 2, 3};
  const size_t b[3] = {4, 5, 6};
  const size_t c[7] = {0, 1, 4, 5, 6, 2, 3};
  struct symstring sa;
  struct symstring sb;
  symstringInit(&sa);
  symstringInit(&sb);
  size_tArrayAppend(&sa.syms, a, 4);
  size_tArrayAppend(&sb.syms, b, 3);
  symstringInsert(&sa, 2, &sb);
  ut_assert(sa.syms.size == 7, "size is %lu, expected 7", sa.syms.size);
  size_t i;
  for (i = 0; i < 7; i++) {
    ut_assert(sa.syms.vals[i] == c[i], "sa[%lu] == %lu, expected %lu", i,
      sa.syms.vals[i], c[i]);
  }
  return 0;
}

static int
test_symstringDelete(void)
{
  const size_t a[4] = {6, 7, 8, 9};
  const size_t b[3] = {6, 8, 9};
  struct symstring s;
  symstringInit(&s);
  size_tArrayAppend(&s.syms, a, 4);
  symstringDelete(&s, 1);
  ut_assert(s.syms.size == 3, "size is %lu, expected 3", s.syms.size);
  size_t i;
  for (i = 0; i < 3; i++) {
    ut_assert(s.syms.vals[i] == b[i], "s[%lu] == %lu, expected %lu", i,
      s.syms.vals[i], b[i]);
  }
  return 0;
}

static int
test_symstringSubstitute(void)
{
  const size_t a[5] = {1, 4, 1, 3};
  const size_t b[2] = {1, 2};
  const size_t c[7] = {1, 2, 4, 1, 2, 3};
  struct symstring sa;
  struct symstring sb;
  symstringInit(&sa);
  symstringInit(&sb);
  size_tArrayAppend(&sa.syms, a, 5);
  size_tArrayAppend(&sb.syms, b, 2);
  symstringSubstitute(&sa, 1, &sb);
  ut_assert(sa.syms.size == 7, "size is %lu, expected 7", sa.syms.size);
  size_t i;
  for (i = 0; i < 6; i++) {
    ut_assert(sa.syms.vals[i] == c[i], "s[%lu] == %lu, expected %lu", i,
      sa.syms.vals[i], c[i]);
  }
  return 0;
}

static int
test_all(void)
{
  ut_run(test_symstringInsert);
  ut_run(test_symstringDelete);
  ut_run(test_symstringSubstitute);
  return 0;
}

RUN(test_all)
