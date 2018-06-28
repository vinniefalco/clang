//===--- SourceLocExprContext.h ---------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the SourceLocExprContext used to track the current
//  context and location when evaluating source location builtins.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_SOURCE_LOC_EXPR_CONTEXT_H
#define LLVM_CLANG_AST_SOURCE_LOC_EXPR_CONTEXT_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/DenseMapInfo.h"

namespace clang {
class ASTContext;
class DeclContext;
class Expr;
class SourceLocExpr;
class SourceLocation;
class QualType;

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

  friend inline bool operator==(SourceLocExprContext const &LHS,
                                SourceLocExprContext const &RHS) {
    return LHS.Type == RHS.Type && LHS.Loc == RHS.Loc &&
           LHS.Context == RHS.Context;
  }
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
