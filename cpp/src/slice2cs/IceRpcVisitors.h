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

        bool visitClassDefStart(const ClassDefPtr&) final;
        void visitClassDefEnd(const ClassDefPtr&) final;

        bool visitExceptionStart(const ExceptionPtr&) final;
        void visitExceptionEnd(const ExceptionPtr&) final;

        void visitDataMember(const DataMemberPtr&) final;

        void visitEnum(const EnumPtr&) final;

        // For proxies.
        bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
        void visitInterfaceDefEnd(const InterfaceDefPtr&) final;
        void visitOperation(const OperationPtr&) final;

    private:
        bool writePrimaryConstructor(
            const ContainedPtr& p,
            const DataMemberList& fields,
            const DataMemberList& allBaseFields,
            const std::string& kind);

        void writeEncodeDecode(
            int compactId,
            const std::string& ns,
            bool hasBase,
            const DataMemberList& fields,
            const DataMemberList& allBaseFields);

        void writeProxyRequestClass(const InterfaceDefPtr& interface);

        void writeProxyResponseClass(const InterfaceDefPtr& interface);

        void writeMethod(const OperationPtr& operation, const std::string& ns, const std::vector<std::string>& extraParams, bool dispatch);


    };
}

#endif
