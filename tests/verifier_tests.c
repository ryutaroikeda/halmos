/* verifier_tests.c */
/* 15 Oct 2015 */
#include "unittest.h"
#include "reader.h"
#include "verifier.h"

static int
Test_frameInit(void)
{
  struct frame frm;
  frameInit(&frm);
  ut_assert(frm.stmts.size == 0, "size == %lu, expected 0", frm.stmts.size);
  frameClean(&frm);
  return 0;
}

static int
Test_verifierInit(void)
{
  char f[] = "";
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader file;
  readerInitString(&file, f);
  charArrayAppend(&file.filename, "test", 4);
  verifierAddFile(&vrf, &file);
  vrf.r = &vrf.files.vals[0];
  ut_assert(vrf.files.size == 1, "files.size == %lu, expected 1",
    vrf.files.size);
  // struct statement stmt;
  // statementInit(&stmt);
  // statementArrayAdd(&vrf.stmts, stmt);
  // ut_assert(vrf.stmts.size == 1, "stmts.size == %lu, expected 1",
   // vrf.stmts.size);
  // struct frame frm;
  // frameInit(&frm);
  // frameArrayAdd(&vrf.frames, frm);
  // ut_assert(vrf.frames.size == 1, "frames.size == %lu, expected 1", 
  //  vrf.frames.size);
  verifierClean(&vrf);
  return 0;
}

#define check_err(actual, expected) \
do { \
  ut_assert(actual == expected, "err = %s, expected %s", errorString(actual), \
   errorString(expected)); \
} while (0)

static int
Test_verifierParseSymbol(void)
{
  char* file;
  struct reader r;
  struct verifier vrf;
  int isEndOfStatement;
#define testfile(f, e) \
  do { \
    file = f; \
    readerInitString(&r, file); \
    verifierInit(&vrf); \
    verifierAddFile(&vrf, &r); \
    vrf.r = &vrf.files.vals[0]; \
    verifierParseSymbol(&vrf, &isEndOfStatement); \
    check_err(vrf.err, e); \
    verifierClean(&vrf); \
  } while (0)

  // testfile("$$", error_expectedEndStatement);
  // ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
  //  isEndOfStatement);
  // testfile("$..", error_expectedWhitespace);
  // ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
   // isEndOfStatement);
  // testfile("$.", error_expectedNewLine);
  // ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
  //  isEndOfStatement);
  testfile("forced:", error_unterminatedStatement);
  ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
   isEndOfStatement);
  testfile("bursts$. ", error_invalidSymbol);
  ut_assert(isEndOfStatement == 0, "isEndOfStatement is %d, expected 0",
   isEndOfStatement);
  testfile("$. ", error_none);
  ut_assert(isEndOfStatement == 1, "isEndOfStatement is %d, expected 1",
   isEndOfStatement);
#undef testfile
  return 0;
}

static int
Test_verifierParseConstants(void)
{
  char* file;
  struct reader r;
  struct verifier vrf;
#define testparse(f, e, n) \
  do { \
    file = f; \
    readerInitString(&r, file); \
    verifierInit(&vrf); \
    verifierAddFile(&vrf, &r); \
    vrf.r = &vrf.files.vals[0]; \
    verifierParseConstants(&vrf); \
    check_err(vrf.err, e); \
    ut_assert(vrf.symbols.size == n, "size == %lu, expected %lu", \
     vrf.symbols.size, n); \
    verifierClean(&vrf); \
  } while (0)
  testparse("", error_unterminatedStatement, (size_t)0);
  testparse("A savage place! as holy and enchanted $.\n", error_none,
   (size_t)7);
  readerInitString(&r, "savage $.\n");
  charArrayAppend(&r.filename, "file1", 5 + 1);
  verifierInit(&vrf);
  verifierAddFile(&vrf, &r);
  vrf.r = &vrf.files.vals[0];
  verifierParseConstants(&vrf);
  readerInitString(&r, "savage $.\n");
  charArrayAppend(&r.filename, "file2", 5 + 1);
  verifierAddFile(&vrf, &r);
  vrf.r = &vrf.files.vals[1];
  verifierParseConstants(&vrf);
  check_err(vrf.err, error_duplicateSymbol);
  verifierClean(&vrf);
  return 0;
#undef testparse
}

