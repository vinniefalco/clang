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

enum ResumableObjectFunctionKind {
  RFK_Result,
  RFK_Resume,
  RFK_Ready,
  RFK_Destructor
};

enum ResumableFieldIdx { RFI_Data, RFI_Result, RFI_Ready };

struct ResumableFields {
  const FieldDecl *Data = nullptr;
  const FieldDecl *Result = nullptr;
  const FieldDecl *Ready = nullptr;
};

static ResumableFields GetResumableFields(const CXXRecordDecl *RD) {
  ResumableFields Fields;
  for (auto *F : RD->fields()) {
    StringRef Name = F->getIdentifier()->getName();
    if (Name == "__data_")
      Fields.Data = F;
    else if (Name == "__result_")
      Fields.Result = F;
    else if (Name == "__ready_")
      Fields.Ready = F;
    else
      llvm_unreachable("unexpected field");
  }
  assert(Fields.Data && Fields.Result && Fields.Ready);
  return Fields;
}

static ResumableObjectFunctionKind
GetResumableFunctionKind(const CXXMethodDecl *MD) {
  if (isa<CXXDestructorDecl>(MD))
    return RFK_Destructor;
  StringRef Name = MD->getIdentifier()->getName();
  ResumableObjectFunctionKind RFK =
      llvm::StringSwitch<ResumableObjectFunctionKind>(Name)
          .Case("result", RFK_Result)
          .Case("resume", RFK_Resume)
          .Case("ready", RFK_Ready);
  return RFK;
}

void CodeGenFunction::EmitImplicitResumableObjectFunctionBody(
    const CXXMethodDecl *MD) {
  assert(MD->isResumableObjectFunction());
  const CXXRecordDecl *RD = MD->getParent();

  ResumableFields Fields = GetResumableFields(RD);
  switch (GetResumableFunctionKind(MD)) {
  case RFK_Result: {
    Address ResultAddr = Builder.CreateStructGEP(
        LoadCXXThisAddress(), RFI_Result, CharUnits::Zero(), "__result_");
    llvm::Type *ResultTy = CGM.getTypes().ConvertType(MD->getReturnType());
    Address Cast = Builder.CreateElementBitCast(ResultAddr, ResultTy);

    RValue RV = RValue::getAggregate(Cast);
    EmitReturnOfRValue(RV, MD->getReturnType());
    return;
  }
  case RFK_Ready: {
    LValue ThisLV = MakeNaturalAlignAddrLValue(
        LoadCXXThis(), getContext().getTypeDeclType(RD));
    RValue RV = EmitRValueForField(ThisLV, Fields.Ready, SourceLocation());
    EmitReturnOfRValue(RV, MD->getReturnType());
    return;
  }
  case RFK_Destructor: {
    EmitBranchThroughCleanup(ReturnBlock);
    return;
  }
  case RFK_Resume: {
    assert(false);
  }
  }

  llvm_unreachable("unhandled case");
}

static const TypedefDecl *GetResultTypedef(const CXXRecordDecl *RD) {
  assert(RD->isResumable());
  for (auto *D : RD->decls()) {
    if (auto *TD = dyn_cast<TypedefDecl>(D))
      return TD;
  }
  return nullptr;
}

void CodeGenFunction::EmitResumableVarDecl(VarDecl const &VD) {
  const ResumableExpr *RE = dyn_cast<ResumableExpr>(VD.getInit());
  if (!RE && isa<ExprWithCleanups>(VD.getInit()))
    RE =
        cast<ResumableExpr>(cast<ExprWithCleanups>(VD.getInit())->getSubExpr());
  assert(RE);
  const ResumableExpr &E = *RE;
  EnterResumableExprScope Guard(*this, CGResumableData(&VD));
  CXXRecordDecl *RD = VD.getType()->getAsCXXRecordDecl();
  ResumableFields Fields = GetResumableFields(RD);
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
  Address DataAddr =
      Builder.CreateStructGEP(Loc, RFI_Data, CharUnits::Zero(), "__data_");

  llvm::Type *ResultTy =
      CGM.getTypes().ConvertType(E.getSourceExpr()->getType());
  Address Cast = Builder.CreateElementBitCast(DataAddr, ResultTy);
  EmitAnyExprToMem(E.getSourceExpr(), Cast, Qualifiers(),
                   /*IsInitializer*/ true);

  LValue PromiseLV = MakeNaturalAlignAddrLValue(
      Loc.getPointer(), getContext().getTypeDeclType(RD));
  LValue ReadyLV = EmitLValueForField(PromiseLV, Fields.Ready);
  RValue Src = RValue::get(Builder.getTrue());
  EmitStoreThroughLValue(Src, ReadyLV, /*IsInit*/ false);
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
