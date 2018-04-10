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

/// \brief An enumeration representing the different comparison categories.
///   The values map to the category types defined in the standard library.
///   i.e. 'std::weak_equality'.
enum class ComparisonCategoryKind : unsigned char {
  WeakEquality,
  StrongEquality,
  PartialOrdering,
  WeakOrdering,
  StrongOrdering,
  Last = StrongOrdering,
  First = WeakEquality
};

/// \brief An enumeration representing the possible results of a three-way
///   comparison. These values map onto instances of comparison category types
///   defined in the standard library. i.e. 'std::strong_ordering::less'.
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
  /// \brief Return an expression referencing the member of the specified
  ///   comparison category. For example 'std::strong_equality::equal'
  const DeclRefExpr *getResultValue(ComparisonCategoryResult ValueKind) const {
    const auto *DR = getResultValueUnsafe(ValueKind);
    assert(DR &&
           "comparison category does not contain the specified result kind");
    return DR;
  }

  const DeclRefExpr *
  getResultValueUnsafe(ComparisonCategoryResult ValueKind) const {
    assert(ValueKind != ComparisonCategoryResult::Invalid &&
           "invalid value specified");
    char Key = static_cast<char>(ValueKind);
    return Objects.lookup(Key);
  }

  /// \brief True iff the comparison category is an equality comparison.
  bool isEquality() const { return !isOrdered(); }

  /// \brief True iff the comparison category is a relational comparison.
  bool isOrdered() const {
    using CCK = ComparisonCategoryKind;
    return Kind == CCK::PartialOrdering || Kind == CCK::WeakOrdering ||
           Kind == CCK::StrongOrdering;
  }

  /// \brief True iff the comparison is "strong". i.e. it checks equality and
  ///    not equivalence.
  bool isStrong() const {
    using CCK = ComparisonCategoryKind;
    return Kind == CCK::StrongEquality || Kind == CCK::StrongOrdering;
  }

  /// \brief True iff the comparison is not totally ordered.
  bool isPartial() const {
    using CCK = ComparisonCategoryKind;
    return Kind == CCK::PartialOrdering;
  }

  /// \brief Converts the specified result kind into the the correct result kind
  ///    for this category. Specifically it lowers strong equality results to
  ///    weak equivalence if needed.
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
  static StringRef getCategoryString(ComparisonCategoryKind Kind);
  static StringRef getResultString(ComparisonCategoryResult Kind);

  /// \brief Return the comparison category information fo the category
  ///   specified by 'Kind'.
  const ComparisonCategoryInfo &getInfo(ComparisonCategoryKind Kind) const {
    assert(HasData && "comparison category information not yet built");
    return Data[static_cast<unsigned>(Kind)];
  }

  /// \brief Return the comparison category information as specified by
  ///   `getCategoryForType(Ty)`.
  const ComparisonCategoryInfo &getInfoForType(QualType Ty) const;

  /// \brief Return the comparison category kind coorisponding to the specified
  ///   type. 'Ty' is expected to refer to the type of one of the comparison
  ///   category decls; if it doesn't no value is returned.
  Optional<ComparisonCategoryKind> getCategoryForType(QualType Ty) const;

  /// \brief returns true if the comparison category data has already been
  /// built,
  ///    and false otherwise.
  bool hasData() const { return HasData; }

public: // private
  using InfoList =
      std::array<ComparisonCategoryInfo,
                 static_cast<unsigned>(ComparisonCategoryKind::Last) + 1>;

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
