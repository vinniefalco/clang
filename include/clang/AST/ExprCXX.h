//===- ExprCXX.h - Classes for representing expressions ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief Defines the clang::Expr interface and subclasses for C++ expressions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_EXPRCXX_H
#define LLVM_CLANG_AST_EXPRCXX_H

#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/Expr.h"
#include "clang/AST/NestedNameSpecifier.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/TemplateBase.h"
#include "clang/AST/Type.h"
#include "clang/AST/UnresolvedSet.h"
#include "clang/Basic/ExceptionSpecificationType.h"
#include "clang/Basic/ExpressionTraits.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/Lambda.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/OperatorKinds.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/Specifiers.h"
#include "clang/Basic/TypeTraits.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/TrailingObjects.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace clang {

class ASTContext;
class DeclAccessPair;
class IdentifierInfo;
class LambdaCapture;
class NonTypeTemplateParmDecl;
class TemplateParameterList;

//===--------------------------------------------------------------------===//
// C++ Expressions.
//===--------------------------------------------------------------------===//

/// \brief A call to an overloaded operator written using operator
/// syntax.
///
/// Represents a call to an overloaded operator written using operator
/// syntax, e.g., "x + y" or "*p". While semantically equivalent to a
/// normal call, this AST node provides better information about the
/// syntactic representation of the call.
///
/// In a C++ template, this expression node kind will be used whenever
/// any of the arguments are type-dependent. In this case, the
/// function itself will be a (possibly empty) set of functions and
/// function templates that were found by name lookup at template
/// definition time.
class CXXOperatorCallExpr : public CallExpr {
  /// \brief The overloaded operator.
  OverloadedOperatorKind Operator;

  SourceRange Range;

  // Only meaningful for floating point types.
  FPOptions FPFeatures;

  SourceRange getSourceRangeImpl() const LLVM_READONLY;

public:
  friend class ASTReader;
  friend class ASTWriter;
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  CXXOperatorCallExpr(ASTContext& C, OverloadedOperatorKind Op, Expr *fn,
                      ArrayRef<Expr*> args, QualType t, ExprValueKind VK,
                      SourceLocation operatorloc, FPOptions FPFeatures)
      : CallExpr(C, CXXOperatorCallExprClass, fn, args, t, VK, operatorloc),
        Operator(Op), FPFeatures(FPFeatures) {
    Range = getSourceRangeImpl();
  }

  explicit CXXOperatorCallExpr(ASTContext& C, EmptyShell Empty)
      : CallExpr(C, CXXOperatorCallExprClass, Empty) {}

  /// \brief Returns the kind of overloaded operator that this
  /// expression refers to.
  OverloadedOperatorKind getOperator() const { return Operator; }

  static bool isAssignmentOp(OverloadedOperatorKind Opc) {
    return Opc == OO_Equal || Opc == OO_StarEqual ||
           Opc == OO_SlashEqual || Opc == OO_PercentEqual ||
           Opc == OO_PlusEqual || Opc == OO_MinusEqual ||
           Opc == OO_LessLessEqual || Opc == OO_GreaterGreaterEqual ||
           Opc == OO_AmpEqual || Opc == OO_CaretEqual ||
           Opc == OO_PipeEqual;
  }
  bool isAssignmentOp() const { return isAssignmentOp(getOperator()); }

  /// \brief Is this written as an infix binary operator?
  bool isInfixBinaryOp() const;

  /// \brief Returns the location of the operator symbol in the expression.
  ///
  /// When \c getOperator()==OO_Call, this is the location of the right
  /// parentheses; when \c getOperator()==OO_Subscript, this is the location
  /// of the right bracket.
  SourceLocation getOperatorLoc() const { return getRParenLoc(); }

  SourceLocation getExprLoc() const LLVM_READONLY {
    return (Operator < OO_Plus || Operator >= OO_Arrow ||
            Operator == OO_PlusPlus || Operator == OO_MinusMinus)
               ? getLocStart()
               : getOperatorLoc();
  }

  SourceLocation getLocStart() const LLVM_READONLY { return Range.getBegin(); }
  SourceLocation getLocEnd() const LLVM_READONLY { return Range.getEnd(); }
  SourceRange getSourceRange() const { return Range; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXOperatorCallExprClass;
  }

  // Set the FP contractability status of this operator. Only meaningful for
  // operations on floating point types.
  void setFPFeatures(FPOptions F) { FPFeatures = F; }

  FPOptions getFPFeatures() const { return FPFeatures; }

  // Get the FP contractability status of this operator. Only meaningful for
  // operations on floating point types.
  bool isFPContractableWithinStatement() const {
    return FPFeatures.allowFPContractWithinStatement();
  }
};

/// Represents a call to a member function that
/// may be written either with member call syntax (e.g., "obj.func()"
/// or "objptr->func()") or with normal function-call syntax
/// ("func()") within a member function that ends up calling a member
/// function. The callee in either case is a MemberExpr that contains
/// both the object argument and the member function, while the
/// arguments are the arguments within the parentheses (not including
/// the object argument).
class CXXMemberCallExpr : public CallExpr {
public:
  CXXMemberCallExpr(ASTContext &C, Expr *fn, ArrayRef<Expr*> args,
                    QualType t, ExprValueKind VK, SourceLocation RP)
      : CallExpr(C, CXXMemberCallExprClass, fn, args, t, VK, RP) {}

  CXXMemberCallExpr(ASTContext &C, EmptyShell Empty)
      : CallExpr(C, CXXMemberCallExprClass, Empty) {}

  /// \brief Retrieves the implicit object argument for the member call.
  ///
  /// For example, in "x.f(5)", this returns the sub-expression "x".
  Expr *getImplicitObjectArgument() const;

  /// \brief Retrieves the declaration of the called method.
  CXXMethodDecl *getMethodDecl() const;

  /// \brief Retrieves the CXXRecordDecl for the underlying type of
  /// the implicit object argument.
  ///
  /// Note that this is may not be the same declaration as that of the class
  /// context of the CXXMethodDecl which this function is calling.
  /// FIXME: Returns 0 for member pointer call exprs.
  CXXRecordDecl *getRecordDecl() const;

  SourceLocation getExprLoc() const LLVM_READONLY {
    SourceLocation CLoc = getCallee()->getExprLoc();
    if (CLoc.isValid())
      return CLoc;

    return getLocStart();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXMemberCallExprClass;
  }
};

/// \brief Represents a call to a CUDA kernel function.
class CUDAKernelCallExpr : public CallExpr {
private:
  enum { CONFIG, END_PREARG };

public:
  CUDAKernelCallExpr(ASTContext &C, Expr *fn, CallExpr *Config,
                     ArrayRef<Expr*> args, QualType t, ExprValueKind VK,
                     SourceLocation RP)
      : CallExpr(C, CUDAKernelCallExprClass, fn, Config, args, t, VK, RP) {}

  CUDAKernelCallExpr(ASTContext &C, EmptyShell Empty)
      : CallExpr(C, CUDAKernelCallExprClass, END_PREARG, Empty) {}

  const CallExpr *getConfig() const {
    return cast_or_null<CallExpr>(getPreArg(CONFIG));
  }
  CallExpr *getConfig() { return cast_or_null<CallExpr>(getPreArg(CONFIG)); }

  /// \brief Sets the kernel configuration expression.
  ///
  /// Note that this method cannot be called if config has already been set to a
  /// non-null value.
  void setConfig(CallExpr *E) {
    assert(!getConfig() &&
           "Cannot call setConfig if config is not null");
    setPreArg(CONFIG, E);
    setInstantiationDependent(isInstantiationDependent() ||
                              E->isInstantiationDependent());
    setContainsUnexpandedParameterPack(containsUnexpandedParameterPack() ||
                                       E->containsUnexpandedParameterPack());
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CUDAKernelCallExprClass;
  }
};

/// \brief Abstract class common to all of the C++ "named"/"keyword" casts.
///
/// This abstract class is inherited by all of the classes
/// representing "named" casts: CXXStaticCastExpr for \c static_cast,
/// CXXDynamicCastExpr for \c dynamic_cast, CXXReinterpretCastExpr for
/// reinterpret_cast, and CXXConstCastExpr for \c const_cast.
class CXXNamedCastExpr : public ExplicitCastExpr {
private:
  // the location of the casting op
  SourceLocation Loc;

  // the location of the right parenthesis
  SourceLocation RParenLoc;

  // range for '<' '>'
  SourceRange AngleBrackets;

protected:
  friend class ASTStmtReader;

  CXXNamedCastExpr(StmtClass SC, QualType ty, ExprValueKind VK,
                   CastKind kind, Expr *op, unsigned PathSize,
                   TypeSourceInfo *writtenTy, SourceLocation l,
                   SourceLocation RParenLoc,
                   SourceRange AngleBrackets)
      : ExplicitCastExpr(SC, ty, VK, kind, op, PathSize, writtenTy), Loc(l),
        RParenLoc(RParenLoc), AngleBrackets(AngleBrackets) {}

  explicit CXXNamedCastExpr(StmtClass SC, EmptyShell Shell, unsigned PathSize)
      : ExplicitCastExpr(SC, Shell, PathSize) {}

public:
  const char *getCastName() const;

  /// \brief Retrieve the location of the cast operator keyword, e.g.,
  /// \c static_cast.
  SourceLocation getOperatorLoc() const { return Loc; }

  /// \brief Retrieve the location of the closing parenthesis.
  SourceLocation getRParenLoc() const { return RParenLoc; }

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return RParenLoc; }
  SourceRange getAngleBrackets() const LLVM_READONLY { return AngleBrackets; }

  static bool classof(const Stmt *T) {
    switch (T->getStmtClass()) {
    case CXXStaticCastExprClass:
    case CXXDynamicCastExprClass:
    case CXXReinterpretCastExprClass:
    case CXXConstCastExprClass:
      return true;
    default:
      return false;
    }
  }
};

/// \brief A C++ \c static_cast expression (C++ [expr.static.cast]).
///
/// This expression node represents a C++ static cast, e.g.,
/// \c static_cast<int>(1.0).
class CXXStaticCastExpr final
    : public CXXNamedCastExpr,
      private llvm::TrailingObjects<CXXStaticCastExpr, CXXBaseSpecifier *> {
  CXXStaticCastExpr(QualType ty, ExprValueKind vk, CastKind kind, Expr *op,
                    unsigned pathSize, TypeSourceInfo *writtenTy,
                    SourceLocation l, SourceLocation RParenLoc,
                    SourceRange AngleBrackets)
      : CXXNamedCastExpr(CXXStaticCastExprClass, ty, vk, kind, op, pathSize,
                         writtenTy, l, RParenLoc, AngleBrackets) {}

  explicit CXXStaticCastExpr(EmptyShell Empty, unsigned PathSize)
      : CXXNamedCastExpr(CXXStaticCastExprClass, Empty, PathSize) {}

public:
  friend class CastExpr;
  friend TrailingObjects;

  static CXXStaticCastExpr *Create(const ASTContext &Context, QualType T,
                                   ExprValueKind VK, CastKind K, Expr *Op,
                                   const CXXCastPath *Path,
                                   TypeSourceInfo *Written, SourceLocation L,
                                   SourceLocation RParenLoc,
                                   SourceRange AngleBrackets);
  static CXXStaticCastExpr *CreateEmpty(const ASTContext &Context,
                                        unsigned PathSize);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXStaticCastExprClass;
  }
};

/// \brief A C++ @c dynamic_cast expression (C++ [expr.dynamic.cast]).
///
/// This expression node represents a dynamic cast, e.g.,
/// \c dynamic_cast<Derived*>(BasePtr). Such a cast may perform a run-time
/// check to determine how to perform the type conversion.
class CXXDynamicCastExpr final
    : public CXXNamedCastExpr,
      private llvm::TrailingObjects<CXXDynamicCastExpr, CXXBaseSpecifier *> {
  CXXDynamicCastExpr(QualType ty, ExprValueKind VK, CastKind kind,
                     Expr *op, unsigned pathSize, TypeSourceInfo *writtenTy,
                     SourceLocation l, SourceLocation RParenLoc,
                     SourceRange AngleBrackets)
      : CXXNamedCastExpr(CXXDynamicCastExprClass, ty, VK, kind, op, pathSize,
                         writtenTy, l, RParenLoc, AngleBrackets) {}

  explicit CXXDynamicCastExpr(EmptyShell Empty, unsigned pathSize)
      : CXXNamedCastExpr(CXXDynamicCastExprClass, Empty, pathSize) {}

public:
  friend class CastExpr;
  friend TrailingObjects;

  static CXXDynamicCastExpr *Create(const ASTContext &Context, QualType T,
                                    ExprValueKind VK, CastKind Kind, Expr *Op,
                                    const CXXCastPath *Path,
                                    TypeSourceInfo *Written, SourceLocation L,
                                    SourceLocation RParenLoc,
                                    SourceRange AngleBrackets);

  static CXXDynamicCastExpr *CreateEmpty(const ASTContext &Context,
                                         unsigned pathSize);

  bool isAlwaysNull() const;

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXDynamicCastExprClass;
  }
};

/// \brief A C++ @c reinterpret_cast expression (C++ [expr.reinterpret.cast]).
///
/// This expression node represents a reinterpret cast, e.g.,
/// @c reinterpret_cast<int>(VoidPtr).
///
/// A reinterpret_cast provides a differently-typed view of a value but
/// (in Clang, as in most C++ implementations) performs no actual work at
/// run time.
class CXXReinterpretCastExpr final
    : public CXXNamedCastExpr,
      private llvm::TrailingObjects<CXXReinterpretCastExpr,
                                    CXXBaseSpecifier *> {
  CXXReinterpretCastExpr(QualType ty, ExprValueKind vk, CastKind kind,
                         Expr *op, unsigned pathSize,
                         TypeSourceInfo *writtenTy, SourceLocation l,
                         SourceLocation RParenLoc,
                         SourceRange AngleBrackets)
      : CXXNamedCastExpr(CXXReinterpretCastExprClass, ty, vk, kind, op,
                         pathSize, writtenTy, l, RParenLoc, AngleBrackets) {}

  CXXReinterpretCastExpr(EmptyShell Empty, unsigned pathSize)
      : CXXNamedCastExpr(CXXReinterpretCastExprClass, Empty, pathSize) {}

public:
  friend class CastExpr;
  friend TrailingObjects;

  static CXXReinterpretCastExpr *Create(const ASTContext &Context, QualType T,
                                        ExprValueKind VK, CastKind Kind,
                                        Expr *Op, const CXXCastPath *Path,
                                 TypeSourceInfo *WrittenTy, SourceLocation L,
                                        SourceLocation RParenLoc,
                                        SourceRange AngleBrackets);
  static CXXReinterpretCastExpr *CreateEmpty(const ASTContext &Context,
                                             unsigned pathSize);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXReinterpretCastExprClass;
  }
};

/// \brief A C++ \c const_cast expression (C++ [expr.const.cast]).
///
/// This expression node represents a const cast, e.g.,
/// \c const_cast<char*>(PtrToConstChar).
///
/// A const_cast can remove type qualifiers but does not change the underlying
/// value.
class CXXConstCastExpr final
    : public CXXNamedCastExpr,
      private llvm::TrailingObjects<CXXConstCastExpr, CXXBaseSpecifier *> {
  CXXConstCastExpr(QualType ty, ExprValueKind VK, Expr *op,
                   TypeSourceInfo *writtenTy, SourceLocation l,
                   SourceLocation RParenLoc, SourceRange AngleBrackets)
      : CXXNamedCastExpr(CXXConstCastExprClass, ty, VK, CK_NoOp, op,
                         0, writtenTy, l, RParenLoc, AngleBrackets) {}

  explicit CXXConstCastExpr(EmptyShell Empty)
      : CXXNamedCastExpr(CXXConstCastExprClass, Empty, 0) {}

public:
  friend class CastExpr;
  friend TrailingObjects;

  static CXXConstCastExpr *Create(const ASTContext &Context, QualType T,
                                  ExprValueKind VK, Expr *Op,
                                  TypeSourceInfo *WrittenTy, SourceLocation L,
                                  SourceLocation RParenLoc,
                                  SourceRange AngleBrackets);
  static CXXConstCastExpr *CreateEmpty(const ASTContext &Context);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXConstCastExprClass;
  }
};

/// \brief A call to a literal operator (C++11 [over.literal])
/// written as a user-defined literal (C++11 [lit.ext]).
///
/// Represents a user-defined literal, e.g. "foo"_bar or 1.23_xyz. While this
/// is semantically equivalent to a normal call, this AST node provides better
/// information about the syntactic representation of the literal.
///
/// Since literal operators are never found by ADL and can only be declared at
/// namespace scope, a user-defined literal is never dependent.
class UserDefinedLiteral : public CallExpr {
  /// \brief The location of a ud-suffix within the literal.
  SourceLocation UDSuffixLoc;

public:
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  UserDefinedLiteral(const ASTContext &C, Expr *Fn, ArrayRef<Expr*> Args,
                     QualType T, ExprValueKind VK, SourceLocation LitEndLoc,
                     SourceLocation SuffixLoc)
      : CallExpr(C, UserDefinedLiteralClass, Fn, Args, T, VK, LitEndLoc),
        UDSuffixLoc(SuffixLoc) {}

  explicit UserDefinedLiteral(const ASTContext &C, EmptyShell Empty)
      : CallExpr(C, UserDefinedLiteralClass, Empty) {}

  /// The kind of literal operator which is invoked.
  enum LiteralOperatorKind {
    /// Raw form: operator "" X (const char *)
    LOK_Raw,

    /// Raw form: operator "" X<cs...> ()
    LOK_Template,

    /// operator "" X (unsigned long long)
    LOK_Integer,

    /// operator "" X (long double)
    LOK_Floating,

    /// operator "" X (const CharT *, size_t)
    LOK_String,

    /// operator "" X (CharT)
    LOK_Character
  };

  /// \brief Returns the kind of literal operator invocation
  /// which this expression represents.
  LiteralOperatorKind getLiteralOperatorKind() const;

  /// \brief If this is not a raw user-defined literal, get the
  /// underlying cooked literal (representing the literal with the suffix
  /// removed).
  Expr *getCookedLiteral();
  const Expr *getCookedLiteral() const {
    return const_cast<UserDefinedLiteral*>(this)->getCookedLiteral();
  }

  SourceLocation getLocStart() const {
    if (getLiteralOperatorKind() == LOK_Template)
      return getRParenLoc();
    return getArg(0)->getLocStart();
  }

  SourceLocation getLocEnd() const { return getRParenLoc(); }

  /// \brief Returns the location of a ud-suffix in the expression.
  ///
  /// For a string literal, there may be multiple identical suffixes. This
  /// returns the first.
  SourceLocation getUDSuffixLoc() const { return UDSuffixLoc; }

  /// \brief Returns the ud-suffix specified for this literal.
  const IdentifierInfo *getUDSuffix() const;

  static bool classof(const Stmt *S) {
    return S->getStmtClass() == UserDefinedLiteralClass;
  }
};

/// \brief A boolean literal, per ([C++ lex.bool] Boolean literals).
class CXXBoolLiteralExpr : public Expr {
  bool Value;
  SourceLocation Loc;

public:
  CXXBoolLiteralExpr(bool val, QualType Ty, SourceLocation l)
      : Expr(CXXBoolLiteralExprClass, Ty, VK_RValue, OK_Ordinary, false, false,
             false, false),
        Value(val), Loc(l) {}

  explicit CXXBoolLiteralExpr(EmptyShell Empty)
      : Expr(CXXBoolLiteralExprClass, Empty) {}

