//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <IceUtil/Functional.h>
#include <IceUtil/StringUtil.h>
#include <IceUtil/FileUtil.h>
#include <Gen.h>

#include <limits>
#ifndef _WIN32
#  include <unistd.h>
#else
#  include <direct.h>
#endif

#include <IceUtil/Iterator.h>
#include <IceUtil/UUID.h>
#include <Slice/FileTracker.h>
#include <Slice/Util.h>
#include <string.h>

using namespace std;
using namespace Slice;
using namespace IceUtil;
using namespace IceUtilInternal;

namespace
{

bool
isIdempotent(const OperationPtr& operation)
{
    // TODO: eliminate Nonmutating enumerator in the parser together with the nonmutating metadata.
    return operation->mode() != Operation::Normal;
}

bool
isDefaultInitialized(const DataMemberPtr& member, bool considerDefaultValue)
{
    if (considerDefaultValue && member->defaultValueType())
    {
        return true;
    }

    if (OptionalPtr::dynamicCast(member->type()))
    {
        return true;
    }

    auto st = StructPtr::dynamicCast(member->type());
    if (st)
    {
        for (auto m: st->dataMembers())
        {
            if (!isDefaultInitialized(m, false))
            {
                return false;
            }
        }
        return true;
    }

    return isValueType(member->type());
}

string
opFormatTypeToString(const OperationPtr& op)
{
    switch (op->format())
    {
        case DefaultFormat:
        {
            return "null";
        }
        case CompactFormat:
        {
            return "ZeroC.Ice.FormatType.Compact";
        }
        case SlicedFormat:
        {
            return "ZeroC.Ice.FormatType.Sliced";
        }
        default:
        {
            assert(false);
        }
    }

    return "???";
}

string
getDeprecateReason(const ContainedPtr& p1, const ContainedPtr& p2, const string& type)
{
    string deprecateMetadata, deprecateReason;
    if(p1->findMetaData("deprecate", deprecateMetadata) ||
       (p2 != 0 && p2->findMetaData("deprecate", deprecateMetadata)))
    {
        deprecateReason = "This " + type + " has been deprecated.";
        const string prefix = "deprecate:";
        if(deprecateMetadata.find(prefix) == 0 && deprecateMetadata.size() > prefix.size())
        {
            deprecateReason = deprecateMetadata.substr(prefix.size());
        }
    }
    return deprecateReason;
}

void
emitDeprecate(const ContainedPtr& p1, const ContainedPtr& p2, Output& out, const string& type)
{
    string reason = getDeprecateReason(p1, p2, type);
    if(!reason.empty())
    {
        out << nl << "[global::System.Obsolete(\"" << reason << "\")]";
    }
}

string
getEscapedParamName(const OperationPtr& p, const string& name)
{
    ParamDeclList params = p->parameters();

    for(ParamDeclList::const_iterator i = params.begin(); i != params.end(); ++i)
    {
        if((*i)->name() == name)
        {
            return name + "_";
        }
    }
    return name;
}

string
getEscapedParamName(const ExceptionPtr& p, const string& name)
{
    DataMemberList params = p->allDataMembers();

    for(DataMemberList::const_iterator i = params.begin(); i != params.end(); ++i)
    {
        if((*i)->name() == name)
        {
            return name + "_";
        }
    }
    return name;
}

bool
hasDataMemberWithName(const DataMemberList& dataMembers, const string& name)
{
    return find_if(dataMembers.begin(), dataMembers.end(), [name](const auto& m)
                                                           {
                                                               return m->name() == name;
                                                           }) != dataMembers.end();
}

}

Slice::CsVisitor::CsVisitor(Output& out) :
    _out(out)
{
}

Slice::CsVisitor::~CsVisitor()
{
}

void
Slice::CsVisitor::writeMarshalParams(const OperationPtr& op,
                                     const list<ParamInfo>& requiredParams,
                                     const list<ParamInfo>& taggedParams,
                                     const string& customStream,
                                     const string& obj)
{
    const string stream = customStream.empty() ? "ostr" : customStream;
    string ns = getNamespace(op->interface());

    int bitSequenceIndex = -1;
    size_t bitSequenceSize = 0;
    if (requiredParams.size() > 0)
    {
        if (requiredParams.front().param == 0 || requiredParams.front().param->isOutParam())
        {
            bitSequenceSize = op->returnBitSequenceSize();
        }
        else
        {
            bitSequenceSize = op->inBitSequenceSize();
        }

        if (bitSequenceSize > 0)
        {
            _out << nl << "var bitSequence = " << stream << ".WriteBitSequence(" << bitSequenceSize << ");";
            bitSequenceIndex = 0;
        }
    }

    for (const auto& param : requiredParams)
    {
        writeMarshalCode(_out, param.type, bitSequenceIndex, false, ns, obj + param.name, stream);
    }
    if (bitSequenceSize > 0)
    {
        assert(static_cast<size_t>(bitSequenceIndex) == bitSequenceSize);
    }

    for (const auto& param : taggedParams)
    {
        writeTaggedMarshalCode(_out, OptionalPtr::dynamicCast(param.type), false, ns, obj + param.name, param.tag,
            stream);
    }
}

void
Slice::CsVisitor::writeUnmarshalParams(const OperationPtr& op,
                                       const list<ParamInfo>& requiredParams,
                                       const list<ParamInfo>& taggedParams,
                                       const string& customStream)
{
    const string stream = customStream.empty() ? "istr" : customStream;
    string ns = getNamespace(op->interface());

    int bitSequenceIndex = -1;
    size_t bitSequenceSize = 0;
    if (requiredParams.size() > 0)
    {
        if (requiredParams.front().param == 0 || requiredParams.front().param->isOutParam())
        {
            bitSequenceSize = op->returnBitSequenceSize();
        }
        else
        {
            bitSequenceSize = op->inBitSequenceSize();
        }

        if (bitSequenceSize > 0)
        {
            _out << nl << "var bitSequence = " << stream << ".ReadBitSequence(" << bitSequenceSize << ");";
            bitSequenceIndex = 0;
        }
    }

    for (const auto& param : requiredParams)
    {
        _out << nl << param.typeStr;
        _out << " ";
        writeUnmarshalCode(_out, param.type, bitSequenceIndex, ns, param.name, stream);
    }
    if (bitSequenceSize > 0)
    {
        assert(static_cast<size_t>(bitSequenceIndex) == bitSequenceSize);
    }

    for (const auto& param : taggedParams)
    {
        _out << nl << param.typeStr << " ";
        writeTaggedUnmarshalCode(_out, OptionalPtr::dynamicCast(param.type), ns, param.name, param.tag, nullptr,
            stream);
    }
}

void
Slice::CsVisitor::writeMarshalDataMembers(const DataMemberList& p, const string& ns, unsigned int baseTypes)
{
#ifndef NDEBUG
    int currentTag = -1; // just to verify sortForMarshaling sorts correctly
#endif

    int bitSequenceIndex = -1;
    size_t bitSequenceSize = getBitSequenceSize(p);
    if (bitSequenceSize > 0)
    {
        _out << nl << "var bitSequence = ostr.WriteBitSequence(" << bitSequenceSize << ");";
        bitSequenceIndex = 0;
    }

    for (const auto& member : sortForMarshaling(p))
    {
        if (!member->tagged())
        {
#ifndef NDEBUG
            assert(currentTag == -1);
#endif
            writeMarshalCode(_out, member->type(), bitSequenceIndex, true, ns,
                "this." + fixId(dataMemberName(member), baseTypes), "ostr");
        }
        else
        {
#ifndef NDEBUG
            assert(member->tag() > currentTag);
            currentTag = member->tag();
#endif
            writeTaggedMarshalCode(_out, OptionalPtr::dynamicCast(member->type()), true, ns,
                "this." + fixId(dataMemberName(member), baseTypes), member->tag(), "ostr");
        }
    }

    if (bitSequenceSize > 0)
    {
        assert(static_cast<size_t>(bitSequenceIndex) == bitSequenceSize);
    }
}

void
Slice::CsVisitor::writeUnmarshalDataMembers(const DataMemberList& p, const string& ns, unsigned int baseTypes)
{
    int bitSequenceIndex = -1;
    size_t bitSequenceSize = getBitSequenceSize(p);
    if (bitSequenceSize > 0)
    {
        _out << nl << "var bitSequence = istr.ReadBitSequence(" << bitSequenceSize << ");";
        bitSequenceIndex = 0;
    }

    for (const auto& member : sortForMarshaling(p))
    {
        if (!member->tagged())
        {
            _out << nl;
            writeUnmarshalCode(_out, member->type(), bitSequenceIndex, ns,
                "this." + fixId(dataMemberName(member), baseTypes), "istr");
        }
        else
        {
            _out << nl;
            writeTaggedUnmarshalCode(_out, OptionalPtr::dynamicCast(member->type()), ns,
                "this." + fixId(dataMemberName(member), baseTypes), member->tag(), member, "istr");
        }
    }

    if (bitSequenceSize > 0)
    {
        assert(static_cast<size_t>(bitSequenceIndex) == bitSequenceSize);
    }
}

string
getParamAttributes(const ParamDeclPtr& p)
{
    string result;
    for(const auto& s : p->getMetaData())
    {
        static const string prefix = "cs:attribute:";
        if(s.find(prefix) == 0)
        {
            result += "[" + s.substr(prefix.size()) + "] ";
        }
    }
    return result;
}

vector<string>
getInvocationParams(const OperationPtr& op, const string& ns)
{
    vector<string> params;
    ParamDeclList paramList = op->parameters();
    for(const auto& p : op->parameters())
    {
        if(p->isOutParam())
        {
            continue;
        }
        ostringstream param;
        param << getParamAttributes(p);
        if(StructPtr::dynamicCast(p->type()))
        {
            param << "in ";
        }
        param << CsGenerator::typeToString(p->type(), ns, true)
              << " "
              << fixId(p->name());
        params.push_back(param.str());
    }
    params.push_back("global::System.Collections.Generic.IReadOnlyDictionary<string, string>? " +
                     getEscapedParamName(op, "context") + " = null");
    return params;
}

vector<string>
getInvocationParamsAMI(const OperationPtr& op, const string& ns, bool defaultValues, const string& prefix = "")
{
    vector<string> params;
    for(const auto& p : op->parameters())
    {
        if(p->isOutParam())
        {
            continue;
        }

        ostringstream param;
        param << getParamAttributes(p);
        if(StructPtr::dynamicCast(p->type()))
        {
            param << "in ";
        }
        param << CsGenerator::typeToString(p->type(), ns, true)
              << " "
              << fixId(prefix + p->name());
        params.push_back(param.str());
    }

    string context = prefix.empty() ? getEscapedParamName(op, "context") : "context";
    string progress = prefix.empty() ? getEscapedParamName(op, "progress") : "progress";
    string cancel = prefix.empty() ? getEscapedParamName(op, "cancel") : "cancel";

    if(defaultValues)
    {
        params.push_back("global::System.Collections.Generic.IReadOnlyDictionary<string, string>? " + context + " = null");
        params.push_back("global::System.IProgress<bool>? " + progress + " = null");
        params.push_back("global::System.Threading.CancellationToken " + cancel +
                         " = default");
    }
    else
    {
        params.push_back("global::System.Collections.Generic.IReadOnlyDictionary<string, string>? " + context);
        params.push_back("global::System.IProgress<bool>? " + progress);
        params.push_back("global::System.Threading.CancellationToken " + cancel);
    }
    return params;
}

vector<string>
getInvocationArgsAMI(const OperationPtr& op,
                     const string& context = "",
                     const string& progress = "null",
                     const string cancelationToken = "global::System.Threading.CancellationToken.None",
                     const string& async = "true")
{
    vector<string> args = getNames(getAllInParams(op, true));

    if(context.empty())
    {
        args.push_back(getEscapedParamName(op, "context"));
    }
    else
    {
        args.push_back(context);
    }

    args.push_back(progress);
    args.push_back(cancelationToken);
    args.push_back(async);

    return args;
}

void
Slice::CsVisitor::emitCommonAttributes()
{
   // _out << nl << "[global::System.CodeDom.Compiler.GeneratedCode(\"slice2cs\", \"" << ICE_STRING_VERSION << "\")]";
}

void
Slice::CsVisitor::emitCustomAttributes(const ContainedPtr& p)
{
    StringList metaData = p->getMetaData();
    for(StringList::const_iterator i = metaData.begin(); i != metaData.end(); ++i)
    {
        static const string prefix = "cs:attribute:";
        if(i->find(prefix) == 0)
        {
            _out << nl << '[' << i->substr(prefix.size()) << ']';
        }
    }
}

