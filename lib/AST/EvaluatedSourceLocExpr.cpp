//===--- EvaluatedSourceLocExpr.cpp -------------------------------*- C++ -*-===//
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

#include "clang/AST/EvaluatedSourceLocExpr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
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
//                    EvaluatedSourceLocExpr Definitions
//===----------------------------------------------------------------------===//

QualType EvaluatedSourceLocExpr::getType(const ASTContext &Ctx) const {
  assert(!empty());
  if (hasIntValue())
    return Ctx.UnsignedIntTy;
  return SourceLocExpr::BuildStringArrayType(Ctx, FileOrFunc.size() + 1);
}


EvaluatedSourceLocExpr llvm::DenseMapInfo<EvaluatedSourceLocExpr>::getEmptyKey() {
  return EvaluatedSourceLocExpr(
          EvaluatedSourceLocExpr::MakeKeyTag{},
          DenseMapInfo<char>::getEmptyKey());
}

EvaluatedSourceLocExpr
llvm::DenseMapInfo<EvaluatedSourceLocExpr>::getTombstoneKey() {
  return EvaluatedSourceLocExpr(
      EvaluatedSourceLocExpr::MakeKeyTag{},
      DenseMapInfo<char>::getTombstoneKey());
}

unsigned llvm::DenseMapInfo<EvaluatedSourceLocExpr>::getHashValue(
    EvaluatedSourceLocExpr const &Val) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(Val.Kind);
  switch (Val.Kind) {
  case EvaluatedSourceLocExpr::IT_None:
    ID.AddInteger(Val.Empty);
    break;
  case EvaluatedSourceLocExpr::IT_LineOrCol:
    ID.AddInteger(Val.LineOrCol);
    break;
  case EvaluatedSourceLocExpr::IT_File:
    ID.AddString(Val.FileName);
    break;
  case EvaluatedSourceLocExpr::IT_Func:
    ID.AddPointer(Val.FuncDecl);
    break;
  }
  return ID.ComputeHash();
}

bool llvm::DenseMapInfo<EvaluatedSourceLocExpr>::isEqual(
    EvaluatedSourceLocExpr const &LHS, EvaluatedSourceLocExpr const &RHS) {
  return LHS == RHS;
}

EvaluatedSourceLocExpr EvaluatedSourceLocExpr::Create(const ASTContext &Ctx,
                                                  const SourceLocExpr *E,
                                                  const Expr *DefaultExpr) {
  SourceLocation Loc;
  const DeclContext *Context;

  std::tie(Loc, Context) = [&]() -> std::pair<SourceLocation, const DeclContext *> {
    if (auto *DIE = dyn_cast_or_null<CXXDefaultInitExpr>(DefaultExpr))
      return {DIE->getUsedLocation(), DIE->getUsedContext()};
    if (auto *DAE = dyn_cast_or_null<CXXDefaultArgExpr>(DefaultExpr))
      return {DAE->getUsedLocation(), DAE->getUsedContext()};
    return {E->getLocation(), E->getParentContext()};
  }();

  PresumedLoc PLoc = getPresumedSourceLoc(Ctx, Loc);

  switch (E->getIdentType()) {
  case SourceLocExpr::Function: {
    DeclarationName FuncName;
     if (const auto *FD = dyn_cast_or_null<FunctionDecl>(Context))
       return EvaluatedSourceLocExpr(FileTag{}, FD->getDeclName());
      return EvaluatedSourceLocExpr(FileTag{}, DeclarationName{});
  }
  case SourceLocExpr::File:
    return EvaluatedSourceLocExpr(FileTag{}, PLoc.getFilename());
  case SourceLocExpr::Line:
  case SourceLocExpr::Column:
    return EvaluatedSourceLocExpr(LineOrColTag{},
            E->getIdentType() == SourceLocExpr::Line ? PLoc.getLine() : PLoc.getColumn());

  }
  llvm_unreachable("unhandled case");

}

APValue EvaluatedSourceLocExpr::Evaluate(const ASTContext &Ctx,
                                       const SourceLocExpr *E) const {
  switch (E->getIdentType()) {
  case SourceLocExpr::File:
  case SourceLocExpr::Function: {
    std::string Str = makeStringValue(Ctx, E, getLocation(), getContext());
    APValue::LValueBase LVBase(E);
    LVBase.setEvaluatedSourceLocExpr(*this);
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

uint64_t EvaluatedSourceLocExpr::getIntValue(const ASTContext &Ctx,
                                           const SourceLocExpr *E) const {
  assert(E->isIntType() && "SourceLocExpr is not a integer expression");
  return Evaluate(Ctx, E).getInt().getZExtValue();
}

std::string EvaluatedSourceLocExpr::getStringValue(const ASTContext &Ctx,
                                                 const SourceLocExpr *E) const {
  assert(false);
  return "";
}
