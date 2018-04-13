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
ComparisonCategories::getInfoUnchecked(ComparisonCategoryType Kind) const {
  auto It = Data.find(static_cast<char>(Kind));
  if (It == Data.end())
    return nullptr;
  return &It->second;
}

const ComparisonCategoryType *
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
  const ComparisonCategoryType *Kind = getCategoryForType(Ty);
  assert(Kind &&
         "return type for operator<=> is not a comparison category type or the "
         "specified comparison category kind has not been built yet");
  return getInfo(*Kind);
}

StringRef ComparisonCategories::getCategoryString(ComparisonCategoryType Kind) {
  using CCKT = ComparisonCategoryType;
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
  }
  llvm_unreachable("unhandled case in switch");
}

const VarDecl *ComparisonCategoryInfo::lookupResultValue(
    const ASTContext &Ctx, ComparisonCategoryResult ValueKind) const {
  char Key = static_cast<char>(ValueKind);
  const VarDecl *VD = Objects.lookup(Key);
  if (VD)
    return VD;

  StringRef StrName = ComparisonCategories::getResultString(ValueKind);
  const RecordDecl *RD = cast<RecordDecl>(CCDecl->getCanonicalDecl());

  const IdentifierInfo &II = Ctx.Idents.get(StrName);

  DeclarationName Name(&II);
  DeclContextLookupResult Lookup =
      RD->getDeclContext()->getRedeclContext()->lookup(Name);
  assert(Lookup.size() == 1);
  const NamedDecl *ND = Lookup.front();
  assert(isa<VarDecl>(ND));
  auto ItPair = Objects.try_emplace((char)ValueKind, cast<VarDecl>(ND));
  return ItPair.first->second;
}