void
Slice::CsVisitor::emitSerializableAttribute()
{
    _out << nl << "[global::System.Serializable]";
}

void
Slice::CsVisitor::emitTypeIdAttribute(const string& typeId)
{
    _out << nl << "[ZeroC.Ice.TypeId(\"" << typeId << "\")]";
}

string
Slice::CsVisitor::writeValue(const TypePtr& type, const string& ns)
{
    assert(type);

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    if(builtin)
    {
        switch(builtin->kind())
        {
            case Builtin::KindBool:
            {
                return "false";
            }
            case Builtin::KindByte:
            case Builtin::KindShort:
            case Builtin::KindUShort:
            case Builtin::KindInt:
            case Builtin::KindUInt:
            case Builtin::KindVarInt:
            case Builtin::KindVarUInt:
            case Builtin::KindLong:
            case Builtin::KindULong:
            case Builtin::KindVarLong:
            case Builtin::KindVarULong:
            {
                return "0";
            }
            case Builtin::KindFloat:
            {
                return "0.0f";
            }
            case Builtin::KindDouble:
            {
                return "0.0";
            }
            default:
            {
                return "null";
            }
        }
    }

    EnumPtr en = EnumPtr::dynamicCast(type);
    if(en)
    {
        return typeToString(type, ns) + "." + fixId((*en->enumerators().begin())->name());
    }

    StructPtr st = StructPtr::dynamicCast(type);
    if(st)
    {
        return "new " + typeToString(type, ns) + "()";
    }

    return "null";
}

void
Slice::CsVisitor::writeDataMemberDefaultValues(const DataMemberList& members, const string& ns, unsigned int baseTypes)
{
    // This helper function is called only for class/exception data members.

    for (const auto& p: members)
    {
        TypePtr memberType = p->type();
        if (p->defaultValueType())
        {
            _out << nl << "this." << fixId(dataMemberName(p), baseTypes) << " = ";
            writeConstantValue(_out, memberType, p->defaultValueType(), p->defaultValue(), ns);
            _out << ";";
        }
    }
}

void
Slice::CsVisitor::writeSuppressNonNullableWarnings(const DataMemberList& members, unsigned int baseTypes)
{
    // This helper function is called only for class/exception data members.

    for (const auto& p: members)
    {
        TypePtr memberType = p->type();
        BuiltinPtr builtin = BuiltinPtr::dynamicCast(memberType);
        SequencePtr seq = SequencePtr::dynamicCast(memberType);
        DictionaryPtr dict = DictionaryPtr::dynamicCast(memberType);

        if (seq || dict || (builtin && builtin->kind() == Builtin::KindString))
        {
            // This is to suppress compiler warnings for non-nullable fields.
            _out << nl << "this." << fixId(dataMemberName(p), baseTypes) << " = null!;";
        }
    }
}

namespace
{

//
// Convert the identifier part of a Java doc Link tag to a CSharp identifier. If the identifier
// is an interface the link should point to the corresponding generated proxy, we apply the
// case conversions required to match the C# generated code.
//
string
csharpIdentifier(const ContainedPtr& contained, const string& identifier)
{
    string ns = getNamespace(contained);
    string typeName;
    string memberName;
    string::size_type pos = identifier.find('#');
    if(pos == 0)
    {
        memberName = identifier.substr(1);
    }
    else if(pos == string::npos)
    {
        typeName = identifier;
    }
    else
    {
        typeName = identifier.substr(0, pos);
        memberName = identifier.substr(pos + 1);
    }

    // lookup the Slice definition for the identifier
    ContainedPtr definition;
    if(typeName.empty())
    {
        definition = contained;
    }
    else
    {
        TypeList types = contained->unit()->lookupTypeNoBuiltin(typeName, false, true);
        definition = types.empty() ? nullptr : ContainedPtr::dynamicCast(types.front());
    }

    ostringstream os;
    if(!definition || !normalizeCase(definition))
    {
        if(typeName == "::Ice::Object")
        {
            os << "Ice.IObjectPrx";
        }
        else
        {
            os << fixId(typeName);
        }
    }
    else
    {
        InterfaceDefPtr def = InterfaceDefPtr::dynamicCast(definition);
        if(!def)
        {
            InterfaceDeclPtr decl = InterfaceDeclPtr::dynamicCast(definition);
            if(decl)
            {
                def = decl->definition();
            }
        }

        if(def)
        {
            os << getUnqualified(fixId(definition->scope()) + interfaceName(def), ns) << "Prx";
        }
        else
        {
            typeName = fixId(typeName);
            pos = typeName.rfind(".");
            if(pos == string::npos)
            {
                os << pascalCase(fixId(typeName));
            }
            else
            {
                os << typeName.substr(0, pos) << "." << pascalCase(typeName.substr(pos + 1));
            }
        }
    }

    if(!memberName.empty())
    {
        os << "." << (definition && normalizeCase(definition) ? pascalCase(fixId(memberName)) : fixId(memberName));
    }
    string result = os.str();
    //
    // strip global:: prefix if present, it is not supported in doc comment cref attributes
    //
    const string global = "global::";
    if(result.find(global) == 0)
    {
        result = result.substr(global.size());
    }
    return result;
}

vector<string>
splitLines(const string& s)
{
    vector<string> lines;
    istringstream is(s);
    for(string line; getline(is, line, '\n');)
    {
        lines.push_back(trim(line));
    }
    return lines;
}

//
// Transform a Java doc style tag to a C# doc style tag, returns a map indexed by the C#
// tag name attribute and the value contains all the lines in the comment.
//
// @param foo is the Foo argument -> {"foo": ["foo is the Foo argument"]}
//
map<string, vector<string>>
processTag(const string& sourceTag, const string& s)
{
    map<string, vector<string>> result;
    for(string::size_type pos = s.find(sourceTag); pos != string::npos; pos = s.find(sourceTag, pos + 1))
    {
        string::size_type startIdent = s.find_first_not_of(" \t", pos + sourceTag.size());
        string::size_type endIdent = s.find_first_of(" \t", startIdent);
        string::size_type endComment = s.find_first_of("@", endIdent);
        if(endIdent != string::npos)
        {
            string ident = s.substr(startIdent, endIdent - startIdent);
            string comment = s.substr(endIdent + 1,
                                      endComment == string::npos ? endComment : endComment - endIdent - 1);
            result[ident] = splitLines(trim(comment));
        }
    }
    return result;
}

CommentInfo
processComment(const ContainedPtr& contained, const string& deprecateReason)
{
    //
    // Strip HTML markup and javadoc links that are not displayed by Visual Studio.
    //
    string data = contained->comment();
    for(string::size_type pos = data.find('<'); pos != string::npos; pos = data.find('<', pos))
    {
        string::size_type endpos = data.find('>', pos);
        if(endpos == string::npos)
        {
            break;
        }
        data.erase(pos, endpos - pos + 1);
    }

    const string link = "{@link ";
    for(string::size_type pos = data.find(link); pos != string::npos; pos = data.find(link, pos))
    {
        data.erase(pos, link.size());
        string::size_type endpos = data.find('}', pos);
        if(endpos != string::npos)
        {
            string ident = data.substr(pos, endpos - pos);
            data.erase(pos, endpos - pos + 1);
            data.insert(pos, csharpIdentifier(contained, ident));
        }
    }

    const string see = "{@see ";
    for(string::size_type pos = data.find(see); pos != string::npos; pos = data.find(see, pos))
    {
        string::size_type endpos = data.find('}', pos);
        if(endpos != string::npos)
        {
            string ident = data.substr(pos + see.size(), endpos - pos - see.size());
            data.erase(pos, endpos - pos + 1);
            data.insert(pos, "<see cref=\"" + csharpIdentifier(contained, ident) + "\"/>");
        }
    }

    CommentInfo comment;

    const string paramTag = "@param";
    const string throwsTag = "@throws";
    const string exceptionTag = "@exception";
    const string returnTag = "@return";

    string::size_type pos;
    for(pos = data.find('@'); pos != string::npos; pos = data.find('@', pos + 1))
    {
        if((data.substr(pos, paramTag.size()) == paramTag ||
            data.substr(pos, throwsTag.size()) == throwsTag ||
            data.substr(pos, exceptionTag.size()) == exceptionTag ||
            data.substr(pos, returnTag.size()) == returnTag))
        {
            break;
        }
    }

    if(pos > 0)
    {
        ostringstream os;
        os << trim(data.substr(0, pos));
        if(!deprecateReason.empty())
        {
            os << "<para>" << deprecateReason << "</para>";
        }
        comment.summaryLines = splitLines(os.str());
    }

    if(comment.summaryLines.empty() && !deprecateReason.empty())
    {
        comment.summaryLines.push_back(deprecateReason);
    }

    comment.params = processTag("@param", data);
    comment.exceptions = processTag("@throws", data);

    pos = data.find(returnTag);
    if(pos != string::npos)
    {
        pos += returnTag.size();
        string::size_type endComment = data.find("@", pos);
        comment.returnLines = splitLines(
            trim(data.substr(pos , endComment == string::npos ? endComment : endComment - pos)));
    }

    return comment;
}

}

void writeDocCommentLines(IceUtilInternal::Output& out, const vector<string>& lines)
{
    for(vector<string>::const_iterator i = lines.begin(); i != lines.end(); ++i)
    {
        if(i == lines.begin())
        {
            out << *i;
        }
        else
        {
            out << nl << "///";
            if(!i->empty())
            {
                out << " " << (*i);
            }
        }
    }
}

void writeDocCommentLines(IceUtilInternal::Output& out,
                          const vector<string>& lines,
                          const string& tag,
                          const string& name = "",
                          const string& value = "")
{
    if (!lines.empty())
    {
        out << nl << "/// <" << tag;
        if (!name.empty())
        {
            out << " " << name << "=\"" << value << "\"";
        }
        out << ">";
        writeDocCommentLines(out, lines);
        out << "</" << tag << ">";
    }
}

void
Slice::CsVisitor::writeTypeDocComment(const ContainedPtr& p,
                                      const string& deprecateReason)
{
    CommentInfo comment = processComment(p, deprecateReason);
    writeDocCommentLines(_out, comment.summaryLines, "summary");
}

void
Slice::CsVisitor::writeProxyDocComment(const InterfaceDefPtr& p, const std::string& deprecatedReason)
{
    CommentInfo comment = processComment(p, deprecatedReason);
    comment.summaryLines.insert(comment.summaryLines.cbegin(),
        "Proxy interface used to call remote Ice objects that implement Slice interface " + p->name() + ".");
    comment.summaryLines.push_back("<seealso cref=\"" + fixId(interfaceName(p)) + "\"/>.");
    writeDocCommentLines(_out, comment.summaryLines, "summary");
}

void
Slice::CsVisitor::writeServantDocComment(const InterfaceDefPtr& p, const std::string& deprecatedReason)
{
    CommentInfo comment = processComment(p, deprecatedReason);
    comment.summaryLines.insert(comment.summaryLines.cbegin(),
        "Interface used to implement servants for Slice interface " + p->name() + ".");
    comment.summaryLines.push_back("<seealso cref=\"" + interfaceName(p) + "Prx\"/>.");
    writeDocCommentLines(_out, comment.summaryLines, "summary");
}

void
Slice::CsVisitor::writeOperationDocComment(const OperationPtr& p, const string& deprecateReason,
                                           bool dispatch, bool async)
{
    CommentInfo comment = processComment(p, deprecateReason);
    writeDocCommentLines(_out, comment.summaryLines, "summary");
    writeParamDocComment(p, comment, InParam);

    list<ParamInfo> outParams = getAllOutParams(p, dispatch, "", true);

    if(dispatch)
    {
        _out << nl << "/// <param name=\"" << getEscapedParamName(p, "current")
             << "\">The Current object for the dispatch.</param>";
    }
    else
    {
        _out << nl << "/// <param name=\"" << getEscapedParamName(p, "context")
             << "\">Context map to send with the invocation.</param>";

        if(async)
        {
            _out << nl << "/// <param name=\"" << getEscapedParamName(p, "progress")
                 << "\">Sent progress provider.</param>";
            _out << nl << "/// <param name=\"" << getEscapedParamName(p, "cancel")
                 << "\">A cancellation token that receives the cancellation requests.</param>";
        }
    }

    if(dispatch && p->hasMarshaledResult())
    {
        _out << nl << "/// <returns>The operation marshaled result.</returns>";
    }
    else if(async)
    {
        _out << nl << "/// <returns>The task object representing the asynchronous operation.</returns>";
    }
    else if(outParams.size() == 1)
    {
        writeDocCommentLines(_out, comment.returnLines, "returns");
    }
    else if(outParams.size() > 1)
    {
        _out << nl << "/// <returns>Named tuple with the following fields:";

        for(const auto& paramInfo : outParams)
        {
            if(paramInfo.param)
            {
                auto i = comment.params.find(paramInfo.name);
                if(i != comment.params.end())
                {
                    _out << nl << "/// <para> " << paramName(paramInfo) << ": ";
                    writeDocCommentLines(_out, i->second);
                    _out << "</para>";
                }
            }
            else if(!comment.returnLines.empty()) // Return type
            {
                _out << nl << "/// <para> " << paramInfo.name << ": ";
                writeDocCommentLines(_out, comment.returnLines);
                _out << "</para>";
            }
        }
        _out << nl << "/// </returns>";
    }

    for(const auto& e : comment.exceptions)
    {
        writeDocCommentLines(_out, e.second, "exceptions", "cref", e.first);
    }
}