  bool getValue() const { return Value; }
  void setValue(bool V) { Value = V; }

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return Loc; }

  SourceLocation getLocation() const { return Loc; }
  void setLocation(SourceLocation L) { Loc = L; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXBoolLiteralExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief The null pointer literal (C++11 [lex.nullptr])
///
/// Introduced in C++11, the only literal of type \c nullptr_t is \c nullptr.
class CXXNullPtrLiteralExpr : public Expr {
  SourceLocation Loc;

public:
  CXXNullPtrLiteralExpr(QualType Ty, SourceLocation l)
      : Expr(CXXNullPtrLiteralExprClass, Ty, VK_RValue, OK_Ordinary, false,
             false, false, false),
        Loc(l) {}

  explicit CXXNullPtrLiteralExpr(EmptyShell Empty)
      : Expr(CXXNullPtrLiteralExprClass, Empty) {}

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return Loc; }

  SourceLocation getLocation() const { return Loc; }
  void setLocation(SourceLocation L) { Loc = L; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXNullPtrLiteralExprClass;
  }

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief Implicit construction of a std::initializer_list<T> object from an
/// array temporary within list-initialization (C++11 [dcl.init.list]p5).
class CXXStdInitializerListExpr : public Expr {
  Stmt *SubExpr = nullptr;

  CXXStdInitializerListExpr(EmptyShell Empty)
      : Expr(CXXStdInitializerListExprClass, Empty) {}

public:
  friend class ASTReader;
  friend class ASTStmtReader;

  CXXStdInitializerListExpr(QualType Ty, Expr *SubExpr)
      : Expr(CXXStdInitializerListExprClass, Ty, VK_RValue, OK_Ordinary,
             Ty->isDependentType(), SubExpr->isValueDependent(),
             SubExpr->isInstantiationDependent(),
             SubExpr->containsUnexpandedParameterPack()),
        SubExpr(SubExpr) {}

  Expr *getSubExpr() { return static_cast<Expr*>(SubExpr); }
  const Expr *getSubExpr() const { return static_cast<const Expr*>(SubExpr); }

  SourceLocation getLocStart() const LLVM_READONLY {
    return SubExpr->getLocStart();
  }

  SourceLocation getLocEnd() const LLVM_READONLY {
    return SubExpr->getLocEnd();
  }

  SourceRange getSourceRange() const LLVM_READONLY {
    return SubExpr->getSourceRange();
  }

  static bool classof(const Stmt *S) {
    return S->getStmtClass() == CXXStdInitializerListExprClass;
  }

  child_range children() { return child_range(&SubExpr, &SubExpr + 1); }
};

/// A C++ \c typeid expression (C++ [expr.typeid]), which gets
/// the \c type_info that corresponds to the supplied type, or the (possibly
/// dynamic) type of the supplied expression.
///
/// This represents code like \c typeid(int) or \c typeid(*objPtr)
class CXXTypeidExpr : public Expr {
private:
  llvm::PointerUnion<Stmt *, TypeSourceInfo *> Operand;
  SourceRange Range;

public:
  CXXTypeidExpr(QualType Ty, TypeSourceInfo *Operand, SourceRange R)
      : Expr(CXXTypeidExprClass, Ty, VK_LValue, OK_Ordinary,
             // typeid is never type-dependent (C++ [temp.dep.expr]p4)
             false,
             // typeid is value-dependent if the type or expression are
             // dependent
             Operand->getType()->isDependentType(),
             Operand->getType()->isInstantiationDependentType(),
             Operand->getType()->containsUnexpandedParameterPack()),
        Operand(Operand), Range(R) {}

  CXXTypeidExpr(QualType Ty, Expr *Operand, SourceRange R)
      : Expr(CXXTypeidExprClass, Ty, VK_LValue, OK_Ordinary,
             // typeid is never type-dependent (C++ [temp.dep.expr]p4)
             false,
             // typeid is value-dependent if the type or expression are
             // dependent
             Operand->isTypeDependent() || Operand->isValueDependent(),
             Operand->isInstantiationDependent(),
             Operand->containsUnexpandedParameterPack()),
        Operand(Operand), Range(R) {}

  CXXTypeidExpr(EmptyShell Empty, bool isExpr)
      : Expr(CXXTypeidExprClass, Empty) {
    if (isExpr)
      Operand = (Expr*)nullptr;
    else
      Operand = (TypeSourceInfo*)nullptr;
  }

  /// Determine whether this typeid has a type operand which is potentially
  /// evaluated, per C++11 [expr.typeid]p3.
  bool isPotentiallyEvaluated() const;

  bool isTypeOperand() const { return Operand.is<TypeSourceInfo *>(); }

  /// \brief Retrieves the type operand of this typeid() expression after
  /// various required adjustments (removing reference types, cv-qualifiers).
  QualType getTypeOperand(ASTContext &Context) const;

  /// \brief Retrieve source information for the type operand.
  TypeSourceInfo *getTypeOperandSourceInfo() const {
    assert(isTypeOperand() && "Cannot call getTypeOperand for typeid(expr)");
    return Operand.get<TypeSourceInfo *>();
  }

  void setTypeOperandSourceInfo(TypeSourceInfo *TSI) {
    assert(isTypeOperand() && "Cannot call getTypeOperand for typeid(expr)");
    Operand = TSI;
  }

  Expr *getExprOperand() const {
    assert(!isTypeOperand() && "Cannot call getExprOperand for typeid(type)");
    return static_cast<Expr*>(Operand.get<Stmt *>());
  }

  void setExprOperand(Expr *E) {
    assert(!isTypeOperand() && "Cannot call getExprOperand for typeid(type)");
    Operand = E;
  }

  SourceLocation getLocStart() const LLVM_READONLY { return Range.getBegin(); }
  SourceLocation getLocEnd() const LLVM_READONLY { return Range.getEnd(); }
  SourceRange getSourceRange() const LLVM_READONLY { return Range; }
  void setSourceRange(SourceRange R) { Range = R; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXTypeidExprClass;
  }

  // Iterators
  child_range children() {
    if (isTypeOperand())
      return child_range(child_iterator(), child_iterator());
    auto **begin = reinterpret_cast<Stmt **>(&Operand);
    return child_range(begin, begin + 1);
  }
};

/// \brief A member reference to an MSPropertyDecl. 
///
/// This expression always has pseudo-object type, and therefore it is
/// typically not encountered in a fully-typechecked expression except
/// within the syntactic form of a PseudoObjectExpr.
class MSPropertyRefExpr : public Expr {
  Expr *BaseExpr;
  MSPropertyDecl *TheDecl;
  SourceLocation MemberLoc;
  bool IsArrow;
  NestedNameSpecifierLoc QualifierLoc;

public:
  friend class ASTStmtReader;

  MSPropertyRefExpr(Expr *baseExpr, MSPropertyDecl *decl, bool isArrow,
                    QualType ty, ExprValueKind VK,
                    NestedNameSpecifierLoc qualifierLoc,
                    SourceLocation nameLoc)
      : Expr(MSPropertyRefExprClass, ty, VK, OK_Ordinary,
             /*type-dependent*/ false, baseExpr->isValueDependent(),
             baseExpr->isInstantiationDependent(),
             baseExpr->containsUnexpandedParameterPack()),
        BaseExpr(baseExpr), TheDecl(decl),
        MemberLoc(nameLoc), IsArrow(isArrow),
        QualifierLoc(qualifierLoc) {}

  MSPropertyRefExpr(EmptyShell Empty) : Expr(MSPropertyRefExprClass, Empty) {}

  SourceRange getSourceRange() const LLVM_READONLY {
    return SourceRange(getLocStart(), getLocEnd());
  }

  bool isImplicitAccess() const {
    return getBaseExpr() && getBaseExpr()->isImplicitCXXThis();
  }

  SourceLocation getLocStart() const {
    if (!isImplicitAccess())
      return BaseExpr->getLocStart();
    else if (QualifierLoc)
      return QualifierLoc.getBeginLoc();
    else
        return MemberLoc;
  }

  SourceLocation getLocEnd() const { return getMemberLoc(); }

  child_range children() {
    return child_range((Stmt**)&BaseExpr, (Stmt**)&BaseExpr + 1);
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == MSPropertyRefExprClass;
  }

  Expr *getBaseExpr() const { return BaseExpr; }
  MSPropertyDecl *getPropertyDecl() const { return TheDecl; }
  bool isArrow() const { return IsArrow; }
  SourceLocation getMemberLoc() const { return MemberLoc; }
  NestedNameSpecifierLoc getQualifierLoc() const { return QualifierLoc; }
};

/// MS property subscript expression.
/// MSVC supports 'property' attribute and allows to apply it to the
/// declaration of an empty array in a class or structure definition.
/// For example:
/// \code
/// __declspec(property(get=GetX, put=PutX)) int x[];
/// \endcode
/// The above statement indicates that x[] can be used with one or more array
/// indices. In this case, i=p->x[a][b] will be turned into i=p->GetX(a, b), and
/// p->x[a][b] = i will be turned into p->PutX(a, b, i).
/// This is a syntactic pseudo-object expression.
class MSPropertySubscriptExpr : public Expr {
  friend class ASTStmtReader;

  enum { BASE_EXPR, IDX_EXPR, NUM_SUBEXPRS = 2 };

  Stmt *SubExprs[NUM_SUBEXPRS];
  SourceLocation RBracketLoc;

  void setBase(Expr *Base) { SubExprs[BASE_EXPR] = Base; }
  void setIdx(Expr *Idx) { SubExprs[IDX_EXPR] = Idx; }

public:
  MSPropertySubscriptExpr(Expr *Base, Expr *Idx, QualType Ty, ExprValueKind VK,
                          ExprObjectKind OK, SourceLocation RBracketLoc)
      : Expr(MSPropertySubscriptExprClass, Ty, VK, OK, Idx->isTypeDependent(),
             Idx->isValueDependent(), Idx->isInstantiationDependent(),
             Idx->containsUnexpandedParameterPack()),
        RBracketLoc(RBracketLoc) {
    SubExprs[BASE_EXPR] = Base;
    SubExprs[IDX_EXPR] = Idx;
  }

  /// \brief Create an empty array subscript expression.
  explicit MSPropertySubscriptExpr(EmptyShell Shell)
      : Expr(MSPropertySubscriptExprClass, Shell) {}

  Expr *getBase() { return cast<Expr>(SubExprs[BASE_EXPR]); }
  const Expr *getBase() const { return cast<Expr>(SubExprs[BASE_EXPR]); }

  Expr *getIdx() { return cast<Expr>(SubExprs[IDX_EXPR]); }
  const Expr *getIdx() const { return cast<Expr>(SubExprs[IDX_EXPR]); }

  SourceLocation getLocStart() const LLVM_READONLY {
    return getBase()->getLocStart();
  }

  SourceLocation getLocEnd() const LLVM_READONLY { return RBracketLoc; }

  SourceLocation getRBracketLoc() const { return RBracketLoc; }
  void setRBracketLoc(SourceLocation L) { RBracketLoc = L; }

  SourceLocation getExprLoc() const LLVM_READONLY {
    return getBase()->getExprLoc();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == MSPropertySubscriptExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(&SubExprs[0], &SubExprs[0] + NUM_SUBEXPRS);
  }
};

/// A Microsoft C++ @c __uuidof expression, which gets
/// the _GUID that corresponds to the supplied type or expression.
///
/// This represents code like @c __uuidof(COMTYPE) or @c __uuidof(*comPtr)
class CXXUuidofExpr : public Expr {
private:
  llvm::PointerUnion<Stmt *, TypeSourceInfo *> Operand;
  StringRef UuidStr;
  SourceRange Range;

public:
  CXXUuidofExpr(QualType Ty, TypeSourceInfo *Operand, StringRef UuidStr,
                SourceRange R)
      : Expr(CXXUuidofExprClass, Ty, VK_LValue, OK_Ordinary, false,
             Operand->getType()->isDependentType(),
             Operand->getType()->isInstantiationDependentType(),
             Operand->getType()->containsUnexpandedParameterPack()),
        Operand(Operand), UuidStr(UuidStr), Range(R) {}

  CXXUuidofExpr(QualType Ty, Expr *Operand, StringRef UuidStr, SourceRange R)
      : Expr(CXXUuidofExprClass, Ty, VK_LValue, OK_Ordinary, false,
             Operand->isTypeDependent(), Operand->isInstantiationDependent(),
             Operand->containsUnexpandedParameterPack()),
        Operand(Operand), UuidStr(UuidStr), Range(R) {}

  CXXUuidofExpr(EmptyShell Empty, bool isExpr)
    : Expr(CXXUuidofExprClass, Empty) {
    if (isExpr)
      Operand = (Expr*)nullptr;
    else
      Operand = (TypeSourceInfo*)nullptr;
  }

  bool isTypeOperand() const { return Operand.is<TypeSourceInfo *>(); }

  /// \brief Retrieves the type operand of this __uuidof() expression after
  /// various required adjustments (removing reference types, cv-qualifiers).
  QualType getTypeOperand(ASTContext &Context) const;

  /// \brief Retrieve source information for the type operand.
  TypeSourceInfo *getTypeOperandSourceInfo() const {
    assert(isTypeOperand() && "Cannot call getTypeOperand for __uuidof(expr)");
    return Operand.get<TypeSourceInfo *>();
  }

  void setTypeOperandSourceInfo(TypeSourceInfo *TSI) {
    assert(isTypeOperand() && "Cannot call getTypeOperand for __uuidof(expr)");
    Operand = TSI;
  }

  Expr *getExprOperand() const {
    assert(!isTypeOperand() && "Cannot call getExprOperand for __uuidof(type)");
    return static_cast<Expr*>(Operand.get<Stmt *>());
  }

  void setExprOperand(Expr *E) {
    assert(!isTypeOperand() && "Cannot call getExprOperand for __uuidof(type)");
    Operand = E;
  }

  void setUuidStr(StringRef US) { UuidStr = US; }
  StringRef getUuidStr() const { return UuidStr; }

  SourceLocation getLocStart() const LLVM_READONLY { return Range.getBegin(); }
  SourceLocation getLocEnd() const LLVM_READONLY { return Range.getEnd(); }
  SourceRange getSourceRange() const LLVM_READONLY { return Range; }
  void setSourceRange(SourceRange R) { Range = R; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXUuidofExprClass;
  }

  // Iterators
  child_range children() {
    if (isTypeOperand())
      return child_range(child_iterator(), child_iterator());
    auto **begin = reinterpret_cast<Stmt **>(&Operand);
    return child_range(begin, begin + 1);
  }
};

/// \brief Represents the \c this expression in C++.
///
/// This is a pointer to the object on which the current member function is
/// executing (C++ [expr.prim]p3). Example:
///
/// \code
/// class Foo {
/// public:
///   void bar();
///   void test() { this->bar(); }
/// };
/// \endcode
class CXXThisExpr : public Expr {
  SourceLocation Loc;
  bool Implicit : 1;

public:
  CXXThisExpr(SourceLocation L, QualType Type, bool isImplicit)
      : Expr(CXXThisExprClass, Type, VK_RValue, OK_Ordinary,
             // 'this' is type-dependent if the class type of the enclosing
             // member function is dependent (C++ [temp.dep.expr]p2)
             Type->isDependentType(), Type->isDependentType(),
             Type->isInstantiationDependentType(),
             /*ContainsUnexpandedParameterPack=*/false),
        Loc(L), Implicit(isImplicit) {}

  CXXThisExpr(EmptyShell Empty) : Expr(CXXThisExprClass, Empty) {}

  SourceLocation getLocation() const { return Loc; }
  void setLocation(SourceLocation L) { Loc = L; }

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return Loc; }

  bool isImplicit() const { return Implicit; }
  void setImplicit(bool I) { Implicit = I; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXThisExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief A C++ throw-expression (C++ [except.throw]).
///
/// This handles 'throw' (for re-throwing the current exception) and
/// 'throw' assignment-expression.  When assignment-expression isn't
/// present, Op will be null.
class CXXThrowExpr : public Expr {
  friend class ASTStmtReader;

  Stmt *Op;
  SourceLocation ThrowLoc;

  /// \brief Whether the thrown variable (if any) is in scope.
  unsigned IsThrownVariableInScope : 1;

public:
  // \p Ty is the void type which is used as the result type of the
  // expression.  The \p l is the location of the throw keyword.  \p expr
  // can by null, if the optional expression to throw isn't present.
  CXXThrowExpr(Expr *expr, QualType Ty, SourceLocation l,
               bool IsThrownVariableInScope)
      : Expr(CXXThrowExprClass, Ty, VK_RValue, OK_Ordinary, false, false,
             expr && expr->isInstantiationDependent(),
             expr && expr->containsUnexpandedParameterPack()),
        Op(expr), ThrowLoc(l),
        IsThrownVariableInScope(IsThrownVariableInScope) {}
  CXXThrowExpr(EmptyShell Empty) : Expr(CXXThrowExprClass, Empty) {}

  const Expr *getSubExpr() const { return cast_or_null<Expr>(Op); }
  Expr *getSubExpr() { return cast_or_null<Expr>(Op); }

  SourceLocation getThrowLoc() const { return ThrowLoc; }

  /// \brief Determines whether the variable thrown by this expression (if any!)
  /// is within the innermost try block.
  ///
  /// This information is required to determine whether the NRVO can apply to
  /// this variable.
  bool isThrownVariableInScope() const { return IsThrownVariableInScope; }

  SourceLocation getLocStart() const LLVM_READONLY { return ThrowLoc; }

  SourceLocation getLocEnd() const LLVM_READONLY {
    if (!getSubExpr())
      return ThrowLoc;
    return getSubExpr()->getLocEnd();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXThrowExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(&Op, Op ? &Op+1 : &Op);
  }
};

/// \brief A default argument (C++ [dcl.fct.default]).
///
/// This wraps up a function call argument that was created from the
/// corresponding parameter's default argument, when the call did not
/// explicitly supply arguments for all of the parameters.
class CXXDefaultArgExpr final : public Expr {
  /// \brief The parameter whose default is being used.
  ParmVarDecl *Param;

  /// \brief The location where the default argument expression was used.
  SourceLocation Loc;

  CXXDefaultArgExpr(StmtClass SC, SourceLocation Loc, ParmVarDecl *param)
      : Expr(SC,
             param->hasUnparsedDefaultArg()
               ? param->getType().getNonReferenceType()
               : param->getDefaultArg()->getType(),
             param->getDefaultArg()->getValueKind(),
             param->getDefaultArg()->getObjectKind(), false, false, false,
             false),
        Param(param), Loc(Loc) {}

public:
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  CXXDefaultArgExpr(EmptyShell Empty) : Expr(CXXDefaultArgExprClass, Empty) {}

  // \p Param is the parameter whose default argument is used by this
  // expression.
  static CXXDefaultArgExpr *Create(const ASTContext &C, SourceLocation Loc,
                                   ParmVarDecl *Param) {
    return new (C) CXXDefaultArgExpr(CXXDefaultArgExprClass, Loc, Param);
  }

  // Retrieve the parameter that the argument was created from.
  const ParmVarDecl *getParam() const { return Param; }
  ParmVarDecl *getParam() { return Param; }

  // Retrieve the actual argument to the function call.
  const Expr *getExpr() const {
    return getParam()->getDefaultArg();
  }
  Expr *getExpr() {
    return getParam()->getDefaultArg();
  }

  /// \brief Retrieve the location where this default argument was actually
  /// used.
  SourceLocation getUsedLocation() const { return Loc; }

  /// Default argument expressions have no representation in the
  /// source, so they have an empty source range.
  SourceLocation getLocStart() const LLVM_READONLY { return SourceLocation(); }
  SourceLocation getLocEnd() const LLVM_READONLY { return SourceLocation(); }

  SourceLocation getExprLoc() const LLVM_READONLY { return Loc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXDefaultArgExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief A use of a default initializer in a constructor or in aggregate
/// initialization.
///
/// This wraps a use of a C++ default initializer (technically,
/// a brace-or-equal-initializer for a non-static data member) when it
/// is implicitly used in a mem-initializer-list in a constructor
/// (C++11 [class.base.init]p8) or in aggregate initialization
/// (C++1y [dcl.init.aggr]p7).
class CXXDefaultInitExpr : public Expr {
  /// \brief The field whose default is being used.
  FieldDecl *Field;

  /// \brief The location where the default initializer expression was used.
  SourceLocation Loc;

  CXXDefaultInitExpr(const ASTContext &C, SourceLocation Loc, FieldDecl *Field,
                     QualType T);

  CXXDefaultInitExpr(EmptyShell Empty) : Expr(CXXDefaultInitExprClass, Empty) {}

public:
  friend class ASTReader;
  friend class ASTStmtReader;

  /// \p Field is the non-static data member whose default initializer is used
  /// by this expression.
  static CXXDefaultInitExpr *Create(const ASTContext &C, SourceLocation Loc,
                                    FieldDecl *Field) {
    return new (C) CXXDefaultInitExpr(C, Loc, Field, Field->getType());
  }

  /// \brief Get the field whose initializer will be used.
  FieldDecl *getField() { return Field; }
  const FieldDecl *getField() const { return Field; }

  /// \brief Get the initialization expression that will be used.
  const Expr *getExpr() const {
    assert(Field->getInClassInitializer() && "initializer hasn't been parsed");
    return Field->getInClassInitializer();
  }
  Expr *getExpr() {
    assert(Field->getInClassInitializer() && "initializer hasn't been parsed");
    return Field->getInClassInitializer();
  }

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return Loc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXDefaultInitExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief Represents a C++ temporary.
class CXXTemporary {
  /// \brief The destructor that needs to be called.
  const CXXDestructorDecl *Destructor;

  explicit CXXTemporary(const CXXDestructorDecl *destructor)
      : Destructor(destructor) {}

public:
  static CXXTemporary *Create(const ASTContext &C,
                              const CXXDestructorDecl *Destructor);

  const CXXDestructorDecl *getDestructor() const { return Destructor; }

  void setDestructor(const CXXDestructorDecl *Dtor) {
    Destructor = Dtor;
  }
};

/// \brief Represents binding an expression to a temporary.
///
/// This ensures the destructor is called for the temporary. It should only be
/// needed for non-POD, non-trivially destructable class types. For example:
///
/// \code
///   struct S {
///     S() { }  // User defined constructor makes S non-POD.
///     ~S() { } // User defined destructor makes it non-trivial.
///   };
///   void test() {
///     const S &s_ref = S(); // Requires a CXXBindTemporaryExpr.
///   }
/// \endcode
class CXXBindTemporaryExpr : public Expr {
  CXXTemporary *Temp = nullptr;
  Stmt *SubExpr = nullptr;

  CXXBindTemporaryExpr(CXXTemporary *temp, Expr* SubExpr)
      : Expr(CXXBindTemporaryExprClass, SubExpr->getType(),
             VK_RValue, OK_Ordinary, SubExpr->isTypeDependent(),
             SubExpr->isValueDependent(),
             SubExpr->isInstantiationDependent(),
             SubExpr->containsUnexpandedParameterPack()),
        Temp(temp), SubExpr(SubExpr) {}

public:
  CXXBindTemporaryExpr(EmptyShell Empty)
      : Expr(CXXBindTemporaryExprClass, Empty) {}

  static CXXBindTemporaryExpr *Create(const ASTContext &C, CXXTemporary *Temp,
                                      Expr* SubExpr);

  CXXTemporary *getTemporary() { return Temp; }
  const CXXTemporary *getTemporary() const { return Temp; }
  void setTemporary(CXXTemporary *T) { Temp = T; }

  const Expr *getSubExpr() const { return cast<Expr>(SubExpr); }
  Expr *getSubExpr() { return cast<Expr>(SubExpr); }
  void setSubExpr(Expr *E) { SubExpr = E; }

  SourceLocation getLocStart() const LLVM_READONLY {
    return SubExpr->getLocStart();
  }

  SourceLocation getLocEnd() const LLVM_READONLY { return SubExpr->getLocEnd();}

  // Implement isa/cast/dyncast/etc.
  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXBindTemporaryExprClass;
  }

  // Iterators
  child_range children() { return child_range(&SubExpr, &SubExpr + 1); }
};

/// \brief Represents a call to a C++ constructor.
class CXXConstructExpr : public Expr {
public:
  enum ConstructionKind {
    CK_Complete,
    CK_NonVirtualBase,
    CK_VirtualBase,
    CK_Delegating
  };

private:
  CXXConstructorDecl *Constructor = nullptr;
  SourceLocation Loc;
  SourceRange ParenOrBraceRange;
  unsigned NumArgs : 16;
  unsigned Elidable : 1;
  unsigned HadMultipleCandidates : 1;
  unsigned ListInitialization : 1;
  unsigned StdInitListInitialization : 1;
  unsigned ZeroInitialization : 1;
  unsigned ConstructKind : 2;
  Stmt **Args = nullptr;

  void setConstructor(CXXConstructorDecl *C) { Constructor = C; }

protected:
  CXXConstructExpr(const ASTContext &C, StmtClass SC, QualType T,
                   SourceLocation Loc,
                   CXXConstructorDecl *Ctor,
                   bool Elidable,
                   ArrayRef<Expr *> Args,
                   bool HadMultipleCandidates,
                   bool ListInitialization,
                   bool StdInitListInitialization,
                   bool ZeroInitialization,
                   ConstructionKind ConstructKind,
                   SourceRange ParenOrBraceRange);

  /// \brief Construct an empty C++ construction expression.
  CXXConstructExpr(StmtClass SC, EmptyShell Empty)
      : Expr(SC, Empty), NumArgs(0), Elidable(false),
        HadMultipleCandidates(false), ListInitialization(false),
        ZeroInitialization(false), ConstructKind(0) {}

public:
  friend class ASTStmtReader;

  /// \brief Construct an empty C++ construction expression.
  explicit CXXConstructExpr(EmptyShell Empty)
      : CXXConstructExpr(CXXConstructExprClass, Empty) {}

  static CXXConstructExpr *Create(const ASTContext &C, QualType T,
                                  SourceLocation Loc,
                                  CXXConstructorDecl *Ctor,
                                  bool Elidable,
                                  ArrayRef<Expr *> Args,
                                  bool HadMultipleCandidates,
                                  bool ListInitialization,
                                  bool StdInitListInitialization,
                                  bool ZeroInitialization,
                                  ConstructionKind ConstructKind,
                                  SourceRange ParenOrBraceRange);

  /// \brief Get the constructor that this expression will (ultimately) call.
  CXXConstructorDecl *getConstructor() const { return Constructor; }

  SourceLocation getLocation() const { return Loc; }
  void setLocation(SourceLocation Loc) { this->Loc = Loc; }

  /// \brief Whether this construction is elidable.
  bool isElidable() const { return Elidable; }
  void setElidable(bool E) { Elidable = E; }

  /// \brief Whether the referred constructor was resolved from
  /// an overloaded set having size greater than 1.
  bool hadMultipleCandidates() const { return HadMultipleCandidates; }
  void setHadMultipleCandidates(bool V) { HadMultipleCandidates = V; }

  /// \brief Whether this constructor call was written as list-initialization.
  bool isListInitialization() const { return ListInitialization; }
  void setListInitialization(bool V) { ListInitialization = V; }

  /// \brief Whether this constructor call was written as list-initialization,
  /// but was interpreted as forming a std::initializer_list<T> from the list
  /// and passing that as a single constructor argument.
  /// See C++11 [over.match.list]p1 bullet 1.
  bool isStdInitListInitialization() const { return StdInitListInitialization; }
  void setStdInitListInitialization(bool V) { StdInitListInitialization = V; }

  /// \brief Whether this construction first requires
  /// zero-initialization before the initializer is called.
  bool requiresZeroInitialization() const { return ZeroInitialization; }
  void setRequiresZeroInitialization(bool ZeroInit) {
    ZeroInitialization = ZeroInit;
  }

  /// \brief Determine whether this constructor is actually constructing
  /// a base class (rather than a complete object).
  ConstructionKind getConstructionKind() const {
    return (ConstructionKind)ConstructKind;
  }
  void setConstructionKind(ConstructionKind CK) {
    ConstructKind = CK;
  }

  using arg_iterator = ExprIterator;
  using const_arg_iterator = ConstExprIterator;
  using arg_range = llvm::iterator_range<arg_iterator>;
  using arg_const_range = llvm::iterator_range<const_arg_iterator>;

  arg_range arguments() { return arg_range(arg_begin(), arg_end()); }
  arg_const_range arguments() const {
    return arg_const_range(arg_begin(), arg_end());
  }

  arg_iterator arg_begin() { return Args; }
  arg_iterator arg_end() { return Args + NumArgs; }
  const_arg_iterator arg_begin() const { return Args; }
  const_arg_iterator arg_end() const { return Args + NumArgs; }

  Expr **getArgs() { return reinterpret_cast<Expr **>(Args); }
  const Expr *const *getArgs() const {
    return const_cast<CXXConstructExpr *>(this)->getArgs();
  }
  unsigned getNumArgs() const { return NumArgs; }

  /// \brief Return the specified argument.
  Expr *getArg(unsigned Arg) {
    assert(Arg < NumArgs && "Arg access out of range!");
    return cast<Expr>(Args[Arg]);
  }
  const Expr *getArg(unsigned Arg) const {
    assert(Arg < NumArgs && "Arg access out of range!");
    return cast<Expr>(Args[Arg]);
  }

  /// \brief Set the specified argument.
  void setArg(unsigned Arg, Expr *ArgExpr) {
    assert(Arg < NumArgs && "Arg access out of range!");
    Args[Arg] = ArgExpr;
  }

  SourceLocation getLocStart() const LLVM_READONLY;
  SourceLocation getLocEnd() const LLVM_READONLY;
  SourceRange getParenOrBraceRange() const { return ParenOrBraceRange; }
  void setParenOrBraceRange(SourceRange Range) { ParenOrBraceRange = Range; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXConstructExprClass ||
      T->getStmtClass() == CXXTemporaryObjectExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(&Args[0], &Args[0]+NumArgs);
  }
};

/// \brief Represents a call to an inherited base class constructor from an
/// inheriting constructor. This call implicitly forwards the arguments from
/// the enclosing context (an inheriting constructor) to the specified inherited
/// base class constructor.
class CXXInheritedCtorInitExpr : public Expr {
private:
  CXXConstructorDecl *Constructor = nullptr;

  /// The location of the using declaration.
  SourceLocation Loc;

  /// Whether this is the construction of a virtual base.
  unsigned ConstructsVirtualBase : 1;

  /// Whether the constructor is inherited from a virtual base class of the
  /// class that we construct.
  unsigned InheritedFromVirtualBase : 1;

public:
  friend class ASTStmtReader;

  /// \brief Construct a C++ inheriting construction expression.
  CXXInheritedCtorInitExpr(SourceLocation Loc, QualType T,
                           CXXConstructorDecl *Ctor, bool ConstructsVirtualBase,
                           bool InheritedFromVirtualBase)
      : Expr(CXXInheritedCtorInitExprClass, T, VK_RValue, OK_Ordinary, false,
             false, false, false),
        Constructor(Ctor), Loc(Loc),
        ConstructsVirtualBase(ConstructsVirtualBase),
        InheritedFromVirtualBase(InheritedFromVirtualBase) {
    assert(!T->isDependentType());
  }

  /// \brief Construct an empty C++ inheriting construction expression.
  explicit CXXInheritedCtorInitExpr(EmptyShell Empty)
      : Expr(CXXInheritedCtorInitExprClass, Empty),
        ConstructsVirtualBase(false), InheritedFromVirtualBase(false) {}

  /// \brief Get the constructor that this expression will call.
  CXXConstructorDecl *getConstructor() const { return Constructor; }

  /// \brief Determine whether this constructor is actually constructing
  /// a base class (rather than a complete object).
  bool constructsVBase() const { return ConstructsVirtualBase; }
  CXXConstructExpr::ConstructionKind getConstructionKind() const {
    return ConstructsVirtualBase ? CXXConstructExpr::CK_VirtualBase
                                 : CXXConstructExpr::CK_NonVirtualBase;
  }

  /// \brief Determine whether the inherited constructor is inherited from a
  /// virtual base of the object we construct. If so, we are not responsible
  /// for calling the inherited constructor (the complete object constructor
  /// does that), and so we don't need to pass any arguments.
  bool inheritedFromVBase() const { return InheritedFromVirtualBase; }

  SourceLocation getLocation() const LLVM_READONLY { return Loc; }
  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return Loc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXInheritedCtorInitExprClass;
  }

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief Represents an explicit C++ type conversion that uses "functional"
/// notation (C++ [expr.type.conv]).
///
/// Example:
/// \code
///   x = int(0.5);
/// \endcode
class CXXFunctionalCastExpr final
    : public ExplicitCastExpr,
      private llvm::TrailingObjects<CXXFunctionalCastExpr, CXXBaseSpecifier *> {
  SourceLocation LParenLoc;
  SourceLocation RParenLoc;

  CXXFunctionalCastExpr(QualType ty, ExprValueKind VK,
                        TypeSourceInfo *writtenTy,
                        CastKind kind, Expr *castExpr, unsigned pathSize,
                        SourceLocation lParenLoc, SourceLocation rParenLoc)
      : ExplicitCastExpr(CXXFunctionalCastExprClass, ty, VK, kind,
                         castExpr, pathSize, writtenTy),
        LParenLoc(lParenLoc), RParenLoc(rParenLoc) {}

  explicit CXXFunctionalCastExpr(EmptyShell Shell, unsigned PathSize)
      : ExplicitCastExpr(CXXFunctionalCastExprClass, Shell, PathSize) {}

public:
  friend class CastExpr;
  friend TrailingObjects;

  static CXXFunctionalCastExpr *Create(const ASTContext &Context, QualType T,
                                       ExprValueKind VK,
                                       TypeSourceInfo *Written,
                                       CastKind Kind, Expr *Op,
                                       const CXXCastPath *Path,
                                       SourceLocation LPLoc,
                                       SourceLocation RPLoc);
  static CXXFunctionalCastExpr *CreateEmpty(const ASTContext &Context,
                                            unsigned PathSize);

  SourceLocation getLParenLoc() const { return LParenLoc; }
  void setLParenLoc(SourceLocation L) { LParenLoc = L; }
  SourceLocation getRParenLoc() const { return RParenLoc; }
  void setRParenLoc(SourceLocation L) { RParenLoc = L; }

  /// Determine whether this expression models list-initialization.
  bool isListInitialization() const { return LParenLoc.isInvalid(); }

  SourceLocation getLocStart() const LLVM_READONLY;
  SourceLocation getLocEnd() const LLVM_READONLY;

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXFunctionalCastExprClass;
  }
};

/// @brief Represents a C++ functional cast expression that builds a
/// temporary object.
///
/// This expression type represents a C++ "functional" cast
/// (C++[expr.type.conv]) with N != 1 arguments that invokes a
/// constructor to build a temporary object. With N == 1 arguments the
/// functional cast expression will be represented by CXXFunctionalCastExpr.
/// Example:
/// \code
/// struct X { X(int, float); }
///
/// X create_X() {
///   return X(1, 3.14f); // creates a CXXTemporaryObjectExpr
/// };
/// \endcode
class CXXTemporaryObjectExpr : public CXXConstructExpr {
  TypeSourceInfo *Type = nullptr;

public:
  friend class ASTStmtReader;

  CXXTemporaryObjectExpr(const ASTContext &C,
                         CXXConstructorDecl *Cons,
                         QualType Type,
                         TypeSourceInfo *TSI,
                         ArrayRef<Expr *> Args,
                         SourceRange ParenOrBraceRange,
                         bool HadMultipleCandidates,
                         bool ListInitialization,
                         bool StdInitListInitialization,
                         bool ZeroInitialization);
  explicit CXXTemporaryObjectExpr(EmptyShell Empty)
      : CXXConstructExpr(CXXTemporaryObjectExprClass, Empty) {}

  TypeSourceInfo *getTypeSourceInfo() const { return Type; }

  SourceLocation getLocStart() const LLVM_READONLY;
  SourceLocation getLocEnd() const LLVM_READONLY;

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXTemporaryObjectExprClass;
  }
};

/// \brief A C++ lambda expression, which produces a function object
/// (of unspecified type) that can be invoked later.
///
/// Example:
/// \code
/// void low_pass_filter(std::vector<double> &values, double cutoff) {
///   values.erase(std::remove_if(values.begin(), values.end(),
///                               [=](double value) { return value > cutoff; });
/// }
/// \endcode
///
/// C++11 lambda expressions can capture local variables, either by copying
/// the values of those local variables at the time the function
/// object is constructed (not when it is called!) or by holding a
/// reference to the local variable. These captures can occur either
/// implicitly or can be written explicitly between the square
/// brackets ([...]) that start the lambda expression.
///
/// C++1y introduces a new form of "capture" called an init-capture that
/// includes an initializing expression (rather than capturing a variable),
/// and which can never occur implicitly.
class LambdaExpr final : public Expr,
                         private llvm::TrailingObjects<LambdaExpr, Stmt *> {
  /// \brief The source range that covers the lambda introducer ([...]).
  SourceRange IntroducerRange;

  /// \brief The source location of this lambda's capture-default ('=' or '&').
  SourceLocation CaptureDefaultLoc;

  /// \brief The number of captures.
  unsigned NumCaptures : 16;
  
  /// \brief The default capture kind, which is a value of type
  /// LambdaCaptureDefault.
  unsigned CaptureDefault : 2;

  /// \brief Whether this lambda had an explicit parameter list vs. an
  /// implicit (and empty) parameter list.
  unsigned ExplicitParams : 1;

  /// \brief Whether this lambda had the result type explicitly specified.
  unsigned ExplicitResultType : 1;
  
  /// \brief The location of the closing brace ('}') that completes
  /// the lambda.
  /// 
  /// The location of the brace is also available by looking up the
  /// function call operator in the lambda class. However, it is
  /// stored here to improve the performance of getSourceRange(), and
  /// to avoid having to deserialize the function call operator from a
  /// module file just to determine the source range.
  SourceLocation ClosingBrace;

  /// \brief Construct a lambda expression.
  LambdaExpr(QualType T, SourceRange IntroducerRange,
             LambdaCaptureDefault CaptureDefault,
             SourceLocation CaptureDefaultLoc, ArrayRef<LambdaCapture> Captures,
             bool ExplicitParams, bool ExplicitResultType,
             ArrayRef<Expr *> CaptureInits, SourceLocation ClosingBrace,
             bool ContainsUnexpandedParameterPack);

  /// \brief Construct an empty lambda expression.
  LambdaExpr(EmptyShell Empty, unsigned NumCaptures)
      : Expr(LambdaExprClass, Empty), NumCaptures(NumCaptures),
        CaptureDefault(LCD_None), ExplicitParams(false),
        ExplicitResultType(false) {
    getStoredStmts()[NumCaptures] = nullptr;
  }

  Stmt **getStoredStmts() { return getTrailingObjects<Stmt *>(); }

  Stmt *const *getStoredStmts() const { return getTrailingObjects<Stmt *>(); }

public:
  friend class ASTStmtReader;
  friend class ASTStmtWriter;
  friend TrailingObjects;

  /// \brief Construct a new lambda expression.
  static LambdaExpr *
  Create(const ASTContext &C, CXXRecordDecl *Class, SourceRange IntroducerRange,
         LambdaCaptureDefault CaptureDefault, SourceLocation CaptureDefaultLoc,
         ArrayRef<LambdaCapture> Captures, bool ExplicitParams,
         bool ExplicitResultType, ArrayRef<Expr *> CaptureInits,
         SourceLocation ClosingBrace, bool ContainsUnexpandedParameterPack);

  /// \brief Construct a new lambda expression that will be deserialized from
  /// an external source.
  static LambdaExpr *CreateDeserialized(const ASTContext &C,
                                        unsigned NumCaptures);

  /// \brief Determine the default capture kind for this lambda.
  LambdaCaptureDefault getCaptureDefault() const {
    return static_cast<LambdaCaptureDefault>(CaptureDefault);
  }

  /// \brief Retrieve the location of this lambda's capture-default, if any.
  SourceLocation getCaptureDefaultLoc() const {
    return CaptureDefaultLoc;
  }

  /// \brief Determine whether one of this lambda's captures is an init-capture.
  bool isInitCapture(const LambdaCapture *Capture) const;

  /// \brief An iterator that walks over the captures of the lambda,
  /// both implicit and explicit.
  using capture_iterator = const LambdaCapture *;

  /// \brief An iterator over a range of lambda captures.
  using capture_range = llvm::iterator_range<capture_iterator>;

  /// \brief Retrieve this lambda's captures.
  capture_range captures() const;
  
  /// \brief Retrieve an iterator pointing to the first lambda capture.
  capture_iterator capture_begin() const;

  /// \brief Retrieve an iterator pointing past the end of the
  /// sequence of lambda captures.
  capture_iterator capture_end() const;

  /// \brief Determine the number of captures in this lambda.
  unsigned capture_size() const { return NumCaptures; }

  /// \brief Retrieve this lambda's explicit captures.
  capture_range explicit_captures() const;
  
  /// \brief Retrieve an iterator pointing to the first explicit
  /// lambda capture.
  capture_iterator explicit_capture_begin() const;

  /// \brief Retrieve an iterator pointing past the end of the sequence of
  /// explicit lambda captures.
  capture_iterator explicit_capture_end() const;

  /// \brief Retrieve this lambda's implicit captures.
  capture_range implicit_captures() const;

  /// \brief Retrieve an iterator pointing to the first implicit
  /// lambda capture.
  capture_iterator implicit_capture_begin() const;

  /// \brief Retrieve an iterator pointing past the end of the sequence of
  /// implicit lambda captures.
  capture_iterator implicit_capture_end() const;

  /// \brief Iterator that walks over the capture initialization
  /// arguments.
  using capture_init_iterator = Expr **;

  /// \brief Const iterator that walks over the capture initialization
  /// arguments.
  using const_capture_init_iterator = Expr *const *;

  /// \brief Retrieve the initialization expressions for this lambda's captures.
  llvm::iterator_range<capture_init_iterator> capture_inits() {
    return llvm::make_range(capture_init_begin(), capture_init_end());
  }

  /// \brief Retrieve the initialization expressions for this lambda's captures.
  llvm::iterator_range<const_capture_init_iterator> capture_inits() const {
    return llvm::make_range(capture_init_begin(), capture_init_end());
  }

  /// \brief Retrieve the first initialization argument for this
  /// lambda expression (which initializes the first capture field).
  capture_init_iterator capture_init_begin() {
    return reinterpret_cast<Expr **>(getStoredStmts());
  }

  /// \brief Retrieve the first initialization argument for this
  /// lambda expression (which initializes the first capture field).
  const_capture_init_iterator capture_init_begin() const {
    return reinterpret_cast<Expr *const *>(getStoredStmts());
  }

  /// \brief Retrieve the iterator pointing one past the last
  /// initialization argument for this lambda expression.
  capture_init_iterator capture_init_end() {
    return capture_init_begin() + NumCaptures;
  }

  /// \brief Retrieve the iterator pointing one past the last
  /// initialization argument for this lambda expression.
  const_capture_init_iterator capture_init_end() const {
    return capture_init_begin() + NumCaptures;
  }

  /// \brief Retrieve the source range covering the lambda introducer,
  /// which contains the explicit capture list surrounded by square
  /// brackets ([...]).
  SourceRange getIntroducerRange() const { return IntroducerRange; }

  /// \brief Retrieve the class that corresponds to the lambda.
  /// 
  /// This is the "closure type" (C++1y [expr.prim.lambda]), and stores the
  /// captures in its fields and provides the various operations permitted
  /// on a lambda (copying, calling).
  CXXRecordDecl *getLambdaClass() const;

  /// \brief Retrieve the function call operator associated with this
  /// lambda expression. 
  CXXMethodDecl *getCallOperator() const;

  /// \brief If this is a generic lambda expression, retrieve the template 
  /// parameter list associated with it, or else return null. 
  TemplateParameterList *getTemplateParameterList() const;

  /// \brief Whether this is a generic lambda.
  bool isGenericLambda() const { return getTemplateParameterList(); }

  /// \brief Retrieve the body of the lambda.
  CompoundStmt *getBody() const;

  /// \brief Determine whether the lambda is mutable, meaning that any
  /// captures values can be modified.
  bool isMutable() const;

  /// \brief Determine whether this lambda has an explicit parameter
  /// list vs. an implicit (empty) parameter list.
  bool hasExplicitParameters() const { return ExplicitParams; }

  /// \brief Whether this lambda had its result type explicitly specified.
  bool hasExplicitResultType() const { return ExplicitResultType; }
    
  static bool classof(const Stmt *T) {
    return T->getStmtClass() == LambdaExprClass;
  }

  SourceLocation getLocStart() const LLVM_READONLY {
    return IntroducerRange.getBegin();
  }

  SourceLocation getLocEnd() const LLVM_READONLY { return ClosingBrace; }

  child_range children() {
    // Includes initialization exprs plus body stmt
    return child_range(getStoredStmts(), getStoredStmts() + NumCaptures + 1);
  }
};

/// An expression "T()" which creates a value-initialized rvalue of type
/// T, which is a non-class type.  See (C++98 [5.2.3p2]).
class CXXScalarValueInitExpr : public Expr {
  friend class ASTStmtReader;

  SourceLocation RParenLoc;
  TypeSourceInfo *TypeInfo;

public:
  /// \brief Create an explicitly-written scalar-value initialization
  /// expression.
  CXXScalarValueInitExpr(QualType Type, TypeSourceInfo *TypeInfo,
                         SourceLocation rParenLoc)
      : Expr(CXXScalarValueInitExprClass, Type, VK_RValue, OK_Ordinary,
             false, false, Type->isInstantiationDependentType(),
             Type->containsUnexpandedParameterPack()),
        RParenLoc(rParenLoc), TypeInfo(TypeInfo) {}

  explicit CXXScalarValueInitExpr(EmptyShell Shell)
      : Expr(CXXScalarValueInitExprClass, Shell) {}

  TypeSourceInfo *getTypeSourceInfo() const {
    return TypeInfo;
  }

  SourceLocation getRParenLoc() const { return RParenLoc; }

  SourceLocation getLocStart() const LLVM_READONLY;
  SourceLocation getLocEnd() const LLVM_READONLY { return RParenLoc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXScalarValueInitExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief Represents a new-expression for memory allocation and constructor
/// calls, e.g: "new CXXNewExpr(foo)".
class CXXNewExpr : public Expr {
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  /// Contains an optional array size expression, an optional initialization
  /// expression, and any number of optional placement arguments, in that order.
  Stmt **SubExprs = nullptr;

  /// \brief Points to the allocation function used.
  FunctionDecl *OperatorNew;

  /// \brief Points to the deallocation function used in case of error. May be
  /// null.
  FunctionDecl *OperatorDelete;

  /// \brief The allocated type-source information, as written in the source.
  TypeSourceInfo *AllocatedTypeInfo;

  /// \brief If the allocated type was expressed as a parenthesized type-id,
  /// the source range covering the parenthesized type-id.
  SourceRange TypeIdParens;

  /// \brief Range of the entire new expression.
  SourceRange Range;

  /// \brief Source-range of a paren-delimited initializer.
  SourceRange DirectInitRange;

  /// Was the usage ::new, i.e. is the global new to be used?
  unsigned GlobalNew : 1;

  /// Do we allocate an array? If so, the first SubExpr is the size expression.
  unsigned Array : 1;

  /// Should the alignment be passed to the allocation function?
  unsigned PassAlignment : 1;

  /// If this is an array allocation, does the usual deallocation
  /// function for the allocated type want to know the allocated size?
  unsigned UsualArrayDeleteWantsSize : 1;

  /// The number of placement new arguments.
  unsigned NumPlacementArgs : 26;

  /// What kind of initializer do we have? Could be none, parens, or braces.
  /// In storage, we distinguish between "none, and no initializer expr", and
  /// "none, but an implicit initializer expr".
  unsigned StoredInitializationStyle : 2;

public:
  enum InitializationStyle {
    /// New-expression has no initializer as written.
    NoInit,

    /// New-expression has a C++98 paren-delimited initializer.
    CallInit,

    /// New-expression has a C++11 list-initializer.
    ListInit
  };

  CXXNewExpr(const ASTContext &C, bool globalNew, FunctionDecl *operatorNew,
             FunctionDecl *operatorDelete, bool PassAlignment,
             bool usualArrayDeleteWantsSize, ArrayRef<Expr*> placementArgs,
             SourceRange typeIdParens, Expr *arraySize,
             InitializationStyle initializationStyle, Expr *initializer,
             QualType ty, TypeSourceInfo *AllocatedTypeInfo,
             SourceRange Range, SourceRange directInitRange);
  explicit CXXNewExpr(EmptyShell Shell)
      : Expr(CXXNewExprClass, Shell) {}

  void AllocateArgsArray(const ASTContext &C, bool isArray,
                         unsigned numPlaceArgs, bool hasInitializer);

  QualType getAllocatedType() const {
    assert(getType()->isPointerType());
    return getType()->getAs<PointerType>()->getPointeeType();
  }

  TypeSourceInfo *getAllocatedTypeSourceInfo() const {
    return AllocatedTypeInfo;
  }

  /// \brief True if the allocation result needs to be null-checked.
  ///
  /// C++11 [expr.new]p13:
  ///   If the allocation function returns null, initialization shall
  ///   not be done, the deallocation function shall not be called,
  ///   and the value of the new-expression shall be null.
  ///
  /// C++ DR1748:
  ///   If the allocation function is a reserved placement allocation
  ///   function that returns null, the behavior is undefined.
  ///
  /// An allocation function is not allowed to return null unless it
  /// has a non-throwing exception-specification.  The '03 rule is
  /// identical except that the definition of a non-throwing
  /// exception specification is just "is it throw()?".
  bool shouldNullCheckAllocation(const ASTContext &Ctx) const;

  FunctionDecl *getOperatorNew() const { return OperatorNew; }
  void setOperatorNew(FunctionDecl *D) { OperatorNew = D; }
  FunctionDecl *getOperatorDelete() const { return OperatorDelete; }
  void setOperatorDelete(FunctionDecl *D) { OperatorDelete = D; }

  bool isArray() const { return Array; }

  Expr *getArraySize() {
    return Array ? cast<Expr>(SubExprs[0]) : nullptr;
  }
  const Expr *getArraySize() const {
    return Array ? cast<Expr>(SubExprs[0]) : nullptr;
  }

  unsigned getNumPlacementArgs() const { return NumPlacementArgs; }

  Expr **getPlacementArgs() {
    return reinterpret_cast<Expr **>(SubExprs + Array + hasInitializer());
  }

  Expr *getPlacementArg(unsigned i) {
    assert(i < NumPlacementArgs && "Index out of range");
    return getPlacementArgs()[i];
  }
  const Expr *getPlacementArg(unsigned i) const {
    assert(i < NumPlacementArgs && "Index out of range");
    return const_cast<CXXNewExpr*>(this)->getPlacementArg(i);
  }

  bool isParenTypeId() const { return TypeIdParens.isValid(); }
  SourceRange getTypeIdParens() const { return TypeIdParens; }

  bool isGlobalNew() const { return GlobalNew; }

  /// \brief Whether this new-expression has any initializer at all.
  bool hasInitializer() const { return StoredInitializationStyle > 0; }

  /// \brief The kind of initializer this new-expression has.
  InitializationStyle getInitializationStyle() const {
    if (StoredInitializationStyle == 0)
      return NoInit;
    return static_cast<InitializationStyle>(StoredInitializationStyle-1);
  }

  /// \brief The initializer of this new-expression.
  Expr *getInitializer() {
    return hasInitializer() ? cast<Expr>(SubExprs[Array]) : nullptr;
  }
  const Expr *getInitializer() const {
    return hasInitializer() ? cast<Expr>(SubExprs[Array]) : nullptr;
  }

  /// \brief Returns the CXXConstructExpr from this new-expression, or null.
  const CXXConstructExpr *getConstructExpr() const {
    return dyn_cast_or_null<CXXConstructExpr>(getInitializer());
  }

  /// Indicates whether the required alignment should be implicitly passed to
  /// the allocation function.
  bool passAlignment() const {
    return PassAlignment;
  }

  /// Answers whether the usual array deallocation function for the
  /// allocated type expects the size of the allocation as a
  /// parameter.
  bool doesUsualArrayDeleteWantSize() const {
    return UsualArrayDeleteWantsSize;
  }

  using arg_iterator = ExprIterator;
  using const_arg_iterator = ConstExprIterator;

  llvm::iterator_range<arg_iterator> placement_arguments() {
    return llvm::make_range(placement_arg_begin(), placement_arg_end());
  }

  llvm::iterator_range<const_arg_iterator> placement_arguments() const {
    return llvm::make_range(placement_arg_begin(), placement_arg_end());
  }

  arg_iterator placement_arg_begin() {
    return SubExprs + Array + hasInitializer();
  }
  arg_iterator placement_arg_end() {
    return SubExprs + Array + hasInitializer() + getNumPlacementArgs();
  }
  const_arg_iterator placement_arg_begin() const {
    return SubExprs + Array + hasInitializer();
  }
  const_arg_iterator placement_arg_end() const {
    return SubExprs + Array + hasInitializer() + getNumPlacementArgs();
  }

  using raw_arg_iterator = Stmt **;

  raw_arg_iterator raw_arg_begin() { return SubExprs; }
  raw_arg_iterator raw_arg_end() {
    return SubExprs + Array + hasInitializer() + getNumPlacementArgs();
  }
  const_arg_iterator raw_arg_begin() const { return SubExprs; }
  const_arg_iterator raw_arg_end() const {
    return SubExprs + Array + hasInitializer() + getNumPlacementArgs();
  }

  SourceLocation getStartLoc() const { return Range.getBegin(); }
  SourceLocation getEndLoc() const { return Range.getEnd(); }

  SourceRange getDirectInitRange() const { return DirectInitRange; }

  SourceRange getSourceRange() const LLVM_READONLY {
    return Range;
  }

  SourceLocation getLocStart() const LLVM_READONLY { return getStartLoc(); }
  SourceLocation getLocEnd() const LLVM_READONLY { return getEndLoc(); }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXNewExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(raw_arg_begin(), raw_arg_end());
  }
};

/// \brief Represents a \c delete expression for memory deallocation and
/// destructor calls, e.g. "delete[] pArray".
class CXXDeleteExpr : public Expr {
  /// Points to the operator delete overload that is used. Could be a member.
  FunctionDecl *OperatorDelete = nullptr;

  /// The pointer expression to be deleted.
  Stmt *Argument = nullptr;

  /// Location of the expression.
  SourceLocation Loc;

  /// Is this a forced global delete, i.e. "::delete"?
  bool GlobalDelete : 1;

  /// Is this the array form of delete, i.e. "delete[]"?
  bool ArrayForm : 1;

  /// ArrayFormAsWritten can be different from ArrayForm if 'delete' is applied
  /// to pointer-to-array type (ArrayFormAsWritten will be false while ArrayForm
  /// will be true).
  bool ArrayFormAsWritten : 1;

  /// Does the usual deallocation function for the element type require
  /// a size_t argument?
  bool UsualArrayDeleteWantsSize : 1;

public:
  friend class ASTStmtReader;

  CXXDeleteExpr(QualType ty, bool globalDelete, bool arrayForm,
                bool arrayFormAsWritten, bool usualArrayDeleteWantsSize,
                FunctionDecl *operatorDelete, Expr *arg, SourceLocation loc)
      : Expr(CXXDeleteExprClass, ty, VK_RValue, OK_Ordinary, false, false,
             arg->isInstantiationDependent(),
             arg->containsUnexpandedParameterPack()),
        OperatorDelete(operatorDelete), Argument(arg), Loc(loc),
        GlobalDelete(globalDelete),
        ArrayForm(arrayForm), ArrayFormAsWritten(arrayFormAsWritten),
        UsualArrayDeleteWantsSize(usualArrayDeleteWantsSize) {}
  explicit CXXDeleteExpr(EmptyShell Shell) : Expr(CXXDeleteExprClass, Shell) {}

  bool isGlobalDelete() const { return GlobalDelete; }
  bool isArrayForm() const { return ArrayForm; }
  bool isArrayFormAsWritten() const { return ArrayFormAsWritten; }

  /// Answers whether the usual array deallocation function for the
  /// allocated type expects the size of the allocation as a
  /// parameter.  This can be true even if the actual deallocation
  /// function that we're using doesn't want a size.
  bool doesUsualArrayDeleteWantSize() const {
    return UsualArrayDeleteWantsSize;
  }

  FunctionDecl *getOperatorDelete() const { return OperatorDelete; }

  Expr *getArgument() { return cast<Expr>(Argument); }
  const Expr *getArgument() const { return cast<Expr>(Argument); }

  /// \brief Retrieve the type being destroyed. 
  ///
  /// If the type being destroyed is a dependent type which may or may not
  /// be a pointer, return an invalid type.
  QualType getDestroyedType() const;

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY {return Argument->getLocEnd();}

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXDeleteExprClass;
  }

  // Iterators
  child_range children() { return child_range(&Argument, &Argument+1); }
};

/// \brief Stores the type being destroyed by a pseudo-destructor expression.
class PseudoDestructorTypeStorage {
  /// \brief Either the type source information or the name of the type, if
  /// it couldn't be resolved due to type-dependence.
  llvm::PointerUnion<TypeSourceInfo *, IdentifierInfo *> Type;

  /// \brief The starting source location of the pseudo-destructor type.
  SourceLocation Location;

public:
  PseudoDestructorTypeStorage() = default;

  PseudoDestructorTypeStorage(IdentifierInfo *II, SourceLocation Loc)
      : Type(II), Location(Loc) {}

  PseudoDestructorTypeStorage(TypeSourceInfo *Info);

  TypeSourceInfo *getTypeSourceInfo() const {
    return Type.dyn_cast<TypeSourceInfo *>();
  }

  IdentifierInfo *getIdentifier() const {
    return Type.dyn_cast<IdentifierInfo *>();
  }

  SourceLocation getLocation() const { return Location; }
};

/// \brief Represents a C++ pseudo-destructor (C++ [expr.pseudo]).
///
/// A pseudo-destructor is an expression that looks like a member access to a
/// destructor of a scalar type, except that scalar types don't have
/// destructors. For example:
///
/// \code
/// typedef int T;
/// void f(int *p) {
///   p->T::~T();
/// }
/// \endcode
///
/// Pseudo-destructors typically occur when instantiating templates such as:
///
/// \code
/// template<typename T>
/// void destroy(T* ptr) {
///   ptr->T::~T();
/// }
/// \endcode
///
/// for scalar types. A pseudo-destructor expression has no run-time semantics
/// beyond evaluating the base expression.
class CXXPseudoDestructorExpr : public Expr {
  friend class ASTStmtReader;

  /// \brief The base expression (that is being destroyed).
  Stmt *Base = nullptr;

  /// \brief Whether the operator was an arrow ('->'); otherwise, it was a
  /// period ('.').
  bool IsArrow : 1;

  /// \brief The location of the '.' or '->' operator.
  SourceLocation OperatorLoc;

  /// \brief The nested-name-specifier that follows the operator, if present.
  NestedNameSpecifierLoc QualifierLoc;

  /// \brief The type that precedes the '::' in a qualified pseudo-destructor
  /// expression.
  TypeSourceInfo *ScopeType = nullptr;

  /// \brief The location of the '::' in a qualified pseudo-destructor
  /// expression.
  SourceLocation ColonColonLoc;

  /// \brief The location of the '~'.
  SourceLocation TildeLoc;

  /// \brief The type being destroyed, or its name if we were unable to
  /// resolve the name.
  PseudoDestructorTypeStorage DestroyedType;

public:
  CXXPseudoDestructorExpr(const ASTContext &Context,
                          Expr *Base, bool isArrow, SourceLocation OperatorLoc,
                          NestedNameSpecifierLoc QualifierLoc,
                          TypeSourceInfo *ScopeType,
                          SourceLocation ColonColonLoc,
                          SourceLocation TildeLoc,
                          PseudoDestructorTypeStorage DestroyedType);

  explicit CXXPseudoDestructorExpr(EmptyShell Shell)
      : Expr(CXXPseudoDestructorExprClass, Shell), IsArrow(false) {}

  Expr *getBase() const { return cast<Expr>(Base); }

  /// \brief Determines whether this member expression actually had
  /// a C++ nested-name-specifier prior to the name of the member, e.g.,
  /// x->Base::foo.
  bool hasQualifier() const { return QualifierLoc.hasQualifier(); }

  /// \brief Retrieves the nested-name-specifier that qualifies the type name,
  /// with source-location information.
  NestedNameSpecifierLoc getQualifierLoc() const { return QualifierLoc; }

  /// \brief If the member name was qualified, retrieves the
  /// nested-name-specifier that precedes the member name. Otherwise, returns
  /// null.
  NestedNameSpecifier *getQualifier() const {
    return QualifierLoc.getNestedNameSpecifier();
  }

  /// \brief Determine whether this pseudo-destructor expression was written
  /// using an '->' (otherwise, it used a '.').
  bool isArrow() const { return IsArrow; }

  /// \brief Retrieve the location of the '.' or '->' operator.
  SourceLocation getOperatorLoc() const { return OperatorLoc; }

  /// \brief Retrieve the scope type in a qualified pseudo-destructor
  /// expression.
  ///
  /// Pseudo-destructor expressions can have extra qualification within them
  /// that is not part of the nested-name-specifier, e.g., \c p->T::~T().
  /// Here, if the object type of the expression is (or may be) a scalar type,
  /// \p T may also be a scalar type and, therefore, cannot be part of a
  /// nested-name-specifier. It is stored as the "scope type" of the pseudo-
  /// destructor expression.
  TypeSourceInfo *getScopeTypeInfo() const { return ScopeType; }

  /// \brief Retrieve the location of the '::' in a qualified pseudo-destructor
  /// expression.
  SourceLocation getColonColonLoc() const { return ColonColonLoc; }

  /// \brief Retrieve the location of the '~'.
  SourceLocation getTildeLoc() const { return TildeLoc; }

  /// \brief Retrieve the source location information for the type
  /// being destroyed.
  ///
  /// This type-source information is available for non-dependent
  /// pseudo-destructor expressions and some dependent pseudo-destructor
  /// expressions. Returns null if we only have the identifier for a
  /// dependent pseudo-destructor expression.
  TypeSourceInfo *getDestroyedTypeInfo() const {
    return DestroyedType.getTypeSourceInfo();
  }

  /// \brief In a dependent pseudo-destructor expression for which we do not
  /// have full type information on the destroyed type, provides the name
  /// of the destroyed type.
  IdentifierInfo *getDestroyedTypeIdentifier() const {
    return DestroyedType.getIdentifier();
  }

  /// \brief Retrieve the type being destroyed.
  QualType getDestroyedType() const;

  /// \brief Retrieve the starting location of the type being destroyed.
  SourceLocation getDestroyedTypeLoc() const {
    return DestroyedType.getLocation();
  }

  /// \brief Set the name of destroyed type for a dependent pseudo-destructor
  /// expression.
  void setDestroyedType(IdentifierInfo *II, SourceLocation Loc) {
    DestroyedType = PseudoDestructorTypeStorage(II, Loc);
  }

  /// \brief Set the destroyed type.
  void setDestroyedType(TypeSourceInfo *Info) {
    DestroyedType = PseudoDestructorTypeStorage(Info);
  }

  SourceLocation getLocStart() const LLVM_READONLY {return Base->getLocStart();}
  SourceLocation getLocEnd() const LLVM_READONLY;

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXPseudoDestructorExprClass;
  }

  // Iterators
  child_range children() { return child_range(&Base, &Base + 1); }
};

/// \brief A type trait used in the implementation of various C++11 and
/// Library TR1 trait templates.
///
/// \code
///   __is_pod(int) == true
///   __is_enum(std::string) == false
///   __is_trivially_constructible(vector<int>, int*, int*)
/// \endcode
class TypeTraitExpr final
    : public Expr,
      private llvm::TrailingObjects<TypeTraitExpr, TypeSourceInfo *> {
  /// \brief The location of the type trait keyword.
  SourceLocation Loc;
  
  /// \brief  The location of the closing parenthesis.
  SourceLocation RParenLoc;
  
  // Note: The TypeSourceInfos for the arguments are allocated after the
  // TypeTraitExpr.
  
  TypeTraitExpr(QualType T, SourceLocation Loc, TypeTrait Kind,
                ArrayRef<TypeSourceInfo *> Args,
                SourceLocation RParenLoc,
                bool Value);

  TypeTraitExpr(EmptyShell Empty) : Expr(TypeTraitExprClass, Empty) {}

  size_t numTrailingObjects(OverloadToken<TypeSourceInfo *>) const {
    return getNumArgs();
  }

public:
  friend class ASTStmtReader;
  friend class ASTStmtWriter;
  friend TrailingObjects;

  /// \brief Create a new type trait expression.
  static TypeTraitExpr *Create(const ASTContext &C, QualType T,
                               SourceLocation Loc, TypeTrait Kind,
                               ArrayRef<TypeSourceInfo *> Args,
                               SourceLocation RParenLoc,
                               bool Value);

  static TypeTraitExpr *CreateDeserialized(const ASTContext &C,
                                           unsigned NumArgs);
  
  /// \brief Determine which type trait this expression uses.
  TypeTrait getTrait() const {
    return static_cast<TypeTrait>(TypeTraitExprBits.Kind);
  }

  bool getValue() const { 
    assert(!isValueDependent()); 
    return TypeTraitExprBits.Value; 
  }
  
  /// \brief Determine the number of arguments to this type trait.
  unsigned getNumArgs() const { return TypeTraitExprBits.NumArgs; }
  
  /// \brief Retrieve the Ith argument.
  TypeSourceInfo *getArg(unsigned I) const {
    assert(I < getNumArgs() && "Argument out-of-range");
    return getArgs()[I];
  }
  
  /// \brief Retrieve the argument types.
  ArrayRef<TypeSourceInfo *> getArgs() const {
    return llvm::makeArrayRef(getTrailingObjects<TypeSourceInfo *>(),
                              getNumArgs());
  }

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return RParenLoc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == TypeTraitExprClass;
  }
  
  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief An Embarcadero array type trait, as used in the implementation of
/// __array_rank and __array_extent.
///
/// Example:
/// \code
///   __array_rank(int[10][20]) == 2
///   __array_extent(int, 1)    == 20
/// \endcode
class ArrayTypeTraitExpr : public Expr {
  /// \brief The trait. An ArrayTypeTrait enum in MSVC compat unsigned.
  unsigned ATT : 2;

  /// \brief The value of the type trait. Unspecified if dependent.
  uint64_t Value = 0;

  /// \brief The array dimension being queried, or -1 if not used.
  Expr *Dimension;

  /// \brief The location of the type trait keyword.
  SourceLocation Loc;

  /// \brief The location of the closing paren.
  SourceLocation RParen;

  /// \brief The type being queried.
  TypeSourceInfo *QueriedType = nullptr;

  virtual void anchor();

public:
  friend class ASTStmtReader;

  ArrayTypeTraitExpr(SourceLocation loc, ArrayTypeTrait att,
                     TypeSourceInfo *queried, uint64_t value,
                     Expr *dimension, SourceLocation rparen, QualType ty)
      : Expr(ArrayTypeTraitExprClass, ty, VK_RValue, OK_Ordinary,
             false, queried->getType()->isDependentType(),
             (queried->getType()->isInstantiationDependentType() ||
              (dimension && dimension->isInstantiationDependent())),
             queried->getType()->containsUnexpandedParameterPack()),
        ATT(att), Value(value), Dimension(dimension),
        Loc(loc), RParen(rparen), QueriedType(queried) {}

  explicit ArrayTypeTraitExpr(EmptyShell Empty)
      : Expr(ArrayTypeTraitExprClass, Empty), ATT(0) {}

  virtual ~ArrayTypeTraitExpr() = default;

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return RParen; }

  ArrayTypeTrait getTrait() const { return static_cast<ArrayTypeTrait>(ATT); }

  QualType getQueriedType() const { return QueriedType->getType(); }

  TypeSourceInfo *getQueriedTypeSourceInfo() const { return QueriedType; }

  uint64_t getValue() const { assert(!isTypeDependent()); return Value; }

  Expr *getDimensionExpression() const { return Dimension; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ArrayTypeTraitExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief An expression trait intrinsic.
///
/// Example:
/// \code
///   __is_lvalue_expr(std::cout) == true
///   __is_lvalue_expr(1) == false
/// \endcode
class ExpressionTraitExpr : public Expr {
  /// \brief The trait. A ExpressionTrait enum in MSVC compatible unsigned.
  unsigned ET : 31;

  /// \brief The value of the type trait. Unspecified if dependent.
  unsigned Value : 1;

  /// \brief The location of the type trait keyword.
  SourceLocation Loc;

  /// \brief The location of the closing paren.
  SourceLocation RParen;

  /// \brief The expression being queried.
  Expr* QueriedExpression = nullptr;

public:
  friend class ASTStmtReader;

  ExpressionTraitExpr(SourceLocation loc, ExpressionTrait et,
                     Expr *queried, bool value,
                     SourceLocation rparen, QualType resultType)
      : Expr(ExpressionTraitExprClass, resultType, VK_RValue, OK_Ordinary,
             false, // Not type-dependent
             // Value-dependent if the argument is type-dependent.
             queried->isTypeDependent(),
             queried->isInstantiationDependent(),
             queried->containsUnexpandedParameterPack()),
        ET(et), Value(value), Loc(loc), RParen(rparen),
        QueriedExpression(queried) {}

  explicit ExpressionTraitExpr(EmptyShell Empty)
      : Expr(ExpressionTraitExprClass, Empty), ET(0), Value(false) {}

  SourceLocation getLocStart() const LLVM_READONLY { return Loc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return RParen; }

  ExpressionTrait getTrait() const { return static_cast<ExpressionTrait>(ET); }

  Expr *getQueriedExpression() const { return QueriedExpression; }

  bool getValue() const { return Value; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ExpressionTraitExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief A reference to an overloaded function set, either an
/// \c UnresolvedLookupExpr or an \c UnresolvedMemberExpr.
class OverloadExpr : public Expr {
  /// \brief The common name of these declarations.
  DeclarationNameInfo NameInfo;

  /// \brief The nested-name-specifier that qualifies the name, if any.
  NestedNameSpecifierLoc QualifierLoc;

  /// The results.  These are undesugared, which is to say, they may
  /// include UsingShadowDecls.  Access is relative to the naming
  /// class.
  // FIXME: Allocate this data after the OverloadExpr subclass.
  DeclAccessPair *Results = nullptr;

  unsigned NumResults = 0;

protected:
  /// \brief Whether the name includes info for explicit template
  /// keyword and arguments.
  bool HasTemplateKWAndArgsInfo = false;

  OverloadExpr(StmtClass K, const ASTContext &C,
               NestedNameSpecifierLoc QualifierLoc,
               SourceLocation TemplateKWLoc,
               const DeclarationNameInfo &NameInfo,
               const TemplateArgumentListInfo *TemplateArgs,
               UnresolvedSetIterator Begin, UnresolvedSetIterator End,
               bool KnownDependent,
               bool KnownInstantiationDependent,
               bool KnownContainsUnexpandedParameterPack);

  OverloadExpr(StmtClass K, EmptyShell Empty) : Expr(K, Empty) {}

  /// \brief Return the optional template keyword and arguments info.
  ASTTemplateKWAndArgsInfo *
  getTrailingASTTemplateKWAndArgsInfo(); // defined far below.

  /// \brief Return the optional template keyword and arguments info.
  const ASTTemplateKWAndArgsInfo *getTrailingASTTemplateKWAndArgsInfo() const {
    return const_cast<OverloadExpr *>(this)
        ->getTrailingASTTemplateKWAndArgsInfo();
  }

  /// Return the optional template arguments.
  TemplateArgumentLoc *getTrailingTemplateArgumentLoc(); // defined far below

  void initializeResults(const ASTContext &C,
                         UnresolvedSetIterator Begin,
                         UnresolvedSetIterator End);

public:
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  struct FindResult {
    OverloadExpr *Expression;
    bool IsAddressOfOperand;
    bool HasFormOfMemberPointer;
  };

  /// \brief Finds the overloaded expression in the given expression \p E of
  /// OverloadTy.
  ///
  /// \return the expression (which must be there) and true if it has
  /// the particular form of a member pointer expression
  static FindResult find(Expr *E) {
    assert(E->getType()->isSpecificBuiltinType(BuiltinType::Overload));

    FindResult Result;

    E = E->IgnoreParens();
    if (isa<UnaryOperator>(E)) {
      assert(cast<UnaryOperator>(E)->getOpcode() == UO_AddrOf);
      E = cast<UnaryOperator>(E)->getSubExpr();
      auto *Ovl = cast<OverloadExpr>(E->IgnoreParens());

      Result.HasFormOfMemberPointer = (E == Ovl && Ovl->getQualifier());
      Result.IsAddressOfOperand = true;
      Result.Expression = Ovl;
    } else {
      Result.HasFormOfMemberPointer = false;
      Result.IsAddressOfOperand = false;
      Result.Expression = cast<OverloadExpr>(E);
    }

    return Result;
  }

  /// \brief Gets the naming class of this lookup, if any.
  CXXRecordDecl *getNamingClass() const;

  using decls_iterator = UnresolvedSetImpl::iterator;

  decls_iterator decls_begin() const { return UnresolvedSetIterator(Results); }
  decls_iterator decls_end() const {
    return UnresolvedSetIterator(Results + NumResults);
  }
  llvm::iterator_range<decls_iterator> decls() const {
    return llvm::make_range(decls_begin(), decls_end());
  }

  /// \brief Gets the number of declarations in the unresolved set.
  unsigned getNumDecls() const { return NumResults; }

  /// \brief Gets the full name info.
  const DeclarationNameInfo &getNameInfo() const { return NameInfo; }

  /// \brief Gets the name looked up.
  DeclarationName getName() const { return NameInfo.getName(); }

  /// \brief Gets the location of the name.
  SourceLocation getNameLoc() const { return NameInfo.getLoc(); }

  /// \brief Fetches the nested-name qualifier, if one was given.
  NestedNameSpecifier *getQualifier() const {
    return QualifierLoc.getNestedNameSpecifier();
  }

  /// \brief Fetches the nested-name qualifier with source-location
  /// information, if one was given.
  NestedNameSpecifierLoc getQualifierLoc() const { return QualifierLoc; }

  /// \brief Retrieve the location of the template keyword preceding
  /// this name, if any.
  SourceLocation getTemplateKeywordLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingASTTemplateKWAndArgsInfo()->TemplateKWLoc;
  }

  /// \brief Retrieve the location of the left angle bracket starting the
  /// explicit template argument list following the name, if any.
  SourceLocation getLAngleLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingASTTemplateKWAndArgsInfo()->LAngleLoc;
  }

  /// \brief Retrieve the location of the right angle bracket ending the
  /// explicit template argument list following the name, if any.
  SourceLocation getRAngleLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingASTTemplateKWAndArgsInfo()->RAngleLoc;
  }

  /// \brief Determines whether the name was preceded by the template keyword.
  bool hasTemplateKeyword() const { return getTemplateKeywordLoc().isValid(); }

  /// \brief Determines whether this expression had explicit template arguments.
  bool hasExplicitTemplateArgs() const { return getLAngleLoc().isValid(); }

  TemplateArgumentLoc const *getTemplateArgs() const {
    if (!hasExplicitTemplateArgs())
      return nullptr;
    return const_cast<OverloadExpr *>(this)->getTrailingTemplateArgumentLoc();
  }

  unsigned getNumTemplateArgs() const {
    if (!hasExplicitTemplateArgs())
      return 0;

    return getTrailingASTTemplateKWAndArgsInfo()->NumTemplateArgs;
  }

  ArrayRef<TemplateArgumentLoc> template_arguments() const {
    return {getTemplateArgs(), getNumTemplateArgs()};
  }

  /// \brief Copies the template arguments into the given structure.
  void copyTemplateArgumentsInto(TemplateArgumentListInfo &List) const {
    if (hasExplicitTemplateArgs())
      getTrailingASTTemplateKWAndArgsInfo()->copyInto(getTemplateArgs(), List);
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == UnresolvedLookupExprClass ||
           T->getStmtClass() == UnresolvedMemberExprClass;
  }
};

/// \brief A reference to a name which we were able to look up during
/// parsing but could not resolve to a specific declaration.
///
/// This arises in several ways:
///   * we might be waiting for argument-dependent lookup;
///   * the name might resolve to an overloaded function;
/// and eventually:
///   * the lookup might have included a function template.
///
/// These never include UnresolvedUsingValueDecls, which are always class
/// members and therefore appear only in UnresolvedMemberLookupExprs.
class UnresolvedLookupExpr final
    : public OverloadExpr,
      private llvm::TrailingObjects<
          UnresolvedLookupExpr, ASTTemplateKWAndArgsInfo, TemplateArgumentLoc> {
  friend class ASTStmtReader;
  friend class OverloadExpr;
  friend TrailingObjects;

  /// True if these lookup results should be extended by
  /// argument-dependent lookup if this is the operand of a function
  /// call.
  bool RequiresADL = false;

  /// True if these lookup results are overloaded.  This is pretty
  /// trivially rederivable if we urgently need to kill this field.
  bool Overloaded = false;

  /// The naming class (C++ [class.access.base]p5) of the lookup, if
  /// any.  This can generally be recalculated from the context chain,
  /// but that can be fairly expensive for unqualified lookups.  If we
  /// want to improve memory use here, this could go in a union
  /// against the qualified-lookup bits.
  CXXRecordDecl *NamingClass = nullptr;

  UnresolvedLookupExpr(const ASTContext &C,
                       CXXRecordDecl *NamingClass,
                       NestedNameSpecifierLoc QualifierLoc,
                       SourceLocation TemplateKWLoc,
                       const DeclarationNameInfo &NameInfo,
                       bool RequiresADL, bool Overloaded,
                       const TemplateArgumentListInfo *TemplateArgs,
                       UnresolvedSetIterator Begin, UnresolvedSetIterator End)
      : OverloadExpr(UnresolvedLookupExprClass, C, QualifierLoc, TemplateKWLoc,
                     NameInfo, TemplateArgs, Begin, End, false, false, false),
        RequiresADL(RequiresADL),
        Overloaded(Overloaded), NamingClass(NamingClass) {}

  UnresolvedLookupExpr(EmptyShell Empty)
      : OverloadExpr(UnresolvedLookupExprClass, Empty) {}

  size_t numTrailingObjects(OverloadToken<ASTTemplateKWAndArgsInfo>) const {
    return HasTemplateKWAndArgsInfo ? 1 : 0;
  }

public:
  static UnresolvedLookupExpr *Create(const ASTContext &C,
                                      CXXRecordDecl *NamingClass,
                                      NestedNameSpecifierLoc QualifierLoc,
                                      const DeclarationNameInfo &NameInfo,
                                      bool ADL, bool Overloaded,
                                      UnresolvedSetIterator Begin,
                                      UnresolvedSetIterator End) {
    return new(C) UnresolvedLookupExpr(C, NamingClass, QualifierLoc,
                                       SourceLocation(), NameInfo,
                                       ADL, Overloaded, nullptr, Begin, End);
  }

  static UnresolvedLookupExpr *Create(const ASTContext &C,
                                      CXXRecordDecl *NamingClass,
                                      NestedNameSpecifierLoc QualifierLoc,
                                      SourceLocation TemplateKWLoc,
                                      const DeclarationNameInfo &NameInfo,
                                      bool ADL,
                                      const TemplateArgumentListInfo *Args,
                                      UnresolvedSetIterator Begin,
                                      UnresolvedSetIterator End);

  static UnresolvedLookupExpr *CreateEmpty(const ASTContext &C,
                                           bool HasTemplateKWAndArgsInfo,
                                           unsigned NumTemplateArgs);

  /// True if this declaration should be extended by
  /// argument-dependent lookup.
  bool requiresADL() const { return RequiresADL; }

  /// True if this lookup is overloaded.
  bool isOverloaded() const { return Overloaded; }

  /// Gets the 'naming class' (in the sense of C++0x
  /// [class.access.base]p5) of the lookup.  This is the scope
  /// that was looked in to find these results.
  CXXRecordDecl *getNamingClass() const { return NamingClass; }

  SourceLocation getLocStart() const LLVM_READONLY {
    if (NestedNameSpecifierLoc l = getQualifierLoc())
      return l.getBeginLoc();
    return getNameInfo().getLocStart();
  }

  SourceLocation getLocEnd() const LLVM_READONLY {
    if (hasExplicitTemplateArgs())
      return getRAngleLoc();
    return getNameInfo().getLocEnd();
  }

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == UnresolvedLookupExprClass;
  }
};

