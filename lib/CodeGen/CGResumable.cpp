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

void CodeGenFunction::EmitBreakResumableStmt(BreakResumableStmt const &S) {
  ErrorUnsupported(&S, "break resumable statement");
}