void
Slice::CsVisitor::writeParamDocComment(const OperationPtr& p, const CommentInfo& comment, ParamDir paramType)
{
    //
    // Collect the names of the in- or -out parameters to be documented.
    //
    for(const auto param : p->parameters())
    {
        if((param->isOutParam() && paramType == OutParam) || (!param->isOutParam() && paramType == InParam))
        {
            auto i = comment.params.find(param->name());
            if(i != comment.params.end())
            {
                writeDocCommentLines(_out, i->second, "param", "name", fixId(param->name()));
            }
        }
    }
}

void
Slice::CsVisitor::openNamespace(const ModulePtr& p, string prefix)
{
    if (prefix.empty())
    {
        if (_namespaceStack.empty())
        {
            // If it's a top-level module, check if it's itself enclosed in a namespace.
            prefix = getNamespacePrefix(p);
        }
        else
        {
            prefix = _namespaceStack.top();
        }
    }
    if (!prefix.empty())
    {
        prefix += ".";
    }

    if (p->hasOnlySubModules())
    {
        _namespaceStack.push(prefix + fixId(p->name()));
    }
    else
    {
        _out << sp;
        emitCustomAttributes(p);
        _out << nl << "namespace " << prefix << fixId(p->name());
        _out << sb;

        _namespaceStack.push("");
    }
}

void
Slice::CsVisitor::closeNamespace()
{
    if (_namespaceStack.top().empty())
    {
        _out << eb;
    }
    _namespaceStack.pop();
}

Slice::Gen::Gen(const string& base, const vector<string>& includePaths, const string& dir, bool impl) :
    _includePaths(includePaths)
{
    string fileBase = base;
    string::size_type pos = base.find_last_of("/\\");
    if(pos != string::npos)
    {
        fileBase = base.substr(pos + 1);
    }
    string file = fileBase + ".cs";
    string fileImpl = fileBase + "I.cs";

    if(!dir.empty())
    {
        file = dir + '/' + file;
        fileImpl = dir + '/' + fileImpl;
    }

    _out.open(file.c_str());
    if(!_out)
    {
        ostringstream os;
        os << "cannot open `" << file << "': " << IceUtilInternal::errorToString(errno);
        throw FileException(__FILE__, __LINE__, os.str());
    }
    FileTracker::instance()->addFile(file);

    printHeader();
    printGeneratedHeader(_out, fileBase + ".ice");

    _out << nl << "#nullable enable";
    // Disable some analyzer warnings in the generated code
    _out << nl << "#pragma warning disable SA1300 // Element must begin with upper case letter";
    _out << nl << "#pragma warning disable SA1306 // Field names must begin with lower case letter";
    _out << nl << "#pragma warning disable SA1309 // Field names must not begin with underscore";
    _out << nl << "#pragma warning disable SA1312 // Variable names must begin with lower case letter";
    _out << nl << "#pragma warning disable SA1313 // Parameter names must begin with lower case letter";

    _out << sp << nl << "#pragma warning disable 1591"; // See bug 3654
    if(impl)
    {
        IceUtilInternal::structstat st;
        if(!IceUtilInternal::stat(fileImpl, &st))
        {
            ostringstream os;
            os << "`" << fileImpl << "' already exists - will not overwrite";
            throw FileException(__FILE__, __LINE__, os.str());
        }

        _impl.open(fileImpl.c_str());
        if(!_impl)
        {
            ostringstream os;
            os << ": cannot open `" << fileImpl << "': " << IceUtilInternal::errorToString(errno);
            throw FileException(__FILE__, __LINE__, os.str());
        }

        FileTracker::instance()->addFile(fileImpl);
    }
}

Slice::Gen::~Gen()
{
    if(_out.isOpen())
    {
        _out << '\n';
    }
    if(_impl.isOpen())
    {
        _impl << '\n';
    }
}

void
Slice::Gen::generate(const UnitPtr& p)
{
    CsGenerator::validateMetaData(p);

    UnitVisitor unitVisitor(_out);
    p->visit(&unitVisitor, false);

    TypesVisitor typesVisitor(_out);
    p->visit(&typesVisitor, false);

    ProxyVisitor proxyVisitor(_out);
    p->visit(&proxyVisitor, false);

    DispatcherVisitor dispatcherVisitor(_out, false);
    p->visit(&dispatcherVisitor, false);

    DispatcherVisitor asyncDispatcherVisitor(_out, true);
    p->visit(&asyncDispatcherVisitor, false);

    ClassFactoryVisitor classFactoryVisitor(_out);
    p->visit(&classFactoryVisitor, false);

    CompactIdVisitor compactIdVisitor(_out);
    p->visit(&compactIdVisitor, false);

    RemoteExceptionFactoryVisitor remoteExceptionFactoryVisitor(_out);
    p->visit(&remoteExceptionFactoryVisitor, false);
}

void
Slice::Gen::generateImpl(const UnitPtr& p)
{
    ImplVisitor implVisitor(_impl);
    p->visit(&implVisitor, false);
}

void
Slice::Gen::closeOutput()
{
    _out.close();
    _impl.close();
}

void
Slice::Gen::printHeader()
{
    static const char* header =
        "//\n"
        "// Copyright (c) ZeroC, Inc. All rights reserved.\n"
        "//\n";

    _out << header;
    _out << "// Ice version " << ICE_STRING_VERSION << "\n";
}

Slice::Gen::UnitVisitor::UnitVisitor(IceUtilInternal::Output& out) :
    CsVisitor(out)
{
}

bool
Slice::Gen::UnitVisitor::visitUnitStart(const UnitPtr& p)
{
    DefinitionContextPtr dc = p->findDefinitionContext(p->topLevelFile());
    assert(dc);
    StringList globalMetaData = dc->getMetaData();

    static const string attributePrefix = "cs:attribute:";

    bool sep = false;
    for(StringList::const_iterator q = globalMetaData.begin(); q != globalMetaData.end(); ++q)
    {
        string::size_type pos = q->find(attributePrefix);
        if(pos == 0 && q->size() > attributePrefix.size())
        {
            if(!sep)
            {
                _out << sp;
                sep = true;
            }
            string attrib = q->substr(pos + attributePrefix.size());
            _out << nl << '[' << attrib << ']';
        }
    }
    return false;
}

Slice::Gen::TypesVisitor::TypesVisitor(IceUtilInternal::Output& out) :
    CsVisitor(out)
{
}

bool
Slice::Gen::TypesVisitor::visitModuleStart(const ModulePtr& p)
{
    if (p->hasOnlyClassDecls() || p->hasOnlyInterfaces())
    {
        return false; // avoid empty namespace
    }

    openNamespace(p);

    // Write constants if there are any
    if (!p->consts().empty())
    {
        emitCommonAttributes();
        _out << nl << "public static partial class Constants";
        _out << sb;
        bool firstOne = true;
        for (auto q : p->consts())
        {
            if (firstOne)
            {
                firstOne = false;
            }
            else
            {
                _out << sp;
            }

            // TODO: doc comments

            string name = fixId(q->name());
            string ns = getNamespace(q);
            emitCustomAttributes(q);
            _out << nl << "public const " << typeToString(q->type(), ns) << " " << name << " = ";
            writeConstantValue(_out, q->type(), q->valueType(), q->value(), ns);
            _out << ";";
        }
        _out << eb;
    }
    return true;
}

void
Slice::Gen::TypesVisitor::visitModuleEnd(const ModulePtr&)
{
    closeNamespace();
}

bool
Slice::Gen::TypesVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    string name = p->name();
    string scoped = fixId(p->scoped(), Slice::ObjectType);
    string ns = getNamespace(p);
    _out << sp;
    writeTypeDocComment(p, getDeprecateReason(p, 0, "type"));

    emitCommonAttributes();
    emitSerializableAttribute();
    emitTypeIdAttribute(p->scoped());
    emitCustomAttributes(p);
    _out << nl << "public partial class " << fixId(name) << " : "
         << (p->base() ? getUnqualified(p->base(), ns) : "ZeroC.Ice.AnyClass")
         << sb;
    return true;
}

void
Slice::Gen::TypesVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    string name = fixId(p->name());
    string ns = getNamespace(p);
    DataMemberList dataMembers = p->dataMembers();
    DataMemberList allDataMembers = p->allDataMembers();
    bool hasBaseClass = p->base();

    _out << sp;
    _out << nl << "public static readonly new ZeroC.Ice.InputStreamReader<" << name << "> IceReader =";
    _out.inc();
    _out << nl << "istr => istr.ReadClass<" << name << ">(IceTypeId);";
    _out.dec();

    _out << sp;
    _out << nl << "public static readonly new ZeroC.Ice.InputStreamReader<" << name << "?> IceReaderIntoNullable =";
    _out.inc();
    _out << nl << "istr => istr.ReadNullableClass<" << name << ">(IceTypeId);";
    _out.dec();

    _out << sp;
    _out << nl << "public static " << (hasBaseClass ? "new " : "") << "string IceTypeId => _iceAllTypeIds[0];";

    _out << sp;
    _out << nl << "public static readonly new ZeroC.Ice.OutputStreamWriter<" << name << "> IceWriter =";
    _out.inc();
    _out << nl << "(ostr, value) => ostr.WriteClass(value, IceTypeId);";
    _out.dec();

    _out << sp;
    _out << nl << "public static readonly new ZeroC.Ice.OutputStreamWriter<" << name << "?> IceWriterFromNullable =";
    _out.inc();
    _out << nl << "(ostr, value) => ostr.WriteNullableClass(value, IceTypeId);";
    _out.dec();

    _out << sp;
    _out << nl << "private static readonly string[] _iceAllTypeIds = ZeroC.Ice.TypeExtensions.GetAllIceTypeIds(typeof("
         << name << "));";

    bool partialInitialize = !hasDataMemberWithName(allDataMembers, "Initialize");
    if(partialInitialize)
    {
        _out << sp << nl << "partial void Initialize();";
    }

    if (allDataMembers.empty())
    {
        // There is always at least another constructor, so we need to generate the parameterless constructor.
        _out << sp;
        _out << nl << "public " << name << spar << epar;
        _out << sb;
        if (partialInitialize)
        {
            _out << nl << "Initialize();";
        }
        _out << eb;
    }
    else
    {
        // "One-shot" constructor
        _out << sp;
        _out << nl << "public " << name
             << spar
             << mapfn<DataMemberPtr>(allDataMembers,
                                     [&ns](const auto& i)
                                     {
                                         return typeToString(i->type(), ns) + " " + fixId(i->name());
                                     })
             << epar;
        if (hasBaseClass && allDataMembers.size() != dataMembers.size())
        {
            _out.inc();
            _out << nl << ": base" << spar;
            vector<string> baseParamNames;
            for (const auto& d : p->base()->allDataMembers())
            {
                baseParamNames.push_back(fixId(d->name()));
            }
            _out << baseParamNames << epar;
            _out.dec();
        } // else we call implicitly the parameterless constructor of the base class (if there is a base class).

        _out << sb;
        for (const auto& d: dataMembers)
        {
            _out << nl << "this." << fixId(dataMemberName(d), Slice::ObjectType) << " = "
                 << fixId(d->name(), Slice::ObjectType) << ";";
        }
        if (partialInitialize)
        {
            _out << nl << "Initialize();";
        }
        _out << eb;

        // Second public constructor for all data members minus those with a default initializer. Can be parameterless.
        DataMemberList allMandatoryDataMembers;
        for (const auto& member: allDataMembers)
        {
            if (!isDefaultInitialized(member, true))
            {
                allMandatoryDataMembers.push_back(member);
            }
        }

        if (allMandatoryDataMembers.size() < allDataMembers.size()) // else, it's identical to the first ctor.
        {
            _out << sp;
            _out << nl << "public " << name
                << spar
                << mapfn<DataMemberPtr>(allMandatoryDataMembers,
                                       [&ns](const auto &i) {
                                            return typeToString(i->type(), ns) + " " + fixId(i->name());
                                       })
                << epar;
            if (hasBaseClass)
            {
                vector<string> baseParamNames;
                for (const auto& d: p->base()->allDataMembers())
                {
                    if (!isDefaultInitialized(d, true))
                    {
                        baseParamNames.push_back(fixId(d->name()));
                    }
                }
                if (!baseParamNames.empty())
                {
                    _out.inc();
                    _out << nl << ": base" << spar << baseParamNames << epar;
                    _out.dec();
                }
                // else we call implicitly the parameterless constructor of the base class.
            }
            _out << sb;
            for (const auto& d : dataMembers)
            {
                if (!isDefaultInitialized(d, true))
                {
                    _out << nl << "this." << fixId(dataMemberName(d), Slice::ObjectType) << " = "
                         << fixId(d->name(), Slice::ObjectType) << ";";
                }
            }
            writeDataMemberDefaultValues(dataMembers, ns, ObjectType);
            if (partialInitialize)
            {
                _out << nl << "Initialize();";
            }
            _out << eb;
        }
    }

    // protected internal constructor used for unmarshaling (always generated).
    // the factory parameter is used to distinguish this ctor from the parameterless ctor that users may want to add to
    // the partial class; it's not used otherwise.
    _out << sp;
    if (!hasBaseClass)
    {
        _out << nl << "[global::System.Diagnostics.CodeAnalysis.SuppressMessage(\"Microsoft.Performance\", "
            << "\"CA1801:ReviewUnusedParameters\", Justification=\"Special constructor used for Ice unmarshaling\")]";
    }
    _out << nl << "protected internal " << name << "(ZeroC.Ice.IClassFactory factory)";
    if (hasBaseClass)
    {
        // We call the base class constructor to initialize the base class fields.
        _out.inc();
        _out << nl << ": base(factory)";
        _out.dec();
    }
    _out << sb;
    writeSuppressNonNullableWarnings(dataMembers, ObjectType);
    _out << eb;

    writeMarshaling(p);
    _out << eb;
}

