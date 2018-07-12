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

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/ASTMutationListener.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Overload.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/Sema.h"

#include "clang/Sema/SemaInternal.h"
#include <cursesf.h>

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

static bool HasTrivialDtor(Sema &Self, SourceLocation KeyLoc, QualType T) {
  ASTContext &C = Self.Context;
  assert(!T->isDependentType());
  // C++14 [meta.unary.prop]:
  //   For reference types, is_destructible<T>::value is true.
  if (T->isReferenceType())
    return true;

  // Objective-C++ ARC: autorelease types don't require destruction.
  if (T->isObjCLifetimeType() &&
      T.getObjCLifetime() == Qualifiers::OCL_Autoreleasing)
    return true;

  // C++14 [meta.unary.prop]:
  //   For incomplete types and function types, is_destructible<T>::value is
  //   false.
  if (T->isIncompleteType() || T->isFunctionType())
    return false;

  // A type that requires destruction (via a non-trivial destructor or ARC
  // lifetime semantics) is not trivially-destructible.
  if (T.isDestructedType())
    return false;

  return true;
}

CXXRecordDecl *Sema::BuildResumableObjectType(Expr *Init, SourceLocation Loc) {
  CXXRecordDecl *RD =
      CXXRecordDecl::CreateResumableClass(Context, CurContext, Loc);
  assert(RD->isBeingDefined() && RD->isImplicit());

  auto Error = [&]() {
    RD->setInvalidDecl();
    return RD;
  };

  // Create buffer field.
  auto MakeField = [&](const char *Name, unsigned Size, unsigned Align) {
    QualType CArrTy = Context.getConstantArrayType(
        Context.CharTy, llvm::APInt(32, Size), ArrayType::Normal, 0);
    // FIXME(EricWF): Align this field to the maximum alignment.
    FieldDecl *Field = FieldDecl::Create(
        Context, RD, SourceLocation(), SourceLocation(),
        &PP.getIdentifierTable().get(Name), CArrTy, /*TInfo=*/nullptr,
        /*BitWidth=*/nullptr,
        /*Mutable=*/false, ICIS_NoInit);
    Field->setAccess(AS_private);

    llvm::APInt Val(32, Align);
    Expr *ValE = IntegerLiteral::Create(Context, Val, Context.UnsignedIntTy,
                                        SourceLocation());
    AlignedAttr *AA =
        new (Context) AlignedAttr(SourceRange(), Context, true, ValE, 3);
    Field->addAttr(AA);
    RD->addDecl(Field);
  };
  MakeField("__data_", 1024, 16);
  {
    QualType Ty = Init->isTypeDependent() ? Context.IntTy : Init->getType();
    TypeInfo Info = Context.getTypeInfo(Ty);
    MakeField("__result_", Info.Width, Info.Align);
  }
  {
    FieldDecl *Field =
        FieldDecl::Create(Context, RD, SourceLocation(), SourceLocation(),
                          &PP.getIdentifierTable().get("__ready_"),
                          Context.BoolTy, /*TInfo=*/nullptr,
                          /*BitWidth=*/nullptr,
                          /*Mutable=*/false, ICIS_NoInit);
    Field->setAccess(AS_private);
    RD->addDecl(Field);
  }
  // Build typedef	decltype(	expression )	result_type;
  QualType ResultTy;
  {
    EnterExpressionEvaluationContext Unevaluated(
        *this, ExpressionEvaluationContext::Unevaluated, nullptr,
        /*IsDecltype*/ true);
    ExprResult DecltypeRes = ActOnDecltypeExpression(Init);
    if (DecltypeRes.isInvalid())
      return Error();

    Expr *DeclExpr = DecltypeRes.get();
    ResultTy = BuildDecltypeType(DeclExpr, Loc, /*AsUnevaluated*/ true);

    TypedefDecl *ResultTypedef =
        TypedefDecl::Create(Context, RD, SourceLocation(), SourceLocation(),
                            &PP.getIdentifierTable().get("result_type"),
                            Context.getTrivialTypeSourceInfo(ResultTy, Loc));
    ResultTypedef->setAccess(AS_public);
    RD->addDecl(ResultTypedef);
  }

  auto AddMethod = [&](StringRef Name, QualType ReturnTy,
                       FunctionProtoType::ExtProtoInfo Proto) {
    QualType MethodTy = Context.getFunctionType(ReturnTy, None, Proto);
    CXXMethodDecl *MD = CXXMethodDecl::Create(
        Context, RD, Loc,
        DeclarationNameInfo(&PP.getIdentifierTable().get(Name), Loc), MethodTy,
        Context.getTrivialTypeSourceInfo(MethodTy, Loc), SC_None,
        /*IsInline*/ true, /*isConstexpr*/ false,
        /*IsResumable*/ false, Loc);
    MD->setAccess(AS_public);
    MD->setImplicit(true);
    MD->setWillHaveBody(true);

    RD->addDecl(MD);
  };

  // Create result_type result();
  AddMethod("result", ResultTy, FunctionProtoType::ExtProtoInfo{});

  // Create bool	ready()	const	noexcept;
  {
    FunctionProtoType::ExtProtoInfo Proto;
    Proto.ExceptionSpec.Type = EST_BasicNoexcept;
    Proto.TypeQuals = Qualifiers::Const;
    AddMethod("ready", Context.BoolTy, Proto);
  }
  // Create void resume() noexcept(noexcept(expression));
  {
    FunctionProtoType::ExtProtoInfo Proto;
    Proto.ExceptionSpec.Type = EST_BasicNoexcept;
    ExprResult CondRes = BuildCXXNoexceptExpr(Loc, Init, Loc);
    if (CondRes.isInvalid())
      return Error();
    ExprResult NoexceptRes =
        ActOnNoexceptSpec(Loc, CondRes.get(), Proto.ExceptionSpec.Type);
    if (NoexceptRes.isInvalid())
      return Error();
    Proto.ExceptionSpec.NoexceptExpr = NoexceptRes.get();
    AddMethod("resume", Context.VoidTy, Proto);
  }

  bool DeclareDtor = !ResultTy->isDependentType() &&
                     !HasTrivialDtor(*this, SourceLocation(), ResultTy);

  if (DeclareDtor) {
    CanQualType ClassType =
        Context.getCanonicalType(Context.getTypeDeclType(RD));
    DeclarationName Name =
        Context.DeclarationNames.getCXXDestructorName(ClassType);
    DeclarationNameInfo NameInfo(Name, Loc);
    CXXDestructorDecl *Destructor = CXXDestructorDecl::Create(
        Context, RD, Loc, NameInfo, QualType(), nullptr, /*isInline=*/true,
        /*isImplicitlyDeclared=*/false);
    Destructor->setAccess(AS_public);
    // Destructor->setImplicit(true);
    Destructor->setWillHaveBody(true);

    FunctionProtoType::ExtProtoInfo EPI;

    // Build an exception specification pointing back at this member.
    EPI.ExceptionSpec.Type = EST_Unevaluated;
    EPI.ExceptionSpec.SourceDecl = Destructor;

    // Set the calling convention to the default for C++ instance methods.
    EPI.ExtInfo = EPI.ExtInfo.withCallingConv(
        Context.getDefaultCallingConvention(/*IsVariadic=*/false,
                                            /*IsCXXMethod=*/true));

    Destructor->setType(Context.getFunctionType(Context.VoidTy, None, EPI));

    RD->addDecl(Destructor);
  }

  SmallVector<Decl *, 4> Fields(RD->fields());
  ActOnFields(nullptr, Loc, RD, Fields, SourceLocation(), SourceLocation(),
              nullptr);
  CheckCompletedCXXClass(RD);

  if (!Init->getType()->isDependentType())
    DefineResultObjectFunctions(RD, Loc);

  return RD;
}

