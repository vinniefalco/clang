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
  switch (CurrentKind) {
  case CCK_DefaultArg:
    return DefaultArg->getUsedLocation();
  case CCK_DefaultInit:
    return DefaultInit->getUsedLocation();
  case CCK_None:
    break;
  }

  llvm_unreachable("no current source location scope");
}

const DeclContext *CurrentSourceLocExprScope::getContext() const {
  switch (CurrentKind) {
  case CCK_DefaultArg:
    return DefaultArg->getUsedContext();
  case CCK_DefaultInit:
    return DefaultInit->getUsedContext();
  case CCK_None:
    break;
  }

  llvm_unreachable("no current source location scope");
}

const Expr *CurrentSourceLocExprScope::getDefaultExpr() const {
  switch (CurrentKind) {
  case CCK_DefaultArg:
    return DefaultArg;
  case CCK_DefaultInit:
    return DefaultInit;
  case CCK_None:
    return nullptr;
  }

  llvm_unreachable("unhandled case");
}

SourceLocExprScopeGuard::SourceLocExprScopeGuard(
    CurrentSourceLocExprScope NewScope, CurrentSourceLocExprScope &Current)
    : Current(Current), OldVal(Current), Enable(false) {
  if ((Enable = ShouldEnable()))
    Current = NewScope;
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
