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

//===----------------------------------------------------------------------===//
//                    EvaluatedSourceLocExpr Definitions
//===----------------------------------------------------------------------===//

QualType EvaluatedSourceLocExpr::getType(const ASTContext &Ctx) const {
  assert(!empty());
  if (hasIntValue())
    return Ctx.UnsignedIntTy;

  return SourceLocExpr::BuildStringArrayType(Ctx, getStringSize() + 1);
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

  PresumedLoc PLoc = Ctx.getSourceManager().getPresumedLoc(
      Ctx.getSourceManager().getExpansionRange(Loc).getEnd());

  switch (E->getIdentType()) {
  case SourceLocExpr::Function:
    const auto *FD = dyn_cast_or_null<FunctionDecl>(Context);
    return EvaluatedSourceLocExpr(E, FD ? Ctx.getReadableFunctionName(FD) : "");
  case SourceLocExpr::File:
    return EvaluatedSourceLocExpr(E, PLoc.getFilename());
  case SourceLocExpr::Line:
  case SourceLocExpr::Column:
    return EvaluatedSourceLocExpr(E, E->getIdentType() == SourceLocExpr::Line
                                         ? PLoc.getLine()
                                         : PLoc.getColumn());
  }
  llvm_unreachable("unhandled case");

}

APValue EvaluatedSourceLocExpr::getValue() const {
  switch (E->getIdentType()) {
  case SourceLocExpr::File:
  case SourceLocExpr::Function: {
    APValue::LValueBase LVBase(E);
    LVBase.setLValueString(FileOrFunc);
    APValue StrVal(LVBase, CharUnits::Zero(), APValue::NoLValuePath{});
    return StrVal;
  }
  case SourceLocExpr::Line:
  case SourceLocExpr::Column: {
    llvm::APSInt TmpRes(
        llvm::APInt(Ctx.getTargetInfo().getIntWidth(), LineOrCol));
    APValue NewVal(TmpRes);
    return NewVal;
  }
  }

  llvm_unreachable("unhandled case");
}

uint64_t EvaluatedSourceLocExpr::getIntValue() const {
  assert(hasIntValue() && "SourceLocExpr is not a integer expression");
  return LineOrCol;
}

const char *EvaluatedSourceLocExpr::getStringValue() const {
  assert(hasStringValue() && "no string value");
  return FileOrFunc;
}
