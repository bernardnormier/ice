// Copyright (c) ZeroC, Inc.

#include "IceRpcCsUtil.h"
#include "CsUtil.h"
#include "../Slice/Util.h"

#include <cassert>

using namespace std;
using namespace Slice;
using namespace IceInternal;

namespace
{
    [[nodiscard]] bool hasFixedSizeBuiltinElements(const SequencePtr& seq)
    {
        BuiltinPtr elementTypeBuiltin = dynamic_pointer_cast<Builtin>(seq->type());
        return elementTypeBuiltin && !elementTypeBuiltin->isVariableLength();
    }

    [[nodiscard]] bool isBool(const TypePtr& type)
    {
        BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
        return builtin && builtin->kind() == Builtin::KindBool;
    }

    [[nodiscard]] bool isString(const TypePtr& type)
    {
        BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
        return builtin && builtin->kind() == Builtin::KindString;
    }

    void castToNestedFieldType(Output& out, const TypePtr& type, const string& ns)
    {
        if (dynamic_pointer_cast<Sequence>(type) || dynamic_pointer_cast<Dictionary>(type))
        {
            out << "(" << Slice::Csharp::csFieldType(type, ns) << ")";
        }
        // else no need to cast
    }
}

bool
Slice::Csharp::isCsValueType(const TypePtr& type)
{
    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        switch (builtin->kind())
        {
            case Builtin::KindByte:
            case Builtin::KindBool:
            case Builtin::KindShort:
            case Builtin::KindInt:
            case Builtin::KindLong:
            case Builtin::KindFloat:
            case Builtin::KindDouble:
                return true;
            default:
                return false;
        }
    }

    return dynamic_pointer_cast<Enum>(type) ||
        dynamic_pointer_cast<Struct>(type) ||
        dynamic_pointer_cast<InterfaceDecl>(type);
}

string
Slice::Csharp::csFieldType(const TypePtr& type, const string& ns, bool optional)
{
    assert(type);

    if (optional && !isProxyType(type))
    {
        // Proxy types are mapped the same way for optional and non-optional types.
        return csFieldType(type, ns) + "?";
    }
    // else, just use the regular mapping.

    static const char* builtinTable[] =
        {"byte", "bool", "short", "int", "long", "float", "double", "string", "IceRpc.ServiceAddress?", "SliceClass?"};

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        return builtinTable[builtin->kind()];
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    if (cl)
    {
        assert(!optional); // Optional classes are disallowed by the parser.
        return getUnqualified(cl, ns) + "?";
    }

    InterfaceDeclPtr proxy = dynamic_pointer_cast<InterfaceDecl>(type);
    if (proxy)
    {
        return getUnqualified(proxy, ns) + "Proxy?";
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        return "global::System.Collections.Generic.IList<" + csFieldType(seq->type(), ns) + ">";
    }

    DictionaryPtr d = dynamic_pointer_cast<Dictionary>(type);
    if (d)
    {
        return "global::System.Collections.Generic.IDictionary<" + csFieldType(d->keyType(), ns) + ", " +
               csFieldType(d->valueType(), ns) + ">";
    }

    ContainedPtr contained = dynamic_pointer_cast<Contained>(type);
    if (contained)
    {
        return getUnqualified(contained, ns);
    }

    return "???";
}

string
Slice::Csharp::csIncomingParamType(const TypePtr& type, const string& ns, bool optional)
{
    if (optional && !isProxyType(type))
    {
        // Proxy types are mapped the same way for optional and non-optional types.
        return csIncomingParamType(type, ns) + "?";
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        if (auto metadata = seq->getMetadataArgs("cs:generic"))
        {
            const string& customType = *metadata;
            if (customType == "List" || customType == "LinkedList" || customType == "Queue" || customType == "Stack")
            {
                return "global::System.Collections.Generic." + customType + "<" + csFieldType(seq->type(), ns) + ">";
            }
            else
            {
                return "global::" + customType + "<" + csFieldType(seq->type(), ns) + ">";
            }
        }
        return csFieldType(seq->type(), ns) + "[]";
    }

    DictionaryPtr d = dynamic_pointer_cast<Dictionary>(type);
    if (d)
    {
        string typeName = d->getMetadataArgs("cs:generic").value_or("Dictionary");
        return "global::System.Collections.Generic." + typeName + "<" + csFieldType(d->keyType(), ns) + ", " +
               csFieldType(d->valueType(), ns) + ">";
    }

    return csFieldType(type, ns);
}

