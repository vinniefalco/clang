//===----- CGCoroutine.cpp - Emit LLVM Code for C++ coroutines ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This contains code dealing with C++ code generation of coroutines.
//
//===----------------------------------------------------------------------===//

#include "CGCleanup.h"
#include "CodeGenFunction.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtVisitor.h"
#include "llvm/ADT/ScopeExit.h"

using namespace clang;
using namespace CodeGen;

using llvm::BasicBlock;
using llvm::Value;

struct clang::CodeGen::CGResumableData {
  CGResumableData(const VarDecl *VD) : ResumableObject(VD) {}

  const VarDecl *ResumableObject;
};

CodeGenFunction::CGResumableInfo::CGResumableInfo(const CGResumableData &D)
    : Data(new CGResumableData(D)) {}

CodeGenFunction::CGResumableInfo::CGResumableInfo() {}
CodeGenFunction::CGResumableInfo::~CGResumableInfo() {}

namespace {
struct EnterResumableExprScope {
  CodeGenFunction &CGF;
  std::unique_ptr<CGResumableData> Old;
  EnterResumableExprScope(CodeGenFunction &CGF, const CGResumableData &NewData)
      : CGF(CGF), Old(std::move(CGF.CurResumableExpr.Data)) {
    CGF.CurResumableExpr.Data.reset(new CGResumableData(NewData));
  }
  ~EnterResumableExprScope() { CGF.CurResumableExpr.Data = std::move(Old); }
};
} // namespace

void CodeGenFunction::EmitResumableVarDecl(VarDecl const &VD) {
  const ResumableExpr &E = *cast<ResumableExpr>(VD.getInit());
  EnterResumableExprScope Guard(*this, CGResumableData(&VD));
}

void CodeGenFunction::EmitResumableFunctionBody(
    clang::CodeGen::FunctionArgList &Args, clang::Stmt const *Body) {
  EmitFunctionBody(Args, Body);
}

void CodeGenFunction::EmitBreakResumableStmt(BreakResumableStmt const &S) {
  ErrorUnsupported(&S, "break resumable statement");
}
