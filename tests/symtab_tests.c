#include "unittest.h"
#include "symtab.h"

static int
test_symnodeInit(void)
{
  struct symnode n;
  symnodeInit(&n);
  ut_assert(n.p == NULL, "n.p not NULL");
  ut_assert(n.next == NULL, "n.next not NULL");
  symnodeClean(&n);
  return 0;
}

static int
test_symnodeInsert(void)
{
  struct symnode n;
  symnodeInit(&n);
  struct symbol s;
  symnodeInsert(&n, 0, &s);
  ut_assert(n.p == &s, " n.p is wrong");
  struct symbol s2;
  symnodeInsert(&n, 0, &s2);
  ut_assert(n.p == &s, "n.p changed");
  ut_assert(n.next != NULL, "n.next is NULL");
  ut_assert(n.next->p == &s2, "n.next->p is wrong");
  symnodeClean(&n);
  return 0;
}

static int
test_symtreeInsert(void)
{
  struct symtree t;
  symtreeInit(&t);
  struct symbol s1, s2, s3, s4;
  symtreeInsert(&t, 50, &s1);
  ut_assert(t.node.p == &s1, "t.node.p wrong");
  symtreeInsert(&t, 25, &s2);
  ut_assert(t.less != NULL, "t.less is NULL");
  ut_assert(t.less->node.p == &s2, "t.less->node.p is wrong");
  symtreeInsert(&t, 75, &s3);
  ut_assert(t.more != NULL, "t.more is NULL");
  ut_assert(t.more->node.p == &s3, "t.more->node.p is wrong");
  symtreeInsert(&t, 50, &s4);
  ut_assert(t.node.next != NULL, "t.node.next is NULL");
  return 0;
}

static int
all(void)
{
  ut_run(test_symnodeInit);
  ut_run(test_symnodeInsert);
  ut_run(test_symtreeInsert);
  return 0;
}

RUN(all)
