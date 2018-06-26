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

int Depth = -1;

SourceManager *MySM;

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

static std::string getStringValue(const ASTContext &Ctx,
                                  EvaluatedSourceLocInfoBase &Info) {
  switch (Info.E->getIdentType()) {
  case SourceLocExpr::File:
    return getPresumedSourceLoc(Ctx, Info.Loc).getFilename();
  case SourceLocExpr::Function:
    if (const auto *FD = dyn_cast_or_null<FunctionDecl>(Info.Context)) {
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
EvaluatedSourceLocInfo::CreateStringLiteral(const ASTContext &Ctx) const {
  assert(E && E->isStringType() && !Type.isNull());
  return StringLiteral::Create(Ctx, getStringValue(), StringLiteral::Ascii,
                               /*Pascal*/ false, Type, SourceLocation());
}

IntegerLiteral *
EvaluatedSourceLocInfo::CreateIntegerLiteral(const ASTContext &Ctx) const {
  assert(E && E->isIntType() && Result.isInt());
  return IntegerLiteral::Create(Ctx, Result.getInt(), Ctx.UnsignedIntTy, Loc);
}

EvaluatedSourceLocInfoBase
CurrentSourceLocExprScope::getEvaluatedInfoBase(ASTContext const &Ctx,
                                                SourceLocExpr const *E) const {
  EvaluatedSourceLocInfoBase Info;
  Info.E = E;
  if (auto *DIE = dyn_cast_or_null<CXXDefaultInitExpr>(DefaultExpr)) {
    Info.Loc = DIE->getUsedLocation();
    Info.Context = DIE->getUsedContext();
  } else if (auto *DAE = dyn_cast_or_null<CXXDefaultArgExpr>(DefaultExpr)) {
    Info.Loc = DIE->getUsedLocation();
    Info.Context = DIE->getUsedContext();
  } else {
    Info.Loc = E->getLocation();
    Info.Context = E->getParentContext();
  }

  if (E->isStringType()) {
    Info.Type = SourceLocExpr::BuildStringArrayType(
        Ctx, getStringValue(Ctx, Info).size() + 1);
  } else {
    Info.Type = Ctx.UnsignedIntTy;
  }

  return Info;
}

EvaluatedSourceLocInfo CurrentSourceLocExprScope::getEvaluatedInfoFromBase(
    ASTContext const &Ctx, clang::EvaluatedSourceLocInfoBase Base) {
  EvaluatedSourceLocInfo Info{Base};
  const SourceLocExpr *E = Info.E;

  PresumedLoc PLoc = getPresumedSourceLoc(Ctx, Info.Loc);
  assert(PLoc.isValid());

  switch (E->getIdentType()) {
  case SourceLocExpr::File:
  case SourceLocExpr::Function: {
    std::string Val = getStringValue(Ctx, Info);
    Info.setStringValue(std::move(Val));

    APValue::LValueBase LVBase(E);
    LVBase.setSourceLocContext({Info.Type, Info.Loc, Info.Context});
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

EvaluatedSourceLocInfo
CurrentSourceLocExprScope::getEvaluatedInfo(ASTContext const &Ctx,
                                            SourceLocExpr const *E) const {
  EvaluatedSourceLocInfo Info{getEvaluatedInfoBase(Ctx, E)};
}

static const DeclContext *getUsedContext(const Expr *E) {
  if (!E)
    return nullptr;
  if (auto *DAE = dyn_cast<CXXDefaultArgExpr>(E))
    return DAE->getUsedContext();
  if (auto *DIE = dyn_cast<CXXDefaultInitExpr>(E))
    return DIE->getUsedContext();
  llvm_unreachable("unhandled expression kind");
}

static SourceLocation getUsedLoc(const Expr *E) {
  if (!E)
    return SourceLocation();
  if (auto *DAE = dyn_cast<CXXDefaultArgExpr>(E))
    return DAE->getUsedLocation();
  if (auto *DIE = dyn_cast<CXXDefaultInitExpr>(E))
    return DIE->getUsedLocation();
  llvm_unreachable("unhandled expression kind");
}

SourceLocExprScopeGuard::SourceLocExprScopeGuard(
    CurrentSourceLocExprScope NewScope, CurrentSourceLocExprScope &Current)
    : Current(Current), OldVal(Current), Enable(false) {
  if ((Enable = ShouldEnable(Current, NewScope))) {
    auto PrintLoc = [&]() {
      SourceLocation Loc = getUsedLoc(NewScope.DefaultExpr);
      assert(MySM);
      Loc.dump(*MySM);
      llvm::errs() << " ";
    };

    llvm::errs() << "#" << std::to_string(++Depth) << " Pushing ";
    if (auto *DAE = dyn_cast<CXXDefaultArgExpr>(NewScope.DefaultExpr)) {
      llvm::errs() << "Default Arg " << DAE->getParam()->getName();
      PrintLoc();
    } else if (auto *DIE = dyn_cast<CXXDefaultInitExpr>(NewScope.DefaultExpr)) {
      llvm::errs() << "Default Init " << DIE->getField()->getName();
      PrintLoc();
      DIE->getExpr()->dumpColor();
    }
    llvm::errs() << "\n";
    Current = NewScope;
  }
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
  if (Enable) {
    llvm::errs() << "#" << std::to_string(Depth--) << " Popping";

    llvm::errs() << "\n";
    Current = OldVal;
  }
}
