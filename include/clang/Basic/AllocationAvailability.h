//===--- AllocationAvailability.h - Allocation Availability -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Defines a functions that return the minimum OS versions supporting various
/// C++ allocation functions. Including C++17's aligned allocation and C++14's
/// sized deallocation.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_ALLOCATION_AVAILABILITY_H
#define LLVM_CLANG_BASIC_ALLOCATION_AVAILABILITY_H

#include "clang/Basic/VersionTuple.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/ErrorHandling.h"

namespace clang {

/// Returns true if the OS name should be used when issuing diagnostics about
/// unavailable allocation functions. Otherwise the STL name is used.
bool reportUnavailableUsingOsName(llvm::Triple::OSType OS);

VersionTuple alignedAllocMinVersion(llvm::Triple::OSType OS,
                                    bool IsLibcxx = false);

VersionTuple sizedDeallocMinVersion(llvm::Triple::OSType OS,
                                    bool IsLibcxx = false);

} // end namespace clang

#endif // LLVM_CLANG_BASIC_ALLOCATION_AVAILABILITY_H
