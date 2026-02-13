// Copyright (c) ZeroC, Inc.

#ifndef ICE_RPC_CS_UTIL_H
#define ICE_RPC _CS_UTIL_H

#include "../Ice/OutputUtil.h"
#include "../Slice/DocCommentParser.h"
#include "../Slice/Parser.h"

// IceRPC-specific helper functions for C# code generation.

namespace Slice::Csharp
{
    enum class TypeContext
    {
        Field,
        IncomingParam,
        OutgoingParam
    };

    /// Does this type maps to a value type in C#?
    [[nodiscard]] bool isCsValueType(const TypePtr& type);

    /// Maps a Slice type to a C# field type.
    /// @param type The Slice type to map.
    /// @param ns The current C# namespace.
    /// @param optional Whether the field is optional.
    /// @return The C# type for the field.
    [[nodiscard]] std::string csFieldType(const TypePtr& type, const std::string& ns, bool optional = false);

    /// Maps a Slice type to a C# incoming parameter type.
    /// @param type The Slice type to map.
    /// @param ns The current C# namespace.
    /// @param optional Whether the parameter is optional.
    /// @return The C# type for the incoming parameter.
    [[nodiscard]] std::string csIncomingParamType(const TypePtr& type, const std::string& ns, bool optional = false);

    /// Maps a Slice type to a C# outgoing parameter type.
    /// @param type The Slice type to map.
    /// @param ns The current C# namespace.
    /// @param optional Whether the parameter is optional.
    /// @return The C# type for the outgoing parameter.
    [[nodiscard]] std::string csOutgoingParamType(const TypePtr& type, const std::string& ns, bool optional = false);

    /// Returns whether the mapped C# field is required or not.
    [[nodiscard]] bool csRequired(const DataMemberPtr& field);

    /// Encodes a non-optional field.
    void encodeField(::IceInternal::Output& out, const std::string& fieldName, const TypePtr& type, const std::string& ns, TypeContext context);

    /// Encodes an optional field.
    void encodeOptionalField(::IceInternal::Output& out, int tag, const std::string& fieldName, const TypePtr& type, const std::string& ns, TypeContext context);

    /// Decodes a non-optional field.
    void decodeField(::IceInternal::Output& out, const TypePtr& type, const std::string& ns, TypeContext context);
}

#endif
