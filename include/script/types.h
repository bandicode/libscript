// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TYPES_H
#define LIBSCRIPT_TYPES_H

#include "libscriptdefs.h"

namespace script
{

class LIBSCRIPT_API Type
{
public:
  Type();
  Type(const Type &) = default;
  ~Type() = default;
  Type(int baseType, int flags = 0);

  enum TypeFlag {
    NoFlag               = 0,
    EnumFlag             = 0x010000,
    ObjectFlag           = 0x020000,
    LambdaFlag           = 0x040000, // used to mark a type as being a lambda
    PrototypeFlag        = 0x080000, // used to mark a type that is a function signature
    ReferenceFlag        = 0x100000,
    ConstFlag            = 0x200000,
    ForwardReferenceFlag = 0x400000,
    ThisFlag             = 0x800000,
    ManagedFlag          = 0x1000000,
    OptionalFlag         = 0x2000000, // used for optional function arguments
    UninitializedFlag    = OptionalFlag,
    UnknownFlag          = OptionalFlag,
    ProtectedFlag        = 0x4000000,
    PrivateFlag          = 0x8000000,
#if defined(LIBSCRIPT_CONFIG_INJECTED_TYPE_FLAGS)
#include LIBSCRIPT_CONFIG_INJECTED_TYPE_FLAGS
#endif // defined(LIBSCRIPT_CONFIG_INJECTED_TYPE_FLAGS)
  };

  enum BuiltInType {
    Null = 0,
    Void = 1,
    Boolean = 2,
    Char = 3,
    Int = 4,
    Float = 5,
    Double = 6,
    String = ObjectFlag | 1,
    InitializerList = 8,
    Auto = 9,
#if defined(LIBSCRIPT_CONFIG_INJECTED_BUILTIN_TYPES)
#include LIBSCRIPT_CONFIG_INJECTED_BUILTIN_TYPES
#endif // defined(LIBSCRIPT_CONFIG_INJECTED_BUILTIN_TYPES)
  };

  inline bool isNull() const { return d == 0; }

  Type baseType() const;

  bool isConst() const;
  void setConst(bool on);

  bool isReference() const;
  void setReference(bool on);

  bool isRefRef() const;
  bool isConstRef() const;

  Type withConst() const;
  Type withoutConst() const;
  Type withoutRef() const;

  bool isFundamentalType() const;
  bool isObjectType() const;
  bool isEnumType() const;
  bool isClosureType() const;
  bool isFunctionType() const;

  bool testFlag(TypeFlag flag) const;
  void setFlag(TypeFlag flag);
  Type withFlag(TypeFlag flag) const;
  Type withoutFlag(TypeFlag flag) const;

  static Type ref(const Type & base);
  static Type cref(const Type & base);
  static Type rref(const Type & base);

  bool operator==(const Type & other) const;
  bool operator==(const BuiltInType & rhs) const;
  inline bool operator!=(const Type & other) const { return !operator==(other); }

  Type & operator=(const Type &) = default;

  int data() const;

protected:
  int d;
};

} // namespace script

#endif // LIBSCRIPT_TYPES_H