void
Slice::Gen::TypesVisitor::writeMarshaling(const ClassDefPtr& p)
{
    string name = fixId(p->name());
    string scoped = p->scoped();
    string ns = getNamespace(p);
    ClassList allBases = p->allBases();

    //
    // Marshaling support
    //
    DataMemberList members = p->dataMembers();
    const bool basePreserved = p->inheritsMetaData("preserve-slice");
    const bool preserved = p->hasMetaData("preserve-slice");

    ClassDefPtr base = p->base();

    if(preserved && !basePreserved)
    {
        _out << sp;
        _out << nl << "protected override ZeroC.Ice.SlicedData? IceSlicedData { get; set; }";
    }

    _out << sp;
    _out << nl << "protected override void IceWrite(ZeroC.Ice.OutputStream ostr, bool firstSlice)";
    _out << sb;
    _out << nl << "if (firstSlice)";
    _out << sb;
    _out << nl << "ostr.IceStartFirstSlice(_iceAllTypeIds";
    if (preserved || basePreserved)
    {
        _out << ", IceSlicedData";
    }
    if (p->compactId() >= 0)
    {
        _out << ", compactId: " << p->compactId();
    }
    _out << ");";
    _out << eb;
    _out << nl << "else";
    _out << sb;
    _out << nl << "ostr.IceStartNextSlice(IceTypeId);";
    _out << eb;

    writeMarshalDataMembers(members, ns, 0);

    if(base)
    {
        _out << nl << "ostr.IceEndSlice(false);";
        _out << nl << "base.IceWrite(ostr, false);";
    }
    else
    {
         _out << nl << "ostr.IceEndSlice(true);"; // last slice
    }
    _out << eb;

    _out << sp;

    _out << nl << "protected override void IceRead(ZeroC.Ice.InputStream istr, bool firstSlice)";
    _out << sb;
    _out << nl << "if (firstSlice)";
    _out << sb;
    if (preserved || basePreserved)
    {
        _out << nl << "IceSlicedData = ";
    }
    else
    {
        _out << nl << "_ = ";
    }
    _out << "istr.IceStartFirstSlice();";
    _out << eb;
    _out << nl << "else";
    _out << sb;
    _out << nl << "istr.IceStartNextSlice();";
    _out << eb;

    writeUnmarshalDataMembers(members, ns, 0);

    _out << nl << "istr.IceEndSlice();";
    if (base)
    {
        _out << nl << "base.IceRead(istr, false);";
    }
    // This slice and its base slices (if any) are now fully initialized.
    if (!hasDataMemberWithName(p->allDataMembers(), "Initialize"))
    {
        _out << nl << "Initialize();";
    }
    _out << eb;
}

bool
Slice::Gen::TypesVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    string name = fixId(p->name());
    string ns = getNamespace(p);
    ExceptionPtr base = p->base();

    _out << sp;
    writeTypeDocComment(p, getDeprecateReason(p, 0, "type"));
    emitDeprecate(p, 0, _out, "type");

    emitCommonAttributes();
    emitSerializableAttribute();
    emitTypeIdAttribute(p->scoped());
    emitCustomAttributes(p);
    _out << nl << "public partial class " << name << " : ";
    if(base)
    {
        _out << getUnqualified(base, ns);
    }
    else
    {
        _out << "ZeroC.Ice.RemoteException";
    }
    _out << sb;
    return true;
}

void
Slice::Gen::TypesVisitor::visitExceptionEnd(const ExceptionPtr& p)
{
    string name = fixId(p->name());
    string ns = getNamespace(p);
    DataMemberList allDataMembers = p->allDataMembers();
    DataMemberList dataMembers = p->dataMembers();

    string messageParamName = getEscapedParamName(p, "message");
    string innerExceptionParamName = getEscapedParamName(p, "innerException");

    bool hasPublicParameterlessCtor = true;
    vector<string> allParamDecl;
    for(DataMemberList::const_iterator q = allDataMembers.begin(); q != allDataMembers.end(); ++q)
    {
        DataMemberPtr member = *q;
        string memberName = fixId(member->name());
        string memberType = typeToString(member->type(), ns);
        allParamDecl.push_back(memberType + " " + memberName);

        if (hasPublicParameterlessCtor)
        {
            hasPublicParameterlessCtor = isDefaultInitialized(member, true);
        }
    }

    vector<string> baseParamNames;
    if(p->base())
    {
        DataMemberList baseDataMembers = p->base()->allDataMembers();
        for(DataMemberList::const_iterator q = baseDataMembers.begin(); q != baseDataMembers.end(); ++q)
        {
            baseParamNames.push_back(fixId((*q)->name()));
        }
    }

    _out << nl << "private readonly string[] _iceAllTypeIds = ZeroC.Ice.TypeExtensions.GetAllIceTypeIds(typeof("
         << name << "));";

    // Up to 3 "one-shot" constructors
    for (int i = 0; i < 3; i++)
    {
        if (allParamDecl.size() > 0)
        {
            _out << sp;
            _out << nl << "public " << name << spar << allParamDecl << epar;
            _out.inc();
            if (baseParamNames.size() > 0)
            {
                _out << nl << ": base" << spar << baseParamNames << epar;
            }
            // else we use the base's parameterless ctor.
            _out.dec();
            _out << sb;
            if (!dataMembers.empty())
            {
                for (DataMemberList::const_iterator q = dataMembers.begin(); q != dataMembers.end(); ++q)
                {
                    string memberName = fixId(dataMemberName(*q), Slice::ExceptionType);
                    _out << nl << "this." << memberName << " = " << fixId((*q)->name()) << ';';
                }
            }
            _out << eb;
        }

        if (i == 0)
        {
            // Insert message first
            allParamDecl.insert(allParamDecl.cbegin(), "string? " + messageParamName);
            baseParamNames.insert(baseParamNames.cbegin(), messageParamName);
        }
        else if (i == 1)
        {
            // Also add innerException
            allParamDecl.push_back("global::System.Exception? " + innerExceptionParamName);
            baseParamNames.push_back(innerExceptionParamName);
        }
    }

    // public parameterless constructor (not always generated, see class comment)
    if (hasPublicParameterlessCtor)
    {
        _out << sp;
        _out << nl << "public " << name << "()";
        _out << sb;
        writeDataMemberDefaultValues(dataMembers, ns, Slice::ExceptionType);
        _out << eb;
    }

    if(!dataMembers.empty())
    {
        _out << sp;
        _out << nl << "public override void GetObjectData(global::System.Runtime.Serialization.SerializationInfo info, "
             << "global::System.Runtime.Serialization.StreamingContext context)";
        _out << sb;
        for(DataMemberList::const_iterator q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            DataMemberPtr m = *q;
            TypePtr memberType = unwrapIfOptional(m->type());

            string mName = fixId(dataMemberName(m), Slice::ExceptionType);
            if(m->tagged() && isValueType(memberType))
            {
                _out << nl << "if (" << mName << " != null)";
                _out << sb;
            }
            _out << nl << "info.AddValue(\"" << mName << "\", " << mName;

            if(m->tagged() && isValueType(memberType))
            {
                _out << ".Value";
            }

            if(ContainedPtr::dynamicCast(memberType))
            {
                _out << ", typeof(" << typeToString(memberType, ns) << ")";
            }

            _out << ");";

            if(m->tagged() && isValueType(memberType))
            {
                _out << eb;
            }
        }
        _out << sp << nl << "base.GetObjectData(info, context);";
        _out << eb;
    }

    // protected internal constructor used for unmarshaling (always generated).
    // the factory parameter is used to distinguish this ctor from the parameterless ctor that users may want to add to
    // the partial class; it's not used otherwise.
    _out << sp;
    if (!p->base())
    {
        _out << nl << "[global::System.Diagnostics.CodeAnalysis.SuppressMessage(\"Microsoft.Performance\", "
            << "\"CA1801:ReviewUnusedParameters\", Justification=\"Special constructor used for Ice unmarshaling\")]";
    }
    _out << nl << "protected internal " << name << "(ZeroC.Ice.IRemoteExceptionFactory factory, string? message)";
    // We call the base class constructor to initialize the base class fields.
    _out.inc();
    if (p->base())
    {
        _out << nl << ": base(factory, message)";
    }
    else
    {
        _out << nl << ": base(message)";
    }
    _out.dec();
    _out << sb;
    writeSuppressNonNullableWarnings(dataMembers, Slice::ExceptionType);
    _out << eb;

    // Serializable constructor
    _out << sp;
    _out << nl << "protected " << name << "(global::System.Runtime.Serialization.SerializationInfo info, "
         << "global::System.Runtime.Serialization.StreamingContext context)";
    _out.inc();
    _out << nl << ": base(info, context)";
    _out.dec();
    _out << sb;
    if(!dataMembers.empty())
    {
        bool hasTaggedMembers = false;
        static const std::array<std::string, 17> builtinGetter =
        {
            "GetBoolean",
            "GetByte",
            "GetInt16",
            "GetUInt16",
            "GetInt32",
            "GetUInt32",
            "GetInt32",
            "GetUInt32",
            "GetInt64",
            "GetUInt64",
            "GetInt64",
            "GetUInt64",
            "GetSingle",
            "GetDouble",
            "GetString",
            "",
            "",
        };

        for(DataMemberList::const_iterator q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            DataMemberPtr m = *q;
            TypePtr memberType = unwrapIfOptional(m->type());

            if(m->tagged() && isValueType(memberType))
            {
                hasTaggedMembers = true;
                continue;
            }
            string getter;
            BuiltinPtr builtin = BuiltinPtr::dynamicCast(memberType);
            if(builtin)
            {
                getter = builtinGetter[builtin->kind()];
            }
            if(getter.empty())
            {
                getter = "GetValue";
            }
            string mName = fixId(dataMemberName(m), Slice::ExceptionType);
            _out << nl << "this." << mName << " = ";

            if(getter == "GetValue")
            {
                _out << "(" << typeToString(memberType, ns) << ")";
            }
            _out << "info." << getter << "(\"" << mName << "\"";
            if(getter == "GetValue")
            {
                _out << ", typeof(" << typeToString(memberType, ns) << ")";
            }
            _out << ")!;";
        }

        if(hasTaggedMembers)
        {
            _out << nl << "foreach (var entry in info)";
            _out << sb;
            _out << nl << "switch (entry.Name)";
            _out << sb;
            for(DataMemberList::const_iterator q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                DataMemberPtr m = *q;
                TypePtr memberType = unwrapIfOptional(m->type());
                if(!m->tagged() || !isValueType(memberType))
                {
                    continue;
                }
                string mName = fixId(dataMemberName(m), Slice::ExceptionType);
                _out << nl << "case \"" << mName << "\":";
                _out << sb;
                _out << nl << "this." << mName << " = ("
                    << typeToString(memberType, ns) << ") entry.Value!;";
                _out << nl << "break;";
                _out << eb;
            }
            _out << eb;
            _out << eb;
        }
    }
    _out << eb;

    string scoped = p->scoped();
    ExceptionPtr base = p->base();

    // Remote exceptions are always "preserved".

    _out << sp;
    _out << nl << "protected override void IceRead(ZeroC.Ice.InputStream istr, bool firstSlice)";
    _out << sb;
    _out << nl << "if (firstSlice)";
    _out << sb;
    _out << nl << "IceSlicedData = istr.IceStartFirstSlice();";
    _out << nl << "ConvertToUnhandled = true;";
    _out << eb;
    _out << nl << "else";
    _out << sb;
    _out << nl << "istr.IceStartNextSlice();";
    _out << eb;
    writeUnmarshalDataMembers(dataMembers, ns, Slice::ExceptionType);
    _out << nl << "istr.IceEndSlice();";

    if (base)
    {
        _out << nl << "base.IceRead(istr, false);";
    }
    _out << eb;

    _out << sp;
    _out << nl << "protected override void IceWrite(ZeroC.Ice.OutputStream ostr, bool firstSlice)";
    _out << sb;
    _out << nl << "if (firstSlice)";
    _out << sb;
    _out << nl << "ostr.IceStartFirstSlice(_iceAllTypeIds, IceSlicedData, errorMessage: Message);";
    _out << eb;
    _out << nl << "else";
    _out << sb;
    _out << nl << "ostr.IceStartNextSlice(_iceAllTypeIds[0]);";
    _out << eb;
    writeMarshalDataMembers(dataMembers, ns, Slice::ExceptionType);

    if(base)
    {
        _out << nl << "ostr.IceEndSlice(false);"; // the current slice is not last slice
        _out << nl << "base.IceWrite(ostr, false);"; // the next one is not the first slice
    }
    else
    {
        _out << nl << "ostr.IceEndSlice(true);"; // this is the last slice.
    }
    _out << eb;

    _out << eb;
}

