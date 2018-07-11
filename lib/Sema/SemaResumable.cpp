//===--- SemaResumable.cpp - Semantic Analysis for Resumable Functions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file implements semantic analysis for C++ Resumable Functions
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTLambda.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Overload.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaInternal.h"

using namespace clang;
using namespace sema;

void Sema::CheckCompletedResumableFunctionBody(FunctionDecl *FD, Stmt *Body,
                                               FunctionScopeInfo *FnScope) {
  if (!Body) {
    assert(FD && FD->isInvalidDecl() && "No body for valid decl?");
    return;
  }
  assert(FD && FD->isResumable() &&
         "No function decl or non-resumable function decl?");
  assert(FnScope && "No function scope info?");
  // FIXME(EricWF): Do something with this
}

static void SetCurFunctionImplicitlyResumableImp(Sema &S) {

  // Find the callee, and mark that callee as implicitly resumable.
  if (FunctionDecl *FD = S.getCurFunctionDecl()) {
    // FIXME(EricWF): Diagnose cases where the function we're marking implicitly
    // resumable isn't inline.
    if (!FD->isResumableSpecified())
      FD->setImplicitlyResumable(true);
  } else if (LambdaScopeInfo *Lam = S.getCurLambda()) {
    if (CXXMethodDecl *MD = Lam->CallOperator)
      MD->setImplicitlyResumable(true);
    else
      llvm_unreachable("lambda does not yet have body?");
  } else {
    // Nothing to do. We should be at the global scope.
  }
}

void Sema::SetCurFunctionImplicitlyResumable(const BreakResumableStmt *S) {
  // Record the location of the resumable function call.
  if (FunctionScopeInfo *FnScope = getCurFunction())
    FnScope->setFirstResumableStmt(S);
  SetCurFunctionImplicitlyResumableImp(*this);
}

void Sema::SetCurFunctionImplicitlyResumable(const FunctionDecl *Callee,
                                             SourceLocation CallLoc) {
  if (VarDecl *VD = ResumableContext.EvaluatingDecl)
    return;
  if (FunctionScopeInfo *FnScope = getCurFunction())
    FnScope->setFirstResumableStmt(Callee, CallLoc);
  SetCurFunctionImplicitlyResumableImp(*this);
}

void Sema::DiagnoseUseOfResumableFunction(const FunctionDecl *FD,
                                          SourceLocation Loc) {
  assert(FD && FD->isResumable() && "Not a resumable function?");
  if (const FunctionDecl *Templ = FD->getTemplateInstantiationPattern())
    FD = Templ;
  if (!FD->isDefined())
    Diag(Loc, diag::err_resumable_function_not_defined) << FD;
}

static void noteResumableDecl(Sema &S, const FunctionDecl *FD,
                              FunctionScopeInfo *FnScope) {
  if (FD->isResumableSpecified()) {
    FD = FD->getCanonicalDecl();
    S.Diag(FD->getLocStart(), diag::note_declared_resumable_here);
    return;
  }

  assert(FnScope && FnScope->hasResumableStmt());

  auto D =
      S.Diag(FnScope->FirstResumableStmtLoc, diag::note_made_resumable_here)
      << !FnScope->isFirstResumableStmtBreakResumable();
  if (const FunctionDecl *FD = FnScope->getFirstResumableCallee())
    D << FD;
}

