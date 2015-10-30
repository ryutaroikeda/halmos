/* verifier_tests.c */
/* 15 Oct 2015 */
#include "unittest.h"
#include "reader.h"
#include "verifier.h"

#define check_err(actual, expected) \
do { \
  ut_assert(actual == expected, "err = %s, expected %s", errorString(actual), \
   errorString(expected)); \
} while (0)

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
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierDeactivateSymbols(void)
{
  struct verifier vrf;
  struct reader r;
  verifierInit(&vrf);
  readerInitString(&r, "");
  verifierAddFile(&vrf, &r);
  verifierBeginReadingFile(&vrf, 0);
  verifierDeactivateSymbols(&vrf);
  check_err(vrf.err, error_none);
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierAddDisjoint(void)
{
  enum {
    disjoints_size = 5
  };
  const size_t disjoints[disjoints_size] = {0, 1, 2, 3, 4};
  struct symstring d;
  symstringInit(&d);
  size_tArrayAppend(&d, disjoints, 5);
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader r;
  readerInitString(&r, "");
  verifierAddFile(&vrf, &r);
  verifierBeginReadingFile(&vrf, 0);
  verifierAddDisjoint(&vrf, &d);
  check_err(vrf.err, error_none);
  size_t size = vrf.active[symType_disjoint].vals[0].size;
  ut_assert(size == 10, 
    "added %lu disjoint pairs, expected 10", size);
  verifierClean(&vrf);
  return 0;
}

static int 
Test_verifierMakeFrame(void)
{
  size_t i;
  enum {
    cst_size = 5,
    var_size = 4,
    flt_size = 4,
    ess_size = 1
  };
  const char* cstSyms[cst_size] = {"|-", "wff", "0", "1", "+"};
  size_t cstIds[cst_size];
  const char* varSyms[var_size] = {"x", "y", "z", "w"};
  size_t varIds[var_size];
  const char* fltSyms[flt_size] = {"wff_x", "wff_y", "wff_z", "wff_w"};
  size_t fltIds[flt_size];
  size_t fltSS[flt_size][2];
  const char* essSyms[ess_size] = {"plus"};
  // size_t essIds[ess_size];
  struct verifier vrf;
  verifierInit(&vrf);
/* make a mock file because verifierAdd... requires vrf->rId to be valid */
  LOG_DEBUG("making mock file because verifierAdd...() requires valid "
    "vrf->rId");
  struct reader file;
  readerInitString(&file, "");
  verifierAddFile(&vrf, &file);
  verifierBeginReadingFile(&vrf, 0);
  LOG_DEBUG("adding constants");
  for (i = 0; i < cst_size; i++) {
    cstIds[i] = verifierAddConstant(&vrf, cstSyms[i]);
  }
  LOG_DEBUG("adding variables");
  for (i = 0; i < var_size; i++) {
    varIds[i] = verifierAddVariable(&vrf, varSyms[i]);
  }
  LOG_DEBUG("preparing floats");
  struct symstring wff[flt_size];
  for (i = 0; i < flt_size; i++) {
    fltSS[i][0] = cstIds[1];
    fltSS[i][1] = varIds[i];
    symstringInit(&wff[i]);
    size_tArrayAppend(&wff[i], fltSS[i], 2);
  }
  LOG_DEBUG("preparing essentials");
  struct symstring plus;
  size_t ssplus[4] = {cstIds[0], varIds[0], cstIds[4], varIds[1]};
  symstringInit(&plus);
  size_tArrayAppend(&plus, ssplus, 4);
  LOG_DEBUG("adding floats and essentials");
  fltIds[0] = verifierAddFloating(&vrf, fltSyms[0], &wff[0]);
  fltIds[1] = verifierAddFloating(&vrf, fltSyms[1], &wff[1]);
  verifierAddEssential(&vrf, essSyms[0], &plus);
  fltIds[2] = verifierAddFloating(&vrf, fltSyms[2], &wff[2]);
  fltIds[3] = verifierAddFloating(&vrf, fltSyms[3], &wff[3]);
  check_err(vrf.err, error_none);
  LOG_DEBUG("preparing assertion");
  struct symstring asr;
  symstringInit(&asr);
  size_t ssasr[6] =
  {cstIds[0], varIds[0], cstIds[4], varIds[1], cstIds[4], varIds[3]};
  size_tArrayAppend(&asr, ssasr, 6);
  LOG_DEBUG("making the frame");
  struct frame frm;
  frameInit(&frm);
  LOG_DEBUG("calling verifierMakeFrame()");
  verifierMakeFrame(&vrf, &frm, &asr);
  check_err(vrf.err, error_none);
  LOG_DEBUG("make sure wff_w has not been added to the frame");
  for (i = 0; i < frm.stmts.size; i++) {
    ut_assert(frm.stmts.vals[i] != fltIds[3],
      "wff_w should not be in the frame");
  }
  LOG_DEBUG("cleaning up");
  frameClean(&frm);
  symstringClean(&asr);
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
/* build the substitution */
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

static int
Test_verifierIsValidSubstitution(void)
{
  struct verifier vrf;
  struct substitution sub;
  struct frame frm;
  size_t v1, v2, v3;
  verifierInit(&vrf);
  verifierAddSymbolExplicit(&vrf, "v1", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  verifierAddSymbolExplicit(&vrf, "v2", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  verifierAddSymbolExplicit(&vrf, "v3", symType_variable, 0, 0, 0, 0, 0, 0, 0,
   0);
  v1 = verifierGetSymId(&vrf, "v1");
  v2 = verifierGetSymId(&vrf, "v2");
  v3 = verifierGetSymId(&vrf, "v3");
  frameInit(&frm);
/* build the substitution */
  struct symstring str1, str2, str3;
  symstringInit(&str1);
  symstringInit(&str2);
  symstringInit(&str3);
  symstringAdd(&str1, v1);
  symstringAdd(&str2, v1);
  symstringAdd(&str3, v3);
  substitutionInit(&sub);
  substitutionAdd(&sub, v1, &str1);
  substitutionAdd(&sub, v2, &str2);
  substitutionAdd(&sub, v3, &str3);
  ut_assert(verifierIsValidSubstitution(&vrf, &frm, &sub), "invalid sub");
  frameAddDisjoint(&frm, v2, v3);
  ut_assert(!verifierIsValidSubstitution(&vrf, &frm, &sub),
    "sub should be invalid");
  frameAddDisjoint(&frm, v1, v3);
  ut_assert(verifierIsValidSubstitution(&vrf, &frm, &sub), "invalid sub");
  substitutionClean(&sub);
  frameClean(&frm);
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierUnify(void)
{
  const size_t type = 0;
  const size_t var = 1;
  const size_t type2 = 2;
  const size_t strVals[4] = {type, var, var, var};
  const size_t floatingVals[2] = {type, var};
  const size_t floatingVals2[2] = {type2, var};
  struct verifier vrf;
  struct substitution sub;
  struct symstring str;
  struct symstring floating;
  verifierInit(&vrf);
  verifierAddSymbolExplicit(&vrf, "type", symType_constant, 0, 0, 0, 0, 0, 0,
    0, 0);
  verifierAddSymbolExplicit(&vrf, "v", symType_variable, 0, 0, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "type2", symType_constant, 0, 0, 0, 0, 0, 0,
    0, 0);
  symstringInit(&str);
  symstringInit(&floating);
  size_tArrayAppend(&str, strVals, 4);
  size_tArrayAppend(&floating, floatingVals, 2);
  substitutionInit(&sub);
  verifierUnify(&vrf, &sub, &str, &floating);
  ut_assert(!vrf.err, "unification failed");
  symstringClean(&floating);
  symstringInit(&floating);
  symstringClean(&str);
  symstringInit(&str);
  size_tArrayAppend(&str, strVals, 4);
  size_tArrayAppend(&floating, floatingVals2, 2);
  verifierUnify(&vrf, &sub, &str, &floating);
  ut_assert(vrf.err == error_mismatchedType, "unifciation should have failed");
  symstringClean(&str);
  symstringClean(&floating);
  substitutionClean(&sub);
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierApplyAssertion(void)
{
  const size_t t = 1;
  const size_t x = 2;
  const size_t y = 3;
  const size_t a = 4;
  const size_t b = 5;
  const size_t tx = 6;
  const size_t ty = 7;
  const size_t txy = 8;
  const size_t tyx = 9;
  const size_t tx_f[2] = {t, x};
  const size_t ty_f[2] = {t, y};
  const size_t txy_e[3] = {t, x, y};
  const size_t tyx_a[3] = {t, y, x};
  const size_t frame_a[3] = {tx, ty, txy};
  const size_t stack1[2] = {t, a};
  const size_t stack2[2] = {t, b};
  const size_t stack3[3] = {t, a, b};
  const size_t res[3] = {t, b, a};
  struct verifier vrf;
  struct symstring stmt;
  struct frame frm;
  verifierInit(&vrf);
  verifierAddSymbolExplicit(&vrf, "t", symType_constant, 1, 0, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "x", symType_variable, 1, 1, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "y", symType_variable, 1, 1, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "a", symType_variable, 1, 1, 0, 0, 0, 0, 0,
    0);
  verifierAddSymbolExplicit(&vrf, "b", symType_variable, 1, 1, 0, 0, 0, 0, 0,
    0);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, tx_f, 2);
  verifierAddStatement(&vrf, &stmt);
  verifierAddSymbolExplicit(&vrf, "tx", symType_floating, 1, 0, 0, 0, 0, 0, 0,
    0);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, ty_f, 2);
  verifierAddStatement(&vrf, &stmt);
  verifierAddSymbolExplicit(&vrf, "ty", symType_floating, 1, 0, 0, 1, 0, 0,
    0, 0);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, txy_e, 3);
  verifierAddStatement(&vrf, &stmt);
  verifierAddSymbolExplicit(&vrf, "txy", symType_essential, 1, 0, 0, 2, 0, 0,
    0, 0);
/* the frame for tyx */
  frameInit(&frm);
  size_tArrayAppend(&frm.stmts, frame_a, 3);
  verifierAddFrame(&vrf, &frm);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, tyx_a, 3);
  verifierAddStatement(&vrf, &stmt);
  verifierAddSymbolExplicit(&vrf, "tyx", symType_assertion, 1, 0, 0, 3, 0, 0,
    0, 0);
/* prepare the stack */
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, stack1, 2);
  symstringArrayAdd(&vrf.stack, stmt);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, stack2, 2);
  symstringArrayAdd(&vrf.stack, stmt);
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, stack3, 3);
  symstringArrayAdd(&vrf.stack, stmt);