bool
Slice::Gen::TypesVisitor::visitStructStart(const StructPtr& p)
{
    string name = fixId(p->name());
    string ns = getNamespace(p);
    _out << sp;

    writeTypeDocComment(p, getDeprecateReason(p, 0, "type"));
    emitDeprecate(p, 0, _out, "type");
    emitCommonAttributes();
    emitSerializableAttribute();
    emitCustomAttributes(p);
    _out << nl << "public ";
    if(p->hasMetaData("cs:readonly"))
    {
        _out << "readonly ";
    }
    _out << "partial struct " << name <<  " : global::System.IEquatable<" << name << ">, ZeroC.Ice.IStreamableStruct";
    _out << sb;

    _out << sp;
    _out << nl << "public static ZeroC.Ice.InputStreamReader<" << name << "> IceReader =";
    _out.inc();
    _out << nl << "istr => new " << name << "(istr);";
    _out.dec();

    _out << sp;
    _out << nl << "public static ZeroC.Ice.OutputStreamValueWriter<" << name << "> IceWriter =";
    _out.inc();
    _out << nl << "(ZeroC.Ice.OutputStream ostr, in " << name << " value) => value.IceWrite(ostr);";
    _out.dec();
    return true;
}

void
Slice::Gen::TypesVisitor::visitStructEnd(const StructPtr& p)
{
    string name = fixId(p->name());
    string scope = fixId(p->scope());
    string ns = getNamespace(p);
    DataMemberList dataMembers = p->dataMembers();

    bool partialInitialize = !hasDataMemberWithName(dataMembers, "Initialize");

    if(partialInitialize)
    {
        _out << sp << nl << "partial void Initialize();";
    }

    _out << sp;
    _out << nl << "public ";
    _out << name
         << spar
         << mapfn<DataMemberPtr>(dataMembers,
                                 [&ns](const auto& i)
                                 {
                                     return typeToString(i->type(), ns) + " " + fixId(i->name());
                                 })
         << epar;
    _out << sb;
    for(const auto& i : dataMembers)
    {
        string paramName = fixId(i->name());
        string memberName = fixId(dataMemberName(i), Slice::ObjectType);
        _out << nl << (paramName == memberName ? "this." : "") << memberName  << " = " << paramName << ";";
    }
    if(partialInitialize)
    {
        _out << nl << "Initialize();";
    }
    _out << eb;

    _out << sp;
    _out << nl << "public " << name << "(ZeroC.Ice.InputStream istr)";
    _out << sb;

    writeUnmarshalDataMembers(dataMembers, ns, 0);

    if(partialInitialize)
    {
        _out << nl << "Initialize();";
    }

    _out << eb;

    _out << sp;
    _out << nl << "public override int GetHashCode()";
    _out << sb;
    _out << nl << "var hash = new global::System.HashCode();";
    for(const auto& i : dataMembers)
    {
        _out << nl << "hash.Add(this." << fixId(dataMemberName(i), Slice::ObjectType) << ");";
    }
    _out << nl << "return hash.ToHashCode();";
    _out << eb;

    //
    // Equals implementation
    //
    _out << sp;
    _out << nl << "public bool Equals(" << fixId(p->name()) << " other)";

    _out << " =>";
    _out.inc();
    _out << nl;
    for(DataMemberList::const_iterator q = dataMembers.begin(); q != dataMembers.end();)
    {
        string mName = fixId(dataMemberName(*q));
        TypePtr mType = (*q)->type();

        if (mType->isInterfaceType())
        {
            _out << "ZeroC.Ice.IObjectPrx.Equals(this." << mName << ", other." << mName << ")";
        }
        else
        {
            _out << "this." << mName << " == other." << mName;
        }

        if(++q != dataMembers.end())
        {
            _out << " &&" << nl;
        }
        else
        {
            _out << ";";
        }
    }
    _out.dec();

    _out << sp;
    _out << nl << "public override bool Equals(object? other) => other is " << name << " value && this.Equals(value);";

    _out << sp;
    _out << nl << "public static bool operator ==(" << name << " lhs, " << name << " rhs)";
    _out << " => lhs.Equals(rhs);";

    _out << sp;
    _out << nl << "public static bool operator !=(" << name << " lhs, " << name << " rhs)";
    _out << " => !lhs.Equals(rhs);";

    _out << sp;
    _out << nl << "public readonly void IceWrite(ZeroC.Ice.OutputStream ostr)";
    _out << sb;

    writeMarshalDataMembers(dataMembers, ns, 0);

    _out << eb;
    _out << eb;
}

void
Slice::Gen::TypesVisitor::visitEnum(const EnumPtr& p)
{
    string name = fixId(p->name());
    string ns = getNamespace(p);
    string scoped = fixId(p->scoped());
    EnumeratorList enumerators = p->enumerators();

    // When the number of enumerators is smaller than the distance between the min and max values, the values are not
    // consecutive and we need to use a set to validate the value during unmarshaling.
    // Note that the values are not necessarily in order, e.g. we can use a simple range check for
    // enum E { A = 3, B = 2, C = 1 } during unmarshaling.
    const bool useSet = !p->isUnchecked() &&
        static_cast<int64_t>(enumerators.size()) < p->maxValue() - p->minValue() + 1;
    string underlying = p->underlying() ? typeToString(p->underlying(), "") : "int";

    _out << sp;
    emitDeprecate(p, 0, _out, "type");
    emitCommonAttributes();
    emitCustomAttributes(p);
    _out << nl << "public enum " << name << " : " << underlying;
    _out << sb;
    bool firstEn = true;
    for (const auto& en : enumerators)
    {
        if (firstEn)
        {
            firstEn = false;
        }
        else
        {
            _out << ',';
        }
        _out << nl << fixId(en->name());
        if (p->explicitValue())
        {
            _out << " = " << en->value();
        }
    }
    _out << eb;

    _out << sp;
    emitCommonAttributes();
    _out << nl << "public static class " << p->name() << "Helper";
    _out << sb;
    if (useSet)
    {
        _out << sp;
        _out << nl << "public static readonly global::System.Collections.Generic.HashSet<" << underlying
            << "> EnumeratorValues =";
        _out.inc();
        _out << nl << "new global::System.Collections.Generic.HashSet<" << underlying << "> { ";
        firstEn = true;
        for (const auto& en : enumerators)
        {
            if (firstEn)
            {
                firstEn = false;
            }
            else
            {
                _out << ", ";
            }
            _out << en->value();
        }
        _out << " };";
        _out.dec();
    }

    _out << sp;
    _out << nl << "public static void Write(this ZeroC.Ice.OutputStream ostr, " << name << " value) =>";
    _out.inc();
    if (p->underlying())
    {
        _out << nl << "ostr.Write" << builtinSuffix(p->underlying()) << "((" << underlying << ")value);";
    }
    else
    {
        _out << nl << "ostr.WriteSize((int)value);";
    }
    _out.dec();

    _out << sp;
    _out << nl << "public static readonly ZeroC.Ice.OutputStreamWriter<" << name << "> IceWriter = Write;";

    _out << sp;
    _out << nl << "public static " << name << " As" << p->name() << "(this " << underlying << " value) =>";
    if (p->isUnchecked())
    {
        _out << " (" << name << ")value;";
    }
    else
    {
        _out.inc();
        if (useSet)
        {
            _out << nl << "EnumeratorValues.Contains(value)";
        }
        else
        {
            _out << nl << p->minValue() << " <= value && value <= " << p->maxValue();
        }
        _out << " ? (" << name
             << ")value : throw new ZeroC.Ice.InvalidDataException($\"invalid enumerator value `{value}' for "
             << fixId(p->scoped()) << "\");";
        _out.dec();
    }

    _out << sp;
    _out << nl << "public static " << name << " Read" << p->name() << "(this ZeroC.Ice.InputStream istr) =>";
    _out.inc();
    _out << nl << "As" << p->name() << "(istr.";
    if (p->underlying())
    {
        _out << "Read" << builtinSuffix(p->underlying()) << "()";
    }
    else
    {
        _out << "ReadSize()";
    }
    _out << ");";
    _out.dec();

    _out << sp;
    _out << nl << "public static readonly ZeroC.Ice.InputStreamReader<" << name << "> IceReader = Read" << p->name()
        << ";";

    _out << eb;
}

void
Slice::Gen::TypesVisitor::visitDataMember(const DataMemberPtr& p)
{
    ContainedPtr cont = ContainedPtr::dynamicCast(p->container());
    assert(cont);

    _out << sp;

    bool readonly = StructPtr::dynamicCast(cont) && cont->hasMetaData("cs:readonly");

    writeTypeDocComment(p, getDeprecateReason(p, cont, "member"));
    emitDeprecate(p, cont, _out, "member");
    emitCustomAttributes(p);
    _out << nl << "public ";
    if(readonly)
    {
        _out << "readonly ";
    }
    _out << typeToString(p->type(), getNamespace(cont));
    _out << " " << fixId(dataMemberName(p), ExceptionPtr::dynamicCast(cont) ? Slice::ExceptionType : Slice::ObjectType);
    if(cont->hasMetaData("cs:property"))
    {
        _out << "{ get; set; }";
    }
    else
    {
        _out << ";";
    }
}

