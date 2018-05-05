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
#include "llvm/ADT/SmallVector.h"

using namespace clang;

static CXXRecordDecl *getDefiningDecl(CXXRecordDecl *RD) {
  if (RD->hasDefinition())
    return RD->getDefinition();
  return RD;
}


bool ComparisonCategoryInfo::ValueInfo::updateIntValue(const ASTContext &Ctx) {
  if (hasValidIntValue())
    return false;

  Expr::EvalResult Result;
  if (!VD->hasInit() || !VD->getInit()->EvaluateAsRValue(Result, Ctx))
    return true;

  assert(Result.Val.isStruct());
  IntValue = Result.Val.getStructField(0).getInt();
  HasValue = true;
  return false;
}

QualType ComparisonCategoryInfo::getType() const {
  return QualType(Record->getTypeForDecl(), 0);
}

ComparisonCategoryInfo::ValueInfo *ComparisonCategoryInfo::lookupValueInfo(
    ComparisonCategoryResult ValueKind) const {
  // Check if we already have a cache entry for this value.
  auto It = llvm::find_if(
      Objects, [&](ValueInfo const &Info) { return Info.Kind == ValueKind; });

  // We don't have a cached result. Lookup the variable declaration and create
  // a new entry representing it.
  if (It == Objects.end()) {
    DeclContextLookupResult Lookup = Record->getCanonicalDecl()->lookup(
        &Ctx.Idents.get(ComparisonCategories::getResultString(ValueKind)));
    if (Lookup.size() != 1 || !isa<VarDecl>(Lookup.front()))
      return nullptr;
    Objects.emplace_back(ValueKind, cast<VarDecl>(Lookup.front()));
    It = Objects.end() - 1;
  }
  assert(It != Objects.end());
  // Success! Attempt to update the int value in case the variables initializer
  // wasn't present the last time we were here.
  ValueInfo *Info = &(*It);
  Info->updateIntValue(Ctx);

  return Info;
}

static const NamespaceDecl *lookupStdNamespace(const ASTContext &Ctx,
                                               NamespaceDecl *&StdNS) {
  if (!StdNS) {
    DeclContextLookupResult Lookup =
        Ctx.getTranslationUnitDecl()->lookup(&Ctx.Idents.get("std"));
    if (Lookup.size() == 1)
      StdNS = dyn_cast<NamespaceDecl>(Lookup.front());
  }
  return StdNS;
}

static CXXRecordDecl *lookupCXXRecordDecl(const ASTContext &Ctx,
                                          const NamespaceDecl *StdNS,
                                          ComparisonCategoryType Kind) {
  StringRef Name = ComparisonCategories::getCategoryString(Kind);
  DeclContextLookupResult Lookup = StdNS->lookup(&Ctx.Idents.get(Name));
  if (Lookup.size() == 1)
    if (CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(Lookup.front()))
      return getDefiningDecl(RD);
  return nullptr;
}

const ComparisonCategoryInfo *
ComparisonCategories::lookupInfo(ComparisonCategoryType Kind) const {
  auto It = Data.find(static_cast<char>(Kind));
  if (It != Data.end())
    return &It->second;

  if (const NamespaceDecl *NS = lookupStdNamespace(Ctx, StdNS))
    if (CXXRecordDecl *RD = lookupCXXRecordDecl(Ctx, NS, Kind))
      return &Data.try_emplace((char)Kind, Ctx, RD, Kind).first->second;

  return nullptr;
}

const ComparisonCategoryInfo *
ComparisonCategories::lookupInfoForType(QualType Ty) const {
  assert(!Ty.isNull() && "type must be non-null");
  using CCT = ComparisonCategoryType;
  auto *RD = Ty->getAsCXXRecordDecl();
  if (!RD)
    return nullptr;

  // Check to see if we have information for the specified type cached.
  const auto *CanonRD = RD->getCanonicalDecl();
  for (auto &KV : Data) {
    const ComparisonCategoryInfo &Info = KV.second;
    if (CanonRD == Info.Record->getCanonicalDecl())
      return &Info;
  }

  if (!RD->getEnclosingNamespaceContext()->isStdNamespace())
    return nullptr;

  // If not, check to see if the decl names a type in namespace std with a name
  // matching one of the comparison category types.
  for (unsigned I = static_cast<unsigned>(CCT::First),
                End = static_cast<unsigned>(CCT::Last);
       I <= End; ++I) {
    CCT Kind = static_cast<CCT>(I);

    // We've found the comparison category type. Build a new cache entry for
    // it.
    if (getCategoryString(Kind) == RD->getName()) {
      return &Data.try_emplace((char)Kind, Ctx, getDefiningDecl(RD), Kind)
                  .first->second;
    }
  }

  // We've found nothing. This isn't a comparison category type.
  return nullptr;
}

const ComparisonCategoryInfo &ComparisonCategories::getInfoForType(QualType Ty) const {
  const ComparisonCategoryInfo *Info = lookupInfoForType(Ty);
  assert(Info && "info for comparison category not found");
  return *Info;
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

std::vector<ComparisonCategoryResult>
ComparisonCategories::getPossibleResultsForType(ComparisonCategoryType Type) {
  using CCT = ComparisonCategoryType;
  using CCR = ComparisonCategoryResult;
  std::vector<CCR> Values;
  Values.reserve(6);
  Values.push_back(CCR::Equivalent);
  bool IsStrong = (Type == CCT::StrongEquality || Type == CCT::StrongOrdering);
  if (IsStrong)
    Values.push_back(CCR::Equal);
  if (Type == CCT::StrongOrdering || Type == CCT::WeakOrdering ||
      Type == CCT::PartialOrdering) {
    Values.push_back(CCR::Less);
    Values.push_back(CCR::Greater);
  } else {
    Values.push_back(CCR::Nonequivalent);
    if (IsStrong)
      Values.push_back(CCR::Nonequal);
  }
  if (Type == CCT::PartialOrdering)
    Values.push_back(CCR::Unordered);
  return Values;
}
