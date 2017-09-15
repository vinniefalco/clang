//===--- SourceLocExprScope.h -----------------------------------*- C++ -*-===//
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

#ifndef LLVM_CLANG_AST_SOURCE_LOC_EXPR_SCOPE_H
#define LLVM_CLANG_AST_SOURCE_LOC_EXPR_SCOPE_H

#include "clang/AST/APValue.h"
#include "clang/AST/ASTVector.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/TemplateBase.h"
#include "clang/AST/Type.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

namespace clang {

template <class EvalContextType> class SourceLocExprScopeBase {
  bool ShouldEnable() const {
    // Only update the default argument scope if we've entered a new
    // evaluation context, and not when it's nested within another default
    // argument. Example:
    //    int bar(int x = __builtin_LINE()) { return x; }
    //    int foo(int x = bar())  { return x; }
    //    static_assert(foo() == __LINE__);
    return OldVal == nullptr || isa<CXXDefaultInitExpr>(DefaultExpr) ||
           !OldVal->isInSameContext(EvalContext);
  }

public:
  SourceLocExprScopeBase(const Expr *DefaultExpr, SourceLocExprScopeBase **Dest,
                         EvalContextType *EvalContext)
      : DefaultExpr(DefaultExpr), Dest(Dest), OldVal(*Dest),
        EvalContext(EvalContext), Enable(false) {
    if ((Enable = ShouldEnable()))
      *Dest = this;
  }

  ~SourceLocExprScopeBase() {
    if (Enable)
      *Dest = OldVal;
  }

  bool isInSameContext(EvalContextType *CurEvalContext) const {
    return EvalContext == CurEvalContext;
  }

  SourceLocation getLoc() const {
    if (const auto *DAE = dyn_cast<CXXDefaultArgExpr>(DefaultExpr))
      return DAE->getUsedLocation();
    return dyn_cast<CXXDefaultInitExpr>(DefaultExpr)->getUsedLocation();
  }

  const DeclContext *getContext() const {
    if (const auto *DAE = dyn_cast<CXXDefaultArgExpr>(DefaultExpr))
      return DAE->getUsedContext();
    return dyn_cast<CXXDefaultInitExpr>(DefaultExpr)->getUsedContext();
  }

private:
  const Expr *DefaultExpr;
  SourceLocExprScopeBase **Dest;
  SourceLocExprScopeBase *OldVal;
  EvalContextType *EvalContext;
  bool Enable;
};

} // end namespace clang

#endif // LLVM_CLANG_AST_SOURCE_LOC_EXPR_SCOPE_H