/// \brief A qualified reference to a name whose declaration cannot
/// yet be resolved.
///
/// DependentScopeDeclRefExpr is similar to DeclRefExpr in that
/// it expresses a reference to a declaration such as
/// X<T>::value. The difference, however, is that an
/// DependentScopeDeclRefExpr node is used only within C++ templates when
/// the qualification (e.g., X<T>::) refers to a dependent type. In
/// this case, X<T>::value cannot resolve to a declaration because the
/// declaration will differ from one instantiation of X<T> to the
/// next. Therefore, DependentScopeDeclRefExpr keeps track of the
/// qualifier (X<T>::) and the name of the entity being referenced
/// ("value"). Such expressions will instantiate to a DeclRefExpr once the
/// declaration can be found.
class DependentScopeDeclRefExpr final
    : public Expr,
      private llvm::TrailingObjects<DependentScopeDeclRefExpr,
                                    ASTTemplateKWAndArgsInfo,
                                    TemplateArgumentLoc> {
  /// \brief The nested-name-specifier that qualifies this unresolved
  /// declaration name.
  NestedNameSpecifierLoc QualifierLoc;

  /// \brief The name of the entity we will be referencing.
  DeclarationNameInfo NameInfo;

  /// \brief Whether the name includes info for explicit template
  /// keyword and arguments.
  bool HasTemplateKWAndArgsInfo;

  DependentScopeDeclRefExpr(QualType T,
                            NestedNameSpecifierLoc QualifierLoc,
                            SourceLocation TemplateKWLoc,
                            const DeclarationNameInfo &NameInfo,
                            const TemplateArgumentListInfo *Args);

  size_t numTrailingObjects(OverloadToken<ASTTemplateKWAndArgsInfo>) const {
    return HasTemplateKWAndArgsInfo ? 1 : 0;
  }

public:
  friend class ASTStmtReader;
  friend class ASTStmtWriter;
  friend TrailingObjects;

  static DependentScopeDeclRefExpr *Create(const ASTContext &C,
                                           NestedNameSpecifierLoc QualifierLoc,
                                           SourceLocation TemplateKWLoc,
                                           const DeclarationNameInfo &NameInfo,
                              const TemplateArgumentListInfo *TemplateArgs);

  static DependentScopeDeclRefExpr *CreateEmpty(const ASTContext &C,
                                                bool HasTemplateKWAndArgsInfo,
                                                unsigned NumTemplateArgs);

  /// \brief Retrieve the name that this expression refers to.
  const DeclarationNameInfo &getNameInfo() const { return NameInfo; }

  /// \brief Retrieve the name that this expression refers to.
  DeclarationName getDeclName() const { return NameInfo.getName(); }

  /// \brief Retrieve the location of the name within the expression.
  ///
  /// For example, in "X<T>::value" this is the location of "value".
  SourceLocation getLocation() const { return NameInfo.getLoc(); }

  /// \brief Retrieve the nested-name-specifier that qualifies the
  /// name, with source location information.
  NestedNameSpecifierLoc getQualifierLoc() const { return QualifierLoc; }

  /// \brief Retrieve the nested-name-specifier that qualifies this
  /// declaration.
  NestedNameSpecifier *getQualifier() const {
    return QualifierLoc.getNestedNameSpecifier();
  }

  /// \brief Retrieve the location of the template keyword preceding
  /// this name, if any.
  SourceLocation getTemplateKeywordLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingObjects<ASTTemplateKWAndArgsInfo>()->TemplateKWLoc;
  }

  /// \brief Retrieve the location of the left angle bracket starting the
  /// explicit template argument list following the name, if any.
  SourceLocation getLAngleLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingObjects<ASTTemplateKWAndArgsInfo>()->LAngleLoc;
  }

  /// \brief Retrieve the location of the right angle bracket ending the
  /// explicit template argument list following the name, if any.
  SourceLocation getRAngleLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingObjects<ASTTemplateKWAndArgsInfo>()->RAngleLoc;
  }

  /// Determines whether the name was preceded by the template keyword.
  bool hasTemplateKeyword() const { return getTemplateKeywordLoc().isValid(); }

  /// Determines whether this lookup had explicit template arguments.
  bool hasExplicitTemplateArgs() const { return getLAngleLoc().isValid(); }

  /// \brief Copies the template arguments (if present) into the given
  /// structure.
  void copyTemplateArgumentsInto(TemplateArgumentListInfo &List) const {
    if (hasExplicitTemplateArgs())
      getTrailingObjects<ASTTemplateKWAndArgsInfo>()->copyInto(
          getTrailingObjects<TemplateArgumentLoc>(), List);
  }

  TemplateArgumentLoc const *getTemplateArgs() const {
    if (!hasExplicitTemplateArgs())
      return nullptr;

    return getTrailingObjects<TemplateArgumentLoc>();
  }

  unsigned getNumTemplateArgs() const {
    if (!hasExplicitTemplateArgs())
      return 0;

    return getTrailingObjects<ASTTemplateKWAndArgsInfo>()->NumTemplateArgs;
  }

  ArrayRef<TemplateArgumentLoc> template_arguments() const {
    return {getTemplateArgs(), getNumTemplateArgs()};
  }

  /// Note: getLocStart() is the start of the whole DependentScopeDeclRefExpr,
  /// and differs from getLocation().getStart().
  SourceLocation getLocStart() const LLVM_READONLY {
    return QualifierLoc.getBeginLoc();
  }

  SourceLocation getLocEnd() const LLVM_READONLY {
    if (hasExplicitTemplateArgs())
      return getRAngleLoc();
    return getLocation();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == DependentScopeDeclRefExprClass;
  }

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// Represents an expression -- generally a full-expression -- that
/// introduces cleanups to be run at the end of the sub-expression's
/// evaluation.  The most common source of expression-introduced
/// cleanups is temporary objects in C++, but several other kinds of
/// expressions can create cleanups, including basically every
/// call in ARC that returns an Objective-C pointer.
///
/// This expression also tracks whether the sub-expression contains a
/// potentially-evaluated block literal.  The lifetime of a block
/// literal is the extent of the enclosing scope.
class ExprWithCleanups final
    : public Expr,
      private llvm::TrailingObjects<ExprWithCleanups, BlockDecl *> {
public:
  /// The type of objects that are kept in the cleanup.
  /// It's useful to remember the set of blocks;  we could also
  /// remember the set of temporaries, but there's currently
  /// no need.
  using CleanupObject = BlockDecl *;

private:
  friend class ASTStmtReader;
  friend TrailingObjects;

  Stmt *SubExpr;

  ExprWithCleanups(EmptyShell, unsigned NumObjects);
  ExprWithCleanups(Expr *SubExpr, bool CleanupsHaveSideEffects,
                   ArrayRef<CleanupObject> Objects);

public:
  static ExprWithCleanups *Create(const ASTContext &C, EmptyShell empty,
                                  unsigned numObjects);

  static ExprWithCleanups *Create(const ASTContext &C, Expr *subexpr,
                                  bool CleanupsHaveSideEffects,
                                  ArrayRef<CleanupObject> objects);

  ArrayRef<CleanupObject> getObjects() const {
    return llvm::makeArrayRef(getTrailingObjects<CleanupObject>(),
                              getNumObjects());
  }

  unsigned getNumObjects() const { return ExprWithCleanupsBits.NumObjects; }

  CleanupObject getObject(unsigned i) const {
    assert(i < getNumObjects() && "Index out of range");
    return getObjects()[i];
  }

  Expr *getSubExpr() { return cast<Expr>(SubExpr); }
  const Expr *getSubExpr() const { return cast<Expr>(SubExpr); }

  bool cleanupsHaveSideEffects() const {
    return ExprWithCleanupsBits.CleanupsHaveSideEffects;
  }

  /// As with any mutator of the AST, be very careful
  /// when modifying an existing AST to preserve its invariants.
  void setSubExpr(Expr *E) { SubExpr = E; }

  SourceLocation getLocStart() const LLVM_READONLY {
    return SubExpr->getLocStart();
  }

  SourceLocation getLocEnd() const LLVM_READONLY { return SubExpr->getLocEnd();}

  // Implement isa/cast/dyncast/etc.
  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ExprWithCleanupsClass;
  }

  // Iterators
  child_range children() { return child_range(&SubExpr, &SubExpr + 1); }
};

