//===--- AllocationAvailability.cpp - Allocation Availability ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/AllocationAvailability.h"

namespace clang {

bool reportUnavailableUsingOsName(llvm::Triple::OSType OS) {
  switch (OS) {
  default:
    return false;
  case llvm::Triple::Darwin:
  case llvm::Triple::MacOSX:
  case llvm::Triple::IOS:
  case llvm::Triple::TvOS:
  case llvm::Triple::WatchOS:
    return true;
  }
}

VersionTuple alignedAllocMinVersion(llvm::Triple::OSType OS, bool IsLibcxx) {
  switch (OS) {
  default:
    if (IsLibcxx)
      return VersionTuple(4, 0, 0);
    return VersionTuple(7, 0, 0);
  case llvm::Triple::Darwin:
  case llvm::Triple::MacOSX: // Earliest supporting version is 10.13.
    return VersionTuple(10U, 13U);
  case llvm::Triple::IOS:
  case llvm::Triple::TvOS: // Earliest supporting version is 11.0.0.
    return VersionTuple(11U);
  case llvm::Triple::WatchOS: // Earliest supporting version is 4.0.0.
    return VersionTuple(4U);
  }

  llvm_unreachable("Unexpected OS");
}

VersionTuple sizedDeallocMinVersion(llvm::Triple::OSType OS, bool IsLibcxx) {
  switch (OS) {
  default:
    if (IsLibcxx)
      return VersionTuple(4, 0, 0);
    return VersionTuple(5, 0, 0);
  case llvm::Triple::Darwin:
  case llvm::Triple::MacOSX: // Earliest supporting version is 10.12.
    return VersionTuple(10U, 12U);
  case llvm::Triple::IOS:
  case llvm::Triple::TvOS: // Earliest supporting version is 11.0.0.
    return VersionTuple(10U);
  case llvm::Triple::WatchOS: // Earliest supporting version is 4.0.0.
    return VersionTuple(3U);
  }

  llvm_unreachable("Unexpected OS");
}

} // end namespace clang
