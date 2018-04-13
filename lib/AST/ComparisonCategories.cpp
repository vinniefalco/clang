//===- ComparisonCategories.cpp - Three Way Comparison Data -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Comparison Category enum and data types, which
//  store the types and expressions needed to support operator<=>
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ComparisonCategories.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"

using namespace clang;

const ComparisonCategoryInfo *
ComparisonCategories::getInfoUnsafe(ComparisonCategoryKind Kind) const {
  auto It = Data.find(static_cast<char>(Kind));
  if (It == Data.end())
    return nullptr;
  return &It->second;
}

const ComparisonCategoryKind *
ComparisonCategories::getCategoryForType(QualType Ty) const {
  assert(!Ty.isNull() && "type must be non-null");
  if (const auto *RD = Ty->getAsCXXRecordDecl()) {
    const auto *CanonRD = RD->getCanonicalDecl();
    for (auto &KV : Data) {
      const ComparisonCategoryInfo &Info = KV.second;
      if (CanonRD == Info.CCDecl->getCanonicalDecl())
        return &Info.Kind;
    }
  }
  return nullptr;
}

const ComparisonCategoryInfo &
ComparisonCategories::getInfoForType(QualType Ty) const {
  const ComparisonCategoryKind *Kind = getCategoryForType(Ty);
  assert(Kind &&
         "return type for operator<=> is not a comparison category type or the "
         "specified comparison category kind has not been built yet");
  return getInfo(*Kind);
}

StringRef ComparisonCategories::getCategoryString(ComparisonCategoryKind Kind) {
  using CCKT = ComparisonCategoryKind;
  switch (Kind) {
  case CCKT::WeakEquality:
    return "weak_equality";
  case CCKT::StrongEquality:
    return "strong_equality";
  case CCKT::PartialOrdering:
    return "partial_ordering";
  case CCKT::WeakOrdering:
    return "weak_ordering";
  case CCKT::StrongOrdering:
    return "strong_ordering";
  }
  llvm_unreachable("unhandled cases in switch");
}

StringRef ComparisonCategories::getResultString(ComparisonCategoryResult Kind) {
  using CCVT = ComparisonCategoryResult;
  switch (Kind) {
  case CCVT::Equal:
    return "equal";
  case CCVT::Nonequal:
    return "nonequal";
  case CCVT::Equivalent:
    return "equivalent";
  case CCVT::Nonequivalent:
    return "nonequivalent";
  case CCVT::Less:
    return "less";
  case CCVT::Greater:
    return "greater";
  case CCVT::Unordered:
    return "unordered";
  case CCVT::Invalid:
    return "invalid";
  }
  llvm_unreachable("unhandled case in switch");
}
