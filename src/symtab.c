#include "dbg.h"
#include "symtab.h"
#include "memory.h"

void
symnodeInit(struct symnode* n)
{
  n->h = 0;
  n->p = NULL;
  n->next = NULL;
}

void
symnodeClean(struct symnode* n)
{
  if (n->next != NULL) {
    symnodeClean(n->next);
    free(n->next);
    n->next = NULL;
  }
}

void
symnodeInsert(struct symnode* n, uint32_t h, struct symbol* p)
{
  if (n->p == NULL) {
/* we have an empty node */
    n->h = h;
    n->p = p;
  } else {
    n->next = xmalloc(sizeof(struct symnode));
    symnodeInit(n->next);
    symnodeInsert(n->next, h, p);
  }
}

void
symtreeInit(struct symtree* t)
{
  symnodeInit(&t->node);
  t->less = NULL;
  t->more = NULL;
}

void
symtreeClean(struct symtree* t)
{
  if (t->less != NULL) {
    symtreeClean(t->less);
    free(t->less);
    t->less = NULL;
  }
  if (t->more != NULL) {
    symtreeClean(t->more);
    free(t->more);
    t->more = NULL;
  }
}

void
symtreeInsert(struct symtree* t, uint32_t h, struct symbol* p)
{
  if (t->node.p == NULL) {
/* we are in an empty leaf */
    t->node.h = h;
    t->node.p = p;
    t->node.next = NULL;
  } else {
    if (h < t->node.h) {
      t->less = xmalloc(sizeof(struct symtree));
      symtreeInit(t->less);
      symtreeInsert(t->less, h, p);
    } else if (h > t->node.h) {
      t->more = xmalloc(sizeof(struct symtree));
      symtreeInit(t->more);
      symtreeInsert(t->more, h, p);
    } else {
/* we have a key collision */
      symnodeInsert(&t->node, h, p);
    }
  }
}

struct symnode*
symtreeFind(struct symtree* t, uint32_t h)
{
  if (h < t->node.h) {
    return symtreeFind(t->less, h);
  } else if (h > t->node.h) {
    return symtreeFind(t->more, h);
  } else {
    return &t->node;
  }
}
