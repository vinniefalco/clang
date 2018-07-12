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
  const Expr *CurResumableExpr = nullptr;
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
  CXXRecordDecl *RD = VD.getType()->getAsCXXRecordDecl();
  const FieldDecl *DataF, *ResultF;
  for (auto *F : RD->fields()) {
    StringRef Name = F->getIdentifier()->getName();
    if (Name == "__data_")
      DataF = F;
    else if (Name == "__result_")
      ResultF = F;
    else
      llvm_unreachable("unexpected field");
  }
  assert(DataF && ResultF && "fields not set");
  if (VD.getStorageDuration() != SD_Automatic)
    return;
  EmitVarDecl(VD);
  Address Loc(nullptr, CharUnits::Zero());
  if (VD.hasLocalStorage())
    Loc = GetAddrOfLocalVar(&VD);
  else {
    llvm::Constant *GV = CGM.GetAddrOfGlobalVar(&VD);
    Loc = Address(GV, getContext().getDeclAlign(&VD));
  }
  LValue LV = MakeAddrLValue(Loc, VD.getType());
  LValue ResultLV = EmitLValueForField(LV, ResultF);
  llvm::Type *ResultTy =
      CGM.getTypes().ConvertType(E.getSourceExpr()->getType());
  Address Cast = Builder.CreateElementBitCast(ResultLV.getAddress(), ResultTy);
  EmitAnyExprToMem(E.getSourceExpr(), Cast, Qualifiers(),
                   /*IsInitializer*/ false);
}

void CodeGenFunction::EmitResumableFunctionBody(
    clang::CodeGen::FunctionArgList &Args, clang::Stmt const *Body) {
  EmitFunctionBody(Args, Body);
}

void CodeGenFunction::EmitBreakResumableStmt(BreakResumableStmt const &S) {
  ErrorUnsupported(&S, "break resumable statement");
}

LValue CodeGenFunction::EmitResumableLValue(const ResumableExpr *E) {
  return EmitLValue(E->getSourceExpr());
}

void CodeGenFunction::EmitResumableExpr(const ResumableExpr *E,
                                        AggValueSlot Dest) {
  EmitAnyExpr(E->getSourceExpr(), Dest);
}
