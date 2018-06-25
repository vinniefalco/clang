//===--- SourceLocExprScope.cpp - Mangle C++ Names --------------------------*-
// C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the SourceLocExprScope utility for tracking the current
//  context a source location builtin should be evaluated in.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/SourceLocExprScope.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Basic/SourceLocation.h"

using namespace clang;

SourceLocation CurrentSourceLocExprScope::getLoc() const {
  if (const auto *DAE = dyn_cast<CXXDefaultArgExpr>(DefaultExpr))
    return DAE->getUsedLocation();
  return dyn_cast<CXXDefaultInitExpr>(DefaultExpr)->getUsedLocation();
}

const DeclContext *CurrentSourceLocExprScope::getContext() const {
  if (const auto *DAE = dyn_cast<CXXDefaultArgExpr>(DefaultExpr))
    return DAE->getUsedContext();
  return dyn_cast<CXXDefaultInitExpr>(DefaultExpr)->getUsedContext();
}

SourceLocExprScopeGuard::SourceLocExprScopeGuard(
    const Expr *DefaultExpr, CurrentSourceLocExprScope &Current,
    const void *EvalContext)
    : Current(Current), OldVal(Current), Enable(false) {
  if ((Enable = ShouldEnable()))
    Current = CurrentSourceLocExprScope(DefaultExpr, EvalContext);
}

bool SourceLocExprScopeGuard::ShouldEnable() const {
  // Only update the default argument scope if we've entered a new
  // evaluation context, and not when it's nested within another default
  // argument. Example:
  //    int bar(int x = __builtin_LINE()) { return x; }
  //    int foo(int x = bar())  { return x; }
  //    static_assert(foo() == __LINE__);
  return !OldVal || !OldVal.isInSameContext(Current.EvalContextID);
}

SourceLocExprScopeGuard::~SourceLocExprScopeGuard() {
  if (Enable)
    Current = OldVal;
}