bool Sema::CheckResumableFunctionDecl(const FunctionDecl *FD,
                                      FunctionScopeInfo *FnScope,
                                      bool IsInstantiation) {
  assert(FD->isResumable() && "Checking non-resumable function");
  bool PreviouslyNoted = false;
  auto addNote = [&]() {
    if (!PreviouslyNoted)
      noteResumableDecl(*this, FD, FnScope);
    PreviouslyNoted = true;
  };
  bool HadError = false;

  if (const auto *Method = dyn_cast<CXXMethodDecl>(FD)) {
    if (Method->isVirtual()) {
      // Method = Method->getCanonicalDecl();
      Diag(Method->getLocation(), diag::err_resumable_virtual);

      // If it's not obvious why this function is virtual, find an overridden
      // function which uses the 'virtual' keyword.
      Method = Method->getCanonicalDecl();
      const CXXMethodDecl *WrittenVirtual = Method;
      while (!WrittenVirtual->isVirtualAsWritten())
        WrittenVirtual = *WrittenVirtual->begin_overridden_methods();
      if (WrittenVirtual != Method)
        Diag(WrittenVirtual->getLocation(),
             diag::note_overridden_virtual_function);
      addNote();
      HadError = true;
    }
  }

  if (!IsInstantiation && FD->isConstexpr()) {
    Diag(FD->getInnerLocStart(), diag::err_resumable_function_decl_constexpr)
        << FD;
    addNote();
    HadError = true;
  }

  if (!FD->isResumableSpecified() && !FD->isInlined() &&
      !FD->isInlineSpecified() && FD->hasExternalFormalLinkage()) {
    Diag(FD->getInnerLocStart(), diag::err_implicitly_resumable_not_inline)
        << FD;
    addNote();
    HadError = true;
  }

  int SpecialMemberSelect = [&]() {
    if (isa<CXXConstructorDecl>(FD))
      return 0;
    if (isa<CXXDestructorDecl>(FD))
      return 1;
    if (FD->getOverloadedOperator() == OO_Equal)
      return 2;
    return -1;
  }();
  if (SpecialMemberSelect != -1) {
    assert(!FD->isResumableSpecified() && "this should be diagnosed earlier");
    Diag(FD->getInnerLocStart(), diag::err_special_member_implicitly_resumable)
        << SpecialMemberSelect;
    addNote();
    HadError = true;
  }

  if (FD->getTemplateSpecializationKind() ==
          TSK_ExplicitInstantiationDeclaration ||
      FD->getTemplateSpecializationKind() ==
          TSK_ExplicitInstantiationDefinition) {
    bool IsDefinition = FD->getTemplateSpecializationKind() ==
                        TSK_ExplicitInstantiationDefinition;
    Diag(FD->getInnerLocStart(),
         diag::err_explicit_instantiation_of_resumable_function)
        << IsDefinition << FD;
    if (!IsDefinition)
      Diag(FD->getPointOfInstantiation(),
           diag::note_explicit_instantiation_declaration_here);
    HadError = true;
  }

  return HadError;
}

namespace {
class CheckResumableVarInit
    : public RecursiveASTVisitor<CheckResumableVarInit> {
public:
  Sema &S;
  SourceLocation Loc;
  VarDecl *ResumableDecl;
  Expr *Initializer;
  bool AtTopLevel = true;
  bool IsInvalid = false;
  bool CheckedInit = false;

  struct AtTopLevelGuard {
    bool &TopLevel;
    bool OldValue;

    AtTopLevelGuard(bool &TopLevel) : TopLevel(TopLevel), OldValue(TopLevel) {}
    ~AtTopLevelGuard() { TopLevel = OldValue; }
  };

public:
  typedef RecursiveASTVisitor<CheckResumableVarInit> Inherited;

  CheckResumableVarInit(Sema &S, VarDecl *VD, Expr *Init, SourceLocation Loc)
      : S(S), Loc(Loc), ResumableDecl(VD), Initializer(Init) {}

  bool DiagnoseNonResumableCallee(const CallExpr *CE) {
    const Decl *Callee = CE->getCalleeDecl();
    assert(Callee && "No callee for function call?");
    if (auto *FD = dyn_cast<FunctionDecl>(Callee)) {
      if (!FD->isResumable()) {
        S.Diag(Initializer->getExprLoc(),
               diag::err_resumable_var_decl_init_not_resumable_expr)
            << ResumableDecl << Initializer->getSourceRange();
        S.Diag(FD->getLocation(), diag::note_function_declared_here);
        IsInvalid = true;
        return true;
      }
    }
    return false;
  }

  bool TraverseCallExpr(CallExpr *CE, DataRecursionQueue *Queue = nullptr) {

    // AtTopLevelGuard Gaurd(AtTopLevel);
    if (!WalkUpFromCallExpr(CE))
      return false;

    AtTopLevel = false;
    for (Stmt *SubStmt : getStmtChildren(CE)) {
      if (!TraverseStmt(SubStmt, Queue))
        return false;
    }
    return true;
  }

  bool VisitCallExpr(const CallExpr *CE) {
    if (AtTopLevel) {
      CheckedInit = true;
      if (DiagnoseNonResumableCallee(CE))
        return false;
    } else {
      const FunctionDecl *FD = CE->getDirectCallee();
      assert(FD && "No callee?");
      if (FD->isResumable())
        S.SetCurFunctionImplicitlyResumable(FD, CE->getExprLoc());
    }
    return true;
  }

  bool TraverseConditionalOperator(ConditionalOperator *Op,
                                   DataRecursionQueue *Queue = nullptr) {
    if (AtTopLevel)
      CheckedInit = true;
    {
      AtTopLevelGuard Guard(AtTopLevel);
      AtTopLevel = false;
      if (!TraverseStmt(Op->getCond()))
        return false;
    }
    {
      AtTopLevelGuard Guard(AtTopLevel);
      if (!TraverseStmt(Op->getLHS()))
        return false;
    }
    {
      AtTopLevelGuard Guard(AtTopLevel);
      return TraverseStmt(Op->getRHS());
    }
  }

  bool VisitConditionalExpr(const ConditionalOperator *CE) { return true; }

  bool VisitBinCommaExpr(const BinaryOperator *CE) { return true; }
};
} // namespace

