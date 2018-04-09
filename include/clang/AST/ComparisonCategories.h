#ifndef LLVM_CLANG_AST_COMPARISONCATEGORIES_H
#define LLVM_CLANG_AST_COMPARISONCATEGORIES_H

#include "llvm/ADT/BitmaskEnum.h"
#include "llvm/ADT/DenseMap.h"
#include <array>
#include <type_traits>

namespace clang {

class DeclRefExpr;
class RecordDecl;

enum class ComparisonCategoryValue : unsigned char;

} // namespace clang

namespace llvm {
template <> struct DenseMapInfo<clang::ComparisonCategoryValue>;
} // namespace llvm

namespace clang {

enum class ComparisonCategoryKind : unsigned char {
  WeakEquality,
  StrongEquality,
  PartialOrdering,
  WeakOrdering,
  StrongOrdering,
  Last = StrongOrdering,
  First = WeakEquality,
  Size = Last + 1
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

struct ComparisonCategoryInfo {
  const char *Name = nullptr;

  /// \brief The declaration for the comparison category type from the
  /// standard library.
  RecordDecl *CCDecl = nullptr;

  ComparisonCategoryKind Kind;
  ComparisonCategoryClassification Classification;

  /// \brief A map containing the comparison category values built from the
  /// standard library. The key is a value of ComparisonCategoryValue.
  llvm::DenseMap<char, DeclRefExpr *> Objects;
};

} // namespace clang

namespace llvm {

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

#endif
