//===--- SourceLocExprScope.cpp - Mangle C++ Names --------------------------*-
// C++ -*-===//
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

#include "clang/AST/SourceLocExprScope.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Basic/SourceLocation.h"

using namespace clang;

QualType EvaluatedSourceLocScopeBase::getType() const {
  if (!Type)
    return QualType();
  return QualType::getFromOpaquePtr(Type);
}

SourceLocation EvaluatedSourceLocScopeBase::getLocation() const {
  if (!Loc)
    return SourceLocation();
  return SourceLocation::getFromPtrEncoding(Loc);
}

const DeclContext *EvaluatedSourceLocScopeBase::getContext() const {
  return Context;
}

EvaluatedSourceLocScopeBase::EvaluatedSourceLocScopeBase(
    QualType const &Ty, SourceLocation const &L, const DeclContext *Ctx)
    : Type(Ty.getAsOpaquePtr()), Loc(L.getPtrEncoding()), Context(Ctx) {}

EvaluatedSourceLocScopeBase
llvm::DenseMapInfo<EvaluatedSourceLocScopeBase>::getEmptyKey() {
  return EvaluatedSourceLocScopeBase(
      DenseMapInfo<const void *>::getEmptyKey(),
      DenseMapInfo<const void *>::getEmptyKey(),
      DenseMapInfo<const DeclContext *>::getEmptyKey());
}

EvaluatedSourceLocScopeBase
llvm::DenseMapInfo<EvaluatedSourceLocScopeBase>::getTombstoneKey() {
  return EvaluatedSourceLocScopeBase(
      DenseMapInfo<const void *>::getTombstoneKey(),
      DenseMapInfo<const void *>::getTombstoneKey(),
      DenseMapInfo<const DeclContext *>::getTombstoneKey());
}

unsigned llvm::DenseMapInfo<EvaluatedSourceLocScopeBase>::getHashValue(
    EvaluatedSourceLocScopeBase const &Val) {
  llvm::FoldingSetNodeID ID;
  ID.AddPointer(Val.Type);
  ID.AddPointer(Val.Loc);
  ID.AddPointer(Val.Context);
  return ID.ComputeHash();
}

bool llvm::DenseMapInfo<EvaluatedSourceLocScopeBase>::isEqual(
    EvaluatedSourceLocScopeBase const &LHS,
    EvaluatedSourceLocScopeBase const &RHS) {
  return LHS == RHS;
}

CurrentSourceLocExprScope::CurrentSourceLocExprScope(const Expr *DefaultExpr,
                                                     const void *EvalContextID)
    : DefaultExpr(DefaultExpr), EvalContextID(EvalContextID) {
  assert(DefaultExpr && "expression cannot be null");
  assert(EvalContextID && "context pointer cannot be null");
  assert((isa<CXXDefaultArgExpr>(DefaultExpr) ||
          isa<CXXDefaultInitExpr>(DefaultExpr)) &&
         "expression must be either a default argument or initializer");
}

static PresumedLoc getPresumedSourceLoc(const ASTContext &Ctx,
                                        SourceLocation Loc) {
  auto &SourceMgr = Ctx.getSourceManager();
  PresumedLoc PLoc =
      SourceMgr.getPresumedLoc(SourceMgr.getExpansionRange(Loc).getEnd());
  assert(PLoc.isValid()); // FIXME: Learn how to handle this.
  return PLoc;
}

static std::string getStringValue(const ASTContext &Ctx, const SourceLocExpr *E,
                                  SourceLocation Loc,
                                  const DeclContext *Context) {
  switch (E->getIdentType()) {
  case SourceLocExpr::File:
    return getPresumedSourceLoc(Ctx, Loc).getFilename();
  case SourceLocExpr::Function:
    if (const auto *FD = dyn_cast_or_null<FunctionDecl>(Context)) {
      if (DeclarationName Name = FD->getDeclName())
        return Name.getAsString();
    }
    return "";
  case SourceLocExpr::Line:
  case SourceLocExpr::Column:
    llvm_unreachable("no string value for __builtin_LINE\\COLUMN");
  }
}

StringLiteral *
EvaluatedSourceLocScope::CreateStringLiteral(const ASTContext &Ctx) const {
  assert(E && E->isStringType() && !empty());
  return StringLiteral::Create(Ctx, getStringValue(), StringLiteral::Ascii,
                               /*Pascal*/ false, getType(), getLocation());
}

