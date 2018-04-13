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

const VarDecl *ComparisonCategoryInfo::lookupResultValue(
    ComparisonCategoryResult ValueKind) const {
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
  if (Lookup.size() != 1)
    return nullptr;
  const NamedDecl *ND = Lookup.front();
  if (const auto *VD = dyn_cast<VarDecl>(ND)) {
    auto ItPair =
        Objects.try_emplace((char)ValueKind, const_cast<VarDecl *>(VD));
    return ItPair.first->second;
  }
  return nullptr;
}

static const RecordDecl *lookupRecordDecl(const ASTContext &Ctx,
                                          ComparisonCategoryType Kind) {
  NamespaceDecl *NS = NamespaceDecl::Create(
      const_cast<ASTContext &>(Ctx), Ctx.getTranslationUnitDecl(),
      /*Inline*/ false, SourceLocation(), SourceLocation(),
      &Ctx.Idents.get("std"),
      /*PrevDecl*/ nullptr);
  StringRef StrName = ComparisonCategories::getCategoryString(Kind);
  const IdentifierInfo &II = Ctx.Idents.get(StrName);
  DeclarationName Name(&II);
  DeclContextLookupResult Lookup = NS->lookup(Name);
  if (Lookup.size() == 1) {
    if (const RecordDecl *RD = dyn_cast<RecordDecl>(Lookup.front()))
      return RD;
  }
  return nullptr;
}

const ComparisonCategoryInfo *
ComparisonCategories::lookupInfoUnchecked(ComparisonCategoryType Kind) const {
  auto It = Data.find(static_cast<char>(Kind));
  if (It != Data.end())
    return &It->second;
  const RecordDecl *RD = lookupRecordDecl(Ctx, Kind);
  if (!RD)
    return nullptr;
  ComparisonCategoryInfo Info(Ctx);
  Info.CCDecl = const_cast<RecordDecl *>(RD);
  Info.Kind = Kind;
  return &Data.try_emplace((char)Kind, std::move(Info)).first->second;
}

const ComparisonCategoryInfo *
ComparisonCategories::lookupInfoForTypeUnchecked(QualType Ty) const {
  assert(!Ty.isNull() && "type must be non-null");
  const auto *RD = Ty->getAsCXXRecordDecl();
  if (!RD)
    return nullptr;

  // Check to see if we have information for the specified type cached.
  const auto *CanonRD = RD->getCanonicalDecl();
  for (auto &KV : Data) {
    const ComparisonCategoryInfo &Info = KV.second;
    if (CanonRD == Info.CCDecl->getCanonicalDecl())
      return &Info;
  }

  // If not, check to see if the decl names a type in namespace std with a name
  // matching one of the comparison category types.
  if (RD->getEnclosingNamespaceContext()->isStdNamespace()) {
    using CCT = ComparisonCategoryType;
    for (unsigned I = static_cast<unsigned>(CCT::First),
                  End = static_cast<unsigned>(CCT::Last);
         I <= End; ++I) {
      CCT Kind = static_cast<CCT>(I);

      // We've found the comparison category type. Build a new cache entry for
      // it.
      if (getCategoryString(Kind) == RD->getName()) {
        ComparisonCategoryInfo Info(Ctx);
        Info.CCDecl =
            const_cast<RecordDecl *>(static_cast<const RecordDecl *>(RD));
        Info.Kind = Kind;
        return &Data.try_emplace((char)Kind, std::move(Info)).first->second;
      }
    }
  }

  // We've found nothing. This isn't a comparison category type.
  return nullptr;
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