/// \brief Describes an explicit type conversion that uses functional
/// notion but could not be resolved because one or more arguments are
/// type-dependent.
///
/// The explicit type conversions expressed by
/// CXXUnresolvedConstructExpr have the form <tt>T(a1, a2, ..., aN)</tt>,
/// where \c T is some type and \c a1, \c a2, ..., \c aN are values, and
/// either \c T is a dependent type or one or more of the <tt>a</tt>'s is
/// type-dependent. For example, this would occur in a template such
/// as:
///
/// \code
///   template<typename T, typename A1>
///   inline T make_a(const A1& a1) {
///     return T(a1);
///   }
/// \endcode
///
/// When the returned expression is instantiated, it may resolve to a
/// constructor call, conversion function call, or some kind of type
/// conversion.
class CXXUnresolvedConstructExpr final
    : public Expr,
      private llvm::TrailingObjects<CXXUnresolvedConstructExpr, Expr *> {
  friend class ASTStmtReader;
  friend TrailingObjects;

  /// \brief The type being constructed.
  TypeSourceInfo *Type = nullptr;

  /// \brief The location of the left parentheses ('(').
  SourceLocation LParenLoc;

  /// \brief The location of the right parentheses (')').
  SourceLocation RParenLoc;

  /// \brief The number of arguments used to construct the type.
  unsigned NumArgs;

  CXXUnresolvedConstructExpr(TypeSourceInfo *Type,
                             SourceLocation LParenLoc,
                             ArrayRef<Expr*> Args,
                             SourceLocation RParenLoc);

  CXXUnresolvedConstructExpr(EmptyShell Empty, unsigned NumArgs)
      : Expr(CXXUnresolvedConstructExprClass, Empty), NumArgs(NumArgs) {}

public:
  static CXXUnresolvedConstructExpr *Create(const ASTContext &C,
                                            TypeSourceInfo *Type,
                                            SourceLocation LParenLoc,
                                            ArrayRef<Expr*> Args,
                                            SourceLocation RParenLoc);

  static CXXUnresolvedConstructExpr *CreateEmpty(const ASTContext &C,
                                                 unsigned NumArgs);

  /// \brief Retrieve the type that is being constructed, as specified
  /// in the source code.
  QualType getTypeAsWritten() const { return Type->getType(); }

  /// \brief Retrieve the type source information for the type being
  /// constructed.
  TypeSourceInfo *getTypeSourceInfo() const { return Type; }

  /// \brief Retrieve the location of the left parentheses ('(') that
  /// precedes the argument list.
  SourceLocation getLParenLoc() const { return LParenLoc; }
  void setLParenLoc(SourceLocation L) { LParenLoc = L; }

  /// \brief Retrieve the location of the right parentheses (')') that
  /// follows the argument list.
  SourceLocation getRParenLoc() const { return RParenLoc; }
  void setRParenLoc(SourceLocation L) { RParenLoc = L; }

  /// Determine whether this expression models list-initialization.
  /// If so, there will be exactly one subexpression, which will be
  /// an InitListExpr.
  bool isListInitialization() const { return LParenLoc.isInvalid(); }

  /// \brief Retrieve the number of arguments.
  unsigned arg_size() const { return NumArgs; }

  using arg_iterator = Expr **;

  arg_iterator arg_begin() { return getTrailingObjects<Expr *>(); }
  arg_iterator arg_end() { return arg_begin() + NumArgs; }

  using const_arg_iterator = const Expr* const *;

  const_arg_iterator arg_begin() const { return getTrailingObjects<Expr *>(); }
  const_arg_iterator arg_end() const {
    return arg_begin() + NumArgs;
  }

  Expr *getArg(unsigned I) {
    assert(I < NumArgs && "Argument index out-of-range");
    return *(arg_begin() + I);
  }

  const Expr *getArg(unsigned I) const {
    assert(I < NumArgs && "Argument index out-of-range");
    return *(arg_begin() + I);
  }

  void setArg(unsigned I, Expr *E) {
    assert(I < NumArgs && "Argument index out-of-range");
    *(arg_begin() + I) = E;
  }

  SourceLocation getLocStart() const LLVM_READONLY;

  SourceLocation getLocEnd() const LLVM_READONLY {
    if (!RParenLoc.isValid() && NumArgs > 0)
      return getArg(NumArgs - 1)->getLocEnd();
    return RParenLoc;
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXUnresolvedConstructExprClass;
  }

  // Iterators
  child_range children() {
    auto **begin = reinterpret_cast<Stmt **>(arg_begin());
    return child_range(begin, begin + NumArgs);
  }
};

