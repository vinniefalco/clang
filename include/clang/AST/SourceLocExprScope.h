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

#include <type_traits>

namespace clang {
class SourceLocation;
class Expr;
class CXXDefaultInitExpr;
class CXXDefaultArgExpr;
class DeclContext;

class SourceLocExprScopeGuard;

/// Represents the current source location and context used to determine the
/// value of the source location builtins (ex. __builtin_LINE), including the
/// context of default argument and default initializer expressions,
class CurrentSourceLocExprScope {
public:
  template <class DefaultExprType> struct SourceLocScope {
    const DefaultExprType *DefaultExpr = nullptr;
    const void *EvalContextID = nullptr;

    explicit operator bool() const { return DefaultExpr; }
  };

  friend class SourceLocExprScopeGuard;

  enum CurrentContextKind { CCK_None, CCK_DefaultArg, CCK_DefaultInit };

  CurrentContextKind CurrentKind = CCK_None;
  const Expr *DefaultExpr = nullptr;
  const CXXDefaultArgExpr *DefaultArg = nullptr;
  const CXXDefaultInitExpr *DefaultInit = nullptr;

  const void *EvalContextID = nullptr;

public:
  CurrentSourceLocExprScope() = default;
  CurrentSourceLocExprScope(const CXXDefaultArgExpr *DefaultExpr,
                            const void *EvalContextID,
                            CurrentSourceLocExprScope const &LastScope);

  CurrentSourceLocExprScope(const CXXDefaultInitExpr *DefaultExpr,
                            const void *EvalContextID,
                            CurrentSourceLocExprScope const &LastScope);

  bool empty() const { return CurrentKind == CCK_None; }
  explicit operator bool() const { return !empty(); }
  bool isDefaultArgScope() const { return CurrentKind == CCK_DefaultArg; }
  bool isDefaultInitScope() const { return CurrentKind == CCK_DefaultInit; }
  const Expr *getDefaultExpr() const;

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

  SourceLocation getLoc() const;
  const DeclContext *getContext() const;
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
  SourceLocExprScopeGuard(const CXXDefaultArgExpr *DefaultExpr,
                          CurrentSourceLocExprScope &Current,
                          EvalContextType *EvalContext)
      : SourceLocExprScopeGuard(
            CurrentSourceLocExprScope(DefaultExpr, EvalContext, Current),
            Current) {}

  template <class EvalContextType>
  SourceLocExprScopeGuard(const CXXDefaultInitExpr *DefaultExpr,
                          CurrentSourceLocExprScope &Current,
                          EvalContextType *EvalContext)
      : SourceLocExprScopeGuard(
            CurrentSourceLocExprScope(DefaultExpr, EvalContext, Current),
            Current) {}

  ~SourceLocExprScopeGuard();

private:
  CurrentSourceLocExprScope &Current;
  CurrentSourceLocExprScope OldVal;
  bool Enable;
};

} // end namespace clang

#endif // LLVM_CLANG_AST_SOURCE_LOC_EXPR_SCOPE_H
