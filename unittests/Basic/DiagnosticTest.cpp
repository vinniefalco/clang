//===- unittests/Basic/DiagnosticTest.cpp -- Diagnostic engine tests ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticError.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace clang;

namespace {

// Check that DiagnosticErrorTrap works with SuppressAllDiagnostics.
TEST(DiagnosticTest, suppressAndTrap) {
  DiagnosticsEngine Diags(new DiagnosticIDs(),
                          new DiagnosticOptions,
                          new IgnoringDiagConsumer());
  Diags.setSuppressAllDiagnostics(true);

  {
    DiagnosticErrorTrap trap(Diags);

    // Diag that would set UncompilableErrorOccurred and ErrorOccurred.
    Diags.Report(diag::err_target_unknown_triple) << "unknown";

    // Diag that would set UnrecoverableErrorOccurred and ErrorOccurred.
    Diags.Report(diag::err_cannot_open_file) << "file" << "error";

    // Diag that would set FatalErrorOccurred
    // (via non-note following a fatal error).
    Diags.Report(diag::warn_mt_message) << "warning";

    EXPECT_TRUE(trap.hasErrorOccurred());
    EXPECT_TRUE(trap.hasUnrecoverableErrorOccurred());
  }

  EXPECT_FALSE(Diags.hasErrorOccurred());
  EXPECT_FALSE(Diags.hasFatalErrorOccurred());
  EXPECT_FALSE(Diags.hasUncompilableErrorOccurred());
  EXPECT_FALSE(Diags.hasUnrecoverableErrorOccurred());
}

// Check that SuppressAfterFatalError works as intended
TEST(DiagnosticTest, suppressAfterFatalError) {
  for (unsigned Suppress = 0; Suppress != 2; ++Suppress) {
    DiagnosticsEngine Diags(new DiagnosticIDs(),
                            new DiagnosticOptions,
                            new IgnoringDiagConsumer());
    Diags.setSuppressAfterFatalError(Suppress);

    // Diag that would set UnrecoverableErrorOccurred and ErrorOccurred.
    Diags.Report(diag::err_cannot_open_file) << "file" << "error";

    // Diag that would set FatalErrorOccurred
    // (via non-note following a fatal error).
    Diags.Report(diag::warn_mt_message) << "warning";

    EXPECT_TRUE(Diags.hasErrorOccurred());
    EXPECT_TRUE(Diags.hasFatalErrorOccurred());
    EXPECT_TRUE(Diags.hasUncompilableErrorOccurred());
    EXPECT_TRUE(Diags.hasUnrecoverableErrorOccurred());

    // The warning should be emitted and counted only if we're not suppressing
    // after fatal errors.
    EXPECT_EQ(Diags.getNumWarnings(), Suppress ? 0u : 1u);
  }
}

TEST(DiagnosticTest, diagnosticError) {
  DiagnosticsEngine Diags(new DiagnosticIDs(), new DiagnosticOptions,
                          new IgnoringDiagConsumer());
  PartialDiagnostic::StorageAllocator Alloc;
  llvm::Expected<std::pair<int, int>> Value = DiagnosticError::create(
      SourceLocation(), PartialDiagnostic(diag::err_cannot_open_file, Alloc)
                            << "file"
                            << "error");
  ASSERT_TRUE(!Value);
  llvm::Error Err = Value.takeError();
  Optional<PartialDiagnosticAt> ErrDiag = DiagnosticError::take(Err);
  llvm::cantFail(std::move(Err));
  ASSERT_FALSE(!ErrDiag);
  EXPECT_EQ(ErrDiag->first, SourceLocation());
  EXPECT_EQ(ErrDiag->second.getDiagID(), diag::err_cannot_open_file);

  Value = std::make_pair(20, 1);
  ASSERT_FALSE(!Value);
  EXPECT_EQ(*Value, std::make_pair(20, 1));
  EXPECT_EQ(Value->first, 20);
}

TEST(DiagnosticTest, testArgumentsOver10) {
  DiagnosticsEngine Diags(new DiagnosticIDs(), new DiagnosticOptions,
                          new IgnoringDiagConsumer());
  auto DiagIDs = Diags.getDiagnosticIDs();
  unsigned ID = DiagIDs->getCustomDiagID(DiagnosticIDs::Error, "%0 %1 %2%3 %4 %5 %6 %7 %8 %9 %10 %11%12");

  const char *Args[] = {"zero", "one", "2", " three", "four", "five", "six",
    "seven", "eight", "nine", "ten", "11", " twelve"};
  const unsigned NumArgs = sizeof(Args) / sizeof(Args[0]);
  ASSERT_EQ(NumArgs, 13u);

  const std::string ExpectText =
      "zero one 2 three four five six seven eight nine ten 11 twelve";

  DiagnosticBuilder Builder = Diags.Report(ID);
  for (const char* Arg : Args) {
    if (isDigit(*Arg))
      Builder << std::stoi(Arg);
    else
      Builder << Arg;
  }

  Diagnostic D(&Diags);

  ASSERT_TRUE(Diags.isDiagnosticInFlight());

  ASSERT_EQ(D.getNumArgs(), NumArgs);
  for (unsigned I=0; I < NumArgs; ++I) {
    if (isDigit(*Args[I])) {
      int Expect = std::stoi(Args[I]);
      int Arg = D.getArgSInt(I);
      EXPECT_EQ(Expect, Arg);
    } else {
      const char* Expect = Args[I];
      const char* Arg = D.getArgCStr(I);
      EXPECT_EQ(Expect, Arg);
    }
  }

  SmallVector<char, 1> DiagText;
  DiagText.reserve(1024);
  D.FormatDiagnostic(DiagText);
  std::string Text(DiagText.begin(), DiagText.end());
  EXPECT_EQ(ExpectText, Text);
}
}
