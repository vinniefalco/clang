//===--- EvaluateSourceLocExpr.h --------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines two basic utilities needed to fully evaluate a
//  SourceLocExpr within a particular context and to track said context.
//  Specifically:
//
//  (1) EvaluatedSourceLocExpr - Represents the result of evaluating a
//  SourceLocExpr within a given context.
//
//  (2) CurrentSourceLocExprScope and SourceLocExprScopeGuard - RAII style
//  types for tracking any default argument or initialization expressions that
//  the SourceLocExpr appears within.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_EVALUATE_SOURCE_LOC_EXPR_H
#define LLVM_CLANG_AST_EVALUATE_SOURCE_LOC_EXPR_H

#include "clang/AST/APValue.h"
#include "clang/AST/Expr.h"
#include "clang/AST/SourceLocExprContext.h"
#include "clang/Basic/SourceLocation.h"
#include <string>
#include <type_traits>
#include <utility>

namespace clang {
class ASTContext;
class Expr;
class DeclContext;
class SourceLocExpr;

/// Represents the result of evaluating a SourceLocExpr within a given context.
struct EvaluatedSourceLocExpr : public SourceLocExprContext {
private:
  /// The evaluated string value if the specified SourceLocExpr producing a
  /// string.
  std::string SValue;

public:
  /// The evaluated SourceLocExpr.
  const SourceLocExpr *E = nullptr;

  /// The result of evaluation.
  APValue Result;

  std::string const &getStringValue() const {
    assert(E->isStringType() &&
           "cannot set string value for non-string expression");
    return SValue;
  }
  void setStringValue(std::string Other) {
    assert(E->isStringType() &&
           "cannot set string value for non-string expression");
    SValue = std::move(Other);
  }

  uint32_t getIntValue() const {
    assert(E->isIntType() && "cannot get int value from non-int expression");
    assert(Result.isInt() && "non-int APValue result");
    return Result.getInt().getZExtValue();
  }

  EvaluatedSourceLocExpr() = default;
  EvaluatedSourceLocExpr(EvaluatedSourceLocExpr const &) = default;

  /// Evaluate the SourceLocExpr in the specified context and return the
  /// results.
  static EvaluatedSourceLocExpr Create(const ASTContext &Ctx,
                                       const SourceLocExpr *E,
                                       SourceLocExprContext Base);

  /// Evaluate the specified SourceLocExpr after determining the
  /// SourceLocExprContext coorisponding the the specified default argument
  /// or initializer expression.
  static EvaluatedSourceLocExpr Create(const ASTContext &Ctx,
                                       const SourceLocExpr *E,
                                       const Expr *DefaultExpr) {
    return Create(Ctx, E, SourceLocExprContext::Create(Ctx, E, DefaultExpr));
  }

private:
  EvaluatedSourceLocExpr(SourceLocExprContext Base, const SourceLocExpr *E)
      : SourceLocExprContext(Base), E(E) {}

  friend class CurrentSourceLocExprScope;
};

/// Represents the current source location and context used to determine the
/// value of the source location builtins (ex. __builtin_LINE), including the
/// context of default argument and default initializer expressions.
class CurrentSourceLocExprScope {
  friend class SourceLocExprScopeGuard;

  /// The CXXDefaultArgExpr or CXXDefaultInitExpr we're currently evaluating.
  const Expr *DefaultExpr = nullptr;

  /// A pointer representing the current evaluation context. It is used to
  /// determine context equality.
  // FIXME: using 'void*' is a type-unsafe hack used to avoid making the type
  // a class template.
  const void *EvalContextID = nullptr;

public:
  CurrentSourceLocExprScope() = default;

  const Expr *getDefaultExpr() const { return DefaultExpr; }

private:
  CurrentSourceLocExprScope(const Expr *DefaultExpr, const void *EvalContextID);
  CurrentSourceLocExprScope(CurrentSourceLocExprScope const &) = default;
  CurrentSourceLocExprScope &
  operator=(CurrentSourceLocExprScope const &) = default;
};

/// A RAII style scope gaurd used for tracking the current source
/// location and context as used by the source location builtins
/// (ex. __builtin_LINE).
class SourceLocExprScopeGuard {
public:
  SourceLocExprScopeGuard(const Expr *DefaultExpr,
                          CurrentSourceLocExprScope &Current,
                          const void *EvalContext)
      : SourceLocExprScopeGuard(
            CurrentSourceLocExprScope(DefaultExpr, EvalContext), Current) {}

  ~SourceLocExprScopeGuard();

private:
  static bool ShouldEnable(CurrentSourceLocExprScope const &CurrentScope,
                           CurrentSourceLocExprScope const &NewScope);

  SourceLocExprScopeGuard(CurrentSourceLocExprScope NewScope,
                          CurrentSourceLocExprScope &Current);

  SourceLocExprScopeGuard(SourceLocExprScopeGuard const &) = delete;
  SourceLocExprScopeGuard &operator=(SourceLocExprScopeGuard const &) = delete;

  CurrentSourceLocExprScope &Current;
  CurrentSourceLocExprScope OldVal;
  bool Enable;
};

} // end namespace clang

#endif // LLVM_CLANG_AST_EVALUATE_SOURCE_LOC_EXPR_H
