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

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/DenseMap.h"
#include <array>
#include <cassert>
#include <type_traits>
#include <utility>

namespace llvm {
  class StringRef;
}

namespace clang {

class ASTContext;
class VarDecl;
class RecordDecl;
class QualType;
class NamespaceDecl;

/// \brief An enumeration representing the different comparison categories
/// types.
///
/// C++2a [cmp.categories.pre] The types weak_equality, strong_equality,
/// partial_ordering, weak_ordering, and strong_ordering are collectively
/// termed the comparison category types.
enum class ComparisonCategoryType : unsigned char {
  WeakEquality,
  StrongEquality,
  PartialOrdering,
  WeakOrdering,
  StrongOrdering,
  First = WeakEquality,
  Last = StrongOrdering
};

/// \brief An enumeration representing the possible results of a three-way
///   comparison. These values map onto instances of comparison category types
///   defined in the standard library. i.e. 'std::strong_ordering::less'.
enum class ComparisonCategoryResult : unsigned char {
  Equal,
  Equivalent,
  Nonequivalent,
  Nonequal,
  Less,
  Greater,
  Unordered,
};

class ComparisonCategoryInfo {
  friend class ComparisonCategories;

  const ASTContext &Ctx;

  /// \brief A map containing the comparison category values built from the
  /// standard library. The key is a value of ComparisonCategoryResult.
  mutable llvm::DenseMap<char, VarDecl *> Objects;

  ComparisonCategoryInfo(const ASTContext &Ctx) : Ctx(Ctx) {}

public:
  /// \brief Wether Sema has fully checked the type and result values for this
  ///   comparison category types before.
  bool IsFullyChecked = false;

public:
  /// \brief The declaration for the comparison category type from the
  /// standard library.
  // FIXME: Make this const
  RecordDecl *CCDecl = nullptr;

  /// \brief The Kind of the comparison category type
  ComparisonCategoryType Kind;

public:
  /// \brief Return an expression referencing the member of the specified
  ///   comparison category. For example 'std::strong_equality::equal'
  const VarDecl *getResultValue(ComparisonCategoryResult ValueKind) const {
    const VarDecl *VD = lookupResultValue(ValueKind);
    assert(VD &&
           "comparison category does not contain the specified result kind");
    return VD;
  }

  const VarDecl *lookupResultValue(ComparisonCategoryResult ValueKind) const;

  VarDecl *lookupResultValue(ComparisonCategoryResult ValueKind) {
    const auto &This = *this;
    return const_cast<VarDecl *>(This.lookupResultValue(ValueKind));
  }

  /// \brief True iff the comparison category is an equality comparison.
  bool isEquality() const { return !isOrdered(); }

  /// \brief True iff the comparison category is a relational comparison.
  bool isOrdered() const {
    using CCK = ComparisonCategoryType;
    return Kind == CCK::PartialOrdering || Kind == CCK::WeakOrdering ||
           Kind == CCK::StrongOrdering;
  }

  /// \brief True iff the comparison is "strong". i.e. it checks equality and
  ///    not equivalence.
  bool isStrong() const {
    using CCK = ComparisonCategoryType;
    return Kind == CCK::StrongEquality || Kind == CCK::StrongOrdering;
  }

  /// \brief True iff the comparison is not totally ordered.
  bool isPartial() const {
    using CCK = ComparisonCategoryType;
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

  const VarDecl *getEqualOrEquiv() const {
    return getResultValue(makeWeakResult(ComparisonCategoryResult::Equal));
  }
  const VarDecl *getNonequalOrNonequiv() const {
    return getResultValue(makeWeakResult(ComparisonCategoryResult::Nonequal));
  }
  const VarDecl *getLess() const {
    assert(isOrdered());
    return getResultValue(ComparisonCategoryResult::Less);
  }
  const VarDecl *getGreater() const {
    assert(isOrdered());
    return getResultValue(ComparisonCategoryResult::Greater);
  }
  const VarDecl *getUnordered() const {
    assert(isPartial());
    return getResultValue(ComparisonCategoryResult::Unordered);
  }
};

class ComparisonCategories {
public:
  static StringRef getCategoryString(ComparisonCategoryType Kind);
  static StringRef getResultString(ComparisonCategoryResult Kind);

  /// \brief Return the comparison category information for the category
  ///   specified by 'Kind'.
  const ComparisonCategoryInfo &getInfo(ComparisonCategoryType Kind) const {
    const ComparisonCategoryInfo *Result = lookupInfo(Kind);
    assert(Result != nullptr &&
           "information for specified comparison category has not been built");
    return *Result;
  }

  /// \brief Return the comparison category information as specified by
  ///   `getCategoryForType(Ty)`.
  ///
  /// Note: The comparison category type must have already been built by Sema.
  const ComparisonCategoryInfo &getInfoForType(QualType Ty) const;

public:
  /// \brief Return the comparison category information for the category
  ///   specified by 'Kind', or nullptr if it isn't available.
  const ComparisonCategoryInfo *lookupInfo(ComparisonCategoryType Kind) const;

  ComparisonCategoryInfo *lookupInfo(ComparisonCategoryType Kind) {
    const auto &This = *this;
    return const_cast<ComparisonCategoryInfo *>(This.lookupInfo(Kind));
  }

private:
  const ComparisonCategoryInfo *lookupInfoForType(QualType Ty) const;
  NamespaceDecl *lookupStdNamespace() const;

private:
  friend class ASTContext;

  explicit ComparisonCategories(const ASTContext &Ctx) : Ctx(Ctx) {}

  const ASTContext &Ctx;
  mutable llvm::DenseMap<char, ComparisonCategoryInfo> Data;
  mutable NamespaceDecl *StdNS = nullptr;
};

} // namespace clang

#endif
