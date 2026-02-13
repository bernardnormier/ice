// Copyright (c) ZeroC, Inc.

#ifndef ICE_RPC_VISITORS_H
#define ICE_RPC_VISITORS_H

#include "CsVisitor.h"

// Visitors for generating C# code for IceRPC.

namespace Slice::IceRpc
{
    /// Generates code for Slice types (including proxies) and Slice exceptions.
    class TypesVisitor final : public CsVisitor
    {
    public:
        TypesVisitor(IceInternal::Output&);

        bool visitModuleStart(const ModulePtr&) final;
        void visitModuleEnd(const ModulePtr&) final;

        bool visitStructStart(const StructPtr&) final;
        void visitStructEnd(const StructPtr&) final;
        void visitDataMember(const DataMemberPtr&) final;

        void visitEnum(const EnumPtr&) final;
    };
}

#endif