/// \brief Represents a C++ member access expression where the actual
/// member referenced could not be resolved because the base
/// expression or the member name was dependent.
///
/// Like UnresolvedMemberExprs, these can be either implicit or
/// explicit accesses.  It is only possible to get one of these with
/// an implicit access if a qualifier is provided.
class CXXDependentScopeMemberExpr final
    : public Expr,
      private llvm::TrailingObjects<CXXDependentScopeMemberExpr,
                                    ASTTemplateKWAndArgsInfo,
                                    TemplateArgumentLoc> {
  /// \brief The expression for the base pointer or class reference,
  /// e.g., the \c x in x.f.  Can be null in implicit accesses.
  Stmt *Base;

  /// \brief The type of the base expression.  Never null, even for
  /// implicit accesses.
  QualType BaseType;

  /// \brief Whether this member expression used the '->' operator or
  /// the '.' operator.
  bool IsArrow : 1;

  /// \brief Whether this member expression has info for explicit template
  /// keyword and arguments.
  bool HasTemplateKWAndArgsInfo : 1;

  /// \brief The location of the '->' or '.' operator.
  SourceLocation OperatorLoc;

  /// \brief The nested-name-specifier that precedes the member name, if any.
  NestedNameSpecifierLoc QualifierLoc;

  /// \brief In a qualified member access expression such as t->Base::f, this
  /// member stores the resolves of name lookup in the context of the member
  /// access expression, to be used at instantiation time.
  ///
  /// FIXME: This member, along with the QualifierLoc, could
  /// be stuck into a structure that is optionally allocated at the end of
  /// the CXXDependentScopeMemberExpr, to save space in the common case.
  NamedDecl *FirstQualifierFoundInScope;

  /// \brief The member to which this member expression refers, which
  /// can be name, overloaded operator, or destructor.
  ///
  /// FIXME: could also be a template-id
  DeclarationNameInfo MemberNameInfo;

  size_t numTrailingObjects(OverloadToken<ASTTemplateKWAndArgsInfo>) const {
    return HasTemplateKWAndArgsInfo ? 1 : 0;
  }

  CXXDependentScopeMemberExpr(const ASTContext &C, Expr *Base,
                              QualType BaseType, bool IsArrow,
                              SourceLocation OperatorLoc,
                              NestedNameSpecifierLoc QualifierLoc,
                              SourceLocation TemplateKWLoc,
                              NamedDecl *FirstQualifierFoundInScope,
                              DeclarationNameInfo MemberNameInfo,
                              const TemplateArgumentListInfo *TemplateArgs);

public:
  friend class ASTStmtReader;
  friend class ASTStmtWriter;
  friend TrailingObjects;

  CXXDependentScopeMemberExpr(const ASTContext &C, Expr *Base,
                              QualType BaseType, bool IsArrow,
                              SourceLocation OperatorLoc,
                              NestedNameSpecifierLoc QualifierLoc,
                              NamedDecl *FirstQualifierFoundInScope,
                              DeclarationNameInfo MemberNameInfo);

  static CXXDependentScopeMemberExpr *
  Create(const ASTContext &C, Expr *Base, QualType BaseType, bool IsArrow,
         SourceLocation OperatorLoc, NestedNameSpecifierLoc QualifierLoc,
         SourceLocation TemplateKWLoc, NamedDecl *FirstQualifierFoundInScope,
         DeclarationNameInfo MemberNameInfo,
         const TemplateArgumentListInfo *TemplateArgs);

  static CXXDependentScopeMemberExpr *
  CreateEmpty(const ASTContext &C, bool HasTemplateKWAndArgsInfo,
              unsigned NumTemplateArgs);

  /// \brief True if this is an implicit access, i.e. one in which the
  /// member being accessed was not written in the source.  The source
  /// location of the operator is invalid in this case.
  bool isImplicitAccess() const;

  /// \brief Retrieve the base object of this member expressions,
  /// e.g., the \c x in \c x.m.
  Expr *getBase() const {
    assert(!isImplicitAccess());
    return cast<Expr>(Base);
  }

  QualType getBaseType() const { return BaseType; }

  /// \brief Determine whether this member expression used the '->'
  /// operator; otherwise, it used the '.' operator.
  bool isArrow() const { return IsArrow; }

  /// \brief Retrieve the location of the '->' or '.' operator.
  SourceLocation getOperatorLoc() const { return OperatorLoc; }

  /// \brief Retrieve the nested-name-specifier that qualifies the member
  /// name.
  NestedNameSpecifier *getQualifier() const {
    return QualifierLoc.getNestedNameSpecifier();
  }

  /// \brief Retrieve the nested-name-specifier that qualifies the member
  /// name, with source location information.
  NestedNameSpecifierLoc getQualifierLoc() const { return QualifierLoc; }

  /// \brief Retrieve the first part of the nested-name-specifier that was
  /// found in the scope of the member access expression when the member access
  /// was initially parsed.
  ///
  /// This function only returns a useful result when member access expression
  /// uses a qualified member name, e.g., "x.Base::f". Here, the declaration
  /// returned by this function describes what was found by unqualified name
  /// lookup for the identifier "Base" within the scope of the member access
  /// expression itself. At template instantiation time, this information is
  /// combined with the results of name lookup into the type of the object
  /// expression itself (the class type of x).
  NamedDecl *getFirstQualifierFoundInScope() const {
    return FirstQualifierFoundInScope;
  }

  /// \brief Retrieve the name of the member that this expression
  /// refers to.
  const DeclarationNameInfo &getMemberNameInfo() const {
    return MemberNameInfo;
  }

  /// \brief Retrieve the name of the member that this expression
  /// refers to.
  DeclarationName getMember() const { return MemberNameInfo.getName(); }

  // \brief Retrieve the location of the name of the member that this
  // expression refers to.
  SourceLocation getMemberLoc() const { return MemberNameInfo.getLoc(); }

  /// \brief Retrieve the location of the template keyword preceding the
  /// member name, if any.
  SourceLocation getTemplateKeywordLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingObjects<ASTTemplateKWAndArgsInfo>()->TemplateKWLoc;
  }

  /// \brief Retrieve the location of the left angle bracket starting the
  /// explicit template argument list following the member name, if any.
  SourceLocation getLAngleLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingObjects<ASTTemplateKWAndArgsInfo>()->LAngleLoc;
  }

  /// \brief Retrieve the location of the right angle bracket ending the
  /// explicit template argument list following the member name, if any.
  SourceLocation getRAngleLoc() const {
    if (!HasTemplateKWAndArgsInfo) return SourceLocation();
    return getTrailingObjects<ASTTemplateKWAndArgsInfo>()->RAngleLoc;
  }

  /// Determines whether the member name was preceded by the template keyword.
  bool hasTemplateKeyword() const { return getTemplateKeywordLoc().isValid(); }

  /// \brief Determines whether this member expression actually had a C++
  /// template argument list explicitly specified, e.g., x.f<int>.
  bool hasExplicitTemplateArgs() const { return getLAngleLoc().isValid(); }

  /// \brief Copies the template arguments (if present) into the given
  /// structure.
  void copyTemplateArgumentsInto(TemplateArgumentListInfo &List) const {
    if (hasExplicitTemplateArgs())
      getTrailingObjects<ASTTemplateKWAndArgsInfo>()->copyInto(
          getTrailingObjects<TemplateArgumentLoc>(), List);
  }

  /// \brief Retrieve the template arguments provided as part of this
  /// template-id.
  const TemplateArgumentLoc *getTemplateArgs() const {
    if (!hasExplicitTemplateArgs())
      return nullptr;

    return getTrailingObjects<TemplateArgumentLoc>();
  }

  /// \brief Retrieve the number of template arguments provided as part of this
  /// template-id.
  unsigned getNumTemplateArgs() const {
    if (!hasExplicitTemplateArgs())
      return 0;

    return getTrailingObjects<ASTTemplateKWAndArgsInfo>()->NumTemplateArgs;
  }

  ArrayRef<TemplateArgumentLoc> template_arguments() const {
    return {getTemplateArgs(), getNumTemplateArgs()};
  }

  SourceLocation getLocStart() const LLVM_READONLY {
    if (!isImplicitAccess())
      return Base->getLocStart();
    if (getQualifier())
      return getQualifierLoc().getBeginLoc();
    return MemberNameInfo.getBeginLoc();
  }

  SourceLocation getLocEnd() const LLVM_READONLY {
    if (hasExplicitTemplateArgs())
      return getRAngleLoc();
    return MemberNameInfo.getEndLoc();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXDependentScopeMemberExprClass;
  }

  // Iterators
  child_range children() {
    if (isImplicitAccess())
      return child_range(child_iterator(), child_iterator());
    return child_range(&Base, &Base + 1);
  }
};