static int
Test_verifierParseVariables()
{
  char* file = "And here were forests ancient as the hills $.\n";
  struct reader r;
  struct verifier vrf;
  readerInitString(&r, file);
  charArrayAppend(&r.filename, "file", 4 + 1);
  verifierInit(&vrf);
  verifierAddFile(&vrf, &r);
  vrf.r = &vrf.files.vals[0];
  verifierParseVariables(&vrf);
  check_err(vrf.err, error_none);
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierIsValidDisjointPairSubstitution(void)
{
  struct verifier vrf;
  struct frame frm;
  struct substitution sub;
  verifierInit(&vrf);
  verifierAddSymbolExplicit(&vrf, "v1", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  verifierAddSymbolExplicit(&vrf, "v2", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  verifierAddSymbolExplicit(&vrf, "v3", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  LOG_INFO("added symbol");
  size_t v1 = verifierGetSymId(&vrf, "v1");
  size_t v2 = verifierGetSymId(&vrf, "v2");
  size_t v3 = verifierGetSymId(&vrf, "v3");
  ut_assert(!vrf.err, "failed to find symbol");
  frameInit(&frm);
  frameAddDisjoint(&frm, v1, v2);
  struct symstring str1, str2;
  symstringInit(&str1);
  symstringInit(&str2);
  symstringAdd(&str1, v3);
  symstringAdd(&str2, v3);
  substitutionInit(&sub);
  substitutionAdd(&sub, v1, &str1);
  substitutionAdd(&sub, v2, &str2);
  ut_assert(!verifierIsValidDisjointPairSubstitution(&vrf, &frm, &sub, 0, 1),
   "sub should be invalid");
/* str1 and str2 cleaned here */
  substitutionClean(&sub);
  frameClean(&frm);
  frameInit(&frm);
  frameAddDisjoint(&frm, v1, v2);
  symstringInit(&str1);
  symstringInit(&str2);
  symstringAdd(&str1, v1);
  symstringAdd(&str1, v2);
  symstringAdd(&str2, v3);
  substitutionInit(&sub);
  substitutionAdd(&sub, v1, &str1);
  substitutionAdd(&sub, v2, &str2);
  ut_assert(!verifierIsValidDisjointPairSubstitution(&vrf, &frm, &sub, 0, 1),
    "sub should be invalid");
  frameAddDisjoint(&frm, v1, v3);
  ut_assert(!verifierIsValidDisjointPairSubstitution(&vrf, &frm, &sub, 0, 1),
    "sub should be invalid");
  frameAddDisjoint(&frm, v2, v3);
  ut_assert(verifierIsValidDisjointPairSubstitution(&vrf, &frm, &sub, 0, 1),
    "sub should be valid");
  substitutionClean(&sub);
  frameClean(&frm);
  verifierClean(&vrf);
  return 0;
}

// static int
// Test_verifierIsValidSubstitution(void)
// {
//   struct verifier vrf;
//   struct substitution sub;
//   struct frame frm;
//   size_t v1, v2, v3;
//   verifierInit(&vrf);
//   substitutionInit(&sub);
//   frameInit(&frm);

// }

static int
all(void)
{
  ut_run(Test_frameInit);
  ut_run(Test_verifierInit);
  ut_run(Test_verifierParseSymbol);
  ut_run(Test_verifierParseConstants);
  ut_run(Test_verifierParseVariables);
  ut_run(Test_verifierIsValidDisjointPairSubstitution);
  return 0;
}

RUN(all)