void
Slice::Gen::TypesVisitor::visitSequence(const SequencePtr& p)
{
    if (!isMappedToReadOnlyMemory(p) || EnumPtr::dynamicCast(p->type()))
    {
        string name = p->name();
        string scope = getNamespace(p);
        string seqS = typeToString(p, scope);
        string seqReadOnly = typeToString(p, scope, true);

        _out << sp;
        emitCommonAttributes();
        _out << nl << "public static class " << name << "Helper";
        _out << sb;

        if (isMappedToReadOnlyMemory(p))
        {
            assert(EnumPtr::dynamicCast(p->type()));

            // For such enums, we provide 2 writers but no Write method.
            _out << sp;
            _out << nl << "public static readonly ZeroC.Ice.OutputStreamWriter<" << seqReadOnly
                << "> IceWriterFromSequence = (ostr, v) => ostr.WriteSequence(v.Span);";

            _out << sp;
            _out << nl << "public static readonly ZeroC.Ice.OutputStreamWriter<" << seqS
                << "> IceWriterFromArray = (ostr, v) => ostr.WriteArray(v);";
        }
        else
        {
            _out << sp;
            _out << nl << "public static void Write(this ZeroC.Ice.OutputStream ostr, " << seqReadOnly << " sequence) =>";
            _out.inc();
            _out << nl << sequenceMarshalCode(p, scope, "sequence", "ostr") << ";";
            _out.dec();

            _out << sp;
            _out << nl << "public static readonly ZeroC.Ice.OutputStreamWriter<" << seqReadOnly
                << "> IceWriter = Write;";
        }

        _out << sp;
        _out << nl << "public static " << seqS << " Read" << name << "(this ZeroC.Ice.InputStream istr) =>";
        _out.inc();
        _out << nl << sequenceUnmarshalCode(p, scope, "istr") << ";";
        _out.dec();

        _out << sp;
        _out << nl << "public static readonly ZeroC.Ice.InputStreamReader<" << seqS << "> IceReader = Read"
            << name << ";";

        _out << eb;
    }
}

void
Slice::Gen::TypesVisitor::visitDictionary(const DictionaryPtr& p)
{
    string ns = getNamespace(p);
    string name = p->name();
    TypePtr key = p->keyType();
    TypePtr value = p->valueType();

    bool withBitSequence = false;
    if (auto optional = OptionalPtr::dynamicCast(value); optional && optional->encodedUsingBitSequence())
    {
        withBitSequence = true;
        value = optional->underlying();
    }

    string dictS = typeToString(p, ns);
    string readOnlyDictS = typeToString(p, ns, true);
    string generic = p->findMetaDataWithPrefix("cs:generic:");

    _out << sp;
    emitCommonAttributes();
    _out << nl << "public static class " << name << "Helper";
    _out << sb;
    _out << nl << "public static void Write(this ZeroC.Ice.OutputStream ostr, " << readOnlyDictS << " dictionary) =>";
    _out.inc();
    _out << nl << "ostr.WriteDictionary(dictionary";

    if (withBitSequence && isReferenceType(value))
    {
        _out << ", withBitSequence: true";
    }
    if (!StructPtr::dynamicCast(key))
    {
        _out << ", " << outputStreamWriter(key, ns, true);
    }
    if (!StructPtr::dynamicCast(value))
    {
        _out << ", " << outputStreamWriter(value, ns, true);
    }
    _out << ");";
    _out.dec();

    _out << sp;
    _out << nl << "public static readonly ZeroC.Ice.OutputStreamWriter<" << readOnlyDictS << "> IceWriter = Write;";

    _out << sp;
    _out << nl << "public static " << dictS << " Read" << name << "(this ZeroC.Ice.InputStream istr) =>";
    _out.inc();
    if(generic == "SortedDictionary")
    {
        _out << nl << "istr.ReadSortedDictionary(";
    }
    else
    {
        _out << nl << "istr.ReadDictionary(";
    }
    _out << "minKeySize: " << key->minWireSize() << ", ";
    if (!withBitSequence)
    {
        _out << "minValueSize: " << value->minWireSize() << ", ";
    }
    if (withBitSequence && isReferenceType(value))
    {
         _out << "withBitSequence: true, ";
    }

    _out << inputStreamReader(key, ns) << ", " << inputStreamReader(value, ns) << ");";
    _out.dec();

    _out << sp;
    _out << nl << "public static readonly ZeroC.Ice.InputStreamReader<" << dictS << "> IceReader = Read"
        << name << ";";

    _out << eb;
}

Slice::Gen::ProxyVisitor::ProxyVisitor(IceUtilInternal::Output& out) :
    CsVisitor(out)
{
}

bool
Slice::Gen::ProxyVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasInterfaceDefs())
    {
        return false;
    }
    openNamespace(p);
    return true;
}

void
Slice::Gen::ProxyVisitor::visitModuleEnd(const ModulePtr&)
{
    closeNamespace();
}

bool
Slice::Gen::ProxyVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    string name = p->name();
    string ns = getNamespace(p);

    _out << sp;
    writeProxyDocComment(p, getDeprecateReason(p, 0, "interface"));
    emitCommonAttributes();
    emitTypeIdAttribute(p->scoped());
    emitCustomAttributes(p);
    _out << nl << "public partial interface " << interfaceName(p) << "Prx : ";

    vector<string> baseInterfaces =
        mapfn<InterfaceDefPtr>(p->bases(), [&ns](const auto& c)
                           {
                               return getUnqualified(getNamespace(c) + "." +
                                                     interfaceName(c) + "Prx", ns);
                           });

    if(baseInterfaces.empty())
    {
        baseInterfaces.push_back("ZeroC.Ice.IObjectPrx");
    }

    for(vector<string>::const_iterator q = baseInterfaces.begin(); q != baseInterfaces.end();)
    {
        _out << *q;
        if(++q != baseInterfaces.end())
        {
            _out << ", ";
        }
    }
    _out << sb;

    return true;
}

void
Slice::Gen::ProxyVisitor::visitInterfaceDefEnd(const InterfaceDefPtr& p)
{
    string ns = getNamespace(p);
    InterfaceList bases = p->bases();

    string name = interfaceName(p) + "Prx";
    //
    // Proxy static methods
    //
    _out << sp;
    _out << nl << "public static readonly new ZeroC.Ice.ProxyFactory<" << name << "> Factory =";
    _out.inc();
    _out << nl << "(reference) => new _" << p->name() << "Prx(reference);";
    _out.dec();

    _out << sp;
    _out << nl << "public static readonly new ZeroC.Ice.InputStreamReader<" << name << "> IceReader =";
    _out.inc();
    _out << nl << "istr => istr.ReadProxy(Factory);";
    _out.dec();

    _out << sp;
    _out << nl << "public static readonly new ZeroC.Ice.InputStreamReader<" << name << "?> IceReaderIntoNullable =";
    _out.inc();
    _out << nl << "istr => istr.ReadNullableProxy(Factory);";
    _out.dec();

    _out << sp;
    _out << nl << "public static new " << name << " Parse(string s, ZeroC.Ice.Communicator communicator) => "
         << "new _" << p->name() << "Prx(ZeroC.Ice.Reference.Parse(s, communicator));";

    _out << sp;
    _out << nl << "public static bool TryParse("
         << "string s, ZeroC.Ice.Communicator communicator, "
         << "out " <<name << "? prx)";
    _out << sb;
    _out << nl << "try";
    _out << sb;
    _out << nl << "prx = new _" << p->name() << "Prx(ZeroC.Ice.Reference.Parse(s, communicator));";
    _out << eb;
    _out << nl << "catch (global::System.Exception)";
    _out << sb;
    _out << nl << "prx = null;";
    _out << nl << "return false;";
    _out << eb;
    _out << nl << "return true;";
    _out << eb;

    _out << sp;
    _out << nl << "public static ";
    if(!bases.empty())
    {
        _out << "new ";
    }
    _out << name << " UncheckedCast(ZeroC.Ice.IObjectPrx prx) => new _" << p->name()
        << "Prx(prx.IceReference);";

    _out << sp;
    _out << nl << "public static ";
    if(!bases.empty())
    {
        _out << "new ";
    }
    _out << name << "? CheckedCast(ZeroC.Ice.IObjectPrx prx, "
         << "global::System.Collections.Generic.IReadOnlyDictionary<string, string>? context = null)";
    _out << sb;

    _out << nl << "if (prx.IceIsA(ZeroC.Ice.TypeExtensions.GetIceTypeId(typeof(" << interfaceName(p)
         << "Prx))!, context))";
    _out << sb;
    _out << nl << "return new _" << p->name() << "Prx(prx.IceReference);";
    _out << eb;
    _out << nl << "else";
    _out << sb;
    _out << nl << "return null;";
    _out << eb;

    _out << eb;

    _out << eb;

    //
    // Proxy instance
    //
    _out << sp;
    _out << nl << "[global::System.Serializable]";
    _out << nl << "internal sealed class _" << p->name() << "Prx : ZeroC.Ice.ObjectPrx, "
         << name;
    _out << sb;

    _out << nl << "private _" << p->name() << "Prx("
         << "global::System.Runtime.Serialization.SerializationInfo info, "
         << "global::System.Runtime.Serialization.StreamingContext context)";
    _out.inc();
    _out << nl << ": base(info, context)";
    _out.dec();
    _out << sb;
    _out << eb;

    _out << sp;
    _out << nl << "internal _" << p->name() << "Prx(ZeroC.Ice.Reference reference)";
    _out.inc();
    _out << nl << ": base(reference)";
    _out.dec();
    _out << sb;
    _out << eb;

    _out << sp;
    _out << nl << "ZeroC.Ice.IObjectPrx ZeroC.Ice.IObjectPrx.IceClone(ZeroC.Ice.Reference reference) => new _"
         << p->name() << "Prx(reference);";

    _out << eb;
}

namespace
{

string
requestType(const list<ParamInfo>& inParams, const list<ParamInfo>& outParams)
{
    ostringstream os;
    if(inParams.size() == 0)
    {
        os << "ZeroC.Ice.OutgoingRequestWithEmptyParamList";
        if(outParams.size() > 0)
        {
            os << "<" << toTupleType(outParams) << ">";
        }
    }
    else if(inParams.size() == 1 && (!StructPtr::dynamicCast(inParams.front().type) || inParams.front().tagged))
    {
        os << "ZeroC.Ice.OutgoingRequestWithParam<" << toTupleType(inParams);
        if(outParams.size() > 0)
        {
            os << ", " << toTupleType(outParams);
        }
        os << ">";
    }
    else
    {
        os << "ZeroC.Ice.OutgoingRequestWithStructParam<" << toTupleType(inParams);
        if(outParams.size() > 0)
        {
            os << ", " << toTupleType(outParams);
        }
        os << ">";
    }
    return os.str();
}

}

void
Slice::Gen::ProxyVisitor::visitOperation(const OperationPtr& operation)
{
    list<ParamInfo> outParams = getAllOutParams(operation, false);
    list<ParamInfo> requiredOutParams;
    list<ParamInfo> taggedOutParams;
    getOutParams(operation, false, requiredOutParams, taggedOutParams, "iceP_");

    list<ParamInfo> inParams = getAllInParams(operation, true);
    list<ParamInfo> requiredInParams;
    list<ParamInfo> taggedInParams;
    getInParams(operation, true, requiredInParams, taggedInParams);

    InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(operation->container());
    string deprecateReason = getDeprecateReason(operation, interface, "operation");

    string ns = getNamespace(interface);
    string opName = operationName(operation);
    string name = fixId(opName);
    string asyncName = opName + "Async";
    string internalName = "_iceI_" + opName + "Async";
    bool oneway = operation->hasMetaData("oneway");

    TypePtr ret = operation->returnType();
    string retS = typeToString(operation->returnType(), ns);

    string context = getEscapedParamName(operation, "context");
    string cancel = getEscapedParamName(operation, "cancel");
    string progress = getEscapedParamName(operation, "progress");

    string requestProperty = "IceI_" + opName + "Request";
    string requestObject = "_iceI_" + opName + "Request";

    {
        //
        // Write the synchronous version of the operation.
        //
        _out << sp;
        writeOperationDocComment(operation, deprecateReason, false, false);
        if(!deprecateReason.empty())
        {
            _out << nl << "[global::System.Obsolete(\"" << deprecateReason << "\")]";
        }
        _out << nl << resultType(operation, ns, false)  << " " << name << spar
             << getInvocationParams(operation, ns)
             << epar << " =>";
        _out.inc();
        _out << nl << requestProperty << ".Invoke(this, ";
        if(inParams.size() > 0)
        {
            _out << toTuple(inParams) << ", ";
        }
        _out << context << ");";
        _out.dec();
    }

    {
        //
        // Write the async version of the operation
        //
        _out << sp;
        writeOperationDocComment(operation, deprecateReason, false, true);
        if(!deprecateReason.empty())
        {
            _out << nl << "[global::System.Obsolete(\"" << deprecateReason << "\")]";
        }

        _out << nl << resultTask(operation, ns, false) << " "
             << asyncName << spar << getInvocationParamsAMI(operation, ns, true) << epar << " =>";
        _out.inc();
        _out << nl << requestProperty << ".InvokeAsync(this, ";
        if(inParams.size() > 0)
        {
            _out << toTuple(inParams) << ", ";
        }
        _out << context << ", " << progress << ", " << cancel << ");";
        _out.dec();
    }

    outParams = getAllOutParams(operation, false, "iceP_", true);
    string requestT = requestType(inParams, outParams);

    if(oneway && (outParams.size() > 0))
    {
        const UnitPtr ut = operation->unit();
        const DefinitionContextPtr dc = ut->findDefinitionContext(operation->file());
        assert(dc);
        dc->error(operation->file(), operation->line(), "only void operations can be marked oneway");
    }

    // Write the static outgoing request instance
    _out << sp;
    _out << nl << "private static " << requestT << "? " << requestObject << ";";

    _out << sp;
    _out << nl << "private static " << requestT << " " << requestProperty << " =>";
    _out.inc();
    _out << nl << requestObject << " ?\?= new " << requestT << "(";
    _out.inc();
    _out << nl << "operationName: \"" << operation->name() << "\",";
    _out << nl << "idempotent: " << (isIdempotent(operation) ? "true" : "false");

    if(outParams.size() == 0)
    {
        _out << ",";
        _out << nl << "oneway: " << (oneway ? "true" : "false");
    }

    if(inParams.size() > 0)
    {
        _out << ",";
        _out << nl << "format: " << opFormatTypeToString(operation) << ",";
        _out << nl << "writer: ";
        writeOutgoingRequestWriter(operation);
    }

    if(outParams.size() > 0)
    {
        _out << ",";
        _out << nl << "reader: ";
        writeOutgoingRequestReader(operation);
    }
    _out << ");";
    _out.dec();
    _out.dec();
}

