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

namespace clang {
class SourceLocation;
class Expr;
class DeclContext;

class SourceLocExprScope;

class CurrentSourceLocExprScope {
  friend class ScopeLocExprScope;
  const Expr *DefaultExpr = nullptr;
  const void *EvalContextID = nullptr;

public:
  CurrentSourceLocExprScope() = default;
  CurrentSourceLocExprScope(const Expr *DefaultExpr, const void *EvalContextID)
      : DefaultExpr(DefaultExpr), EvalContextID(EvalContextID) {}

  explicit operator bool() const { return DefaultExpr; }

  template <class EvalContextType>
  bool isInSameContext(EvalContextType *CurEvalContext) const {
    return EvalContextID == static_cast<const void *>(CurEvalContext);
  }

  SourceLocation getLoc() const;
  const DeclContext *getContext() const;
};

class SourceLocExprScope {
  bool ShouldEnable() const;

  SourceLocExprScope(const Expr *DefaultExpr,
                     CurrentSourceLocExprScope &Current,
                     const void *EvalContext);

public:
  template <class EvalContextType>
  SourceLocExprScope(const Expr *DefaultExpr,
                     CurrentSourceLocExprScope &Current,
                     EvalContextType *EvalContext)
      : SourceLocExprScope(DefaultExpr, Current,
                           static_cast<const void *>(EvalContext)) {}

  ~SourceLocExprScope();

private:
  CurrentSourceLocExprScope &Current;
  CurrentSourceLocExprScope OldVal;
  bool Enable;
};

} // end namespace clang

#endif // LLVM_CLANG_AST_SOURCE_LOC_EXPR_SCOPE_H
