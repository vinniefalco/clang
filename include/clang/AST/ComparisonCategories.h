//===- ComparisonCategories.h - Three Way Comparison Data -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Comparison Category enum and data types, which
//  store the types and expressions needed to support operator<=>
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_COMPARISONCATEGORIES_H
#define LLVM_CLANG_AST_COMPARISONCATEGORIES_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Optional.h"
#include "clang/Basic/LLVM.h"
#include <array>
#include <cassert>
#include <type_traits>

namespace llvm {
  class StringRef;
}

namespace clang {

class DeclRefExpr;
class RecordDecl;
class QualType;

enum class ComparisonCategoryKind : unsigned char {
  WeakEquality,
  StrongEquality,
  PartialOrdering,
  WeakOrdering,
  StrongOrdering,
  Last = StrongOrdering,
  First = WeakEquality
};

enum class ComparisonCategoryResult : unsigned char {
  Invalid, // Internal implementation detail
  Equal,
  Equivalent,
  Nonequivalent,
  Nonequal,
  Less,
  Greater,
  Unordered,
};

} // namespace clang

namespace llvm {

template <> struct DenseMapInfo<clang::ComparisonCategoryResult> {
private:
  using ValueType = clang::ComparisonCategoryResult;
  using UnderlyingT = std::underlying_type<ValueType>::type;
  static ValueType Cast(UnderlyingT Val) { return static_cast<ValueType>(Val); }

public:
  static inline ValueType getEmptyKey() { return Cast(0xFF); }
  static inline ValueType getTombstoneKey() { return Cast(0xFF - 1); }

  static unsigned getHashValue(const ValueType &Val) {
    return static_cast<UnderlyingT>(Val) * 37U;
  }

  static bool isEqual(const ValueType &LHS, const ValueType &RHS) {
    return LHS == RHS;
  }
};

} // namespace llvm

namespace clang {

struct ComparisonCategoryInfo {
  /// \brief The declaration for the comparison category type from the
  /// standard library.
  RecordDecl *CCDecl = nullptr;

  /// \brief A map containing the comparison category values built from the
  /// standard library. The key is a value of ComparisonCategoryValue.
  llvm::DenseMap<char, DeclRefExpr *> Objects;

  /// \brief The Kind of the comparison category type
  ComparisonCategoryKind Kind;

public:
  const DeclRefExpr *getResultValue(ComparisonCategoryResult ValueKind) const {
    char Key = static_cast<char>(ValueKind);
    return Objects.lookup(Key);
  }
  bool isEquality() const { return !isOrdered(); }
  bool isOrdered() const {
    using CCK = ComparisonCategoryKind;
    return Kind == CCK::PartialOrdering || Kind == CCK::WeakOrdering ||
           Kind == CCK::StrongOrdering;
  }
  bool isStrong() const {
    using CCK = ComparisonCategoryKind;
    return Kind == CCK::StrongEquality || Kind == CCK::StrongOrdering;
  }
  bool isPartial() const {
    using CCK = ComparisonCategoryKind;
    return Kind == CCK::PartialOrdering;
  }

  ComparisonCategoryResult makeWeakResult(ComparisonCategoryResult Res) const {
    using CCR = ComparisonCategoryResult;
    if (!isStrong()) {
      if (Res == CCR::Equal)
        return CCR::Equivalent;
      if (Res == CCR::Nonequal)
        return CCR::Nonequivalent;
    }
    return Res;
  }

  const DeclRefExpr *getEqualOrEquiv() const {
    return getResultValue(makeWeakResult(ComparisonCategoryResult::Equal));
  }
  const DeclRefExpr *getNonequalOrNonequiv() const {
    return getResultValue(makeWeakResult(ComparisonCategoryResult::Nonequal));
  }
  const DeclRefExpr *getLess() const {
    assert(isOrdered());
    return getResultValue(ComparisonCategoryResult::Less);
  }
  const DeclRefExpr *getGreater() const {
    assert(isOrdered());
    return getResultValue(ComparisonCategoryResult::Greater);
  }
  const DeclRefExpr *getUnordered() const {
    assert(isPartial());
    return getResultValue(ComparisonCategoryResult::Unordered);
  }
};

struct ComparisonCategories {
  using InfoList =
      std::array<ComparisonCategoryInfo,
                 static_cast<unsigned>(ComparisonCategoryKind::Last) + 1>;

  static StringRef getCategoryString(ComparisonCategoryKind Kind);
  static StringRef getResultString(ComparisonCategoryResult Kind);

  bool hasData() const { return HasData; }

  const ComparisonCategoryInfo &getInfo(ComparisonCategoryKind Kind) const {
    assert(HasData && "comparison category information not yet built");
    return Data[static_cast<unsigned>(Kind)];
  }
  const ComparisonCategoryInfo &getInfoForType(QualType Ty) const;

  const RecordDecl *getDecl(ComparisonCategoryKind Kind) const {
    return getInfo(Kind).CCDecl;
  }

  Optional<ComparisonCategoryKind> getCategoryForType(QualType Ty) const;

  const DeclRefExpr *getResultValue(ComparisonCategoryKind Kind,
                                    ComparisonCategoryResult ValKind) const;

  void setData(InfoList &&NewData) {
    assert(!HasData && "comparison categories already built");
    Data = std::move(NewData);
    HasData = true;
  }

private:
  bool HasData = false;
  InfoList Data;
};

} // namespace clang

#endif
