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
ComparisonCategories::getValue(ComparisonCategoryKind Kind,
                               ComparisonCategoryValue ValueKind) const {
  assert(hasData() && "comparison category data not built");
  auto &Info = getInfo(Kind);
  if (auto *DRE = Info.Objects.lookup(static_cast<char>(ValueKind))
    return DRE;
  return nullptr;
}
