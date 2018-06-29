//===--- SourceLocExprContext.h ---------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines types used to track the current context and location
//  when evaluating source location builtins.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_SOURCE_LOC_EXPR_CONTEXT_H
#define LLVM_CLANG_AST_SOURCE_LOC_EXPR_CONTEXT_H

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

/// Contextual information coorisponding to the AST context of a SourceLocExpr
/// and which are required to fully evaluate it.
///
/// This information is used by APValue to propagate
struct SourceLocExprContext {
private:
  friend struct llvm::DenseMapInfo<SourceLocExprContext>;

  /// The QualType of the SourceLocExpr in the current context. For
  /// __builtin_FILE and __builtin_FUNCTION this is the constant array type
  /// with the correct size for the eventual value.
  const void *Type = nullptr;

  /// The SourceLocation at which the SourceLocExpr should be considered to
  /// appear.
  const void *Loc = nullptr;

  /// The DeclContext in which the SourceLocExpr should be considered to
  /// appear.
  const DeclContext *Context = nullptr;

  // Used for building empty and tombstone keys.
  SourceLocExprContext(const void *Ty, const void *L, const DeclContext *Ctx)
      : Type(Ty), Loc(L), Context(Ctx) {}

public:
  SourceLocExprContext() = default;
  SourceLocExprContext(QualType const &Ty, SourceLocation const &Loc,
                       const DeclContext *Ctx);

  /// Determine and return the contextual information for a SourceLocExpr
  /// appearing in the (possibly null) default argument or initializer
  /// expression.
  static SourceLocExprContext Create(const ASTContext &Ctx,
                                     const SourceLocExpr *E,
                                     const Expr *DefaultExpr);

  bool empty() const { return Type == nullptr; }
  explicit operator bool() const { return !empty(); }

  QualType getType() const;
  void setType(const QualType &T);

  SourceLocation getLocation() const;
  void setLocation(const SourceLocation &L);

  const DeclContext *getContext() const { return Context; }
  void setContext(const DeclContext *C) { Context = C; }

  std::string getStringValue(const ASTContext &Ctx,
                             const SourceLocExpr *E) const;
  uint32_t getIntValue(const ASTContext &Ctx, const SourceLocExpr *E) const;

  APValue Evaluate(const ASTContext &Ctx, const SourceLocExpr *E) const;

  friend inline bool operator==(SourceLocExprContext const &LHS,
                                SourceLocExprContext const &RHS) {
    return LHS.Type == RHS.Type && LHS.Loc == RHS.Loc &&
           LHS.Context == RHS.Context;
  }

private:
  struct EvalResult;

  EvalResult EvaluateInternal(const ASTContext &Ctx,
                              const SourceLocExpr *E) const;
};

/// Represents the current source location and context used to determine the
/// value of the source location builtins (ex. __builtin_LINE), including the
/// context of default argument and default initializer expressions.
///
/// \tparam EvalContextType A type representing the currect evaluation context.
/// Two contexts are considered equal only if their pointers compare equal.
template <class EvalContextType> class CurrentSourceLocExprScope {
  /// The CXXDefaultArgExpr or CXXDefaultInitExpr we're currently evaluating.
  const Expr *DefaultExpr = nullptr;

  /// A pointer representing the current evaluation context. It is used to
  /// determine context equality.
  const EvalContextType *EvalContextID = nullptr;

public:
  /// A RAII style scope gaurd used for tracking the current source
  /// location and context as used by the source location builtins
  /// (ex. __builtin_LINE).
  class SourceLocExprScopeGuard;

  const Expr *getDefaultExpr() const { return DefaultExpr; }

  explicit CurrentSourceLocExprScope() = default;

private:
  CurrentSourceLocExprScope(const Expr *DefaultExpr,
                            const EvalContextType *EvalContextID)
      : DefaultExpr(DefaultExpr), EvalContextID(EvalContextID) {}

  CurrentSourceLocExprScope(CurrentSourceLocExprScope const &) = default;
  CurrentSourceLocExprScope &
  operator=(CurrentSourceLocExprScope const &) = default;
};

template <class EvalContextType>
class CurrentSourceLocExprScope<EvalContextType>::SourceLocExprScopeGuard {
public:
  SourceLocExprScopeGuard(const Expr *DefaultExpr,
                          CurrentSourceLocExprScope &Current,
                          const EvalContextType *EvalContext)
      : Current(Current), OldVal(Current), Enable(false) {
    assert(DefaultExpr && "the new scope should not be empty");
    CurrentSourceLocExprScope NewScope(DefaultExpr, EvalContext);
    if ((Enable = ShouldEnable(EvalContext)))
      Current = NewScope;
  }

  ~SourceLocExprScopeGuard() {
    if (Enable)
      Current = OldVal;
  }

private:
  bool ShouldEnable(const EvalContextType *NewEvalContext) const {
    // Only update the default argument scope if we've entered a new
    // evaluation context, and not when it's nested within another default
    // argument. Example:
    //    int bar(int x = __builtin_LINE()) { return x; }
    //    int foo(int x = bar())  { return x; }
    //    static_assert(foo() == __LINE__);
    return Current.getDefaultExpr() == nullptr ||
           NewEvalContext != Current.EvalContextID;
  }

  SourceLocExprScopeGuard(SourceLocExprScopeGuard const &) = delete;
  SourceLocExprScopeGuard &operator=(SourceLocExprScopeGuard const &) = delete;

  CurrentSourceLocExprScope &Current;
  CurrentSourceLocExprScope OldVal;
  bool Enable;
};

} // end namespace clang

namespace llvm {
template <> struct DenseMapInfo<clang::SourceLocExprContext> {
  using KeyType = clang::SourceLocExprContext;
  static KeyType getEmptyKey();
  static KeyType getTombstoneKey();
  static unsigned getHashValue(const KeyType &Val);
  static bool isEqual(const KeyType &LHS, const KeyType &RHS);
};
} // namespace llvm

#endif // LLVM_CLANG_AST_SOURCE_LOC_EXPR_CONTEXT_H