/// \brief Represents a C++ member access expression for which lookup
/// produced a set of overloaded functions.
///
/// The member access may be explicit or implicit:
/// \code
///    struct A {
///      int a, b;
///      int explicitAccess() { return this->a + this->A::b; }
///      int implicitAccess() { return a + A::b; }
///    };
/// \endcode
///
/// In the final AST, an explicit access always becomes a MemberExpr.
/// An implicit access may become either a MemberExpr or a
/// DeclRefExpr, depending on whether the member is static.
class UnresolvedMemberExpr final
    : public OverloadExpr,
      private llvm::TrailingObjects<
          UnresolvedMemberExpr, ASTTemplateKWAndArgsInfo, TemplateArgumentLoc> {
  friend class ASTStmtReader;
  friend class OverloadExpr;
  friend TrailingObjects;

  /// \brief Whether this member expression used the '->' operator or
  /// the '.' operator.
  bool IsArrow : 1;

  /// \brief Whether the lookup results contain an unresolved using
  /// declaration.
  bool HasUnresolvedUsing : 1;

  /// \brief The expression for the base pointer or class reference,
  /// e.g., the \c x in x.f.
  ///
  /// This can be null if this is an 'unbased' member expression.
  Stmt *Base = nullptr;

  /// \brief The type of the base expression; never null.
  QualType BaseType;

  /// \brief The location of the '->' or '.' operator.
  SourceLocation OperatorLoc;

  UnresolvedMemberExpr(const ASTContext &C, bool HasUnresolvedUsing,
                       Expr *Base, QualType BaseType, bool IsArrow,
                       SourceLocation OperatorLoc,
                       NestedNameSpecifierLoc QualifierLoc,
                       SourceLocation TemplateKWLoc,
                       const DeclarationNameInfo &MemberNameInfo,
                       const TemplateArgumentListInfo *TemplateArgs,
                       UnresolvedSetIterator Begin, UnresolvedSetIterator End);

  UnresolvedMemberExpr(EmptyShell Empty)
      : OverloadExpr(UnresolvedMemberExprClass, Empty), IsArrow(false),
        HasUnresolvedUsing(false) {}

  size_t numTrailingObjects(OverloadToken<ASTTemplateKWAndArgsInfo>) const {
    return HasTemplateKWAndArgsInfo ? 1 : 0;
  }

public:
  static UnresolvedMemberExpr *
  Create(const ASTContext &C, bool HasUnresolvedUsing,
         Expr *Base, QualType BaseType, bool IsArrow,
         SourceLocation OperatorLoc,
         NestedNameSpecifierLoc QualifierLoc,
         SourceLocation TemplateKWLoc,
         const DeclarationNameInfo &MemberNameInfo,
         const TemplateArgumentListInfo *TemplateArgs,
         UnresolvedSetIterator Begin, UnresolvedSetIterator End);

  static UnresolvedMemberExpr *
  CreateEmpty(const ASTContext &C, bool HasTemplateKWAndArgsInfo,
              unsigned NumTemplateArgs);

  /// \brief True if this is an implicit access, i.e., one in which the
  /// member being accessed was not written in the source.
  ///
  /// The source location of the operator is invalid in this case.
  bool isImplicitAccess() const;

  /// \brief Retrieve the base object of this member expressions,
  /// e.g., the \c x in \c x.m.
  Expr *getBase() {
    assert(!isImplicitAccess());
    return cast<Expr>(Base);
  }
  const Expr *getBase() const {
    assert(!isImplicitAccess());
    return cast<Expr>(Base);
  }

  QualType getBaseType() const { return BaseType; }

  /// \brief Determine whether the lookup results contain an unresolved using
  /// declaration.
  bool hasUnresolvedUsing() const { return HasUnresolvedUsing; }

  /// \brief Determine whether this member expression used the '->'
  /// operator; otherwise, it used the '.' operator.
  bool isArrow() const { return IsArrow; }

  /// \brief Retrieve the location of the '->' or '.' operator.
  SourceLocation getOperatorLoc() const { return OperatorLoc; }

  /// \brief Retrieve the naming class of this lookup.
  CXXRecordDecl *getNamingClass() const;

  /// \brief Retrieve the full name info for the member that this expression
  /// refers to.
  const DeclarationNameInfo &getMemberNameInfo() const { return getNameInfo(); }

  /// \brief Retrieve the name of the member that this expression
  /// refers to.
  DeclarationName getMemberName() const { return getName(); }

  // \brief Retrieve the location of the name of the member that this
  // expression refers to.
  SourceLocation getMemberLoc() const { return getNameLoc(); }

  // \brief Return the preferred location (the member name) for the arrow when
  // diagnosing a problem with this expression.
  SourceLocation getExprLoc() const LLVM_READONLY { return getMemberLoc(); }

  SourceLocation getLocStart() const LLVM_READONLY {
    if (!isImplicitAccess())
      return Base->getLocStart();
    if (NestedNameSpecifierLoc l = getQualifierLoc())
      return l.getBeginLoc();
    return getMemberNameInfo().getLocStart();
  }

  SourceLocation getLocEnd() const LLVM_READONLY {
    if (hasExplicitTemplateArgs())
      return getRAngleLoc();
    return getMemberNameInfo().getLocEnd();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == UnresolvedMemberExprClass;
  }

  // Iterators
  child_range children() {
    if (isImplicitAccess())
      return child_range(child_iterator(), child_iterator());
    return child_range(&Base, &Base + 1);
  }
};

