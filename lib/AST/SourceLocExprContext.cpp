//===--- SourceLocExprContext.cpp -------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines two basic utilities needed to track and fully evaluate a
//  SourceLocExpr within a particular context.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/SourceLocExprContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/SourceLocExprContext.h"
#include "clang/Basic/SourceLocation.h"

using namespace clang;

static PresumedLoc getPresumedSourceLoc(const ASTContext &Ctx,
                                        SourceLocation Loc) {
  auto &SourceMgr = Ctx.getSourceManager();
  PresumedLoc PLoc =
      SourceMgr.getPresumedLoc(SourceMgr.getExpansionRange(Loc).getEnd());
  assert(PLoc.isValid()); // FIXME: Learn how to handle this.
  return PLoc;
}

static std::string makeStringValue(const ASTContext &Ctx,
                                   const SourceLocExpr *E, SourceLocation Loc,
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

//===----------------------------------------------------------------------===//
//                    SourceLocExprContext Definitions
//===----------------------------------------------------------------------===//

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
        makeStringValue(Ctx, E, Base.getLocation(), Base.getContext()).size() +
            1));
  else
    Base.setType(Ctx.UnsignedIntTy);

  return Base;
}

APValue SourceLocExprContext::Evaluate(const ASTContext &Ctx,
                                       const SourceLocExpr *E) const {
  switch (E->getIdentType()) {
  case SourceLocExpr::File:
  case SourceLocExpr::Function: {
    std::string Str = makeStringValue(Ctx, E, getLocation(), getContext());
    APValue::LValueBase LVBase(E);
    LVBase.setSourceLocExprContext(*this);
    APValue StrVal(LVBase, CharUnits::Zero(), APValue::NoLValuePath{});
    return StrVal;
  }
  case SourceLocExpr::Line:
  case SourceLocExpr::Column: {
    PresumedLoc PLoc = getPresumedSourceLoc(Ctx, getLocation());
    auto Val = E->getIdentType() == SourceLocExpr::Line ? PLoc.getLine()
                                                        : PLoc.getColumn();
    llvm::APSInt TmpRes(llvm::APInt(Ctx.getTargetInfo().getIntWidth(), Val));
    APValue NewVal(TmpRes);
    return NewVal;
  }
  }

  llvm_unreachable("unhandled case");
}

uint64_t SourceLocExprContext::getIntValue(const ASTContext &Ctx,
                                           const SourceLocExpr *E) const {
  assert(E->isIntType() && "SourceLocExpr is not a integer expression");
  return Evaluate(Ctx, E).getInt().getZExtValue();
}

std::string SourceLocExprContext::getStringValue(const ASTContext &Ctx,
                                                 const SourceLocExpr *E) const {
  assert(E->isStringType() &&
         "SourceLocExpr is not a string literal expression");
  return makeStringValue(Ctx, E, getLocation(), getContext());
}
