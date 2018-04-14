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

VarDecl *
ComparisonCategoryInfo::lookupResultDecl(ComparisonCategoryResult ValueKind) {
  char Key = static_cast<char>(ValueKind);
  VarDecl *VD = Objects.lookup(Key);
  if (VD)
    return VD;

  const RecordDecl *RD = cast<RecordDecl>(CCDecl->getCanonicalDecl());
  StringRef Name = ComparisonCategories::getResultString(ValueKind);
  DeclContextLookupResult Lookup = RD->lookup(&Ctx.Idents.get(Name));
  if (Lookup.size() != 1)
    return nullptr;
  NamedDecl *ND = Lookup.front();
  if (auto *VD = dyn_cast<VarDecl>(ND)) {
    auto ItPair =
        Objects.try_emplace((char)ValueKind, VD);
    return ItPair.first->second;
  }
  return nullptr;
}

NamespaceDecl *ComparisonCategories::lookupStdNamespace() const {
  if (!StdNS) {
    DeclContextLookupResult Lookup =
        Ctx.getTranslationUnitDecl()->lookup(&Ctx.Idents.get("std"));
    if (Lookup.size() == 1)
      StdNS = dyn_cast<NamespaceDecl>(Lookup.front());
  }
  return StdNS;
}

static RecordDecl *lookupRecordDecl(const ASTContext &Ctx, NamespaceDecl *StdNS,
                                    ComparisonCategoryType Kind) {
  StringRef Name = ComparisonCategories::getCategoryString(Kind);
  DeclContextLookupResult Lookup = StdNS->lookup(&Ctx.Idents.get(Name));
  if (Lookup.size() == 1) {
    if (RecordDecl *RD = dyn_cast<RecordDecl>(Lookup.front()))
      return RD;
  }
  return nullptr;
}

const ComparisonCategoryInfo *
ComparisonCategories::lookupInfo(ComparisonCategoryType Kind) const {
  auto It = Data.find(static_cast<char>(Kind));
  if (It != Data.end())
    return &It->second;
  if (NamespaceDecl *NS = lookupStdNamespace()) {
    if (RecordDecl *RD = lookupRecordDecl(Ctx, NS, Kind)) {
      ComparisonCategoryInfo Info(Ctx);
      Info.CCDecl = RD;
      Info.Kind = Kind;
      return &Data.try_emplace((char)Kind, std::move(Info)).first->second;
    }
  }
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
    if (CanonRD == Info.CCDecl->getCanonicalDecl())
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
      ComparisonCategoryInfo Info(Ctx);
      Info.CCDecl = RD;
      Info.Kind = Kind;
      return &Data.try_emplace((char)Kind, std::move(Info)).first->second;
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