void
Slice::Gen::ProxyVisitor::writeOutgoingRequestWriter(const OperationPtr& operation)
{
    InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(operation->container());
    string ns = getNamespace(interface);

    list<ParamInfo> params = getAllInParams(operation, true);
    list<ParamInfo> requiredParams;
    list<ParamInfo> taggedParams;
    getInParams(operation, true, requiredParams, taggedParams, "iceP_");

    bool defaultWriter = params.size() == 1 && operation->inBitSequenceSize() == 0 && !params.front().tagged;
    if (defaultWriter)
    {
        _out << outputStreamWriter(params.front().type, ns, false);
    }
    else if(params.size() > 1)
    {
        _out << "(ZeroC.Ice.OutputStream ostr, in " << toTupleType(params, "iceP_") << " value) =>";
        _out << sb;
        writeMarshalParams(operation, requiredParams, taggedParams, "ostr", "value.");
        _out << eb;
    }
    else
    {
        ParamInfo p = params.front();
        _out << "(ZeroC.Ice.OutputStream ostr, "
             << p.typeStr << " " << (p.param ? fixId("iceP_" + p.param->name()) : fixId("iceP_" + p.name))
             << ") =>";
        _out << sb;
        writeMarshalParams(operation, requiredParams, taggedParams, "ostr");
        _out << eb;
    }
}

void
Slice::Gen::ProxyVisitor::writeOutgoingRequestReader(const OperationPtr& operation)
{
    InterfaceDefPtr interface = operation->interface();
    string ns = getNamespace(interface);

    list<ParamInfo> params = getAllOutParams(operation, false);
    list<ParamInfo> requiredParams;
    list<ParamInfo> taggedParams;
    getOutParams(operation, false, requiredParams, taggedParams, "iceP_");

    bool defaultReader = params.size() == 1 && operation->returnBitSequenceSize() == 0 && !params.front().tagged;
    if (defaultReader)
    {
        _out << inputStreamReader(params.front().type, ns);
    }
    else if(params.size() > 0)
    {
        _out << "istr =>";
        _out << sb;
        writeUnmarshalParams(operation, requiredParams, taggedParams);

        if(params.size() == 1)
        {
            ParamInfo p = params.front();
            _out << nl << "return " << (p.param ? fixId("iceP_" +  p.param->name()) : fixId("iceP_" + p.name)) << ";";
        }
        else
        {
            params = getAllOutParams(operation, false, "iceP_", true);
            _out << nl << "return " << spar << getNames(params) << epar << ";";
        }
        _out << eb;
    }
}

Slice::Gen::DispatcherVisitor::DispatcherVisitor(::IceUtilInternal::Output& out, bool generateAllAsync) :
    CsVisitor(out), _generateAllAsync(generateAllAsync)
{
}

bool
Slice::Gen::DispatcherVisitor::visitModuleStart(const ModulePtr& p)
{
    if (!p->hasInterfaceDefs())
    {
        return false;
    }

    openNamespace(p);
    return true;
}

void
Slice::Gen::DispatcherVisitor::visitModuleEnd(const ModulePtr&)
{
    closeNamespace();
}

bool
Slice::Gen::DispatcherVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    InterfaceList bases = p->bases();
    string name = interfaceName(p) + (_generateAllAsync ? "Async" : "");
    string ns = getNamespace(p);

    _out << sp;
    writeServantDocComment(p, getDeprecateReason(p, 0, "interface"));
    emitCommonAttributes();
    emitTypeIdAttribute(p->scoped());
    emitCustomAttributes(p);
    _out << nl << "public partial interface " << fixId(name) << " : ";
    if (bases.empty())
    {
        _out << "ZeroC.Ice.IObject";
    }
    else
    {
        for(InterfaceList::const_iterator q = bases.begin(); q != bases.end();)
        {
            _out << getUnqualified(getNamespace(*q) + "." + (interfaceName(*q) + (_generateAllAsync ? "Async" : "")), ns);
            if(++q != bases.end())
            {
                _out << ", ";
            }
        }
    }

    _out << sb;

    // The _ice prefix is in case the user "extends" the partial generated interface.
    _out << nl << "private static readonly string _iceTypeId = ZeroC.Ice.TypeExtensions.GetIceTypeId(typeof("
        << name << "))!;";
    _out << nl
        << "private static readonly string[] _iceAllTypeIds = ZeroC.Ice.TypeExtensions.GetAllIceTypeIds(typeof("
        << name << "));";

    for(const auto& op : p->operations())
    {
        writeReturnValueStruct(op);
        writeMethodDeclaration(op);
    }

    _out << sp;
    _out << nl << "string ZeroC.Ice.IObject.IceId(ZeroC.Ice.Current current) => _iceTypeId;";
    _out << sp;
    _out << nl << "global::System.Collections.Generic.IEnumerable<string> "
         << "ZeroC.Ice.IObject.IceIds(ZeroC.Ice.Current current) => _iceAllTypeIds;";

    _out << sp;
    _out << nl << "global::System.Threading.Tasks.ValueTask<ZeroC.Ice.OutgoingResponseFrame> ZeroC.Ice.IObject"
         << ".DispatchAsync(ZeroC.Ice.IncomingRequestFrame request, ZeroC.Ice.Current current)";
    _out.inc();
    _out << nl << " => DispatchAsync(this, request, current);";
    _out.dec();

    _out << sp;
    _out << nl << "// This protected static DispatchAsync allows a derived class to override the instance DispatchAsync";
    _out << nl << "// and reuse the generated implementation.";
    _out << nl << "protected static global::System.Threading.Tasks.ValueTask<ZeroC.Ice.OutgoingResponseFrame> "
        << "DispatchAsync(" << fixId(name) << " servant, "
        << "ZeroC.Ice.IncomingRequestFrame request, ZeroC.Ice.Current current) =>";
    _out.inc();
    _out << nl << "current.Operation switch";
    _out << sb;
    StringList allOpNames;
    for(const auto& op : p->allOperations())
    {
        allOpNames.push_back(op->name());
    }
    allOpNames.push_back("ice_id");
    allOpNames.push_back("ice_ids");
    allOpNames.push_back("ice_isA");
    allOpNames.push_back("ice_ping");

    for(const auto& opName : allOpNames)
    {
        _out << nl << "\"" << opName << "\" => " << "servant.IceD_" << opName << "Async(request, current),";
    }

    _out << nl << "_ => throw new ZeroC.Ice.OperationNotExistException(current.Identity, current.Facet, "
         << "current.Operation)";

    _out << eb << ";"; // switch expression
    _out.dec(); // method
    return true;
}

void
Slice::Gen::DispatcherVisitor::writeReturnValueStruct(const OperationPtr& operation)
{
    InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(operation->container());
    string ns = getNamespace(interface);
    const string opName = pascalCase(operation->name());

    list<ParamInfo> outParams = getAllOutParams(operation, true, "", true);
    list<ParamInfo> requiredOutParams;
    list<ParamInfo> taggedOutParams;
    getOutParams(operation, false, requiredOutParams, taggedOutParams, "iceP_");

    if(operation->hasMarshaledResult())
    {
        _out << sp;
        _out << nl << "public struct " << opName << "MarshaledReturnValue";
        _out << sb;
        _out << nl << "public ZeroC.Ice.OutgoingResponseFrame Response { get; }";

        _out << nl << "public " << opName << "MarshaledReturnValue" << spar
             << getNames(outParams, [](const auto& p)
                                    {
                                        return p.typeStr + " iceP_" + p.name;
                                    })
             << "ZeroC.Ice.Current current"
             << epar;
        _out << sb;
        _out << nl << "Response = ZeroC.Ice.OutgoingResponseFrame.WithReturnValue(";
        _out.inc();
        _out << nl << "current, " << opFormatTypeToString(operation) << ", " << toTuple(outParams, "iceP_") << ",";
        if(outParams.size() > 1)
        {
            _out << nl << "(ZeroC.Ice.OutputStream ostr, " << toTupleType(outParams, "iceP_") << " value) =>";
            _out << sb;
            writeMarshalParams(operation, requiredOutParams, taggedOutParams, "ostr", "value.");
            _out << eb;
        }
        else
        {
            _out << nl << "(ostr, " << getNames(outParams, "iceP_") << ") =>";
            _out << sb;
            writeMarshalParams(operation, requiredOutParams, taggedOutParams, "ostr");
            _out << eb;
        }
        _out << ");";
        _out.dec();
        _out << eb;
        _out << eb;
    }
}

void
Slice::Gen::DispatcherVisitor::writeMethodDeclaration(const OperationPtr& operation)
{
    InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(operation->container());
    string ns = getNamespace(interface);
    string deprecateReason = getDeprecateReason(operation, interface, "operation");
    bool amd = _generateAllAsync || interface->hasMetaData("amd") || operation->hasMetaData("amd");
    const string name = fixId(operationName(operation) + (amd ? "Async" : ""));
    list<ParamInfo> inParams = getAllInParams(operation, false);

    _out << sp;
    writeOperationDocComment(operation, deprecateReason, true, amd);
    _out << nl << "public ";

    if(amd)
    {
        _out << resultTask(operation, ns, true);
    }
    else
    {
        _out << resultType(operation, ns, true);
    }

    _out << " " << name << spar;
    _out << getNames(inParams, [](const auto& param)
                               {
                                   return param.typeStr + " " + param.name;
                               });
    _out << ("ZeroC.Ice.Current " + getEscapedParamName(operation, "current"));
    _out << epar << ';';
}

