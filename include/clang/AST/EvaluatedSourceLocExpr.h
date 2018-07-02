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


#include "clang/AST/DeclarationName.h"
#include "clang/Basic/LLVM.h"
#include "llvm/ADT/DenseMapInfo.h"
#include <cassert>
#include <type_traits>

namespace clang {
class ASTContext;
class DeclContext;
class Expr;
class SourceLocExpr;
class SourceLocation;
class QualType;
class APValue;
class FunctionDecl;

/// Contextual information coorisponding to the AST context of a SourceLocExpr
/// and which are required to fully evaluate it.
///
/// This information is used by APValue to propagate
struct EvaluatedSourceLocExpr {
  /// The types of the source location builtins.
  enum ResultKind : char { IT_None, IT_LineOrCol, IT_FileOrFunc };

private:
  friend struct llvm::DenseMapInfo<EvaluatedSourceLocExpr>;
  union {
    uint64_t LineOrCol;
    const char *FileOrFunc;
    /// Used by DenseMapInfo when creating empty and tombstone keys.
    char Empty;
  };
  ResultKind Kind;

  struct MakeKeyTag {};

  EvaluatedSourceLocExpr(MakeKeyTag, char KeyValue)
    : Empty(KeyValue),  Kind(IT_None) {}
  explicit EvaluatedSourceLocExpr(int64_t IntVal)
      : LineOrCol(IntVal), Kind(IT_LineOrCol) {}
  explicit EvaluatedSourceLocExpr(const char *StrVal)
      : FileOrFunc(StrVal), Kind(IT_FileOrFunc) {}

public:
  EvaluatedSourceLocExpr() : Empty(0), Kind(IT_None) {}


  /// Determine and return the contextual information for a SourceLocExpr
  /// appearing in the (possibly null) default argument or initializer
  /// expression.
  static EvaluatedSourceLocExpr Create(const ASTContext &Ctx,
                                     const SourceLocExpr *E,
                                     const Expr *DefaultExpr);

  bool empty() const { return Kind == IT_None; }
  explicit operator bool() const { return !empty(); }

  bool hasStringValue() const { return Kind == IT_FileOrFunc; }
  bool hasIntValue() const { return Kind == IT_LineOrCol; }

  QualType getType(const ASTContext &Ctx) const;

  /// Evaluate the specified SourceLocExpr within this context and return
  /// the value.
  APValue Evaluate(const ASTContext &Ctx, const SourceLocExpr *E) const;

  /// Evaluate the specified SourceLocExpr within this context and return
  /// the resulting string value.
  StringRef getStringValue() const;
  unsigned getStringSize() const { return getStringValue().size(); }

  /// Evaluate the specified SourceLocExpr within this context and return
  /// the resulting integer value.
  uint64_t getIntValue() const;

  friend inline bool operator==(EvaluatedSourceLocExpr const &LHS,
                                EvaluatedSourceLocExpr const &RHS) {
    if (LHS.Kind != RHS.Kind)
      return false;
    if (LHS.Kind == IT_None)
      return true;
    if (LHS.Kind == IT_LineOrCol)
      return LHS.LineOrCol == RHS.LineOrCol;
    if (LHS.Kind == IT_FileOrFunc)
      return LHS.FileOrFunc == RHS.FileOrFunc;
    return false;
  }
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

namespace llvm {
template <> struct DenseMapInfo<clang::EvaluatedSourceLocExpr> {
  using KeyType = clang::EvaluatedSourceLocExpr;
  static KeyType getEmptyKey();
  static KeyType getTombstoneKey();
  static unsigned getHashValue(const KeyType &Val);
  static bool isEqual(const KeyType &LHS, const KeyType &RHS);
};
} // namespace llvm

#endif // LLVM_CLANG_AST_EVALUATED_SOURCE_LOC_EXPR_H
