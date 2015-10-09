#ifndef _HALMOSVERIFIER_H_
#define _HALMOSVERIFIER_H_

#include "error.h"
#include <stddef.h>

#define HalmosVerifier_ConstantSymbolMax 256

// enum HalmosFlag
// {
//   HalmosFlag_None,
//   HalmosFlag_Max
// };
// typedef enum HalmosFlag HalmosFlag;

// enum HalmosState
// {
//   HalmosState_None,
//   HalmosState_Error,
//   HalmosState_InsideComment,
//   HalmosState_ExpectingWhitespace,
//   HalmosState_InsideBlock,
//   HalmosState_Size,
// };
// typedef enum HalmosState HalmosState;

// const char* HalmosState_String(HalmosState s);

struct HalmosConfiguration
{
  size_t constantMax;
};
typedef struct HalmosConfiguration HalmosConfiguration;

struct HalmosConstant
{
  char symbol[HalmosVerifier_ConstantSymbolMax];
  // uint32_t hash;
};
typedef struct HalmosConstant HalmosConstant;

// typedef int (*HalmosConstantComparator)(HalmosConstant*, HalmosConstant*);

// struct HalmosVariable
// {
//   char symbol[HalmosVerifier_SymbolMax];
// };
// typedef struct HalmosVariable HalmosVariable;

struct HalmosVerifier
{
  size_t warningSize;
  size_t errorSize;
  size_t constantSize;
  // size_t variableSize;
  size_t constantMax;
  // size_t variableMax;
  HalmosConstant* constants;
  // HalmosVariable* variables;
  // HalmosState state;
  // HalmosConstantComparator* ccmp;
  HalmosReader* r;
  HalmosError err;
  HalmosErrorHeader head;
};
typedef struct HalmosVerifier HalmosVerifier;

HalmosError HalmosVerifier_SetError(HalmosVerifier* vrf, HalmosError err);

HalmosError
HalmosVerifier_Init(HalmosVerifier* vrf, HalmosReader* r,
  const HalmosConfiguration* conf);

HalmosError HalmosVerifier_Clear(HalmosVerifier* vrf);

HalmosError HalmosVerifier_AddConstant(HalmosVerifier* vrf, const char* symbol);

HalmosError HalmosVerifier_ParseComment(HalmosVerifier* vrf);

HalmosError HalmosVerifier_ParseConstants(HalmosVerifier* vrf);
// HalmosError HalmosVerifier_ParseBlock(HalmosVerifier* vrf, HalmosReader* r);

#endif