inline ASTTemplateKWAndArgsInfo *
OverloadExpr::getTrailingASTTemplateKWAndArgsInfo() {
  if (!HasTemplateKWAndArgsInfo)
    return nullptr;

  if (isa<UnresolvedLookupExpr>(this))
    return cast<UnresolvedLookupExpr>(this)
        ->getTrailingObjects<ASTTemplateKWAndArgsInfo>();
  else
    return cast<UnresolvedMemberExpr>(this)
        ->getTrailingObjects<ASTTemplateKWAndArgsInfo>();
}

inline TemplateArgumentLoc *OverloadExpr::getTrailingTemplateArgumentLoc() {
  if (isa<UnresolvedLookupExpr>(this))
    return cast<UnresolvedLookupExpr>(this)
        ->getTrailingObjects<TemplateArgumentLoc>();
  else
    return cast<UnresolvedMemberExpr>(this)
        ->getTrailingObjects<TemplateArgumentLoc>();
}

/// \brief Represents a C++11 noexcept expression (C++ [expr.unary.noexcept]).
///
/// The noexcept expression tests whether a given expression might throw. Its
/// result is a boolean constant.
class CXXNoexceptExpr : public Expr {
  friend class ASTStmtReader;

  bool Value : 1;
  Stmt *Operand;
  SourceRange Range;

public:
  CXXNoexceptExpr(QualType Ty, Expr *Operand, CanThrowResult Val,
                  SourceLocation Keyword, SourceLocation RParen)
      : Expr(CXXNoexceptExprClass, Ty, VK_RValue, OK_Ordinary,
             /*TypeDependent*/false,
             /*ValueDependent*/Val == CT_Dependent,
             Val == CT_Dependent || Operand->isInstantiationDependent(),
             Operand->containsUnexpandedParameterPack()),
        Value(Val == CT_Cannot), Operand(Operand), Range(Keyword, RParen) {}

  CXXNoexceptExpr(EmptyShell Empty) : Expr(CXXNoexceptExprClass, Empty) {}

  Expr *getOperand() const { return static_cast<Expr*>(Operand); }

  SourceLocation getLocStart() const LLVM_READONLY { return Range.getBegin(); }
  SourceLocation getLocEnd() const LLVM_READONLY { return Range.getEnd(); }
  SourceRange getSourceRange() const LLVM_READONLY { return Range; }

  bool getValue() const { return Value; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXNoexceptExprClass;
  }

  // Iterators
  child_range children() { return child_range(&Operand, &Operand + 1); }
};

/// \brief Represents a C++11 pack expansion that produces a sequence of
/// expressions.
///
/// A pack expansion expression contains a pattern (which itself is an
/// expression) followed by an ellipsis. For example:
///
/// \code
/// template<typename F, typename ...Types>
/// void forward(F f, Types &&...args) {
///   f(static_cast<Types&&>(args)...);
/// }
/// \endcode
///
/// Here, the argument to the function object \c f is a pack expansion whose
/// pattern is \c static_cast<Types&&>(args). When the \c forward function
/// template is instantiated, the pack expansion will instantiate to zero or
/// or more function arguments to the function object \c f.
class PackExpansionExpr : public Expr {
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  SourceLocation EllipsisLoc;

  /// \brief The number of expansions that will be produced by this pack
  /// expansion expression, if known.
  ///
  /// When zero, the number of expansions is not known. Otherwise, this value
  /// is the number of expansions + 1.
  unsigned NumExpansions;

  Stmt *Pattern;

public:
  PackExpansionExpr(QualType T, Expr *Pattern, SourceLocation EllipsisLoc,
                    Optional<unsigned> NumExpansions)
      : Expr(PackExpansionExprClass, T, Pattern->getValueKind(),
             Pattern->getObjectKind(), /*TypeDependent=*/true,
             /*ValueDependent=*/true, /*InstantiationDependent=*/true,
             /*ContainsUnexpandedParameterPack=*/false),
        EllipsisLoc(EllipsisLoc),
        NumExpansions(NumExpansions ? *NumExpansions + 1 : 0),
        Pattern(Pattern) {}

  PackExpansionExpr(EmptyShell Empty) : Expr(PackExpansionExprClass, Empty) {}

  /// \brief Retrieve the pattern of the pack expansion.
  Expr *getPattern() { return reinterpret_cast<Expr *>(Pattern); }

  /// \brief Retrieve the pattern of the pack expansion.
  const Expr *getPattern() const { return reinterpret_cast<Expr *>(Pattern); }

  /// \brief Retrieve the location of the ellipsis that describes this pack
  /// expansion.
  SourceLocation getEllipsisLoc() const { return EllipsisLoc; }

  /// \brief Determine the number of expansions that will be produced when
  /// this pack expansion is instantiated, if already known.
  Optional<unsigned> getNumExpansions() const {
    if (NumExpansions)
      return NumExpansions - 1;

    return None;
  }

  SourceLocation getLocStart() const LLVM_READONLY {
    return Pattern->getLocStart();
  }

  SourceLocation getLocEnd() const LLVM_READONLY { return EllipsisLoc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == PackExpansionExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(&Pattern, &Pattern + 1);
  }
};

