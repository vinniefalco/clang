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

#include "llvm/ADT/BitmaskEnum.h"
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

enum class ComparisonCategoryValue : unsigned char {
  Equal,
  Equivalent,
  Nonequivalent,
  Nonequal,
  Less,
  Greater,
  Unordered,
};

enum class ComparisonCategoryClassification : unsigned char {
  None = 0,
  Strong = 1 << 0,
  Ordered = 1 << 1,
  Partial = 1 << 2,
  LLVM_MARK_AS_BITMASK_ENUM(Partial)
};

LLVM_ENABLE_BITMASK_ENUMS_IN_NAMESPACE();

} // namespace clang

namespace llvm {

template <> struct DenseMapInfo<clang::ComparisonCategoryValue> {
private:
  using ValueType = clang::ComparisonCategoryValue;
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

  /// \brief The classification of the comparison category type, representing
  /// the values it contains.
  ComparisonCategoryClassification Classification;

public:
  const DeclRefExpr *getValue(ComparisonCategoryValue ValueKind) const {
    char Key = static_cast<char>(ValueKind);
    return Objects.lookup(Key);
  }
};

struct ComparisonCategories {
  using InfoList =
      std::array<ComparisonCategoryInfo,
                 static_cast<unsigned>(ComparisonCategoryKind::Last) + 1>;

  static ComparisonCategoryClassification
  classifyCategory(ComparisonCategoryKind Kind);

  static StringRef getCategoryString(ComparisonCategoryKind Kind);
  static StringRef getValueString(ComparisonCategoryValue Kind);

  bool hasData() const { return HasData; }

  const ComparisonCategoryInfo &getInfo(ComparisonCategoryKind Kind) const {
    assert(HasData && "comparison category information not yet built");
    return Data[static_cast<unsigned>(Kind)];
  }

  const RecordDecl *getDecl(ComparisonCategoryKind Kind) const {
    return getInfo(Kind).CCDecl;
  }

  Optional<ComparisonCategoryKind> getCategoryForType(QualType Ty) const;

  const DeclRefExpr *getValue(ComparisonCategoryKind Kind,
                              ComparisonCategoryValue ValKind) const;

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