string
Slice::Csharp::csOutgoingParamType(const TypePtr& type, const string& ns, bool optional)
{
    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        bool hasGenericMetadata = seq->hasMetadata("cs:generic");

        BuiltinPtr elementTypeBuiltin = dynamic_pointer_cast<Builtin>(seq->type());
        string elementTypeStr = csFieldType(seq->type(), ns);

        if (elementTypeBuiltin && !elementTypeBuiltin->isVariableLength() && !hasGenericMetadata)
        {
            // If the underlying type is a fixed-size primitive, we map to `ReadOnlyMemory` instead,
            // and the mapping is the same for optional and non-optional types.
            return "global::System.ReadOnlyMemory<" + elementTypeStr + ">";
        }
        else
        {
            return "global::System.Collections.Generic.IEnumerable<" + elementTypeStr + ">" + (optional ? "?" : "");
        }
    }

    DictionaryPtr d = dynamic_pointer_cast<Dictionary>(type);
    if (d)
    {
        return "global::System.Collections.Generic.IEnumerable<global::System.Collections.Generic.KeyValuePair<" +
            csFieldType(d->keyType(), ns) + ", " + csFieldType(d->valueType(), ns) + ">>" + (optional ? "?" : "");
    }

    return csFieldType(type, ns, optional);
}

bool
Slice::Csharp::csRequired(const DataMemberPtr& field)
{
    if (field->optional())
    {
        return false;
    }

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(field->type());
    if (builtin)
    {
        return builtin->kind() == Builtin::KindString;
    }

    return dynamic_pointer_cast<Sequence>(field->type()) || dynamic_pointer_cast<Dictionary>(field->type());
}

void
Slice::Csharp::encodeField(Output& out, const string& fieldName, const TypePtr& type, const string& ns, TypeContext context)
{
    assert(type);

    static const char* builtinTable[] =
        {"UInt8", "Bool", "Int16", "Int32", "Int64", "Float", "Double", "String", "NullableServiceAddress", "NullableClass"};

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        out << nl << "encoder.Encode" << builtinTable[builtin->kind()] << "(" << fieldName << ");";
        return;
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    if (cl)
    {
        out << nl << "encoder.EncodeNullableClass(" << fieldName << ");";
        return;
    }

    StructPtr st = dynamic_pointer_cast<Struct>(type);
    if (st)
    {
        out << nl << fieldName << ".Encode(ref encoder);";
        return;
    }

    InterfaceDeclPtr proxy = dynamic_pointer_cast<InterfaceDecl>(type);
    if (proxy)
    {
        out << getUnqualified(proxy, ns) << "ProxySliceEncoderExtensions.EncodeNullable"
            << removeEscapePrefix(proxy->mappedName()) << "Proxy(ref encoder, " << fieldName << ");";
        return;
    }

    EnumPtr en = dynamic_pointer_cast<Enum>(type);
    if (en)
    {
        out << nl << "encoder.EncodeSize((int)" << fieldName << ");";
        return;
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        if (hasFixedSizeBuiltinElements(seq) && !seq->hasMetadata("cs:generic"))
        {
            if (context == TypeContext::OutgoingParam)
            {
                out << nl << "encoder.EncodeSpan(" << fieldName << ".Span);";
            }
            else
            {
                out << nl << "encoder.EncodeSequence(" << fieldName << ");";
            }
        }
        else
        {
            TypePtr elementType = seq->type();

            out << nl << "encoder.EncodeSequence(";
            out.inc();
            out << nl << fieldName << ",";
            out << nl << "(ref SliceEncoder encoder, " << csFieldType(elementType, ns) << " value) =>";
            out << sb;
            encodeField(out, "value", elementType, ns, TypeContext::Field);
            out << eb << ");";
            out.dec();
        }
        return;
    }

    DictionaryPtr dict = dynamic_pointer_cast<Dictionary>(type);
    if (dict)
    {
        TypePtr keyType = dict->keyType();
        TypePtr valueType = dict->valueType();
        out << nl << "encoder.EncodeDictionary(";
        out.inc();
        out << nl << fieldName << ",";
        out << nl << "(ref SliceEncoder encoder, " << csFieldType(keyType, ns) << " key) =>";
        out << sb;
        encodeField(out, "key", keyType, ns, TypeContext::Field);
        out << eb << ",";
        out << nl << "(ref SliceEncoder encoder, " << csFieldType(valueType, ns) << " value) =>";
        out << sb;
        encodeField(out, "value", valueType, ns, TypeContext::Field);
        out << eb << ");";
        out.dec();
    }
}