/// \brief Represents an expression that computes the length of a parameter
/// pack.
///
/// \code
/// template<typename ...Types>
/// struct count {
///   static const unsigned value = sizeof...(Types);
/// };
/// \endcode
class SizeOfPackExpr final
    : public Expr,
      private llvm::TrailingObjects<SizeOfPackExpr, TemplateArgument> {
  friend class ASTStmtReader;
  friend class ASTStmtWriter;
  friend TrailingObjects;

  /// \brief The location of the \c sizeof keyword.
  SourceLocation OperatorLoc;

  /// \brief The location of the name of the parameter pack.
  SourceLocation PackLoc;

  /// \brief The location of the closing parenthesis.
  SourceLocation RParenLoc;

  /// \brief The length of the parameter pack, if known.
  ///
  /// When this expression is not value-dependent, this is the length of
  /// the pack. When the expression was parsed rather than instantiated
  /// (and thus is value-dependent), this is zero.
  ///
  /// After partial substitution into a sizeof...(X) expression (for instance,
  /// within an alias template or during function template argument deduction),
  /// we store a trailing array of partially-substituted TemplateArguments,
  /// and this is the length of that array.
  unsigned Length;

  /// \brief The parameter pack.
  NamedDecl *Pack = nullptr;

  /// \brief Create an expression that computes the length of
  /// the given parameter pack.
  SizeOfPackExpr(QualType SizeType, SourceLocation OperatorLoc, NamedDecl *Pack,
                 SourceLocation PackLoc, SourceLocation RParenLoc,
                 Optional<unsigned> Length, ArrayRef<TemplateArgument> PartialArgs)
      : Expr(SizeOfPackExprClass, SizeType, VK_RValue, OK_Ordinary,
             /*TypeDependent=*/false, /*ValueDependent=*/!Length,
             /*InstantiationDependent=*/!Length,
             /*ContainsUnexpandedParameterPack=*/false),
        OperatorLoc(OperatorLoc), PackLoc(PackLoc), RParenLoc(RParenLoc),
        Length(Length ? *Length : PartialArgs.size()), Pack(Pack) {
    assert((!Length || PartialArgs.empty()) &&
           "have partial args for non-dependent sizeof... expression");
    auto *Args = getTrailingObjects<TemplateArgument>();
    std::uninitialized_copy(PartialArgs.begin(), PartialArgs.end(), Args);
  }

  /// \brief Create an empty expression.
  SizeOfPackExpr(EmptyShell Empty, unsigned NumPartialArgs)
      : Expr(SizeOfPackExprClass, Empty), Length(NumPartialArgs) {}

public:
  static SizeOfPackExpr *Create(ASTContext &Context, SourceLocation OperatorLoc,
                                NamedDecl *Pack, SourceLocation PackLoc,
                                SourceLocation RParenLoc,
                                Optional<unsigned> Length = None,
                                ArrayRef<TemplateArgument> PartialArgs = None);
  static SizeOfPackExpr *CreateDeserialized(ASTContext &Context,
                                            unsigned NumPartialArgs);

  /// \brief Determine the location of the 'sizeof' keyword.
  SourceLocation getOperatorLoc() const { return OperatorLoc; }

  /// \brief Determine the location of the parameter pack.
  SourceLocation getPackLoc() const { return PackLoc; }

  /// \brief Determine the location of the right parenthesis.
  SourceLocation getRParenLoc() const { return RParenLoc; }

  /// \brief Retrieve the parameter pack.
  NamedDecl *getPack() const { return Pack; }

  /// \brief Retrieve the length of the parameter pack.
  ///
  /// This routine may only be invoked when the expression is not
  /// value-dependent.
  unsigned getPackLength() const {
    assert(!isValueDependent() &&
           "Cannot get the length of a value-dependent pack size expression");
    return Length;
  }

  /// \brief Determine whether this represents a partially-substituted sizeof...
  /// expression, such as is produced for:
  ///
  ///   template<typename ...Ts> using X = int[sizeof...(Ts)];
  ///   template<typename ...Us> void f(X<Us..., 1, 2, 3, Us...>);
  bool isPartiallySubstituted() const {
    return isValueDependent() && Length;
  }

  /// \brief Get
  ArrayRef<TemplateArgument> getPartialArguments() const {
    assert(isPartiallySubstituted());
    const auto *Args = getTrailingObjects<TemplateArgument>();
    return llvm::makeArrayRef(Args, Args + Length);
  }

  SourceLocation getLocStart() const LLVM_READONLY { return OperatorLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return RParenLoc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == SizeOfPackExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief Represents a reference to a non-type template parameter
/// that has been substituted with a template argument.
class SubstNonTypeTemplateParmExpr : public Expr {
  friend class ASTReader;
  friend class ASTStmtReader;

  /// \brief The replaced parameter.
  NonTypeTemplateParmDecl *Param;

  /// \brief The replacement expression.
  Stmt *Replacement;

  /// \brief The location of the non-type template parameter reference.
  SourceLocation NameLoc;

  explicit SubstNonTypeTemplateParmExpr(EmptyShell Empty)
      : Expr(SubstNonTypeTemplateParmExprClass, Empty) {}

public:
  SubstNonTypeTemplateParmExpr(QualType type,
                               ExprValueKind valueKind,
                               SourceLocation loc,
                               NonTypeTemplateParmDecl *param,
                               Expr *replacement)
      : Expr(SubstNonTypeTemplateParmExprClass, type, valueKind, OK_Ordinary,
             replacement->isTypeDependent(), replacement->isValueDependent(),
             replacement->isInstantiationDependent(),
             replacement->containsUnexpandedParameterPack()),
        Param(param), Replacement(replacement), NameLoc(loc) {}

  SourceLocation getNameLoc() const { return NameLoc; }
  SourceLocation getLocStart() const LLVM_READONLY { return NameLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return NameLoc; }

  Expr *getReplacement() const { return cast<Expr>(Replacement); }

  NonTypeTemplateParmDecl *getParameter() const { return Param; }

  static bool classof(const Stmt *s) {
    return s->getStmtClass() == SubstNonTypeTemplateParmExprClass;
  }

  // Iterators
  child_range children() { return child_range(&Replacement, &Replacement+1); }
};

/// \brief Represents a reference to a non-type template parameter pack that
/// has been substituted with a non-template argument pack.
///
/// When a pack expansion in the source code contains multiple parameter packs
/// and those parameter packs correspond to different levels of template
/// parameter lists, this node is used to represent a non-type template
/// parameter pack from an outer level, which has already had its argument pack
/// substituted but that still lives within a pack expansion that itself
/// could not be instantiated. When actually performing a substitution into
/// that pack expansion (e.g., when all template parameters have corresponding
/// arguments), this type will be replaced with the appropriate underlying
/// expression at the current pack substitution index.
class SubstNonTypeTemplateParmPackExpr : public Expr {
  friend class ASTReader;
  friend class ASTStmtReader;

  /// \brief The non-type template parameter pack itself.
  NonTypeTemplateParmDecl *Param;

  /// \brief A pointer to the set of template arguments that this
  /// parameter pack is instantiated with.
  const TemplateArgument *Arguments;

  /// \brief The number of template arguments in \c Arguments.
  unsigned NumArguments;

  /// \brief The location of the non-type template parameter pack reference.
  SourceLocation NameLoc;

  explicit SubstNonTypeTemplateParmPackExpr(EmptyShell Empty)
      : Expr(SubstNonTypeTemplateParmPackExprClass, Empty) {}

public:
  SubstNonTypeTemplateParmPackExpr(QualType T,
                                   ExprValueKind ValueKind,
                                   NonTypeTemplateParmDecl *Param,
                                   SourceLocation NameLoc,
                                   const TemplateArgument &ArgPack);

  /// \brief Retrieve the non-type template parameter pack being substituted.
  NonTypeTemplateParmDecl *getParameterPack() const { return Param; }

  /// \brief Retrieve the location of the parameter pack name.
  SourceLocation getParameterPackLocation() const { return NameLoc; }

  /// \brief Retrieve the template argument pack containing the substituted
  /// template arguments.
  TemplateArgument getArgumentPack() const;

  SourceLocation getLocStart() const LLVM_READONLY { return NameLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return NameLoc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == SubstNonTypeTemplateParmPackExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief Represents a reference to a function parameter pack that has been
/// substituted but not yet expanded.
///
/// When a pack expansion contains multiple parameter packs at different levels,
/// this node is used to represent a function parameter pack at an outer level
/// which we have already substituted to refer to expanded parameters, but where
/// the containing pack expansion cannot yet be expanded.
///
/// \code
/// template<typename...Ts> struct S {
///   template<typename...Us> auto f(Ts ...ts) -> decltype(g(Us(ts)...));
/// };
/// template struct S<int, int>;
/// \endcode
class FunctionParmPackExpr final
    : public Expr,
      private llvm::TrailingObjects<FunctionParmPackExpr, ParmVarDecl *> {
  friend class ASTReader;
  friend class ASTStmtReader;
  friend TrailingObjects;

  /// \brief The function parameter pack which was referenced.
  ParmVarDecl *ParamPack;

  /// \brief The location of the function parameter pack reference.
  SourceLocation NameLoc;

  /// \brief The number of expansions of this pack.
  unsigned NumParameters;

  FunctionParmPackExpr(QualType T, ParmVarDecl *ParamPack,
                       SourceLocation NameLoc, unsigned NumParams,
                       ParmVarDecl *const *Params);

public:
  static FunctionParmPackExpr *Create(const ASTContext &Context, QualType T,
                                      ParmVarDecl *ParamPack,
                                      SourceLocation NameLoc,
                                      ArrayRef<ParmVarDecl *> Params);
  static FunctionParmPackExpr *CreateEmpty(const ASTContext &Context,
                                           unsigned NumParams);

  /// \brief Get the parameter pack which this expression refers to.
  ParmVarDecl *getParameterPack() const { return ParamPack; }

  /// \brief Get the location of the parameter pack.
  SourceLocation getParameterPackLocation() const { return NameLoc; }

  /// \brief Iterators over the parameters which the parameter pack expanded
  /// into.
  using iterator = ParmVarDecl * const *;
  iterator begin() const { return getTrailingObjects<ParmVarDecl *>(); }
  iterator end() const { return begin() + NumParameters; }

  /// \brief Get the number of parameters in this parameter pack.
  unsigned getNumExpansions() const { return NumParameters; }

  /// \brief Get an expansion of the parameter pack by index.
  ParmVarDecl *getExpansion(unsigned I) const { return begin()[I]; }

  SourceLocation getLocStart() const LLVM_READONLY { return NameLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return NameLoc; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == FunctionParmPackExprClass;
  }

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief Represents a prvalue temporary that is written into memory so that
/// a reference can bind to it.
///
/// Prvalue expressions are materialized when they need to have an address
/// in memory for a reference to bind to. This happens when binding a
/// reference to the result of a conversion, e.g.,
///
/// \code
/// const int &r = 1.0;
/// \endcode
///
/// Here, 1.0 is implicitly converted to an \c int. That resulting \c int is
/// then materialized via a \c MaterializeTemporaryExpr, and the reference
/// binds to the temporary. \c MaterializeTemporaryExprs are always glvalues
/// (either an lvalue or an xvalue, depending on the kind of reference binding
/// to it), maintaining the invariant that references always bind to glvalues.
///
/// Reference binding and copy-elision can both extend the lifetime of a
/// temporary. When either happens, the expression will also track the
/// declaration which is responsible for the lifetime extension.
class MaterializeTemporaryExpr : public Expr {
private:
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  struct ExtraState {
    /// \brief The temporary-generating expression whose value will be
    /// materialized.
    Stmt *Temporary;

    /// \brief The declaration which lifetime-extended this reference, if any.
    /// Either a VarDecl, or (for a ctor-initializer) a FieldDecl.
    const ValueDecl *ExtendingDecl;

    unsigned ManglingNumber;
  };
  llvm::PointerUnion<Stmt *, ExtraState *> State;

  void initializeExtraState(const ValueDecl *ExtendedBy,
                            unsigned ManglingNumber);

public:
  MaterializeTemporaryExpr(QualType T, Expr *Temporary,
                           bool BoundToLvalueReference)
      : Expr(MaterializeTemporaryExprClass, T,
             BoundToLvalueReference? VK_LValue : VK_XValue, OK_Ordinary,
             Temporary->isTypeDependent(), Temporary->isValueDependent(),
             Temporary->isInstantiationDependent(),
             Temporary->containsUnexpandedParameterPack()),
        State(Temporary) {}

  MaterializeTemporaryExpr(EmptyShell Empty)
      : Expr(MaterializeTemporaryExprClass, Empty) {}

  Stmt *getTemporary() const {
    return State.is<Stmt *>() ? State.get<Stmt *>()
                              : State.get<ExtraState *>()->Temporary;
  }

  /// \brief Retrieve the temporary-generating subexpression whose value will
  /// be materialized into a glvalue.
  Expr *GetTemporaryExpr() const { return static_cast<Expr *>(getTemporary()); }

  /// \brief Retrieve the storage duration for the materialized temporary.
  StorageDuration getStorageDuration() const {
    const ValueDecl *ExtendingDecl = getExtendingDecl();
    if (!ExtendingDecl)
      return SD_FullExpression;
    // FIXME: This is not necessarily correct for a temporary materialized
    // within a default initializer.
    if (isa<FieldDecl>(ExtendingDecl))
      return SD_Automatic;
    // FIXME: This only works because storage class specifiers are not allowed
    // on decomposition declarations.
    if (isa<BindingDecl>(ExtendingDecl))
      return ExtendingDecl->getDeclContext()->isFunctionOrMethod()
                 ? SD_Automatic
                 : SD_Static;
    return cast<VarDecl>(ExtendingDecl)->getStorageDuration();
  }

  /// \brief Get the declaration which triggered the lifetime-extension of this
  /// temporary, if any.
  const ValueDecl *getExtendingDecl() const {
    return State.is<Stmt *>() ? nullptr
                              : State.get<ExtraState *>()->ExtendingDecl;
  }

  void setExtendingDecl(const ValueDecl *ExtendedBy, unsigned ManglingNumber);

  unsigned getManglingNumber() const {
    return State.is<Stmt *>() ? 0 : State.get<ExtraState *>()->ManglingNumber;
  }

  /// \brief Determine whether this materialized temporary is bound to an
  /// lvalue reference; otherwise, it's bound to an rvalue reference.
  bool isBoundToLvalueReference() const {
    return getValueKind() == VK_LValue;
  }

  SourceLocation getLocStart() const LLVM_READONLY {
    return getTemporary()->getLocStart();
  }

  SourceLocation getLocEnd() const LLVM_READONLY {
    return getTemporary()->getLocEnd();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == MaterializeTemporaryExprClass;
  }

  // Iterators
  child_range children() {
    if (State.is<Stmt *>())
      return child_range(State.getAddrOfPtr1(), State.getAddrOfPtr1() + 1);

    auto ES = State.get<ExtraState *>();
    return child_range(&ES->Temporary, &ES->Temporary + 1);
  }
};

/// \brief Represents a folding of a pack over an operator.
///
/// This expression is always dependent and represents a pack expansion of the
/// forms:
///
///    ( expr op ... )
///    ( ... op expr )
///    ( expr op ... op expr )
class CXXFoldExpr : public Expr {
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  SourceLocation LParenLoc;
  SourceLocation EllipsisLoc;
  SourceLocation RParenLoc;
  Stmt *SubExprs[2];
  BinaryOperatorKind Opcode;

public:
  CXXFoldExpr(QualType T, SourceLocation LParenLoc, Expr *LHS,
              BinaryOperatorKind Opcode, SourceLocation EllipsisLoc, Expr *RHS,
              SourceLocation RParenLoc)
      : Expr(CXXFoldExprClass, T, VK_RValue, OK_Ordinary,
             /*Dependent*/ true, true, true,
             /*ContainsUnexpandedParameterPack*/ false),
        LParenLoc(LParenLoc), EllipsisLoc(EllipsisLoc), RParenLoc(RParenLoc),
        Opcode(Opcode) {
    SubExprs[0] = LHS;
    SubExprs[1] = RHS;
  }

  CXXFoldExpr(EmptyShell Empty) : Expr(CXXFoldExprClass, Empty) {}

  Expr *getLHS() const { return static_cast<Expr*>(SubExprs[0]); }
  Expr *getRHS() const { return static_cast<Expr*>(SubExprs[1]); }

  /// Does this produce a right-associated sequence of operators?
  bool isRightFold() const {
    return getLHS() && getLHS()->containsUnexpandedParameterPack();
  }

  /// Does this produce a left-associated sequence of operators?
  bool isLeftFold() const { return !isRightFold(); }

  /// Get the pattern, that is, the operand that contains an unexpanded pack.
  Expr *getPattern() const { return isLeftFold() ? getRHS() : getLHS(); }

  /// Get the operand that doesn't contain a pack, for a binary fold.
  Expr *getInit() const { return isLeftFold() ? getLHS() : getRHS(); }

  SourceLocation getEllipsisLoc() const { return EllipsisLoc; }
  BinaryOperatorKind getOperator() const { return Opcode; }

  SourceLocation getLocStart() const LLVM_READONLY {
    return LParenLoc;
  }

  SourceLocation getLocEnd() const LLVM_READONLY {
    return RParenLoc;
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXFoldExprClass;
  }

  // Iterators
  child_range children() { return child_range(SubExprs, SubExprs + 2); }
};

class CXXRewrittenExpr : public Expr {
  friend class ASTReader;
  friend class ASTWriter;
  friend class ASTStmtReader;
  friend class ASTStmtWriter;

  Stmt *SubExprs[2];

  CXXRewrittenExpr(EmptyShell Empty) : Expr(CXXRewrittenExprClass, Empty) {}

public:
  typedef BinaryOperatorKind Opcode;
  enum RewrittenOperatorKind { ROC_None, ROC_Rewritten, ROC_Synthesized };

public:
  // FIXME(EricWF): Figure out if this will even be built for dependent
  // expressions.
  CXXRewrittenExpr(RewrittenOperatorKind Kind, Expr *Original, Expr *Rewritten)
      : Expr(CXXRewrittenExprClass, Rewritten->getType(),
             Rewritten->getValueKind(), Rewritten->getObjectKind(),
             /*Dependent*/ Rewritten->isTypeDependent(),
             Rewritten->isValueDependent(),
             Original->isInstantiationDependent(),
             Rewritten->containsUnexpandedParameterPack()) {
    SubExprs[0] = Original;
    SubExprs[1] = Rewritten;
  }

  Expr *getOriginalExpr() const { return static_cast<Expr *>(SubExprs[0]); }
  Expr *getRewrittenExpr() const {
    return static_cast<Expr *>(SubExprs[1]);
  }

  SourceLocation getLocStart() const {
    return getOriginalExpr()->getLocStart();
  }
  SourceLocation getLocEnd() const { return getOriginalExpr()->getLocEnd(); }
  SourceLocation getExprLoc() const { return getOriginalExpr()->getExprLoc(); }

  child_range children() { return child_range(SubExprs, SubExprs + 2); }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CXXRewrittenExprClass;
  }
};

/// \brief Represents an expression that might suspend coroutine execution;
/// either a co_await or co_yield expression.
///
/// Evaluation of this expression first evaluates its 'ready' expression. If
/// that returns 'false':
///  -- execution of the coroutine is suspended
///  -- the 'suspend' expression is evaluated
///     -- if the 'suspend' expression returns 'false', the coroutine is
///        resumed
///     -- otherwise, control passes back to the resumer.
/// If the coroutine is not suspended, or when it is resumed, the 'resume'
/// expression is evaluated, and its result is the result of the overall
/// expression.
class CoroutineSuspendExpr : public Expr {
  friend class ASTStmtReader;

  SourceLocation KeywordLoc;

  enum SubExpr { Common, Ready, Suspend, Resume, Count };

  Stmt *SubExprs[SubExpr::Count];
  OpaqueValueExpr *OpaqueValue = nullptr;

public:
  CoroutineSuspendExpr(StmtClass SC, SourceLocation KeywordLoc, Expr *Common,
                       Expr *Ready, Expr *Suspend, Expr *Resume,
                       OpaqueValueExpr *OpaqueValue)
      : Expr(SC, Resume->getType(), Resume->getValueKind(),
             Resume->getObjectKind(), Resume->isTypeDependent(),
             Resume->isValueDependent(), Common->isInstantiationDependent(),
             Common->containsUnexpandedParameterPack()),
        KeywordLoc(KeywordLoc), OpaqueValue(OpaqueValue) {
    SubExprs[SubExpr::Common] = Common;
    SubExprs[SubExpr::Ready] = Ready;
    SubExprs[SubExpr::Suspend] = Suspend;
    SubExprs[SubExpr::Resume] = Resume;
  }

  CoroutineSuspendExpr(StmtClass SC, SourceLocation KeywordLoc, QualType Ty,
                       Expr *Common)
      : Expr(SC, Ty, VK_RValue, OK_Ordinary, true, true, true,
             Common->containsUnexpandedParameterPack()),
        KeywordLoc(KeywordLoc) {
    assert(Common->isTypeDependent() && Ty->isDependentType() &&
           "wrong constructor for non-dependent co_await/co_yield expression");
    SubExprs[SubExpr::Common] = Common;
    SubExprs[SubExpr::Ready] = nullptr;
    SubExprs[SubExpr::Suspend] = nullptr;
    SubExprs[SubExpr::Resume] = nullptr;
  }

  CoroutineSuspendExpr(StmtClass SC, EmptyShell Empty) : Expr(SC, Empty) {
    SubExprs[SubExpr::Common] = nullptr;
    SubExprs[SubExpr::Ready] = nullptr;
    SubExprs[SubExpr::Suspend] = nullptr;
    SubExprs[SubExpr::Resume] = nullptr;
  }

  SourceLocation getKeywordLoc() const { return KeywordLoc; }

  Expr *getCommonExpr() const {
    return static_cast<Expr*>(SubExprs[SubExpr::Common]);
  }

  /// \brief getOpaqueValue - Return the opaque value placeholder.
  OpaqueValueExpr *getOpaqueValue() const { return OpaqueValue; }

  Expr *getReadyExpr() const {
    return static_cast<Expr*>(SubExprs[SubExpr::Ready]);
  }

  Expr *getSuspendExpr() const {
    return static_cast<Expr*>(SubExprs[SubExpr::Suspend]);
  }

  Expr *getResumeExpr() const {
    return static_cast<Expr*>(SubExprs[SubExpr::Resume]);
  }

  SourceLocation getLocStart() const LLVM_READONLY {
    return KeywordLoc;
  }

  SourceLocation getLocEnd() const LLVM_READONLY {
    return getCommonExpr()->getLocEnd();
  }

  child_range children() {
    return child_range(SubExprs, SubExprs + SubExpr::Count);
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CoawaitExprClass ||
           T->getStmtClass() == CoyieldExprClass;
  }
};

/// \brief Represents a 'co_await' expression.
class CoawaitExpr : public CoroutineSuspendExpr {
  friend class ASTStmtReader;

public:
  CoawaitExpr(SourceLocation CoawaitLoc, Expr *Operand, Expr *Ready,
              Expr *Suspend, Expr *Resume, OpaqueValueExpr *OpaqueValue,
              bool IsImplicit = false)
      : CoroutineSuspendExpr(CoawaitExprClass, CoawaitLoc, Operand, Ready,
                             Suspend, Resume, OpaqueValue) {
    CoawaitBits.IsImplicit = IsImplicit;
  }

  CoawaitExpr(SourceLocation CoawaitLoc, QualType Ty, Expr *Operand,
              bool IsImplicit = false)
      : CoroutineSuspendExpr(CoawaitExprClass, CoawaitLoc, Ty, Operand) {
    CoawaitBits.IsImplicit = IsImplicit;
  }

  CoawaitExpr(EmptyShell Empty)
      : CoroutineSuspendExpr(CoawaitExprClass, Empty) {}

  Expr *getOperand() const {
    // FIXME: Dig out the actual operand or store it.
    return getCommonExpr();
  }

  bool isImplicit() const { return CoawaitBits.IsImplicit; }
  void setIsImplicit(bool value = true) { CoawaitBits.IsImplicit = value; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CoawaitExprClass;
  }
};

/// \brief Represents a 'co_await' expression while the type of the promise
/// is dependent.
class DependentCoawaitExpr : public Expr {
  friend class ASTStmtReader;

  SourceLocation KeywordLoc;
  Stmt *SubExprs[2];

public:
  DependentCoawaitExpr(SourceLocation KeywordLoc, QualType Ty, Expr *Op,
                       UnresolvedLookupExpr *OpCoawait)
      : Expr(DependentCoawaitExprClass, Ty, VK_RValue, OK_Ordinary,
             /*TypeDependent*/ true, /*ValueDependent*/ true,
             /*InstantiationDependent*/ true,
             Op->containsUnexpandedParameterPack()),
        KeywordLoc(KeywordLoc) {
    // NOTE: A co_await expression is dependent on the coroutines promise
    // type and may be dependent even when the `Op` expression is not.
    assert(Ty->isDependentType() &&
           "wrong constructor for non-dependent co_await/co_yield expression");
    SubExprs[0] = Op;
    SubExprs[1] = OpCoawait;
  }

  DependentCoawaitExpr(EmptyShell Empty)
      : Expr(DependentCoawaitExprClass, Empty) {}

  Expr *getOperand() const { return cast<Expr>(SubExprs[0]); }

  UnresolvedLookupExpr *getOperatorCoawaitLookup() const {
    return cast<UnresolvedLookupExpr>(SubExprs[1]);
  }

  SourceLocation getKeywordLoc() const { return KeywordLoc; }

  SourceLocation getLocStart() const LLVM_READONLY { return KeywordLoc; }

  SourceLocation getLocEnd() const LLVM_READONLY {
    return getOperand()->getLocEnd();
  }

  child_range children() { return child_range(SubExprs, SubExprs + 2); }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == DependentCoawaitExprClass;
  }
};

/// \brief Represents a 'co_yield' expression.
class CoyieldExpr : public CoroutineSuspendExpr {
  friend class ASTStmtReader;

public:
  CoyieldExpr(SourceLocation CoyieldLoc, Expr *Operand, Expr *Ready,
              Expr *Suspend, Expr *Resume, OpaqueValueExpr *OpaqueValue)
      : CoroutineSuspendExpr(CoyieldExprClass, CoyieldLoc, Operand, Ready,
                             Suspend, Resume, OpaqueValue) {}
  CoyieldExpr(SourceLocation CoyieldLoc, QualType Ty, Expr *Operand)
      : CoroutineSuspendExpr(CoyieldExprClass, CoyieldLoc, Ty, Operand) {}
  CoyieldExpr(EmptyShell Empty)
      : CoroutineSuspendExpr(CoyieldExprClass, Empty) {}

  Expr *getOperand() const {
    // FIXME: Dig out the actual operand or store it.
    return getCommonExpr();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CoyieldExprClass;
  }
};

} // namespace clang

#endif // LLVM_CLANG_AST_EXPRCXX_H
