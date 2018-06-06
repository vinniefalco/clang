//===--- Contiki.h - Contiki ToolChain Implementations ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_CONTIKI_H
#define LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_CONTIKI_H

#include "Gnu.h"
#include "clang/Driver/ToolChain.h"

namespace clang {
namespace driver {
namespace toolchains {

class LLVM_LIBRARY_VISIBILITY Contiki : public Generic_ELF {
public:
  Contiki(const Driver &D, const llvm::Triple &Triple,
          const llvm::opt::ArgList &Args);

  // No support for finding a C++ standard library yet.
  path_list
  getLibCxxIncludePaths(const llvm::opt::ArgList &DriverArgs) const override {
    return {};
  }
  path_list getLibStdCxxIncludePaths(
      const llvm::opt::ArgList &DriverArgs) const override {
    return {};
  }

  SanitizerMask getSupportedSanitizers() const override;
};

} // end namespace toolchains
} // end namespace driver
} // end namespace clang

#endif // LLVM_CLANG_LIB_DRIVER_TOOLCHAINS_CONTIKI_H
