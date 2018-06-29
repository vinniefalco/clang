//===--- EvaluateSourceLocExpr.cpp - ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines two basic utilities needed to fully evaluate a
//  SourceLocExpr within a particular context and to track said context.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/EvaluateSourceLocExpr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/SourceLocExprContext.h"
#include "clang/Basic/SourceLocation.h"

using namespace clang;

QualType SourceLocExprContext::getType() const {
  if (!Type)
    return QualType();
  return QualType::getFromOpaquePtr(Type);
}

void SourceLocExprContext::setType(const QualType &T) {
  Type = T.getAsOpaquePtr();
}

SourceLocation SourceLocExprContext::getLocation() const {
  if (!Loc)
    return SourceLocation();
  return SourceLocation::getFromPtrEncoding(Loc);
}

void SourceLocExprContext::setLocation(const SourceLocation &L) {
  Loc = L.getPtrEncoding();
}

SourceLocExprContext::SourceLocExprContext(QualType const &Ty,
                                           SourceLocation const &L,
                                           const DeclContext *Ctx)
    : Type(Ty.getAsOpaquePtr()), Loc(L.getPtrEncoding()), Context(Ctx) {}

SourceLocExprContext llvm::DenseMapInfo<SourceLocExprContext>::getEmptyKey() {
  return SourceLocExprContext(DenseMapInfo<const void *>::getEmptyKey(),
                              DenseMapInfo<const void *>::getEmptyKey(),
                              DenseMapInfo<const DeclContext *>::getEmptyKey());
}

SourceLocExprContext
llvm::DenseMapInfo<SourceLocExprContext>::getTombstoneKey() {
  return SourceLocExprContext(
      DenseMapInfo<const void *>::getTombstoneKey(),
      DenseMapInfo<const void *>::getTombstoneKey(),
      DenseMapInfo<const DeclContext *>::getTombstoneKey());
}

unsigned llvm::DenseMapInfo<SourceLocExprContext>::getHashValue(
    SourceLocExprContext const &Val) {
  llvm::FoldingSetNodeID ID;
  ID.AddPointer(Val.Type);
  ID.AddPointer(Val.Loc);
  ID.AddPointer(Val.Context);
  return ID.ComputeHash();
}

bool llvm::DenseMapInfo<SourceLocExprContext>::isEqual(
    SourceLocExprContext const &LHS, SourceLocExprContext const &RHS) {
  return LHS == RHS;
}

CurrentSourceLocExprScope::CurrentSourceLocExprScope(const Expr *DefaultExpr,
                                                     const void *EvalContextID)
    : DefaultExpr(DefaultExpr), EvalContextID(EvalContextID) {
  assert(DefaultExpr && "expression cannot be null");
  // assert(EvalContextID && "context pointer cannot be null");
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

SourceLocExprContext SourceLocExprContext::Create(const ASTContext &Ctx,
                                                  const SourceLocExpr *E,
                                                  const Expr *DefaultExpr) {
  SourceLocExprContext Base;

  if (auto *DIE = dyn_cast_or_null<CXXDefaultInitExpr>(DefaultExpr)) {
    Base.setLocation(DIE->getUsedLocation());
    Base.setContext(DIE->getUsedContext());
  } else if (auto *DAE = dyn_cast_or_null<CXXDefaultArgExpr>(DefaultExpr)) {
    Base.setLocation(DAE->getUsedLocation());
    Base.setContext(DAE->getUsedContext());
  } else {
    Base.setLocation(E->getLocation());
    Base.setContext(E->getParentContext());
  }

  if (E->isStringType())
    Base.setType(SourceLocExpr::BuildStringArrayType(
        Ctx,
        getStringValue(Ctx, E, Base.getLocation(), Base.getContext()).size() +
            1));
  else
    Base.setType(Ctx.UnsignedIntTy);

  return Base;
}

EvaluatedSourceLocExpr
EvaluatedSourceLocExpr::Create(ASTContext const &Ctx, const SourceLocExpr *E,
                               SourceLocExprContext Base) {
  EvaluatedSourceLocExpr Info{Base, E};

  PresumedLoc PLoc = getPresumedSourceLoc(Ctx, Info.getLocation());
  assert(PLoc.isValid());

  switch (E->getIdentType()) {
  case SourceLocExpr::File:
  case SourceLocExpr::Function: {
    std::string Val =
        ::getStringValue(Ctx, E, Info.getLocation(), Info.getContext());
    Info.setStringValue(std::move(Val));

    APValue::LValueBase LVBase(E);
    LVBase.setSourceLocExprContext(Base);
    APValue StrVal(LVBase, CharUnits::Zero(), APValue::NoLValuePath{});
    Info.Result.swap(StrVal);
  } break;
  case SourceLocExpr::Line:
  case SourceLocExpr::Column: {
    auto Val = E->getIdentType() == SourceLocExpr::Line ? PLoc.getLine()
                                                        : PLoc.getColumn();

    llvm::APSInt TmpRes(llvm::APInt(Ctx.getTargetInfo().getIntWidth(), Val));
    APValue NewVal(TmpRes);
    Info.Result.swap(NewVal);
  } break;
  }
  return Info;
}

bool SourceLocExprScopeGuard::ShouldEnable(
    CurrentSourceLocExprScope const &CurrentScope,
    CurrentSourceLocExprScope const &NewScope) {
  assert(NewScope.getDefaultExpr() && "the new scope should not be empty");
  // Only update the default argument scope if we've entered a new
  // evaluation context, and not when it's nested within another default
  // argument. Example:
  //    int bar(int x = __builtin_LINE()) { return x; }
  //    int foo(int x = bar())  { return x; }
  //    static_assert(foo() == __LINE__);
  return CurrentScope.getDefaultExpr() == nullptr ||
         NewScope.EvalContextID != CurrentScope.EvalContextID;
}

SourceLocExprScopeGuard::SourceLocExprScopeGuard(
    CurrentSourceLocExprScope NewScope, CurrentSourceLocExprScope &Current)
    : Current(Current), OldVal(Current), Enable(false) {

  if ((Enable = ShouldEnable(Current, NewScope)))
    Current = NewScope;
}

SourceLocExprScopeGuard::~SourceLocExprScopeGuard() {
  if (Enable)
    Current = OldVal;
}
