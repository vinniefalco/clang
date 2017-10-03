//===--- ArgumentDependenceChecker.h ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines ArgumentDependenceChecker, which is used to find argument
//  usages in expressions
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_SEMA_ARGUMENTDEPENDENCECHECKER_H
#define LLVM_CLANG_LIB_SEMA_ARGUMENTDEPENDENCECHECKER_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"

namespace clang {

/// Determines if a given Expr references any of the given function's
/// ParmVarDecls, or the function's implicit `this` parameter (if applicable).
class ArgumentDependenceChecker
    : public RecursiveASTVisitor<ArgumentDependenceChecker> {
#ifndef NDEBUG
  const CXXRecordDecl *ClassType;
#endif
  llvm::SmallPtrSet<const ParmVarDecl *, 16> Parms;
  bool Result;

public:
  ArgumentDependenceChecker(const FunctionDecl *FD) {
#ifndef NDEBUG
    if (const auto *MD = dyn_cast<CXXMethodDecl>(FD))
      ClassType = MD->getParent();
    else
      ClassType = nullptr;
#endif
    Parms.insert(FD->param_begin(), FD->param_end());
  }

  bool referencesArgs(Expr *E) {
    Result = false;
    TraverseStmt(E);
    return Result;
  }

  bool VisitCXXThisExpr(CXXThisExpr *E) {
    assert(E->getType()->getPointeeCXXRecordDecl() == ClassType &&
           "`this` doesn't refer to the enclosing class?");
    Result = true;
    return false;
  }

  bool VisitDeclRefExpr(DeclRefExpr *DRE) {
    if (const auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl()))
      if (Parms.count(PVD)) {
        Result = true;
        return false;
      }
    return true;
  }
};
} // namespace clang

#endif