struct FieldInfo {
  FieldDecl *Data = nullptr;
  FieldDecl *Ready = nullptr;
  FieldDecl *Result = nullptr;

  FieldInfo(CXXRecordDecl *RD) {
    for (auto *F : RD->fields()) {
      StringRef Name = F->getIdentifier()->getName();
      if (Name == "__data_") {
        Data = F;
        continue;
      }
      if (Name == "__ready_") {
        Ready = F;
        continue;
      }
      if (Name == "__result_") {
        Result = F;
        continue;
      }
      llvm_unreachable("unhandled case");
    }
  }
};

bool Sema::DefineResultObjectFunctions(CXXRecordDecl *RD, SourceLocation Loc) {
  assert(RD->isCompleteDefinition() && RD->isResumable());
  FieldInfo Fields(RD);

  if (RD->hasUserDeclaredDestructor()) {
    CXXDestructorDecl *Dtor = RD->getDestructor();
    SynthesizedFunctionScope Scope(*this, Dtor);
    Scope.addContextNote(Loc);
    MarkBaseAndMemberDestructorsReferenced(Dtor->getLocation(),
                                           Dtor->getParent());
    if (CheckDestructor(Dtor)) {
      Dtor->setInvalidDecl();
      return true;
    }
    SourceLocation Loc =
        Dtor->getLocEnd().isValid() ? Dtor->getLocEnd() : Dtor->getLocation();

    SmallVector<Stmt *, 8> Statements;
    FieldDecl *ReadyMember = Fields.Ready;
    Expr *This = ActOnCXXThis(Loc).getAs<Expr>();

    TypeSourceInfo *TInfo = nullptr;
    for (const Decl *D : RD->decls()) {
      if (auto *Typedef = dyn_cast<TypedefDecl>(D)) {
        TInfo = Typedef->getTypeSourceInfo();
        break;
      }
    }
    assert(TInfo);
    QualType WithRef = Context.getLValueReferenceType(TInfo->getType());
    TypeSourceInfo *LVTInfo = Context.getTrivialTypeSourceInfo(WithRef, Loc);

    auto BuildMemberRef = [&](FieldDecl *F) {
      const CXXScopeSpec SS;
      DeclarationNameInfo NameInfo(F->getDeclName(), Loc);
      DeclAccessPair FoundDecl = DeclAccessPair::make(F, AS_public);
      return BuildFieldReferenceExpr(This, /*IsArrow*/ true, Loc, SS, F,
                                     FoundDecl, NameInfo)
          .get();
    };

    Expr *Result = BuildMemberRef(Fields.Result);
    Expr *Casted = BuildCXXNamedCast(Loc, tok::kw_reinterpret_cast, LVTInfo,
                                     Result, SourceRange(), SourceRange())
                       .get();
    assert(Casted);

    assert(TInfo->getType().isDestructedType());

    CXXDestructorDecl *ResultDtor =
        TInfo->getType()->getAsCXXRecordDecl()->getDestructor();
    DeclarationNameInfo DtorNameInfo(ResultDtor->getDeclName(), Loc);
    MemberExpr *ME = new (Context)
        MemberExpr(Casted, false, SourceLocation(), ResultDtor, DtorNameInfo,
                   Context.BoundMemberTy, VK_LValue, OK_Ordinary);
    ExprResult DestroyExprRes = BuildCallToMemberFunction(
        nullptr, ME, SourceLocation(), None, SourceLocation());
    if (DestroyExprRes.isInvalid()) {
      RD->setInvalidDecl();
      return true;
    }
    // CXXScopeSpec SS;
    // SS.Extend(Context, SourceLocation(), TInfo->getTypeLoc(),
    // SourceLocation()); ExprResult DtorRef = BuildMemberReferenceExpr(Casted,
    // LVTInfo->getType(), SourceLocation(), /*IsArrow*/false,
    // SS, SourceLocation(), )
    // PseudoDestructorTypeStorage DtorStorage(LVTInfo);
    // ExprResult DestroyExprRes = BuildPseudoDestructorExpr(Casted, Loc,
    // tok::period, SS,
    //                                             TInfo, SourceLocation(),
    //                                            SourceLocation(),
    //                                            DtorStorage);
    assert(!DestroyExprRes.isInvalid());

    DestroyExprRes.get()->dumpColor();
    StmtResult IfBlock =
        ActOnIfStmt(Loc, false, nullptr,
                    ActOnCondition(nullptr, Loc, BuildMemberRef(Fields.Ready),
                                   Sema::ConditionKind::Boolean),
                    DestroyExprRes.get(), SourceLocation(), nullptr);
    if (IfBlock.isInvalid()) {
      RD->setInvalidDecl();
      return true;
    }
    Statements.push_back(IfBlock.get());
    StmtResult Body;
    {
      CompoundScopeRAII CompoundScope(*this);
      Body = ActOnCompoundStmt(Loc, Loc, Statements,
                               /*isStmtExpr=*/false);
      assert(!Body.isInvalid() && "Compound statement creation cannot fail");
    }
    Dtor->setBody(Body.get());
    Dtor->markUsed(Context);

    if (ASTMutationListener *L = getASTMutationListener()) {
      L->CompletedImplicitDefinition(Dtor);
    }
  }

  return false;
}

bool Sema::CheckResumableVarDeclInit(VarDecl *VD, Expr *Init) {
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

  return false;
}

ExprResult Sema::BuildResumableExpr(VarDecl *VD, Expr *Init) {
  assert(VD && Init && "cannot be null");
  assert(!isa<ResumableExpr>(Init));

  auto Error = [&]() {
    VD->setInvalidDecl();
    return ExprError();
  };

  CXXRecordDecl *RD = BuildResumableObjectType(Init, VD->getTypeSpecStartLoc());
  if (RD->isInvalidDecl()) {
    return Error();
  }

  // Replace 'auto' with the new class type.
  // QualType NewRecordType = Context.getQualifiedType(
  //     RD->getTypeForDecl(), VD->getType().getQualifiers());
  // VD->setType(NewRecordType);

  // Build the new ResumableExpr wrapper around the initializer.
  ResumableExpr *NewInit = ResumableExpr::Create(Context, RD, Init);
  return NewInit;
}
