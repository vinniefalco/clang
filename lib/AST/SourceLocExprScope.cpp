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

CurrentSourceLocExprScope::CurrentSourceLocExprScope(const Expr *DefaultExpr,
                                                     const void *EvalContextID)
    : DefaultExpr(DefaultExpr), EvalContextID(EvalContextID) {
  assert(DefaultExpr && "expression cannot be null");
  assert(EvalContextID && "context pointer cannot be null");
  assert((isa<CXXDefaultArgExpr>(DefaultExpr) ||
          isa<CXXDefaultInitExpr>(DefaultExpr)) &&
         "expression must be either a default argument or initializer");
}

SourceLocation CurrentSourceLocExprScope::getLoc(const SourceLocExpr *E,
                                                 const void *ContextID) const {
  if (auto *DIE = dyn_cast_or_null<CXXDefaultInitExpr>(DefaultExpr))
    return DIE->getUsedLocation();
  else if (auto *DAE = dyn_cast_or_null<CXXDefaultArgExpr>(DefaultExpr)) {
    if (isInSameContext(ContextID))
      return DAE->getUsedLocation();
  } else
    assert(DefaultExpr == nullptr && "unexpected default expression type");
  return E->getExprLoc();
}

const DeclContext *
CurrentSourceLocExprScope::getContext(const SourceLocExpr *E,
                                      const void *ContextID) const {
  if (auto *DIE = dyn_cast_or_null<CXXDefaultInitExpr>(DefaultExpr))
    return DIE->getUsedContext();
  if (auto *DAE = dyn_cast_or_null<CXXDefaultArgExpr>(DefaultExpr)) {
    if (isInSameContext(ContextID))
      return DAE->getUsedContext();
  }
  return E->getParentContext();
}

SourceLocExprScopeGuard::SourceLocExprScopeGuard(
    CurrentSourceLocExprScope NewScope, CurrentSourceLocExprScope &Current)
    : Current(Current), OldVal(Current), Enable(false) {
  if ((Enable = ShouldEnable(Current, NewScope)))
    Current = NewScope;
}

static const DeclContext *getUsedContext(const Expr *E) {
  if (!E)
    return nullptr;
  if (auto *DAE = dyn_cast<CXXDefaultArgExpr>(E))
    return DAE->getUsedContext();
  if (auto *DIE = dyn_cast<CXXDefaultInitExpr>(E))
    return DIE->getUsedContext();
  llvm_unreachable("unhandled expression kind");
}

bool SourceLocExprScopeGuard::ShouldEnable(
    CurrentSourceLocExprScope const &CurrentScope,
    CurrentSourceLocExprScope const &NewScope) {
  assert(!NewScope.empty() && "the new scope should not be empty");
  // Only update the default argument scope if we've entered a new
  // evaluation context, and not when it's nested within another default
  // argument. Example:
  //    int bar(int x = __builtin_LINE()) { return x; }
  //    int foo(int x = bar())  { return x; }
  //    static_assert(foo() == __LINE__);
  return CurrentScope.empty() || !CurrentScope.isInSameContext(NewScope);
}

SourceLocExprScopeGuard::~SourceLocExprScopeGuard() {
  if (Enable)
    Current = OldVal;
}
