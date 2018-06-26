//===--- EvaluatedSourceLocScope.h ------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the EvaluatedSourceLocScopeBase used to track the current
//  context and location when evaluating source location builtins
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_EVALUATED_SOURCE_LOC_SCOPE_H
#define LLVM_CLANG_AST_EVALUATED_SOURCE_LOC_SCOPE_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/DenseMapInfo.h"

namespace clang {
class ASTContext;
class DeclContext;
class Expr;
class SourceLocExpr;
class SourceLocation;
class QualType;

struct EvaluatedSourceLocScopeBase {
private:
  friend struct llvm::DenseMapInfo<EvaluatedSourceLocScopeBase>;

  const void *Type = nullptr;
  const void *Loc = nullptr;
  const DeclContext *Context = nullptr;

  EvaluatedSourceLocScopeBase(const void *Ty, const void *L,
                              const DeclContext *Ctx)
      : Type(Ty), Loc(L), Context(Ctx) {}

public:
  EvaluatedSourceLocScopeBase() = default;
  EvaluatedSourceLocScopeBase(QualType const &Ty, SourceLocation const &Loc,
                              const DeclContext *Ctx);

  static EvaluatedSourceLocScopeBase Create(const ASTContext &Ctx,
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

  friend inline bool operator==(EvaluatedSourceLocScopeBase const &LHS,
                                EvaluatedSourceLocScopeBase const &RHS) {
    return LHS.Type == RHS.Type && LHS.Loc == RHS.Loc &&
           LHS.Context == RHS.Context;
  }
};

} // end namespace clang

namespace llvm {
template <> struct DenseMapInfo<clang::EvaluatedSourceLocScopeBase> {
  using KeyType = clang::EvaluatedSourceLocScopeBase;
  static KeyType getEmptyKey();
  static KeyType getTombstoneKey();
  static unsigned getHashValue(const KeyType &Val);
  static bool isEqual(const KeyType &LHS, const KeyType &RHS);
};
} // namespace llvm

#endif // LLVM_CLANG_AST_EVALUATED_SOURCE_LOC_SCOPE_H