bool Sema::CheckResumableVarDeclInit(VarDecl *VD, Expr *Init) {
  assert(VD && Init && "cannot be null");
  Init = Init->IgnoreParens();
  CheckResumableVarInit Checker{*this, VD, Init, Init->getExprLoc()};
  Checker.TraverseStmt(Init);
  if (Checker.CheckedInit && Checker.IsInvalid)
    return true;
  else if (!Checker.CheckedInit) {

    llvm::errs() << "Unhandled Resumable var decl init case\n";
    Init->dumpColor();
    Diag(Init->getExprLoc(), diag::err_resumable_var_decl_init_not_call_expr)
        << VD << Init->getSourceRange();
    return true;
  }

  CXXRecordDecl *RD = CXXRecordDecl::Create(
      Context, TTK_Class, CurContext, VD->getLocStart(), VD->getLocation(),
      &PP.getIdentifierTable().get("__promise"), /*PrevDecl*/ nullptr,
      /*DelayTypeCreation*/ false);

  RD->startDefinition();

  QualType CArrTy = Context.getConstantArrayType(
      Context.CharTy, llvm::APInt(32, 1024), ArrayType::Normal, 0);

  struct {
    QualType Ty;
    const char *Name;
  } Fields[] = {{CArrTy, "data"}};

  // Create fields
  for (auto &F : Fields) {
    FieldDecl *Field = FieldDecl::Create(
        Context, RD, SourceLocation(), SourceLocation(),
        &PP.getIdentifierTable().get(F.Name), F.Ty, /*TInfo=*/nullptr,
        /*BitWidth=*/nullptr,
        /*Mutable=*/false, ICIS_NoInit);
    Field->setAccess(AS_public);
    RD->addDecl(Field);
  }

  ExprResult DecltypeRes;
  {
    EnterExpressionEvaluationContext Unevaluated(
        *this, ExpressionEvaluationContext::Unevaluated, nullptr,
        /*IsDecltype*/ true);
    DecltypeRes = ActOnDecltypeExpression(Init);
    if (DecltypeRes.isInvalid())
      return true;
  }
  TypedefDecl *ResultTy = TypedefDecl::Create(
      Context, CurContext, SourceLocation(), SourceLocation(),
      &PP.getIdentifierTable().get("result_type"),
      Context.getTrivialTypeSourceInfo(DecltypeRes.get()->getType()));
  RD->addDecl(ResultTy);
  RD->setImplicitCopyConstructorIsDeleted();

  RD->completeDefinition();

  VD->setType(QualType(RD->getTypeForDecl(), 0));

  return false;
}
