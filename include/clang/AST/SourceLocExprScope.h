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

#include "clang/Basic/SourceLocation.h"
#include <type_traits>
#include <utility>

namespace clang {
class Expr;
class DeclContext;
class SourceLocExpr;

class SourceLocExprScopeGuard;

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

private:
  CurrentSourceLocExprScope(CurrentSourceLocExprScope const &) = default;
  CurrentSourceLocExprScope &
  operator=(CurrentSourceLocExprScope const &) = default;

public:
  CurrentSourceLocExprScope() = default;
  CurrentSourceLocExprScope(const Expr *DefaultExpr, const void *EvalContextID);

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

  /// Return the correct SourceLocation for a SourceLocExpr being evaluated
  /// in the current source location expression scope.
  SourceLocation getLoc(const SourceLocExpr *E,
                        const void *CurrentContextID) const;

  /// Return the correct DeclContext for a SourceLocExpr being evaluated in
  /// the current source location expression scope.
  const DeclContext *getContext(const SourceLocExpr *E,
                                const void *ContextID) const;

  std::pair<SourceLocation, const DeclContext *>
  getLocationContextPair(const SourceLocExpr *E, const void *ContextID) const {
    return {getLoc(E, ContextID), getContext(E, ContextID)};
  }
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
