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
    string escapedName = p->mappedName();
    string ns = getNamespace(p);

    bool hasRequiredField = writePrimaryConstructor(p, p->dataMembers(), {}, "struct");

    // Decoding constructor.
    _out << sp;
    writeDocLine(
        _out,
        "summary",
        "Initializes a new instance of the <see cref=\"" + escapedName + "\" /> struct from a SliceDecoder.");

    if (hasRequiredField)
    {
        _out << nl << "[global::System.Diagnostics.CodeAnalysis.SetsRequiredMembers]";
    }

    _out << nl << "public " << escapedName << "(ref SliceDecoder decoder)";
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
    writeDocLine(_out, "summary", "Encodes the fields of this struct with a Slice encoder.");

    _out << nl << "public void Encode(ref SliceEncoder encoder)";
    _out << sb;
    for (const auto& field : p->dataMembers())
    {
        encodeField(_out, "this." + field->mappedName(), field->type(), ns, TypeContext::Field);
    }
    _out << eb;

    _out << eb;
}

bool
Slice::IceRpc::TypesVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    string ns = getNamespace(p);

    _out << sp;
    writeDocComment(p, "class");
    emitObsoleteAttribute(p);
    _out << nl << "[SliceTypeId(\"" << p->scoped() << "\")]";
    if (p->compactId() != -1)
    {
        _out << nl << "[CompactSliceTypeId(" << p->compactId() << ")]";
    }
    _out << nl << "public partial class " << p->mappedName() << " : ";

    ClassDefPtr base = p->base();
    if (base)
    {
        _out << getUnqualified(base, ns);
    }
    else
    {
        _out << "SliceClass";
    }
    _out << sb;
    return true;
}