void
Slice::Gen::DispatcherVisitor::visitOperation(const OperationPtr& operation)
{
    InterfaceDefPtr interface = InterfaceDefPtr::dynamicCast(operation->container());
    bool amd = _generateAllAsync || interface->hasMetaData("amd") || operation->hasMetaData("amd");
    string ns = getNamespace(interface);
    string opName = operationName(operation);
    string name = fixId(opName + (amd ? "Async" : ""));
    string internalName = "IceD_" + operation->name() + "Async";

    list<ParamInfo> inParams = getAllInParams(operation, false, "iceP_");
    list<ParamInfo> requiredInParams;
    list<ParamInfo> taggedInParams;
    getInParams(operation, false, requiredInParams, taggedInParams, "iceP_");

    list<ParamInfo> outParams = getAllOutParams(operation, true, "", true);
    list<ParamInfo> requiredOutParams;
    list<ParamInfo> taggedOutParams;
    getOutParams(operation, true, requiredOutParams, taggedOutParams);

    bool defaultWriter = outParams.size() == 1 && operation->returnBitSequenceSize() == 0 &&
        !outParams.front().tagged;
    string writer = defaultWriter ? outputStreamWriter(outParams.front().type, ns, false) :
        "_iceD_" + opName + "Writer";

    bool defaultReader = inParams.size() == 1 && operation->inBitSequenceSize() == 0 && !inParams.front().tagged;
    string reader = defaultReader ? inputStreamReader(inParams.front().type, ns) : "_iceD_" + opName + "Reader";

    _out << sp;
    _out << nl << "protected ";
    if (amd)
    {
        _out << "async ";
    }
    _out << "global::System.Threading.Tasks.ValueTask<ZeroC.Ice.OutgoingResponseFrame>";
    _out << " " << internalName << "(ZeroC.Ice.IncomingRequestFrame request, ZeroC.Ice.Current current)";
    _out << sb;

    if (!isIdempotent(operation))
    {
         _out << nl << "IceCheckNonIdempotent(current);";
    }

    // Even when the parameters are empty, we verify we could read the data. Note that EndEncapsulation
    // skips tagged members, and needs to understand them.
    if(inParams.empty())
    {
        _out << nl << "request.ReadEmptyParamList(current.Communicator);";
    }
    else if(inParams.size() == 1)
    {
        _out << nl << "var " << inParams.front().name << " = request.ReadParamList(current.Communicator, "
            << reader << ");";
    }
    else
    {
        _out << nl << "var paramList = request.ReadParamList(current.Communicator, " << reader << ");";
    }

    // The 'this.' is necessary only when the operation name matches one of our local variable (current, istr etc.)

    if(operation->hasMarshaledResult())
    {
        if (amd)
        {
            _out << nl << "var result = await this." << name << spar << getNames(inParams) << "current" << epar
                << ".ConfigureAwait(false);";
            _out << nl << "return result.Response;";
        }
        else
        {
            _out << nl << "return new global::System.Threading.Tasks.ValueTask<ZeroC.Ice.OutgoingResponseFrame>(this."
                 << name << spar << getNames(inParams) << "current" << epar << ".Response);";
        }
        _out << eb;
    }
    else
    {
        _out << nl;
        if(outParams.size() >= 1)
        {
            _out << "var result = ";
        }

        if (amd)
        {
            _out << "await ";
        }
        _out << "this." << name << spar << getNames(inParams, inParams.size() > 1 ? "paramList." : "") << "current"
             << epar;
        if (amd)
        {
            _out << ".ConfigureAwait(false)";
        }
        _out << ";";

        if(outParams.size() == 0)
        {
            if(amd)
            {
                _out << nl << "return ZeroC.Ice.OutgoingResponseFrame.WithVoidReturnValue(current);";
            }
            else
            {
                _out << nl << "return new global::System.Threading.Tasks.ValueTask<ZeroC.Ice.OutgoingResponseFrame>(";
                _out.inc();
                _out << nl << "ZeroC.Ice.OutgoingResponseFrame.WithVoidReturnValue(current));";
                _out.dec();
            }
        }
        else
        {
            _out << nl << "var response = ZeroC.Ice.OutgoingResponseFrame.WithReturnValue(current, "
                 << opFormatTypeToString(operation) << ", result, " << writer << ");";

            if(amd)
            {
                _out << nl << "return response;";
            }
            else
            {
                _out << nl << "return new global::System.Threading.Tasks.ValueTask<ZeroC.Ice.OutgoingResponseFrame>("
                     << "response);";
            }
        }
        _out << eb;
    }

    // Write the output stream writer used to fill the request frame
    if (!operation->hasMarshaledResult())
    {
        if (outParams.size() > 1)
        {
            _out << sp;
            _out << nl << "private static readonly ZeroC.Ice.OutputStreamValueWriter<" << toTupleType(outParams)
                 << "> " << writer << " = (ZeroC.Ice.OutputStream ostr, in " << toTupleType(outParams) << " value) =>";
            _out << sb;
            writeMarshalParams(operation, requiredOutParams, taggedOutParams, "ostr", "value.");
            _out << eb << ";";
        }
        else if (outParams.size() == 1 && !defaultWriter)
        {
            auto param = outParams.front();
            _out << sp;

            if (operation->returnBitSequenceSize() > 0)
            {
                _out << nl << "private static readonly ZeroC.Ice.OutputStreamWriter<" << param.typeStr << "> " << writer
                    << " = (ostr, " << param.name << ") =>";
                _out << sb;
                writeMarshalParams(operation, requiredOutParams, taggedOutParams, "ostr");
                _out << eb << ";";
            }
            else
            {
                if (!param.tagged && StructPtr::dynamicCast(param.type))
                {
                    _out << nl << "private static readonly ZeroC.Ice.OutputStreamValueWriter<" << param.typeStr
                         << "> " << writer << " = (ZeroC.Ice.OutputStream ostr, in "
                         << param.typeStr << " " << param.name << ") =>";
                }
                else
                {
                    _out << nl << "private static readonly ZeroC.Ice.OutputStreamWriter<" << param.typeStr << "> " << writer
                         << " = (ostr, " << param.name << ") =>";
                }
                _out.inc();
                writeMarshalParams(operation, requiredOutParams, taggedOutParams, "ostr");
                _out.dec();
            }
        }
    }

    if(inParams.size() > 1)
    {
        _out << sp;
        _out << nl << "private static readonly ZeroC.Ice.InputStreamReader<" << toTupleType(inParams, "iceP_") << "> "
             << reader << " = istr =>";
        _out << sb;
        writeUnmarshalParams(operation, requiredInParams, taggedInParams);
        _out << nl << "return " << toTuple(inParams, "iceP_") << ";";
        _out << eb << ";";
    }
    else if (inParams.size() == 1 && !defaultReader)
    {
        _out << sp;
        _out << nl << "private static readonly ZeroC.Ice.InputStreamReader<" << inParams.front().typeStr << "> "
            << reader << " = istr =>";
        _out << sb;
        writeUnmarshalParams(operation, requiredInParams, taggedInParams);
        _out << nl << "return " << inParams.front().name << ";";
        _out << eb << ";";
    }
}

void
Slice::Gen::DispatcherVisitor::visitInterfaceDefEnd(const InterfaceDefPtr&)
{
    _out << eb; // interface
}

Slice::Gen::ImplVisitor::ImplVisitor(IceUtilInternal::Output& out) :
    CsVisitor(out)
{
}

bool
Slice::Gen::ImplVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasInterfaceDefs())
    {
        return false;
    }

    openNamespace(p);
    return true;
}

void
Slice::Gen::ImplVisitor::visitModuleEnd(const ModulePtr&)
{
    closeNamespace();
}

bool
Slice::Gen::ImplVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    _out << sp << nl << "public class " << p->name() << "I : " << fixId(p->name());
    _out << sb;
    return true;
}

void
Slice::Gen::ImplVisitor::visitOperation(const OperationPtr& op)
{
    InterfaceDefPtr interface = op->interface();
    string ns = getNamespace(interface);
    string opName = operationName(op);

    list<ParamInfo> outParams = getAllOutParams(op, true);

    _out << sp << nl;

    if(interface->hasMetaData("amd") || op->hasMetaData("amd"))
    {
        _out << "public override " << resultTask(op, ns, true) << " " << opName << "Async" << spar
             << getNames(getAllInParams(op, false))
             << ("ZeroC.Ice.Current " + getEscapedParamName(op, "current"))
             << epar;
        _out << sb;

        for(const auto& p : outParams)
        {
            _out << nl << p.typeStr << " " << p.name << " = " << writeValue(p.type, ns) << ';';
        }

        if(outParams.size() == 0)
        {
            _out << nl << "global::System.Threading.Tasks.Task.CompletedTask;";
        }
        else if(op->hasMarshaledResult() || outParams.size() > 1)
        {
            _out << nl << "return new " << opName << (op->hasMarshaledResult() ? "MarshaledResult" : "Result")
                 << spar
                 << getNames(getAllOutParams(op, true, "", true));
            if(op->hasMarshaledResult())
            {
                _out << ("ZeroC.Ice.Current " + getEscapedParamName(op, "current"));
            }
            _out << epar << ";";
        }
        else
        {
            _out << nl << "return " << outParams.front().name << ";";
        }
        _out << eb;
    }
    else
    {
        _out << "public override " << resultType(op, ns, true) << " " << opName << spar
             << getNames(getAllInParams(op, false))
             << ("ZeroC.Ice.Current " + getEscapedParamName(op, "current"))
             << epar;
        _out << sb;

        if(op->hasMarshaledResult())
        {
            _out << nl << "return new " << opName << "MarshaledResult"
                 << spar
                 << getNames(getAllOutParams(op, true, "", true))
                 << ("ZeroC.Ice.Current " + getEscapedParamName(op, "current"))
                 << epar << ";";
        }
        else
        {
            for(const auto& p : outParams)
            {
                if(p.param)
                {
                    _out << nl << p.name << " = " << writeValue(p.type, ns) << ';';
                }
                else
                {
                    _out << nl << "return " << writeValue(p.type, ns) << ';';
                }
            }
        }
        _out << eb;
    }
}

void
Slice::Gen::ImplVisitor::visitInterfaceDefEnd(const InterfaceDefPtr&)
{
    _out << eb;
}

Slice::Gen::ClassFactoryVisitor::ClassFactoryVisitor(IceUtilInternal::Output& out) :
    CsVisitor(out)
{
}

bool
Slice::Gen::ClassFactoryVisitor::visitModuleStart(const ModulePtr& p)
{
    if (p->hasClassDefs())
    {
        string prefix;
        // We are generating code for a top-level module
        if (!ContainedPtr::dynamicCast(p->container()))
        {
            prefix = "ZeroC.Ice.ClassFactory";
        }
        openNamespace(p, prefix);
        return true;
    }
    else
    {
        return false;
    }
}

void
Slice::Gen::ClassFactoryVisitor::visitModuleEnd(const ModulePtr&)
{
    closeNamespace();
}

bool
Slice::Gen::ClassFactoryVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    string name = fixId(p->name());
    _out << sp;
    emitCommonAttributes();
    _out << nl << "public class " << name << " : global::ZeroC.Ice.IClassFactory";
    _out << sb;
    _out << nl << "public global::ZeroC.Ice.AnyClass Create() =>";
    _out.inc();
    _out << nl << "new global::" << getNamespace(p) << "." << name << "(this);";
    _out.dec();
    _out << eb;

    return false;
}

Slice::Gen::CompactIdVisitor::CompactIdVisitor(IceUtilInternal::Output& out) :
    CsVisitor(out)
{
}

bool
Slice::Gen::CompactIdVisitor::visitUnitStart(const UnitPtr& p)
{
    // The CompactIdVisitor does not visit modules, only the unit.
    if (p->hasCompactTypeId())
    {
        _out << sp << nl << "namespace ZeroC.Ice.ClassFactory";
        _out << sb;
        return true;
    }
    return false;
}

void
Slice::Gen::CompactIdVisitor::visitUnitEnd(const UnitPtr&)
{
    _out << eb;
}

bool
Slice::Gen::CompactIdVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if (p->compactId() >= 0)
    {
        _out << sp;
        emitCommonAttributes();
        _out << nl << "public class CompactId_" << p->compactId() << " : global::ZeroC.Ice.IClassFactory";
        _out << sb;
        _out << nl << "public global::ZeroC.Ice.AnyClass Create() =>";
        _out.inc();
        _out << nl << "new global::" << getNamespace(p) << "." << fixId(p->name()) << "(this);";
        _out.dec();
        _out << eb;
    }
    return false;
}

Slice::Gen::RemoteExceptionFactoryVisitor::RemoteExceptionFactoryVisitor(IceUtilInternal::Output& out) :
    CsVisitor(out)
{
}

bool
Slice::Gen::RemoteExceptionFactoryVisitor::visitModuleStart(const ModulePtr& p)
{
    if (p->hasExceptions())
    {
        string prefix;
        // We are generating code for a top-level module
        if (!ContainedPtr::dynamicCast(p->container()))
        {
            prefix = "ZeroC.Ice.RemoteExceptionFactory";
        }
        openNamespace(p, prefix);
        return true;
    }
    else
    {
        return false;
    }
}

void
Slice::Gen::RemoteExceptionFactoryVisitor::visitModuleEnd(const ModulePtr&)
{
    closeNamespace();
}

bool
Slice::Gen::RemoteExceptionFactoryVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    string name = fixId(p->name());
    _out << sp;
    emitCommonAttributes();
    _out << nl << "public class " << name << " : global::ZeroC.Ice.IRemoteExceptionFactory";
    _out << sb;
    _out << nl << "public global::ZeroC.Ice.RemoteException Create(string? message) =>";
    _out.inc();
    _out << nl << "new global::" << getNamespace(p) << "." << name << "(this, message);";
    _out.dec();
    _out << eb;
    return false;
}
