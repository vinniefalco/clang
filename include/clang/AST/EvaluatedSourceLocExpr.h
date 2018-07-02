//===--- EvaluatedSourceLocExpr.h ---------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines types used to evaluated source location expressions and
//  track the current context and location needed to do so.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_EVALUATED_SOURCE_LOC_EXPR_H
#define LLVM_CLANG_AST_EVALUATED_SOURCE_LOC_EXPR_H

#include "clang/AST/Expr.h"
#include "clang/Basic/LLVM.h"
#include "llvm/ADT/StringRef.h"
#include <cassert>

namespace clang {
class ASTContext;
class SourceLocation;
class QualType;
class APValue;

/// Contextual information coorisponding to the AST context of a SourceLocExpr
/// and which are required to fully evaluate it.
///
/// This information is used by APValue to propagate
struct EvaluatedSourceLocExpr {

private:
  const SourceLocExpr *E;
  union {
    uint64_t LineOrCol;
    const char *FileOrFunc;
  };

  explicit EvaluatedSourceLocExpr(const SourceLocExpr *E, int64_t IntVal)
      : E(E), LineOrCol(IntVal) {}
  explicit EvaluatedSourceLocExpr(const SourceLocExpr *E, const char *StrVal)
      : E(E), FileOrFunc(StrVal) {}

public:
  EvaluatedSourceLocExpr() : E(nullptr) {}

  /// Determine and return the contextual information for a SourceLocExpr
  /// appearing in the (possibly null) default argument or initializer
  /// expression.
  static EvaluatedSourceLocExpr Create(const ASTContext &Ctx,
                                       const SourceLocExpr *E,
                                       const Expr *DefaultExpr);

  bool empty() const { return !E; }
  explicit operator bool() const { return !empty(); }

  bool hasStringValue() const { return E && E->isStringType(); }
  bool hasIntValue() const { return E && E->isIntType(); }

  QualType getType(const ASTContext &Ctx) const;

  /// Return the evaluated Value.
  APValue getValue() const;

  /// Evaluate the specified SourceLocExpr within this context and return
  /// the resulting string value.
  const char *getStringValue() const;
  unsigned getStringSize() const { return StringRef(getStringValue()).size(); }

  /// Evaluate the specified SourceLocExpr within this context and return
  /// the resulting integer value.
  uint64_t getIntValue() const;
};

/// Represents the current source location and context used to determine the
/// value of the source location builtins (ex. __builtin_LINE), including the
/// context of default argument and default initializer expressions.
class CurrentSourceLocExprScope {
  /// The CXXDefaultArgExpr or CXXDefaultInitExpr we're currently evaluating.
  const Expr *DefaultExpr = nullptr;

public:
  /// A RAII style scope gaurd used for tracking the current source
  /// location and context as used by the source location builtins
  /// (ex. __builtin_LINE).
  class SourceLocExprScopeGuard;

  /// A RAII style scope guard used to clear the current context and reset it
  /// to null, it will be restored upon destruction.
  class ClearSourceLocExprScopeGuard;

  const Expr *getDefaultExpr() const { return DefaultExpr; }

  explicit CurrentSourceLocExprScope() = default;

private:
  explicit CurrentSourceLocExprScope(const Expr *DefaultExpr)
      : DefaultExpr(DefaultExpr) {}

  CurrentSourceLocExprScope(CurrentSourceLocExprScope const &) = default;
  CurrentSourceLocExprScope &
  operator=(CurrentSourceLocExprScope const &) = default;
};

class CurrentSourceLocExprScope::SourceLocExprScopeGuard {
public:
  SourceLocExprScopeGuard(const Expr *DefaultExpr,
                          CurrentSourceLocExprScope &Current)
      : Current(Current), OldVal(Current), Enable(false) {
    assert(DefaultExpr && "the new scope should not be empty");
    if ((Enable = (Current.getDefaultExpr() == nullptr)))
      Current = CurrentSourceLocExprScope(DefaultExpr);
  }

  ~SourceLocExprScopeGuard() {
    if (Enable)
      Current = OldVal;
  }

private:
  SourceLocExprScopeGuard(SourceLocExprScopeGuard const &) = delete;
  SourceLocExprScopeGuard &operator=(SourceLocExprScopeGuard const &) = delete;

  CurrentSourceLocExprScope &Current;
  CurrentSourceLocExprScope OldVal;
  bool Enable;
};

class CurrentSourceLocExprScope::ClearSourceLocExprScopeGuard {
public:
  ClearSourceLocExprScopeGuard(CurrentSourceLocExprScope &Current)
      : Current(Current), OldVal(Current) {
    Current = CurrentSourceLocExprScope{};
  }

  ~ClearSourceLocExprScopeGuard() { Current = OldVal; }

private:
  ClearSourceLocExprScopeGuard(ClearSourceLocExprScopeGuard const &) = delete;
  ClearSourceLocExprScopeGuard &
  operator=(ClearSourceLocExprScopeGuard const &) = delete;
  CurrentSourceLocExprScope &Current;
  CurrentSourceLocExprScope OldVal;
};

} // end namespace clang

#endif // LLVM_CLANG_AST_EVALUATED_SOURCE_LOC_EXPR_H
