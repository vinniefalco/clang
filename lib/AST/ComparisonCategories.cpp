//===- ComparisonCategories.h - Three Way Comparison Data -------*- C++ -*-===//
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

Optional<ComparisonCategoryKind>
ComparisonCategories::getCategoryForType(QualType Ty) const {
  assert(hasData() && "comparison category data not built");
  assert(!Ty.isNull() && "type must be non-null");
  if (const auto *RD = Ty->getAsCXXRecordDecl()) {
    const auto *CanonRD = RD->getCanonicalDecl();
    for (auto &Info : Data) {
      if (CanonRD == Info.CCDecl->getCanonicalDecl())
        return Info.Kind;
    }
  }
  return None;
}

const DeclRefExpr *
ComparisonCategories::getResultValue(ComparisonCategoryKind Kind,
                                     ComparisonCategoryResult ValueKind) const {
  assert(hasData() && "comparison category data not built");
  assert(ValueKind != ComparisonCategoryResult::Invalid &&
         "invalid result kind");
  auto &Info = getInfo(Kind);
  if (auto *DRE = Info.Objects.lookup(static_cast<char>(ValueKind)))
    return DRE;
  return nullptr;
}


static std::pair<StringRef, ComparisonCategoryClassification>
getNameAndClassify(ComparisonCategoryKind Kind) {
  using CCKT = ComparisonCategoryKind ;
  using CCCT = ComparisonCategoryClassification ;
  switch (Kind) {
      case CCKT::WeakEquality:
        return {"weak_equality", CCCT::None};
      case CCKT::StrongEquality:
        return {"strong_equality", CCCT::Strong};
      case CCKT::PartialOrdering:
        return {"partial_ordering", CCCT::Ordered | CCCT::Partial};
      case CCKT::WeakOrdering:
        return {"weak_ordering", CCCT::Ordered};
      case CCKT::StrongOrdering:
        return {"strong_ordering", CCCT::Ordered | CCCT::Strong};
      }
      llvm_unreachable("unhandled cases in switch");
};


ComparisonCategoryClassification
ComparisonCategories::classifyCategory(ComparisonCategoryKind Kind) {
  return getNameAndClassify(Kind).second;
}

StringRef ComparisonCategories::getCategoryString(ComparisonCategoryKind Kind) {
  return getNameAndClassify(Kind).first;
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