IntegerLiteral *
EvaluatedSourceLocScope::CreateIntegerLiteral(const ASTContext &Ctx) const {
  assert(E && E->isIntType() && Result.isInt());
  return IntegerLiteral::Create(Ctx, Result.getInt(), Ctx.UnsignedIntTy,
                                getLocation());
}

EvaluatedSourceLocScopeBase
CurrentSourceLocExprScope::getEvaluatedInfoBase(ASTContext const &Ctx,
                                                SourceLocExpr const *E) const {
  QualType Type;
  SourceLocation Loc;
  const DeclContext *Context;

  if (auto *DIE = dyn_cast_or_null<CXXDefaultInitExpr>(DefaultExpr)) {
    Loc = DIE->getUsedLocation();
    Context = DIE->getUsedContext();
  } else if (auto *DAE = dyn_cast_or_null<CXXDefaultArgExpr>(DefaultExpr)) {
    Loc = DIE->getUsedLocation();
    Context = DIE->getUsedContext();
  } else {
    Loc = E->getLocation();
    Context = E->getParentContext();
  }

  if (E->isStringType()) {
    Type = SourceLocExpr::BuildStringArrayType(
        Ctx, getStringValue(Ctx, E, Loc, Context).size() + 1);
  } else {
    Type = Ctx.UnsignedIntTy;
  }

  return EvaluatedSourceLocScopeBase(Type, Loc, Context);
}

EvaluatedSourceLocScope
EvaluatedSourceLocScope::Create(ASTContext const &Ctx, const SourceLocExpr *E,
                                EvaluatedSourceLocScopeBase Base) {
  EvaluatedSourceLocScope Info{Base, E};

  PresumedLoc PLoc = getPresumedSourceLoc(Ctx, Info.getLocation());
  assert(PLoc.isValid());

  switch (E->getIdentType()) {
  case SourceLocExpr::File:
  case SourceLocExpr::Function: {
    std::string Val =
        ::getStringValue(Ctx, E, Info.getLocation(), Info.getContext());
    Info.setStringValue(std::move(Val));

    APValue::LValueBase LVBase(E);
    LVBase.setEvaluatedSourceLocScope(Base);
    APValue StrVal(LVBase, CharUnits::Zero(), APValue::NoLValuePath{});
    Info.Result.swap(StrVal);
  } break;
  case SourceLocExpr::Line:
  case SourceLocExpr::Column: {
    auto Val = E->getIdentType() == SourceLocExpr::Line ? PLoc.getLine()
                                                        : PLoc.getColumn();

    llvm::APSInt TmpRes(llvm::APInt(Val, Ctx.getTargetInfo().getIntWidth()));
    APValue NewVal(TmpRes);
    Info.Result.swap(NewVal);
  } break;
  }
  return Info;
}

EvaluatedSourceLocScope
CurrentSourceLocExprScope::getEvaluatedInfo(ASTContext const &Ctx,
                                            SourceLocExpr const *E) const {
  return EvaluatedSourceLocScope::Create(Ctx, E, getEvaluatedInfoBase(Ctx, E));
}

SourceLocExprScopeGuard::SourceLocExprScopeGuard(
    CurrentSourceLocExprScope NewScope, CurrentSourceLocExprScope &Current)
    : Current(Current), OldVal(Current), Enable(false) {
  if ((Enable = ShouldEnable(Current, NewScope)))
    Current = NewScope;
}

bool SourceLocExprScopeGuard::ShouldEnable(
    CurrentSourceLocExprScope const &CurrentScope,
    CurrentSourceLocExprScope const &NewScope) {
  assert(!NewScope.empty() && "the new scope should not be empty");
  // Only update the default argument scope if we've entered a new
  // evaluation context, and not when it's nested within another default
  // argument. Example:
  //    int bar(int x = __builtin_LINE()) { return x; }
  //    int foo(int x = bar())  { return x; }
  //    static_assert(foo() == __LINE__);
  if (CurrentScope.empty())
    return true;
  // if (isa<CXXDefaultInitExpr>(CurrentScope.DefaultExpr) &&
  //        isa<CXXDefaultArgExpr>(NewScope.DefaultExpr))
  //  return false;
  return !CurrentScope.isInSameContext(NewScope);
}

SourceLocExprScopeGuard::~SourceLocExprScopeGuard() {
  if (Enable)
    Current = OldVal;
}
