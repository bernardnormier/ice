// Copyright (c) ZeroC, Inc.

#include "IceRpcVisitors.h"
#include "../Slice/Util.h"
#include "CsUtil.h"
#include "IceRpcCsUtil.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>

using namespace std;
using namespace Slice;
using namespace Slice::Csharp;
using namespace IceInternal;

Slice::IceRpc::TypesVisitor::TypesVisitor(IceInternal::Output& out) : CsVisitor(out) {}

bool
Slice::IceRpc::TypesVisitor::visitModuleStart(const ModulePtr& p)
{
    namespacePrefixStart(p);
    _out << sp << nl << "namespace " << p->mappedName();
    _out << sb;
    return true;
}

void
Slice::IceRpc::TypesVisitor::visitModuleEnd(const ModulePtr& p)
{
    _out << eb;
    namespacePrefixEnd(p);
}

bool
Slice::IceRpc::TypesVisitor::visitStructStart(const StructPtr& p)
{
    _out << sp;
    writeDocComment(p, "record struct");
    emitObsoleteAttribute(p);
    _out << nl << "public partial record struct " << p->mappedName();
    _out << sb;
    return true;
}

void
Slice::IceRpc::TypesVisitor::visitStructEnd(const StructPtr& p)
{
    string name = p->mappedName();
    string ns = getNamespace(p);

    // Primary constructor with parameters for all fields.
    _out << sp;
    writeDocLine(_out, "summary", "Initializes a new instance of the <see cref=\"" + name + "\" /> struct.");

    vector<string> ctorParams;
    vector<string> ctorPropertyInits;
    bool hasRequiredField = false;
    for (const auto& field : p->dataMembers())
    {
        string paramName = field->customMappedName().value_or(toCamelCase(field->name()));
        ctorParams.push_back(csFieldType(field->type(), getNamespace(p), field->optional()) + " " + paramName);
        ctorPropertyInits.push_back("this." + field->mappedName() + " = " + paramName + ';');

        if (csRequired(field))
        {
            hasRequiredField = true;
        }
    }

    if (hasRequiredField)
    {
        _out << nl << "[global::System.Diagnostics.CodeAnalysis.SetsRequiredMembers]";
    }

    _out << nl << "public " << name << spar << ctorParams << epar;
    _out << sb;
    for (const auto& propertyInit : ctorPropertyInits)
    {
        _out << nl << propertyInit;
    }
    _out << eb;

    // Decoding constructor.
    _out << sp;
    writeDocLine(
        _out,
        "summary",
        "Initializes a new instance of the <see cref=\"" + name + "\" /> struct from a SliceDecoder.");

    if (hasRequiredField)
    {
        _out << nl << "[global::System.Diagnostics.CodeAnalysis.SetsRequiredMembers]";
    }

    _out << nl << "public " << name << "(ref SliceDecoder decoder)";
    _out << sb;
    for (const auto& field : p->dataMembers())
    {
        _out << nl << "this." + field->mappedName() << " = ";
        decodeField(_out, field->type(), ns, TypeContext::Field);
        _out << ';';
    }
    _out << eb;

    // Encode method.
    _out << sp;
    writeDocLine(
        _out,
        "summary",
        "Encodes the fields of this struct with a Slice encoder.");

    _out << nl << "public void Encode(ref SliceEncoder encoder)";
    _out << sb;
    for (const auto& field : p->dataMembers())
    {
        encodeField(_out, "this." + field->mappedName(), field->type(), ns, TypeContext::Field);
    }
    _out << eb;

    _out << eb;
}

void
Slice::IceRpc::TypesVisitor::visitDataMember(const DataMemberPtr& p)
{
    ContainedPtr cont = dynamic_pointer_cast<Contained>(p->container());
    assert(cont);
    string ns = getNamespace(cont);

    _out << sp;
    writeDocComment(p);
    emitObsoleteAttribute(p);
    emitAttributes(p);
    _out << nl << "public ";
    if (csRequired(p))
    {
        _out << "required ";
    }
    _out << csFieldType(p->type(), ns, p->optional()) << ' ' << p->mappedName() << " { get; set; }";
}

void
Slice::IceRpc::TypesVisitor::visitEnum(const EnumPtr& p)
{
    string escapedName = p->mappedName();
    string name = removeEscapePrefix(p->mappedName());
    string ns = getNamespace(p);
    EnumeratorList enumerators = p->enumerators();
    const bool hasExplicitValues = p->hasExplicitValues();

    _out << sp;
    writeDocComment(p, "enum");
    emitObsoleteAttribute(p);
    emitAttributes(p);
    _out << nl << "public enum " << escapedName;
    _out << sb;
    for (const auto& enumerator : enumerators)
    {
        if (!isFirstElement(enumerator))
        {
            _out << ',';
            _out << sp;
        }

        writeDocComment(enumerator);
        emitObsoleteAttribute(enumerator);
        emitAttributes(enumerator);
        _out << nl << enumerator->mappedName();
        if (hasExplicitValues)
        {
            _out << " = " << enumerator->value();
        }
    }
    _out << eb;

    _out << sp;
    ostringstream classComment;
    classComment << "Provides an extension method for creating " << getArticleFor(name)
        << " <see cref=\"" << escapedName << "\" /> from an int.";
    writeHelperDocComment(p, classComment.str(), "enum helper class");
    _out << nl << "public static class " << name << "IntExtensions";
    _out << sb;
    _out << sp;

    // When the number of enumerators is smaller than the distance between the min and max
    // values, the values are not consecutive and we need to use a set to validate the value
    // during decoding.
    // Note that the values are not necessarily in order, e.g. we can use a simple range check
    // for enum E { A = 3, B = 2, C = 1 } during decoding.
    bool useSet = p->enumerators().size() < static_cast<size_t>(p->maxValue() - p->minValue() + 1);

    if (useSet)
    {
        _out << nl << "private static readonly global::System.Collections.Generic.HashSet<int> _enumeratorValues = new()";
        _out.spar("{ ");
        for (const auto& enumerator : enumerators)
        {
            _out << enumerator->value();
        }
        _out.epar(" };");
        _out << sp;
    }

    // TODO: doc-comment
    _out << nl << "public static " << escapedName << " As" << name << "(this int value) =>";
    _out.inc();
    if (useSet)
    {
        _out << nl << "_enumeratorValues.Contains(value) ? ";
    }
    else
    {
        _out << nl << "(value >= " << p->minValue() << " && value <= " << p->maxValue() << ") ? ";
    }
    _out << "(" << escapedName << ")value :";
    _out.inc();
    _out << nl
        << "throw new global::System.IO.InvalidDataException($\"Invalid value {value} for enum "
        << escapedName << ".\");";
    _out.dec();
    _out.dec();
    _out << eb;
}