void
Slice::IceRpc::TypesVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    string escapedName = p->mappedName();
    string ns = getNamespace(p);

    if (!p->dataMembers().empty())
    {
        _out << sp;
    }
    _out << nl << "private static readonly string SliceTypeId = typeof(" << escapedName << ").GetSliceTypeId()!;";
    if (p->compactId() != -1)
    {
        _out << nl << "private static readonly int CompactSliceTypeId = typeof(" << escapedName
             << ").GetCompactSliceTypeId()!;";
    }

    if (!p->allDataMembers().empty())
    {
        DataMemberList allBaseFields;
        if (p->base())
        {
            allBaseFields = p->base()->allDataMembers();
        }

        writePrimaryConstructor(p, p->dataMembers(), allBaseFields, "class");

        // Public parameterless constructor.
        _out << sp;
        writeDocLine(_out, "summary", "Initializes a new instance of the <see cref=\"" + escapedName + "\" /> class.");
        _out << nl << "public " << escapedName << "()";
        _out << sb;
        _out << eb;
    }
    // else, no need to generate any constructor.

    _out << sp;
    emitNonBrowsableAttribute();
    _out << nl << "protected override void EncodeCore(ref SliceEncoder encoder)";
    _out << sb;
    _out << nl << "encoder.StartSlice(SliceTypeId";
    if (p->compactId() != -1)
    {
        _out << ", CompactSliceTypeId";
    }
    _out << ");";
    // Encode non-optional fields
    for (const auto& field : p->dataMembers())
    {
        if (!field->optional())
        {
            encodeField(_out, "this." + field->mappedName(), field->type(), ns, TypeContext::Field);
        }
    }
    // Encode optional fields
    for (const auto& field : p->orderedOptionalDataMembers())
    {
        encodeOptionalField(_out, field->tag(), "this." + field->mappedName(), field->type(), ns, TypeContext::Field);
    }

    if (p->base())
    {
        _out << nl << "encoder.EndSlice(false);";
        _out << nl << "base.EncodeCore(ref encoder);";
    }
    else
    {
        _out << nl << "encoder.EndSlice(true);"; // last slice
    }
    _out << eb;

    _out << sp;
    emitNonBrowsableAttribute();
    _out << nl << "protected override void DecodeCore(ref SliceDecoder decoder)";
    _out << sb;
    _out << nl << "decoder.StartSlice();";
    // Decode non-optional fields
    for (const auto& field : p->dataMembers())
    {
        if (!field->optional())
        {
            _out << nl << "this." + field->mappedName() << " = ";
            decodeField(_out, field->type(), ns, TypeContext::Field);
            _out << ';';
        }
    }
    // Decode optional fields
    for (const auto& field : p->orderedOptionalDataMembers())
    {
        _out << nl << "this." + field->mappedName() << " = ";
        decodeOptionalField(_out, field->tag(), field->type(), ns, TypeContext::Field);
        _out << ';';
    }

    _out << nl << "decoder.EndSlice();";
    if (p->base())
    {
        _out << nl << "base.DecodeCore(ref decoder);";
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

    //
    // XxxIntExtensions
    //
    _out << sp;
    ostringstream intExtensionsComment;
    intExtensionsComment << "Provides an extension method for creating " << getArticleFor(name) << " <see cref=\""
                         << escapedName << "\" /> from an int.";
    writeHelperDocComment(p, intExtensionsComment.str(), "enum helper class");
    _out << nl << "public static class " << name << "IntExtensions";
    _out << sb;

    // When the number of enumerators is smaller than the distance between the min and max
    // values, the values are not consecutive and we need to use a set to validate the value
    // during decoding.
    // Note that the values are not necessarily in order, e.g. we can use a simple range check
    // for enum E { A = 3, B = 2, C = 1 } during decoding.
    bool useSet = p->enumerators().size() < static_cast<size_t>(p->maxValue() - p->minValue() + 1);

    if (useSet)
    {
        _out << nl
             << "private static readonly global::System.Collections.Generic.HashSet<int> _enumeratorValues = new()";
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
    _out << nl << "throw new global::System.IO.InvalidDataException($\"Invalid value {value} for enum " << escapedName
         << ".\");";
    _out.dec();
    _out.dec();
    _out << eb;

    //
    // XxxSliceEncoderExtensions and XxxSliceDecoderExtensions
    //
    _out << sp;
    ostringstream encoderExtensionsComment;
    encoderExtensionsComment << "Provides an extension method for encoding " << getArticleFor(name) << " <see cref=\""
                             << escapedName << "\" />.";
    writeHelperDocComment(p, encoderExtensionsComment.str(), "enum helper class");
    _out << nl << "public static class " << name << "SliceEncoderExtensions";
    _out << sb;

    // TODO: doc-comment
    _out << nl << "public static void Encode" << name << "(this ref SliceEncoder encoder, " << escapedName
         << " value) => encoder.EncodeSize((int)value);";
    _out << eb;

    _out << sp;
    ostringstream decoderExtensionsComment;
    decoderExtensionsComment << "Provides an extension method for decoding " << getArticleFor(name) << " <see cref=\""
                             << escapedName << "\" />.";
    writeHelperDocComment(p, decoderExtensionsComment.str(), "enum helper class");
    _out << nl << "public static class " << name << "SliceDecoderExtensions";
    _out << sb;

    // TODO: doc-comment
    _out << nl << "public static " << escapedName << " Decode" << name << "(this ref SliceDecoder decoder) => " << name
         << "IntExtensions.As" << name << "(decoder.DecodeSize());";
    _out << eb;
}

bool
Slice::IceRpc::TypesVisitor::writePrimaryConstructor(
    const ContainedPtr& p,
    const DataMemberList& fields,
    const DataMemberList& allBaseFields,
    const string& kind)
{
    // Primary constructor with parameters for all fields.
    string escapedName = p->mappedName();
    string name = removeEscapePrefix(escapedName);
    string ns = getNamespace(p);

    _out << sp;
    writeDocLine(
        _out,
        "summary",
        "Initializes a new instance of the <see cref=\"" + escapedName + "\" /> " + kind + ".");

    vector<string> ctorParams;
    vector<string> baseParams;
    vector<string> ctorPropertyInits;
    bool hasRequiredField = false;

    for (const auto& field : allBaseFields)
    {
        string paramName = field->customMappedName().value_or(toCamelCase(field->name()));
        ctorParams.push_back(csFieldType(field->type(), ns, field->optional()) + " " + paramName);
        if (csRequired(field))
        {
            hasRequiredField = true;
        }
        baseParams.push_back(paramName);
    }

    for (const auto& field : fields)
    {
        string paramName = field->customMappedName().value_or(toCamelCase(field->name()));
        ctorParams.push_back(csFieldType(field->type(), ns, field->optional()) + " " + paramName);
        if (csRequired(field))
        {
            hasRequiredField = true;
        }

        ctorPropertyInits.push_back("this." + field->mappedName() + " = " + paramName + ';');
    }

    if (hasRequiredField)
    {
        _out << nl << "[global::System.Diagnostics.CodeAnalysis.SetsRequiredMembers]";
    }

    _out << nl << "public " << name << spar << ctorParams << epar;
    if (!baseParams.empty())
    {
        _out.inc();
        _out << nl << ": base" << spar << baseParams << epar;
        _out.dec();
    }

    _out << sb;
    for (const auto& propertyInit : ctorPropertyInits)
    {
        _out << nl << propertyInit;
    }
    _out << eb;
    return hasRequiredField;
}