/* apply the assertion */
  verifierApplyAssertion(&vrf, tyx);
  ut_assert(!vrf.err, "assertion application failed");
  symstringInit(&stmt);
  size_tArrayAppend(&stmt, res, 3);
  ut_assert(symstringIsEqual(&vrf.stack.vals[0], &stmt), "result of assertion "
    "application is wrong");
  symstringClean(&stmt);
/* frm is cleaned here */
  verifierClean(&vrf);
  return 0;
}

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
    verifierParseSymbol(&vrf, &isEndOfStatement, '.'); \
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
#define testparse(f, e) \
do { \
  file = f; \
  readerInitString(&r, file); \
  verifierInit(&vrf); \
  verifierAddFile(&vrf, &r); \
  vrf.r = &vrf.files.vals[0]; \
  vrf.rId = 0; \
  verifierParseConstants(&vrf); \
  check_err(vrf.err, e); \
  verifierClean(&vrf); \
} while (0)
  testparse("", error_unterminatedStatement);
  testparse("A savage place! as holy and enchanted $.\n", error_none);
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
Test_verifierParseVariables(void)
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
Test_verifierParseDisjoints(void)
{
  enum {
    file_size = 2
  };
  const char* file[file_size] = {
    "v c $. ",
    "v v $. "
  };
  const enum error errs[file_size] = {
    error_expectedVariableSymbol,
    error_none
  };
  size_t i;
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader r[file_size];
  struct symstring stmts[file_size];
  for (i = 0; i < file_size; i++) {
    readerInitString(&r[i], file[i]);
    verifierAddFileExplicit(&vrf, &r[i]);
    symstringInit(&stmts[i]);
  }
  LOG_DEBUG("call BeginReadingFile to set valid vrf.rId");
  verifierBeginReadingFile(&vrf, 0);
  size_t c = verifierAddConstant(&vrf, "c");
  size_t v = verifierAddVariable(&vrf, "v");
  struct symstring flt;
  symstringInit(&flt);
  symstringAdd(&flt, c);
  symstringAdd(&flt, v);
  verifierAddFloating(&vrf, "f", &flt);
  for (i = 0; i < file_size; i++) {
    LOG_DEBUG("testing file %lu", i);
    verifierBeginReadingFile(&vrf, i);
    verifierParseDisjoints(&vrf, &stmts[i]);
    check_err(vrf.err, errs[i]);
  }
  LOG_DEBUG("clean up");
  for (i = 0; i < file_size; i++) {
    symstringClean(&stmts[i]);
  }
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierParseFloating(void)
{
  size_t i;
  enum {
    file_size = 5
  };
  const char* file[file_size] = {
    "$. ",
    "And mid these dancing rocks at once and ever $.\n",
    "Chopin piano $. ",
    "wff x $. ",
    "wff y $. "
  };
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader r[file_size];
  struct symstring stmts[file_size];
  for (i = 0; i < file_size; i++) {
    symstringInit(&stmts[i]);
    readerInitString(&r[i], file[i]);
    verifierAddFileExplicit(&vrf, &r[i]);
    check_err(vrf.err, error_none);
  }
#define test_file(rId, error) \
do { \
  LOG_DEBUG("testing file %d", rId); \
  verifierBeginReadingFile(&vrf, rId); \
  verifierParseFloating(&vrf, &stmts[rId]); \
  check_err(vrf.err, error); \
} while (0)
  test_file(0, error_invalidFloatingStatement);
  test_file(1, error_invalidFloatingStatement);
  test_file(2, error_expectedVariableSymbol);
  verifierAddConstant(&vrf, "wff");
  test_file(3, error_expectedVariableSymbol);
  verifierAddVariable(&vrf, "y");
  test_file(4, error_none);
  for (i = 0; i < file_size; i++) {
    symstringClean(&stmts[i]);
  }
  verifierClean(&vrf);
  return 0;
#undef test_file
}

static int
Test_verifierParseEssential(void)
{
  enum {
    file_size = 3
  };
  const char* file[file_size] = {
    "$. ",
    "v |- $. ",
    "|- v $. "
  };
  const enum error errs[file_size] = {
    error_invalidEssentialStatement,
/* we expect this error to be emitted, but it won't be the most recent */
/* for now, use error_none */
    // error_expectedConstantSymbol,
    error_none,
    error_none
  };
  const size_t errcs[file_size] = {
    1,
    1,
    0
  };
  size_t i;
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader r[file_size];
  struct symstring stmts[file_size];
  for (i = 0; i < file_size; i++) {
    readerInitString(&r[i], file[i]);
    verifierAddFileExplicit(&vrf, &r[i]);
    symstringInit(&stmts[i]);
  }
  verifierBeginReadingFile(&vrf, 0);
  size_t c = verifierAddConstant(&vrf, "|-");
  size_t v = verifierAddVariable(&vrf, "v");
  struct symstring flt;
  symstringInit(&flt);
  symstringAdd(&flt, c);
  symstringAdd(&flt, v);
  verifierAddFloating(&vrf, "f", &flt);
  for (i = 0; i < file_size; i++) {
    LOG_DEBUG("testing file %lu", i);
    vrf.errc = 0;
    verifierBeginReadingFile(&vrf, i);
    LOG_DEBUG("parsing essential");
    verifierParseEssential(&vrf, &stmts[i]);
    LOG_DEBUG("done parsing, checking for errors");
    check_err(vrf.err, errs[i]);
    ut_assert(vrf.errc == errcs[i], "error count vrf.errc = %lu, expected %lu",
      vrf.errc, errcs[i]);
  }
  LOG_DEBUG("clean up");
  for (i = 0; i < file_size; i++) {
    symstringClean(&stmts[i]);
  }
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierParseAssertion(void)
{
  enum {
    file_size = 3
  };
  const char* file[file_size] = {
    "$. ",
    "v |- $. ",
    "|- v $. "
  };
  const enum error errs[file_size] = {
    error_invalidAssertionStatement,
/* we expect this error to be emitted, but it won't be the most recent */
/* for now, use error_none */
    // error_expectedConstantSymbol,
    error_none,
    error_none
  };
  const size_t errcs[file_size] = {
    1,
    1,
    0
  };
  size_t i;
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader r[file_size];
  struct symstring stmts[file_size];
  for (i = 0; i < file_size; i++) {
    readerInitString(&r[i], file[i]);
    verifierAddFileExplicit(&vrf, &r[i]);
    symstringInit(&stmts[i]);
  }
  verifierBeginReadingFile(&vrf, 0);
  size_t c = verifierAddConstant(&vrf, "|-");
  size_t v = verifierAddVariable(&vrf, "v");
  struct symstring flt;
  symstringInit(&flt);
  symstringAdd(&flt, c);
  symstringAdd(&flt, v);
  verifierAddFloating(&vrf, "f", &flt);
  for (i = 0; i < file_size; i++) {
    LOG_DEBUG("testing file %lu", i);
    vrf.errc = 0;
    verifierBeginReadingFile(&vrf, i);
    LOG_DEBUG("parsing assertion");
    verifierParseAssertion(&vrf, &stmts[i]);
    LOG_DEBUG("done parsing, checking for errors");
    check_err(vrf.err, errs[i]);
    ut_assert(vrf.errc == errcs[i], "error count vrf.errc = %lu, expected %lu",
      vrf.errc, errcs[i]);
  }
  LOG_DEBUG("clean up");
  for (i = 0; i < file_size; i++) {
    symstringClean(&stmts[i]);
  }
  verifierClean(&vrf);
  return 0;
}

static int
Test_verifierParseProofSymbol(void)
{
  enum {
    file_size = 4
  };
  const char* file[file_size] = {
    "$. ",
    "undefined ",
    "defined_float ",
    "defined_assert ",
  };
  size_t i;
  int isEndOfProof;
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader r[file_size];
  for (i = 0; i < file_size; i++) {
    readerInitString(&r[i], file[i]);
    verifierAddFileExplicit(&vrf, &r[i]);
  }
#define test_file(f, error) \
do { \
  verifierBeginReadingFile(&vrf, f); \
  verifierParseProofSymbol(&vrf, &isEndOfProof); \
  check_err(vrf.err, error); \
} while (0)
  test_file(0, error_none);
  test_file(1, error_undefinedSymbol);
  size_t c = verifierAddConstant(&vrf, "c");
  size_t v = verifierAddVariable(&vrf, "v");
  struct symstring stmt1;
  symstringInit(&stmt1);
  symstringAdd(&stmt1, c);
  symstringAdd(&stmt1, v);
  verifierAddFloating(&vrf, "defined_float", &stmt1);
  test_file(2, error_none);
  struct symstring stmt2;
  symstringInit(&stmt2);
  verifierAddAssertion(&vrf, "defined_assert", &stmt2);
  test_file(3, error_none);
  ut_assert(vrf.stack.size == 2, "stack size == %lu, should be 2",
    vrf.stack.size);
  verifierClean(&vrf);
  return 0;
#undef test_file
}

static int
Test_verifierParseProof(void)
{
  enum {
    file_size = 2
  };
  const char* file[file_size] = {
    "$. ",
    "p $. "
  };
  const size_t thms_len[file_size] = {
    1, 2
  };
  const size_t thms_0[1] = {0};
  const size_t thms_1[2] = {0, 1};
  const size_t* thms_s[file_size] = {
    thms_0,
    thms_1
  };
  size_t i;
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader r[file_size];
  struct symstring thms[file_size];
  for (i = 0; i < file_size; i++) {
    readerInitString(&r[i], file[i]);
    verifierAddFileExplicit(&vrf, &r[i]);
    symstringInit(&thms[i]);
    size_tArrayAppend(&thms[i], thms_s[i], thms_len[i]);
  }
#define test_file(f, error) \
do { \
  verifierBeginReadingFile(&vrf, f); \
  verifierParseProof(&vrf, &thms[f]); \
  check_err(vrf.err, error); \
} while (0)
  test_file(0, error_incorrectProof);
  verifierAddConstant(&vrf, "type");
  verifierAddVariable(&vrf, "var");
  struct symstring stmt;
  symstringInit(&stmt);
  symstringAdd(&stmt, 0);
  symstringAdd(&stmt, 1);
  verifierAddFloating(&vrf, "p", &stmt);
  test_file(1, error_none);
  for (i = 0; i < file_size; i++) {
    symstringClean(&thms[i]);
  }
  verifierClean(&vrf);
  return 0;
#undef test_file
}

static int
Test_verifierParseProvable(void)
{
  enum {
    file_size = 2
  };
  const char* file[file_size] = {
    "type var $= $. ",
    "type var $= flt $. "
  };
  const enum error errs[file_size] = {
    error_incorrectProof,
    error_none
  };
  size_t i;
  struct verifier vrf;
  verifierInit(&vrf);
  struct reader r[file_size];
  for (i = 0; i < file_size; i++) {
    readerInitString(&r[i], file[i]);
    verifierAddFileExplicit(&vrf, &r[i]);
    check_err(vrf.err, error_none);
  }
  verifierBeginReadingFile(&vrf, 0);
  size_t type, var, flt;
  type = verifierAddConstant(&vrf, "type");
  var = verifierAddVariable(&vrf, "var");
  struct symstring floating;
  symstringInit(&floating);
  symstringAdd(&floating, type);
  symstringAdd(&floating, var);
  flt = verifierAddFloating(&vrf, "flt", &floating);
  check_err(vrf.err, error_none);
  for (i = 0; i < file_size; i++) {
    LOG_DEBUG("testing file %lu", i);
    struct symstring stmt;
    symstringInit(&stmt);
    verifierBeginReadingFile(&vrf, i);
    verifierParseProvable(&vrf, &stmt);
    check_err(vrf.err, errs[i]);
    symstringClean(&stmt);
  }
  verifierClean(&vrf);
  return 0;
}

static int
all(void)
{
  ut_run(Test_frameInit);
  ut_run(Test_verifierInit);
  ut_run(Test_verifierDeactivateSymbols);
  ut_run(Test_verifierAddDisjoint);
  ut_run(Test_verifierMakeFrame);
  ut_run(Test_verifierIsValidDisjointPairSubstitution);
  ut_run(Test_verifierIsValidSubstitution);
  ut_run(Test_verifierUnify);
  ut_run(Test_verifierApplyAssertion);
  ut_run(Test_verifierParseSymbol);
  ut_run(Test_verifierParseConstants);
  ut_run(Test_verifierParseVariables);
  ut_run(Test_verifierParseDisjoints);
  ut_run(Test_verifierParseFloating);
  ut_run(Test_verifierParseEssential);
  ut_run(Test_verifierParseAssertion);
  ut_run(Test_verifierParseProofSymbol);
  ut_run(Test_verifierParseProof);
  ut_run(Test_verifierParseProvable);
  return 0;
}

RUN(all)