void
Slice::Csharp::encodeOptionalField(Output& out, int tag, const string& fieldName, const TypePtr& type, const string&, TypeContext context)
{
    assert(type);

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    bool readOnlyMemory = seq && hasFixedSizeBuiltinElements(seq) && !seq->hasMetadata("cs:generic") &&
        context == TypeContext::OutgoingParam;
    string optionalFormat = type->getOptionalFormat();

    if (optionalFormat == "VSize")
    {
        if (isString(type) || (seq && seq->type()->minWireSize() == 1))
        {
            optionalFormat = "OptimizedVSize";
        }
    }

    if (readOnlyMemory)
    {
        out << nl << "if (" << fieldName << ".Span != null)";
    }
    else if (isCsValueType(type))
    {
        out << nl << "if (" << fieldName << ".HasValue)";
    }
    else
    {
        out << nl << "if (" << fieldName << " is not null)";
    }
    out << sb;

    if (optionalFormat == "VSize")
    {
        // Call VSize overload. Compute and transmit the size for double-checking.
        out << nl << "encoder.EncodeTagged(";
        out.inc();
        out << nl << tag << ",";
        out << nl << "size: ";
        if (auto st = dynamic_pointer_cast<Struct>(type))
        {
            out << st->minWireSize();
        }
        else if (readOnlyMemory)
        {
            out << "encoder.GetSizeLength(" << fieldName << ".Length) + " << seq->type()->minWireSize() << " * "
                << fieldName << ".Length";
        }
    }
    else
    {

    }

    out << nl << "encoder.EncodeTagged(";
    out.inc();
    out << nl << tag << ",";
    out << nl << type->getOptionalFormat() << ",";
    out << nl << fieldName << (isCsValueType(type) ? ".Value" : "!");
    out << ");";
    out.dec();

    out << eb;
    // else, don't encode anything for null.
}



void
Slice::Csharp::decodeField(Output& out, const TypePtr& type, const string& ns, TypeContext)
{
    assert(type);

    static const char* builtinTable[] =
        {"UInt8", "Bool", "Int16", "Int32", "Int64", "Float", "Double", "String", "NullableServiceAddress", "Class<SliceClass>"};

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        out << "decoder.Decode" << builtinTable[builtin->kind()] << "()";
        return;
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    if (cl)
    {
        out << "decoder.DecodeClass<" << getUnqualified(cl, ns) << ">();";
        return;
    }

    StructPtr st = dynamic_pointer_cast<Struct>(type);
    if (st)
    {
        out <<  "new " << getUnqualified(st, ns) << "(ref decoder)";
        return;
    }

    InterfaceDeclPtr proxy = dynamic_pointer_cast<InterfaceDecl>(type);
    if (proxy)
    {
        out << getUnqualified(proxy, ns) << "ProxySliceDecoderExtensions.DecodeNullable"
            << removeEscapePrefix(proxy->mappedName()) << "Proxy(ref decoder)";

        return;
    }

    EnumPtr en = dynamic_pointer_cast<Enum>(type);
    if (en)
    {
        out << getUnqualified(en, ns) << "IntExtensions.As" << removeEscapePrefix(en->mappedName())
            << "(decoder.DecodeSize())";
        return;
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        // TODO: review the Rust code, which is much more complex.

        bool hasGenericMetadata = seq->hasMetadata("cs:generic");

        if (hasGenericMetadata)
        {
            // The concrete type we create.
            string csSeq = csIncomingParamType(seq, ns);
            out << "decoder.DecodeSequence(sequenceFactory: size => new " << csSeq << "(size));";
        }
        else if (hasFixedSizeBuiltinElements(seq))
        {
            out << "decoder.DecodeSequence<" << csFieldType(seq->type(), ns) << ">(";
            if (isBool(seq->type()))
            {
               out << "checkElement: SliceDecoder.CheckBoolValue";
            }
            out << ")";
        }
        else
        {
            out << "decoder.DecodeSequence(";
            out.inc();
            out << nl << "(ref SliceDecoder decoder) => ";
            castToNestedFieldType(out, seq->type(), ns);
            decodeField(out, seq->type(), ns, TypeContext::Field);
            out << ")";
            out.dec();
        }
        return;
    }

    DictionaryPtr dict = dynamic_pointer_cast<Dictionary>(type);
    if (dict)
    {
        TypePtr keyType = dict->keyType();
        TypePtr valueType = dict->valueType();

        // The concrete type we create.
        string csDict = csIncomingParamType(dict, ns);

        out << "decoder.DecodeDictionary(";
        out.inc();
        out << nl << "size => new " << csDict << "(size),";
        out << nl << "(ref SliceDecoder decoder) => ";
        decodeField(out, keyType, ns, TypeContext::Field);
        out << ",";
        out << nl << "(ref SliceDecoder decoder) => ";
        castToNestedFieldType(out, valueType, ns);
        decodeField(out, valueType, ns, TypeContext::Field);
        out << ")";
        out.dec();
    }
}
