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

int Depth = -1;

SourceManager *MySM;

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
  SourceLocation Loc = [&]() {
    if (auto *DIE = dyn_cast_or_null<CXXDefaultInitExpr>(DefaultExpr))
      return DIE->getUsedLocation();
    else if (auto *DAE = dyn_cast_or_null<CXXDefaultArgExpr>(DefaultExpr)) {
      // if (isInSameContext(ContextID) || !E->isLineOrColumn())
      return DAE->getUsedLocation();
    } else
      assert(DefaultExpr == nullptr && "unexpected default expression type");
    return E->getLocation();
  }();
  if (E->getIdentType() == SourceLocExpr::File) {
    llvm::errs() << "#" << std::to_string(Depth) << " Evaluating ";
    Loc.dump(*MySM);
    llvm::errs() << "\n";
  }
  return Loc;
}

const DeclContext *
CurrentSourceLocExprScope::getContext(const SourceLocExpr *E,
                                      const void *ContextID) const {
  if (auto *DIE = dyn_cast_or_null<CXXDefaultInitExpr>(DefaultExpr))
    return DIE->getUsedContext();
  if (auto *DAE = dyn_cast_or_null<CXXDefaultArgExpr>(DefaultExpr)) {
    //  if (isInSameContext(ContextID) || !E->isLineOrColumn())
    return DAE->getUsedContext();
  }
  return E->getParentContext();
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

static SourceLocation getUsedLoc(const Expr *E) {
  if (!E)
    return SourceLocation();
  if (auto *DAE = dyn_cast<CXXDefaultArgExpr>(E))
    return DAE->getUsedLocation();
  if (auto *DIE = dyn_cast<CXXDefaultInitExpr>(E))
    return DIE->getUsedLocation();
  llvm_unreachable("unhandled expression kind");
}

SourceLocExprScopeGuard::SourceLocExprScopeGuard(
    CurrentSourceLocExprScope NewScope, CurrentSourceLocExprScope &Current)
    : Current(Current), OldVal(Current), Enable(false) {
  if ((Enable = ShouldEnable(Current, NewScope))) {
    auto PrintLoc = [&]() {
      SourceLocation Loc = getUsedLoc(NewScope.DefaultExpr);
      assert(MySM);
      Loc.dump(*MySM);
      llvm::errs() << " ";
    };

    llvm::errs() << "#" << std::to_string(++Depth) << " Pushing ";
    if (auto *DAE = dyn_cast<CXXDefaultArgExpr>(NewScope.DefaultExpr)) {
      llvm::errs() << "Default Arg " << DAE->getParam()->getName();
      PrintLoc();
    } else if (auto *DIE = dyn_cast<CXXDefaultInitExpr>(NewScope.DefaultExpr)) {
      llvm::errs() << "Default Init " << DIE->getField()->getName();
      PrintLoc();
      DIE->getExpr()->dumpColor();
    }
    llvm::errs() << "\n";
    Current = NewScope;
  }
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
  if (CurrentScope.empty())
    return true;
  // if (isa<CXXDefaultInitExpr>(CurrentScope.DefaultExpr) &&
  //        isa<CXXDefaultArgExpr>(NewScope.DefaultExpr))
  //  return false;
  return !CurrentScope.isInSameContext(NewScope);
}

SourceLocExprScopeGuard::~SourceLocExprScopeGuard() {
  if (Enable) {
    llvm::errs() << "#" << std::to_string(Depth--) << " Popping";

    llvm::errs() << "\n";
    Current = OldVal;
  }
}
