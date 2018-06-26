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
#include "clang/AST/EvaluatedSourceLocScope.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"
#include <string>
#include <type_traits>
#include <utility>

namespace clang {
class ASTContext;
class Expr;
class DeclContext;
class SourceLocExpr;

class SourceLocExprScopeGuard;

struct EvaluatedSourceLocScope : public EvaluatedSourceLocScopeBase {
private:
  std::string SValue;

public:
  const SourceLocExpr *E = nullptr;
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

  StringLiteral *CreateStringLiteral(const ASTContext &Ctx) const;
  IntegerLiteral *CreateIntegerLiteral(const ASTContext &Ctx) const;
  Expr *CreateLiteral(const ASTContext &Ctx) {
    if (E->isStringType())
      return CreateStringLiteral(Ctx);
    return CreateIntegerLiteral(Ctx);
  }

  EvaluatedSourceLocScope() = default;
  EvaluatedSourceLocScope(EvaluatedSourceLocScope const &) = default;

  static EvaluatedSourceLocScope Create(const ASTContext &Ctx,
                                        const SourceLocExpr *E,
                                        EvaluatedSourceLocScopeBase Base);

  static EvaluatedSourceLocScope Create(const ASTContext &Ctx,
                                        const SourceLocExpr *E,
                                        const Expr *DefaultExpr) {
    return Create(Ctx, E,
                  EvaluatedSourceLocScopeBase::Create(Ctx, E, DefaultExpr));
  }

private:
  EvaluatedSourceLocScope(EvaluatedSourceLocScopeBase Base,
                          const SourceLocExpr *E)
      : EvaluatedSourceLocScopeBase(Base), E(E) {}

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
  CurrentSourceLocExprScope(const Expr *DefaultExpr, const void *EvalContextID);

  EvaluatedSourceLocScope getEvaluatedInfo(const ASTContext &Ctx,
                                           const SourceLocExpr *E) const {
    return EvaluatedSourceLocScope::Create(Ctx, E, DefaultExpr);
  }

  bool empty() const { return DefaultExpr == nullptr; }
  explicit operator bool() const { return !empty(); }

  const Expr *getDefaultExpr() const { return DefaultExpr; }

  bool isInSameContext(CurrentSourceLocExprScope const &Other) const {
    return EvalContextID == Other.EvalContextID;
  }

  template <class EvalContextType>
  bool isInSameContext(EvalContextType *CurEvalContext) const {
    static_assert(!std::is_same<typename std::decay<EvalContextType>::type,
                                CurrentSourceLocExprScope>::value,
                  "incorrect overload selected");
    return EvalContextID == static_cast<const void *>(CurEvalContext);
  }

private:
  CurrentSourceLocExprScope(CurrentSourceLocExprScope const &) = default;
  CurrentSourceLocExprScope &
  operator=(CurrentSourceLocExprScope const &) = default;
};

/// A RAII style scope gaurd used for updating and tracking the current source
/// location and context as used by the source location builtins
/// (ex. __builtin_LINE).
class SourceLocExprScopeGuard {
  static bool ShouldEnable(CurrentSourceLocExprScope const &CurrentScope,
                           CurrentSourceLocExprScope const &NewScope);

  SourceLocExprScopeGuard(CurrentSourceLocExprScope NewScope,
                          CurrentSourceLocExprScope &Current);

public:
  template <class EvalContextType>
  SourceLocExprScopeGuard(const Expr *DefaultExpr,
                          CurrentSourceLocExprScope &Current,
                          EvalContextType *EvalContext)
      : SourceLocExprScopeGuard(
            CurrentSourceLocExprScope(DefaultExpr, EvalContext), Current) {
    static_assert(!std::is_same<typename std::decay<EvalContextType>::type,
                                CurrentSourceLocExprScope>::value,
                  "incorrect overload selected");
  }

  ~SourceLocExprScopeGuard();

private:
  SourceLocExprScopeGuard(SourceLocExprScopeGuard const &) = delete;
  SourceLocExprScopeGuard &operator=(SourceLocExprScopeGuard const &) = delete;

  CurrentSourceLocExprScope &Current;
  CurrentSourceLocExprScope OldVal;
  bool Enable;
};

} // end namespace clang

#endif // LLVM_CLANG_AST_SOURCE_LOC_EXPR_SCOPE_H
