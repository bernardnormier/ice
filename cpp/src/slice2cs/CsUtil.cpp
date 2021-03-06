//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <CsUtil.h>
#include <Slice/Util.h>
#include <IceUtil/Functional.h>
#include <IceUtil/StringUtil.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#  include <direct.h>
#else
#  include <unistd.h>
#endif

using namespace std;
using namespace Slice;
using namespace IceUtil;
using namespace IceUtilInternal;

bool
Slice::normalizeCase(const ContainedPtr& c)
{
    auto fileMetaData = c->unit()->findDefinitionContext(c->file())->getMetaData();
    if(find(begin(fileMetaData), end(fileMetaData), "preserve-case") != end(fileMetaData) ||
       find(begin(fileMetaData), end(fileMetaData), "cs:preserve-case") != end(fileMetaData))
    {
        return false;
    }
    return true;
}
std::string
Slice::operationName(const OperationPtr& op)
{
    return normalizeCase(op) ? pascalCase(op->name()) : op->name();
}

std::string
Slice::paramName(const ParamInfo& info)
{
    return normalizeCase(info.operation) ? camelCase(info.name) : info.name;
}

std::string
Slice::interfaceName(const InterfaceDeclPtr& decl)
{
    string name = normalizeCase(decl) ? pascalCase(decl->name()) : decl->name();
    return name.find("II") == 0 ? name : "I" + name;
}

std::string
Slice::interfaceName(const InterfaceDefPtr& def)
{
    return interfaceName(def->declaration());
}

std::string
Slice::dataMemberName(const ParamInfo& info)
{
    return normalizeCase(info.operation) ? pascalCase(info.name) : info.name;
}

std::string
Slice::dataMemberName(const DataMemberPtr& p)
{
    return normalizeCase(p) ? pascalCase(p->name()) : p->name();
}

std::string
Slice::helperName(const TypePtr& type, const string& scope)
{
    ContainedPtr contained = ContainedPtr::dynamicCast(type);
    assert(contained);
    return getUnqualified(contained, scope, "", "Helper");
}

namespace
{

const std::array<std::string, 17> builtinSuffixTable =
{
    "Bool",
    "Byte",
    "Short",
    "UShort",
    "Int",
    "UInt",
    "VarInt",
    "VarUInt",
    "Long",
    "ULong",
    "VarLong",
    "VarULong",
    "Float",
    "Double",
    "String",
    "Proxy",
    "Class"
};

string
mangleName(const string& name, unsigned int baseTypes)
{
    static const char* ObjectNames[] = { "Equals", "Finalize", "GetHashCode", "GetType", "MemberwiseClone",
                                         "ReferenceEquals", "ToString", 0 };

    static const char* ExceptionNames[] = { "Data", "GetBaseException", "GetObjectData", "HelpLink", "HResult",
                                            "InnerException", "Message", "Source", "StackTrace", "TargetSite", 0 };
    string mangled = name;

    if((baseTypes & ExceptionType) == ExceptionType)
    {
        for(int i = 0; ExceptionNames[i] != 0; ++i)
        {
            if(ciequals(name, ExceptionNames[i]))
            {
                return "Ice" + name;
            }
        }
        baseTypes |= ObjectType; // Exception is an Object
    }

    if((baseTypes & ObjectType) == ObjectType)
    {
        for(int i = 0; ObjectNames[i] != 0; ++i)
        {
            if(ciequals(name, ObjectNames[i]))
            {
                return "Ice" + name;
            }
        }
    }

    return mangled;
}

string
lookupKwd(const string& name, unsigned int baseTypes)
{
    //
    // Keyword list. *Must* be kept in alphabetical order.
    //
    static const string keywordList[] =
    {
        "abstract", "as", "async", "await", "base", "bool", "break", "byte", "case", "catch", "char", "checked",
        "class", "const", "continue", "decimal", "default", "delegate", "do", "double", "else", "enum", "event",
        "explicit", "extern", "false", "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit",
        "in", "int", "interface", "internal", "is", "lock", "long", "namespace", "new", "null", "object", "operator",
        "out", "override", "params", "private", "protected", "public", "readonly", "ref", "return", "sbyte", "sealed",
        "short", "sizeof", "stackalloc", "static", "string", "struct", "switch", "this", "throw", "true", "try",
        "typeof", "uint", "ulong", "unchecked", "unsafe", "ushort", "using", "virtual", "void", "volatile", "while"
    };
    bool found = binary_search(&keywordList[0],
                               &keywordList[sizeof(keywordList) / sizeof(*keywordList)],
                               name,
                               Slice::CICompare());
    if(found)
    {
        return "@" + name;
    }
    return mangleName(name, baseTypes);
}

}

std::string
Slice::builtinSuffix(const BuiltinPtr& builtin)
{
    return builtinSuffixTable[builtin->kind()];
}

string
Slice::getNamespacePrefix(const ContainedPtr& cont)
{
    //
    // Traverse to the top-level module.
    //
    ModulePtr m;
    ContainedPtr p = cont;
    while(true)
    {
        if(ModulePtr::dynamicCast(p))
        {
            m = ModulePtr::dynamicCast(p);
        }

        ContainerPtr c = p->container();
        p = ContainedPtr::dynamicCast(c); // This cast fails for Unit.
        if(!p)
        {
            break;
        }
    }

    assert(m);

    static const string prefix = "cs:namespace:";

    string q;
    if(m->findMetaData(prefix, q))
    {
        q = q.substr(prefix.size());
    }
    return q;
}

string
Slice::getNamespace(const ContainedPtr& cont)
{
    string scope = fixId(cont->scope());
    if(scope.rfind(".") == scope.size() - 1)
    {
        scope = scope.substr(0, scope.size() - 1);
    }
    string prefix = getNamespacePrefix(cont);
    if(!prefix.empty())
    {
        if(!scope.empty())
        {
            return prefix + "." + scope;
        }
        else
        {
            return prefix;
        }
    }

    return scope;
}

string
Slice::getUnqualified(const string& type, const string& scope, bool builtin)
{
    if(type.find(".") != string::npos && type.find(scope) == 0 && type.find(".", scope.size() + 1) == string::npos)
    {
        return type.substr(scope.size() + 1);
    }
    else if(builtin)
    {
        return type.find(".") == string::npos ? type : "global::" + type;
    }
    else
    {
        return "global::" + type;
    }
}

string
Slice::getUnqualified(const ContainedPtr& p, const string& package, const string& prefix, const string& suffix)
{
    string name = fixId(prefix + p->name() + suffix);
    string contPkg = getNamespace(p);
    if(contPkg == package || contPkg.empty())
    {
        return name;
    }
    else
    {
        return "global::" + contPkg + "." + name;
    }
}

//
// If the passed name is a scoped name, return the identical scoped name,
// but with all components that are C# keywords replaced by
// their "@"-prefixed version; otherwise, if the passed name is
// not scoped, but a C# keyword, return the "@"-prefixed name;
// otherwise, check if the name is one of the method names of baseTypes;
// if so, prefix it with ice_; otherwise, return the name unchanged.
//
string
Slice::fixId(const string& name, unsigned int baseTypes)
{
    if(name.empty())
    {
        return name;
    }
    if(name[0] != ':')
    {
        return lookupKwd(name, baseTypes);
    }
    vector<string> ids = splitScopedName(name);
    transform(begin(ids), end(ids), begin(ids), [baseTypes](const std::string& i)
                                                {
                                                    return lookupKwd(i, baseTypes);
                                                });
    ostringstream os;
    for(vector<string>::const_iterator i = ids.begin(); i != ids.end();)
    {
        os << *i;
        if(++i != ids.end())
        {
            os << ".";
        }
    }
    return os.str();
}

string
Slice::CsGenerator::typeToString(const TypePtr& type, const string& package, bool readOnly)
{
    if(!type)
    {
        return "void";
    }

    SequencePtr seq;

    auto optional = OptionalPtr::dynamicCast(type);
    if (optional)
    {
        seq = SequencePtr::dynamicCast(optional->underlying());
        if (!seq || !readOnly)
        {
            return typeToString(optional->underlying(), package, readOnly) + "?";
        }
        // else process seq in the code below.
    }
    else
    {
        seq = SequencePtr::dynamicCast(type);
    }

    static const std::array<std::string, 17> builtinTable =
    {
        "bool",
        "byte",
        "short",
        "ushort",
        "int",
        "uint",
        "int",
        "uint",
        "long",
        "ulong",
        "long",
        "ulong",
        "float",
        "double",
        "string",
        "ZeroC.Ice.IObjectPrx",
        "ZeroC.Ice.AnyClass"
    };

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    if(builtin)
    {
        return getUnqualified(builtinTable[builtin->kind()], package, true);
    }

    ClassDeclPtr cl = ClassDeclPtr::dynamicCast(type);
    if(cl)
    {
        return getUnqualified(cl, package);
    }

    InterfaceDeclPtr interface = InterfaceDeclPtr::dynamicCast(type);
    if(interface)
    {
        return getUnqualified(getNamespace(interface) + "." + interfaceName(interface) + "Prx", package);
    }

    if(seq)
    {
        string customType = seq->findMetaDataWithPrefix("cs:generic:");
        string serializableType = seq->findMetaDataWithPrefix("cs:serializable:");
        if (!serializableType.empty())
        {
            return "global::" + serializableType + (optional ? "?" : "");
        }
        else if (readOnly)
        {
            auto elementType = seq->type();
            string elementTypeStr = "<" + typeToString(elementType, package) + ">";
            if (isMappedToReadOnlyMemory(seq))
            {
                return "global::System.ReadOnlyMemory" + elementTypeStr; // same for optional!
            }
            else
            {
                return "global::System.Collections.Generic.IEnumerable" + elementTypeStr + (optional ? "?" : "");
            }
        }
        else if (customType.empty())
        {
            return typeToString(seq->type(), package) + "[]";
        }
        else
        {
            ostringstream out;
            out << "global::";
            if (customType == "List" || customType == "LinkedList" || customType == "Queue" || customType == "Stack")
            {
                out << "System.Collections.Generic.";
            }
            out << customType << "<" << typeToString(seq->type(), package) << ">";
            return out.str();
        }
    }

    DictionaryPtr d = DictionaryPtr::dynamicCast(type);
    if(d)
    {
        string prefix = "cs:generic:";
        string meta;
        string typeName;
        if(d->findMetaData(prefix, meta))
        {
            typeName = meta.substr(prefix.size());
        }
        else
        {
            typeName = readOnly ? "IReadOnlyDictionary" : "Dictionary";
        }
        return "global::System.Collections.Generic." + typeName + "<" +
            typeToString(d->keyType(), package) + ", " +
            typeToString(d->valueType(), package) + ">";
    }

    ContainedPtr contained = ContainedPtr::dynamicCast(type);
    if(contained)
    {
        return getUnqualified(contained, package);
    }

    return "???";
}

string
Slice::returnValueName(const ParamDeclList& outParams)
{
    for(ParamDeclList::const_iterator i = outParams.begin(); i != outParams.end(); ++i)
    {
        if((*i)->name() == "ReturnValue")
        {
            return "ReturnValue_";
        }
    }
    return "ReturnValue";
}

string
Slice::resultType(const OperationPtr& op, const string& scope, bool dispatch)
{
    InterfaceDefPtr interface = op->interface();
    // when dispatch is true, the result-type is read-only
    list<ParamInfo> outParams = getAllOutParams(op, dispatch,  "", true);
    if(outParams.size() == 0)
    {
        return "void";
    }
    else if(dispatch && op->hasMarshaledResult())
    {
        string name = getNamespace(interface) + "." + interfaceName(interface);
        return getUnqualified(name, scope) + "." + pascalCase(op->name()) + "MarshaledReturnValue";
    }
    else if(outParams.size() > 1)
    {
        return toTupleType(outParams);
    }
    else
    {
        return outParams.front().typeStr;
    }
}

string
Slice::resultTask(const OperationPtr& op, const string& ns, bool dispatch)
{
    string t = resultType(op, ns, dispatch);
    if(t == "void")
    {
        if (dispatch)
        {
            return "global::System.Threading.Tasks.ValueTask";
        }
        else
        {
            return "global::System.Threading.Tasks.Task";
        }
    }
    else if (dispatch)
    {
        return "global::System.Threading.Tasks.ValueTask<" + t + '>';
    }
    else
    {
        return "global::System.Threading.Tasks.Task<" + t + '>';
    }
}

bool
Slice::isCollectionType(const TypePtr& type)
{
    return SequencePtr::dynamicCast(type) || DictionaryPtr::dynamicCast(type);
}

bool
Slice::isValueType(const TypePtr& type)
{
    assert(!OptionalPtr::dynamicCast(type));

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    if(builtin)
    {
        switch(builtin->kind())
        {
            case Builtin::KindString:
            case Builtin::KindObject:
            case Builtin::KindAnyClass:
            {
                return false;
            }
            default:
            {
                return true;
            }
        }
    }

    if(EnumPtr::dynamicCast(type))
    {
        return true;
    }
    return StructPtr::dynamicCast(type);
}

bool
Slice::isReferenceType(const TypePtr& type)
{
    return !isValueType(type);
}

bool
Slice::isMappedToReadOnlyMemory(const SequencePtr& seq)
{
    TypePtr type = seq->type();
    if (auto en = EnumPtr::dynamicCast(type); en && en->underlying())
    {
        type = en->underlying();
    }

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    return builtin && builtin->isNumericTypeOrBool() && !builtin->isVariableLength() &&
        !seq->hasMetaDataWithPrefix("cs:serializable") && !seq->hasMetaDataWithPrefix("cs:generic");
}

Slice::ParamInfo::ParamInfo(const OperationPtr& pOperation,
                            const string& pName,
                            const TypePtr& pType,
                            bool readOnly,
                            bool pTagged,
                            int pTag,
                            const string& pPrefix)
{
    this->operation = pOperation;
    this->name = fixId(pPrefix + pName);
    this->type = pType;
    this->typeStr = CsGenerator::typeToString(pType, getNamespace(InterfaceDefPtr::dynamicCast(operation->container())),
                                              readOnly);
    this->tagged = pTagged;
    this->tag = pTag;
    this->param = 0;
}

Slice::ParamInfo::ParamInfo(const ParamDeclPtr& pParam, bool readOnly, const string& pPrefix)
{
    this->operation = OperationPtr::dynamicCast(pParam->container());
    this->name = fixId(pPrefix + pParam->name());
    this->type = pParam->type();
    this->typeStr = CsGenerator::typeToString(type, getNamespace(InterfaceDefPtr::dynamicCast(operation->container())),
                                              readOnly);
    this->tagged = pParam->tagged();
    this->tag = pParam->tag();
    this->param = pParam;
}

list<ParamInfo>
Slice::getAllInParams(const OperationPtr& op, bool readOnly, const string& prefix)
{
    list<ParamInfo> inParams;
    for(const auto& p : op->inParameters())
    {
        inParams.push_back(ParamInfo(p, readOnly, prefix));
    }
    return inParams;
}
void
Slice::getInParams(const OperationPtr& op, bool readOnly, list<ParamInfo>& requiredParams,
    list<ParamInfo>& taggedParams, const string& prefix)
{
    requiredParams.clear();
    taggedParams.clear();
    for(const auto& p : getAllInParams(op, readOnly, prefix))
    {
        if(p.tagged)
        {
            taggedParams.push_back(p);
        }
        else
        {
            requiredParams.push_back(p);
        }
    }

    //
    // Sort tagged parameters by tag.
    //
    taggedParams.sort([](const auto& lhs, const auto& rhs)
                      {
                          return lhs.tag < rhs.tag;
                      });
}

list<ParamInfo>
Slice::getAllOutParams(const OperationPtr& op, bool readOnly, const string& prefix, bool returnTypeIsFirst)
{
    list<ParamInfo> outParams;

    for(const auto& p : op->outParameters())
    {
        outParams.push_back(ParamInfo(p, readOnly, prefix));
    }

    if(op->returnType())
    {
        auto ret = ParamInfo(op,
                             returnValueName(op->outParameters()),
                             op->returnType(),
                             readOnly,
                             op->returnIsTagged(),
                             op->returnTag(),
                             prefix);

        if(returnTypeIsFirst)
        {
            outParams.push_front(ret);
        }
        else
        {
            outParams.push_back(ret);
        }
    }

    return outParams;
}

void
Slice::getOutParams(const OperationPtr& op, bool readOnly, list<ParamInfo>& requiredParams,
    list<ParamInfo>& taggedParams, const string& prefix)
{
    requiredParams.clear();
    taggedParams.clear();

    for(const auto& p : getAllOutParams(op, readOnly, prefix))
    {
        if(p.tagged)
        {
            taggedParams.push_back(p);
        }
        else
        {
            requiredParams.push_back(p);
        }
    }

    //
    // Sort tagged parameters by tag.
    //
    taggedParams.sort([](const auto& lhs, const auto& rhs)
                      {
                          return lhs.tag < rhs.tag;
                      });
}

vector<string>
Slice::getNames(const list<ParamInfo>& params, string prefix)
{
    return getNames(params, [p = move(prefix)](const auto& item)
                            {
                                return p + item.name;
                            });
}

std::string
Slice::toTuple(const list<ParamInfo>& params, const string& paramPrefix)
{
    if(params.size() == 1)
    {
        auto p = params.front();
        return p.param ?  fixId(paramPrefix + p.param->name()) : fixId(paramPrefix + p.name);
    }
    else
    {
        ostringstream os;
        os << "(";
        for(list<ParamInfo>::const_iterator it = params.begin(); it != params.end();)
        {
            os << (it->param ? fixId(paramPrefix + it->param->name()) : fixId(paramPrefix + it->name));
            if(++it != params.end())
            {
                os << ", ";
            }
        }
        os << ")";
        return os.str();
    }
}

std::string
Slice::toTupleType(const list<ParamInfo>& params, const string& prefix)
{
    if(params.size() == 1)
    {
        auto param = params.front();
        return param.typeStr;
    }
    else
    {
        ostringstream os;
        os << "(";
        for(list<ParamInfo>::const_iterator it = params.begin(); it != params.end();)
        {
            os << it->typeStr;
            os << " " << (it->param ? fixId(prefix + it->param->name()) : fixId(prefix + it->name));
            if(++it != params.end())
            {
                os << ", ";
            }
        }
        os << ")";
        return os.str();
    }
}

vector<string>
Slice::getNames(const list<ParamInfo>& params, function<string (const ParamInfo&)> fn)
{
    return mapfn<ParamInfo>(params, move(fn));
}

string
Slice::CsGenerator::outputStreamWriter(const TypePtr& type, const string& scope, bool forNestedType)
{
    ostringstream out;
    if (auto optional = OptionalPtr::dynamicCast(type))
    {
        // Expected for proxy and class types.
        TypePtr underlying = optional->underlying();
        if (underlying->isInterfaceType())
        {
            out << typeToString(underlying->unit()->builtin(Builtin::KindObject), scope) << ".IceWriterFromNullable";
        }
        else
        {
            assert(underlying->isClassType());
            out << typeToString(underlying, scope) << ".IceWriterFromNullable";
        }
    }
    else if (type->isInterfaceType())
    {
        out << typeToString(type->unit()->builtin(Builtin::KindObject), scope) << ".IceWriter";
    }
    else if (type->isClassType())
    {
        out << typeToString(type, scope) << ".IceWriter";
    }
    else if (auto builtin = BuiltinPtr::dynamicCast(type))
    {
        out << "ZeroC.Ice.OutputStream.IceWriterFrom" << builtinSuffixTable[builtin->kind()];
    }
    else if (DictionaryPtr::dynamicCast(type) || EnumPtr::dynamicCast(type))
    {
        out << helperName(type, scope) << ".IceWriter";
    }
    else if (auto seq = SequencePtr::dynamicCast(type))
    {
        if (isMappedToReadOnlyMemory(seq))
        {
            if (EnumPtr::dynamicCast(seq->type()))
            {
                out << helperName(type, scope) << ".IceWriterFrom" << (forNestedType ? "Array" : "Sequence");
            }
            else
            {
                builtin = BuiltinPtr::dynamicCast(seq->type());
                out << "ZeroC.Ice.OutputStream.IceWriterFrom" << builtinSuffixTable[builtin->kind()]
                    << (forNestedType ? "Array" : "Sequence");
            }
        }
        else
        {
            out << helperName(type, scope) << ".IceWriter";
        }
    }
    else
    {
        out << typeToString(type, scope) << ".IceWriter";
    }
    return out.str();
}

void
Slice::CsGenerator::writeMarshalCode(Output& out,
                                     const TypePtr& type,
                                     int& bitSequenceIndex,
                                     bool forNestedType,
                                     const string& scope,
                                     const string& param,
                                     const string& stream)
{
    if (auto optional = OptionalPtr::dynamicCast(type))
    {
        TypePtr underlying = optional->underlying();

        if (underlying->isInterfaceType())
        {
            // does not use bit sequence
            out << nl << stream << ".WriteNullableProxy(" << param << ");";
        }
        else if (underlying->isClassType())
        {
            // does not use bit sequence
            out << nl << stream << ".WriteNullableClass(" << param;
            if (BuiltinPtr::dynamicCast(underlying))
            {
                out << ", null);"; // no formal type optimization
            }
            else
            {
                out << ", " << typeToString(underlying, scope) << ".IceTypeId);";
            }
        }
        else
        {
            assert(bitSequenceIndex >= 0);
            out << nl << "if (" << param << " != null)";
            out << sb;
            string nonNullParam = param + (isReferenceType(underlying) ? "" : ".Value");
            writeMarshalCode(out, underlying, bitSequenceIndex, forNestedType, scope, nonNullParam, stream);
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << "bitSequence[" << bitSequenceIndex++ << "] = false;";
            out << eb;
        }
    }
    else
    {
        if (type->isInterfaceType())
        {
            out << nl << stream << ".WriteProxy(" << param << ");";
        }
        else if (type->isClassType())
        {
            out << nl << stream << ".WriteClass(" << param;
            if (BuiltinPtr::dynamicCast(type))
            {
                out << ", null);"; // no formal type optimization
            }
            else
            {
                out << ", " << typeToString(type, scope) << ".IceTypeId);";
            }
        }
        else if (auto builtin = BuiltinPtr::dynamicCast(type))
        {
            out << nl << stream << ".Write" << builtinSuffixTable[builtin->kind()] << "(" << param << ");";
        }
        else if (auto st = StructPtr::dynamicCast(type))
        {
            out << nl << param << ".IceWrite(" << stream << ");";
        }
        else if (auto seq = SequencePtr::dynamicCast(type); seq && isMappedToReadOnlyMemory(seq))
        {
            out << nl << stream;
            if (forNestedType)
            {
                out << ".WriteArray(" << param << ");";
            }
            else
            {
                out << ".WriteSequence(" << param << ".Span);";
            }
        }
        else
        {
            out << nl << helperName(type, scope) << ".Write(" << stream << ", " << param << ");";
        }
    }
}

string
Slice::CsGenerator::inputStreamReader(const TypePtr& type, const string& scope)
{
    ostringstream out;
    if (auto optional = OptionalPtr::dynamicCast(type))
    {
        TypePtr underlying = optional->underlying();
        // Expected for classes and proxies
        assert(underlying->isClassType() || underlying->isInterfaceType());
        out << typeToString(underlying, scope) << ".IceReaderIntoNullable";
    }
    else if (auto builtin = BuiltinPtr::dynamicCast(type); builtin && !builtin->usesClasses() &&
                builtin->kind() != Builtin::KindObject)
    {
        out << "ZeroC.Ice.InputStream.IceReaderInto" << builtinSuffixTable[builtin->kind()];
    }
    else if (auto seq = SequencePtr::dynamicCast(type))
    {
        if (isMappedToReadOnlyMemory(seq) && !EnumPtr::dynamicCast(seq->type()))
        {
            builtin = BuiltinPtr::dynamicCast(seq->type());
            out << "ZeroC.Ice.InputStream.IceReaderInto" << builtinSuffixTable[builtin->kind()] << "Array";
        }
        else
        {
            out << helperName(type, scope) << ".IceReader";
        }
    }
    else if (DictionaryPtr::dynamicCast(type) || EnumPtr::dynamicCast(type))
    {
        out << helperName(type, scope) << ".IceReader";
    }
    else
    {
        out << typeToString(type, scope) << ".IceReader";
    }
    return out.str();
}

void
Slice::CsGenerator::writeUnmarshalCode(Output &out,
                                       const TypePtr& type,
                                       int& bitSequenceIndex,
                                       const string& scope,
                                       const string& param,
                                       const string& stream)
{
    out << param << " = ";
    auto optional = OptionalPtr::dynamicCast(type);
    TypePtr underlying = optional ? optional->underlying() : type;

    if (optional)
    {
        if (underlying->isInterfaceType())
        {
            // does not use bit sequence
            out << stream << ".ReadNullableProxy(" << typeToString(underlying, scope) << ".Factory);";
            return;
        }
        else if (underlying->isClassType())
        {
            // does not use bit sequence
            out << stream << ".ReadNullableClass<" << typeToString(underlying, scope) << ">(";
            if (BuiltinPtr::dynamicCast(underlying))
            {
                out << "formalTypeId: null";
            }
            else
            {
                out << typeToString(underlying, scope) << ".IceTypeId";
            }
            out << ");";
            return;
        }
        else
        {
            assert(bitSequenceIndex >= 0);
            out << "bitSequence[" << bitSequenceIndex++ << "] ? ";
            // and keep going
        }
    }

    if (underlying->isInterfaceType())
    {
        assert(!optional);
        out << stream << ".ReadProxy(" << typeToString(underlying, scope) << ".Factory)";
    }
    else if (underlying->isClassType())
    {
        assert(!optional);
        out << stream << ".ReadClass<" << typeToString(underlying, scope) << ">(";
        if (BuiltinPtr::dynamicCast(underlying))
        {
            out << "formalTypeId: null";
        }
        else
        {
            out << typeToString(underlying, scope) << ".IceTypeId";
        }
        out << ")";
    }
    else if (auto builtin = BuiltinPtr::dynamicCast(underlying))
    {
        out << stream << ".Read" << builtinSuffixTable[builtin->kind()] << "()";
    }
    else if (auto st = StructPtr::dynamicCast(underlying))
    {
        out << "new " << getUnqualified(st, scope) << "(" << stream << ")";
    }
    else if (auto seq = SequencePtr::dynamicCast(underlying); seq && isMappedToReadOnlyMemory(seq))
    {
        out << sequenceUnmarshalCode(seq, scope, stream);
    }
    else
    {
        auto constructed = ConstructedPtr::dynamicCast(underlying);
        assert(constructed);
        out << helperName(underlying, scope) << ".Read" << constructed->name() << "(" << stream << ")";
    }

    if (optional)
    {
        if (isReferenceType(underlying))
        {
            out << " : null";
        }
        else
        {
            out << " : (" << typeToString(underlying, scope) << "?)null";
        }
    }
    out << ";";
}

void
Slice::CsGenerator::writeTaggedMarshalCode(Output& out,
                                           const OptionalPtr& optionalType,
                                           bool isDataMember,
                                           const string& scope,
                                           const string& param,
                                           int tag,
                                           const string& stream)
{
    assert(optionalType);
    TypePtr type = optionalType->underlying();

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    StructPtr st = StructPtr::dynamicCast(type);
    SequencePtr seq = SequencePtr::dynamicCast(type);

    if (builtin || type->isInterfaceType() || type->isClassType())
    {
        auto kind = builtin ? builtin->kind() : type->isInterfaceType() ? Builtin::KindObject : Builtin::KindAnyClass;
        out << nl << stream << ".WriteTagged" << builtinSuffixTable[kind] << "(" << tag << ", " << param << ");";
    }
    else if(st)
    {
        out << nl << stream << ".WriteTaggedStruct(" << tag << ", " << param;
        if(!st->isVariableLength())
        {
            out << ", fixedSize: " << st->minWireSize();
        }
        out << ");";
    }
    else if (auto en = EnumPtr::dynamicCast(type))
    {
        string suffix = en->underlying() ? builtinSuffix(en->underlying()) : "Size";
        string underlyingType = en->underlying() ? typeToString(en->underlying(), "") : "int";
        out << nl << stream << ".WriteTagged" << suffix << "(" << tag << ", (" << underlyingType << "?)"
            << param << ");";
    }
    else if(seq)
    {
        const TypePtr elementType = seq->type();
        builtin = BuiltinPtr::dynamicCast(elementType);
        if (isMappedToReadOnlyMemory(seq))
        {
            out << nl << stream;
            if (isDataMember)
            {
                out << ".WriteTaggedArray(" << tag << ", " << param;
            }
            else
            {
                out << ".WriteTaggedSequence(" << tag << ", " << param << ".Span";
            }
            out << ");";
        }
        else if (seq->hasMetaDataWithPrefix("cs:serializable:"))
        {
            out << nl << stream << ".WriteTaggedSerializable(" << tag << ", " << param << ");";
        }
        else if (auto optional = OptionalPtr::dynamicCast(elementType); optional && optional->encodedUsingBitSequence())
        {
            TypePtr underlying = optional->underlying();
            out << nl << stream << ".WriteTaggedSequence(" << tag << ", " << param;
            if (isReferenceType(underlying))
            {
                out << ", withBitSequence: true";
            }
            if (!StructPtr::dynamicCast(underlying))
            {
                out << ", " << outputStreamWriter(underlying, scope, true);
            }
            out << ");";
        }
        else if (elementType->isVariableLength())
        {
            out << nl << stream << ".WriteTaggedSequence(" << tag << ", " << param;
            if (!StructPtr::dynamicCast(elementType))
            {
                out << ", " << outputStreamWriter(elementType, scope, true);
            }
            out << ");";
        }
        else
        {
            // Fixed size = min-size
            out << nl << stream << ".WriteTaggedSequence(" << tag << ", " << param << ", "
                << "elementSize: " << elementType->minWireSize();

            if (!StructPtr::dynamicCast(elementType))
            {
                out << ", " << outputStreamWriter(elementType, scope, true);
            }
            out << ");";
        }
    }
    else
    {
        DictionaryPtr d = DictionaryPtr::dynamicCast(type);
        assert(d);
        TypePtr keyType = d->keyType();
        TypePtr valueType = d->valueType();

        bool withBitSequence = false;

        if (auto optional = OptionalPtr::dynamicCast(valueType); optional && optional->encodedUsingBitSequence())
        {
            withBitSequence = true;
            valueType = optional->underlying();
        }

        out << nl << stream << ".WriteTaggedDictionary(" << tag << ", " << param;

        if (!withBitSequence && !keyType->isVariableLength() && !valueType->isVariableLength())
        {
            // Both are fixed size
            out << ", entrySize: " << (keyType->minWireSize() + valueType->minWireSize());
        }
        if (withBitSequence && isReferenceType(valueType))
        {
            out << ", withBitSequence: true";
        }
        if (!StructPtr::dynamicCast(keyType))
        {
            out << ", " << outputStreamWriter(keyType, scope, true);
        }
        if (!StructPtr::dynamicCast(valueType))
        {
            out << ", " << outputStreamWriter(valueType, scope, true);
        }
        out << ");";
    }
}

void
Slice::CsGenerator::writeTaggedUnmarshalCode(Output& out,
                                             const OptionalPtr& optionalType,
                                             const string& scope,
                                             const string& param,
                                             int tag,
                                             const DataMemberPtr& dataMember,
                                             const string& customStream)
{
    assert(optionalType);
    TypePtr type = optionalType->underlying();

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    StructPtr st = StructPtr::dynamicCast(type);
    SequencePtr seq = SequencePtr::dynamicCast(type);

    out << param << " = ";

    const string stream = customStream.empty() ? "istr" : customStream;

    if (type->isClassType())
    {
        out << stream << ".ReadTaggedClass<" << typeToString(type, scope) << ">(" << tag << ")";
    }
    else if (type->isInterfaceType())
    {
        out << stream << ".ReadTaggedProxy(" << tag << ", " << typeToString(type, scope) << ".Factory)";
    }
    else if (builtin)
    {
        out << stream << ".ReadTagged" << builtinSuffixTable[builtin->kind()] << "(" << tag << ")";
    }
    else if (st)
    {
        out << stream << ".ReadTaggedStruct(" << tag << ", fixedSize: " << (st->isVariableLength() ? "false" : "true")
            << ", " << inputStreamReader(st, scope) << ")";
    }
    else if (auto en = EnumPtr::dynamicCast(type))
    {
        const string tmpName = (dataMember ? dataMember->name() : param) + "_";
        string suffix = en->underlying() ? builtinSuffix(en->underlying()) : "Size";
        string underlyingType = en->underlying() ? typeToString(en->underlying(), "") : "int";

        out << stream << ".ReadTagged" << suffix << "(" << tag << ") is " << underlyingType << " " << tmpName << " ? "
            << helperName(en, scope) << ".As" << en->name() << "(" << tmpName << ") : ("
            << typeToString(en, scope) << "?)null";
    }
    else if (seq)
    {
        const TypePtr elementType = seq->type();
        if (isMappedToReadOnlyMemory(seq))
        {
            out << stream << ".ReadTaggedArray";
            if (auto enElement = EnumPtr::dynamicCast(elementType); enElement && !enElement->isUnchecked())
            {
                out << "(" << tag << ", (" << typeToString(enElement, scope) << " e) => _ = "
                    << helperName(enElement, scope) << ".As" << enElement->name()
                    << "((" << typeToString(enElement->underlying(), scope) << ")e))";
            }
            else
            {
                out << "<" << typeToString(elementType, scope) << ">(" << tag << ")";
            }
        }
        else if (seq->hasMetaDataWithPrefix("cs:serializable:"))
        {
            out << stream << ".ReadTaggedSerializable(" << tag << ") as " << typeToString(seq, scope);
        }
        else if (seq->hasMetaDataWithPrefix("cs:generic:"))
        {
            const string tmpName = (dataMember ? dataMember->name() : param) + "_";
            if (auto optional = OptionalPtr::dynamicCast(elementType); optional && optional->encodedUsingBitSequence())
            {
                TypePtr underlying = optional->underlying();
                out << stream << ".ReadTaggedSequence(" << tag << ", "
                    << (isReferenceType(underlying) ? "withBitSequence: true, " : "")
                    << inputStreamReader(elementType, scope)
                    << ") is global::System.Collections.Generic.ICollection<" << typeToString(elementType, scope)
                    << "> " << tmpName << " ? new " << typeToString(seq, scope) << "(" << tmpName << ")"
                    << " : null";
            }
            else
            {
                out << stream << ".ReadTaggedSequence("
                    << tag << ", minElementSize: " << elementType->minWireSize() << ", fixedSize: "
                    << (elementType->isVariableLength() ? "false" : "true")
                    << ", " << inputStreamReader(elementType, scope)
                    << ") is global::System.Collections.Generic.ICollection<" << typeToString(elementType, scope)
                    << "> " << tmpName << " ? new " << typeToString(seq, scope) << "(" << tmpName << ")"
                    << " : null";
            }
        }
        else
        {
            if (auto optional = OptionalPtr::dynamicCast(elementType); optional && optional->encodedUsingBitSequence())
            {
                TypePtr underlying = optional->underlying();
                out << stream << ".ReadTaggedArray(" << tag << ", "
                    << (isReferenceType(underlying) ? "withBitSequence: true, " : "")
                    << inputStreamReader(underlying, scope) << ")";
            }
            else
            {
                out << stream << ".ReadTaggedArray(" << tag << ", minElementSize: " << elementType->minWireSize()
                    << ", fixedSize: " << (elementType->isVariableLength() ? "false" : "true")
                    << ", " << inputStreamReader(elementType, scope) << ")";
            }
        }
    }
    else
    {
        DictionaryPtr d = DictionaryPtr::dynamicCast(type);
        assert(d);
        TypePtr keyType = d->keyType();
        TypePtr valueType = d->valueType();
        bool withBitSequence = false;

        if (auto optional = OptionalPtr::dynamicCast(valueType); optional && optional->encodedUsingBitSequence())
        {
            withBitSequence = true;
            valueType = optional->underlying();
        }

        bool fixedSize = !keyType->isVariableLength() && !valueType->isVariableLength();
        bool sorted = d->findMetaDataWithPrefix("cs:generic:") == "SortedDictionary";

        out << stream << ".ReadTagged" << (sorted ? "Sorted" : "") << "Dictionary(" << tag
            << ", minKeySize: " << keyType->minWireSize();
        if (!withBitSequence)
        {
            out << ", minValueSize: " << valueType->minWireSize();
        }
        if (withBitSequence && isReferenceType(valueType))
        {
            out << ", withBitSequence: true";
        }
        if (!withBitSequence)
        {
            out << ", fixedSize: " << (fixedSize ? "true" : "false");
        }
        out << ", " << inputStreamReader(keyType, scope) << ", " << inputStreamReader(valueType, scope) << ")";
    }
    out << ";";
}

string
Slice::CsGenerator::sequenceMarshalCode(const SequencePtr& seq, const string& scope, const string& param,
                                        const string& stream)
{
    TypePtr type = seq->type();
    ostringstream out;

    if (isMappedToReadOnlyMemory(seq))
    {
        out << stream << ".WriteSequence(" << param << ".Span)";
    }
    else if (seq->hasMetaDataWithPrefix("cs:serializable:"))
    {
        out << stream << ".WriteSerializable(" << param << ")";
    }
    else if (auto optional = OptionalPtr::dynamicCast(type); optional && optional->encodedUsingBitSequence())
    {
        TypePtr underlying = optional->underlying();
        out << stream << ".WriteSequence(" << param;
        if (isReferenceType(underlying))
        {
            out << ", withBitSequence: true";
        }
        if (!StructPtr::dynamicCast(underlying))
        {
            out << ", " << outputStreamWriter(underlying, scope, true);
        }
        out << ")";
    }
    else
    {
        out << stream << ".WriteSequence(" << param;
        if (!StructPtr::dynamicCast(type))
        {
            out << ", " << outputStreamWriter(type, scope, true);
        }
        out << ")";
    }
    return out.str();
}

string
Slice::CsGenerator::sequenceUnmarshalCode(const SequencePtr& seq, const string& scope, const string& stream)
{
    string generic = seq->findMetaDataWithPrefix("cs:generic:");
    string serializable = seq->findMetaDataWithPrefix("cs:serializable:");

    TypePtr type = seq->type();
    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    auto en = EnumPtr::dynamicCast(type);

    ostringstream out;
    if (!serializable.empty())
    {
        out << "(" << serializable << ") " << stream << ".ReadSerializable()";
    }
    else if (generic.empty())
    {
        if ((builtin && builtin->isNumericTypeOrBool() && !builtin->isVariableLength()) ||
            (en && en->underlying() && en->isUnchecked()))
        {
            out << stream << ".ReadArray<" << typeToString(type, scope) << ">()";
        }
        else if (en && en->underlying())
        {
            out << stream << ".ReadArray((" << typeToString(en, scope) << " e) => _ = " << helperName(en, scope)
                << ".As" << en->name() << "((" << typeToString(en->underlying(), scope) << ")e))";
        }
        else if (auto optional = OptionalPtr::dynamicCast(type); optional && optional->encodedUsingBitSequence())
        {
            TypePtr underlying = optional->underlying();
            out << stream << ".ReadArray(" << (isReferenceType(underlying) ? "withBitSequence: true, " : "")
                << inputStreamReader(underlying, scope) << ")";
        }
        else
        {
            out << stream << ".ReadArray(minElementSize: " << type->minWireSize() << ", "
                << inputStreamReader(type, scope) << ")";
        }
    }
    else
    {
        out << "new " << typeToString(seq, scope) << "(";
        if (generic == "Stack")
        {
            out << "global::System.Linq.Enumerable.Reverse(";
        }

        if ((builtin && builtin->isNumericTypeOrBool() && !builtin->isVariableLength()) ||
            (en && en->underlying() && en->isUnchecked()))
        {
            // We always read an array even when mapped to a collection, as it's expected to be faster than unmarshaling
            // the collection elements one by one.
            out << stream << ".ReadArray<" << typeToString(type, scope) << ">()";
        }
        else if (en && en->underlying())
        {
            out << stream << ".ReadArray((" << typeToString(en, scope) << " e) => _ = "
                << helperName(en, scope) << ".As" << en->name()
                << "((" << typeToString(en->underlying(), scope) << ")e))";
        }
        else if (auto optional = OptionalPtr::dynamicCast(type); optional && optional->encodedUsingBitSequence())
        {
            TypePtr underlying = optional->underlying();
            out << stream << ".ReadSequence(" << (isReferenceType(underlying) ? "withBitSequence: true, " : "")
                << inputStreamReader(underlying, scope) << ")";
        }
        else
        {
            out << stream << ".ReadSequence(minElementSize: " << type->minWireSize() << ", "
                << inputStreamReader(type, scope) << ")";
        }

        if (generic == "Stack")
        {
            out << ")";
        }
        out << ")";
    }
    return out.str();
}

void
Slice::CsGenerator::writeConstantValue(Output& out, const TypePtr& type, const SyntaxTreeBasePtr& valueType,
    const string& value, const string& ns)
{
    ConstPtr constant = ConstPtr::dynamicCast(valueType);
    if (constant)
    {
        out << getUnqualified(constant, ns, "Constants.");
    }
    else
    {
        TypePtr underlying = unwrapIfOptional(type);

        if (auto builtin = BuiltinPtr::dynamicCast(underlying))
        {
            switch (builtin->kind())
            {
                case Builtin::KindString:
                    out << "\"" << toStringLiteral(value, "\a\b\f\n\r\t\v\0", "", UCN, 0) << "\"";
                    break;
                case Builtin::KindUShort:
                case Builtin::KindUInt:
                case Builtin::KindVarUInt:
                    out << value << "U";
                    break;
                case Builtin::KindLong:
                case Builtin::KindVarLong:
                    out << value << "L";
                    break;
                case Builtin::KindULong:
                case Builtin::KindVarULong:
                    out << value << "UL";
                    break;
                case Builtin::KindFloat:
                    out << value << "F";
                    break;
                case Builtin::KindDouble:
                    out << value << "D";
                    break;
                default:
                    out << value;
            }
        }
        else if (EnumPtr::dynamicCast(underlying))
        {
            EnumeratorPtr lte = EnumeratorPtr::dynamicCast(valueType);
            assert(lte);
            out << fixId(lte->scoped());
        }
        else
        {
            out << value;
        }
    }
}

void
Slice::CsGenerator::validateMetaData(const UnitPtr& u)
{
    MetaDataVisitor visitor;
    u->visit(&visitor, true);
}

bool
Slice::CsGenerator::MetaDataVisitor::visitUnitStart(const UnitPtr& p)
{
    //
    // Validate global metadata in the top-level file and all included files.
    //
    StringList files = p->allFiles();
    for(StringList::iterator q = files.begin(); q != files.end(); ++q)
    {
        string file = *q;
        DefinitionContextPtr dc = p->findDefinitionContext(file);
        assert(dc);
        StringList globalMetaData = dc->getMetaData();
        StringList newGlobalMetaData;
        static const string csPrefix = "cs:";
        static const string clrPrefix = "clr:";

        for(StringList::iterator r = globalMetaData.begin(); r != globalMetaData.end(); ++r)
        {
            string& s = *r;
            string oldS = s;

            if(s.find(clrPrefix) == 0)
            {
                s.replace(0, clrPrefix.size(), csPrefix);
            }

            if(s.find(csPrefix) == 0)
            {
                static const string csAttributePrefix = csPrefix + "attribute:";
                if(!(s.find(csAttributePrefix) == 0 && s.size() > csAttributePrefix.size()))
                {
                    dc->warning(InvalidMetaData, file, -1, "ignoring invalid global metadata `" + oldS + "'");
                    continue;
                }
            }
            newGlobalMetaData.push_back(oldS);
        }

        dc->setMetaData(newGlobalMetaData);
    }
    return true;
}

bool
Slice::CsGenerator::MetaDataVisitor::visitModuleStart(const ModulePtr& p)
{
    validate(p);
    return true;
}

void
Slice::CsGenerator::MetaDataVisitor::visitModuleEnd(const ModulePtr&)
{
}

void
Slice::CsGenerator::MetaDataVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    validate(p);
}

bool
Slice::CsGenerator::MetaDataVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    validate(p);
    return true;
}

void
Slice::CsGenerator::MetaDataVisitor::visitClassDefEnd(const ClassDefPtr&)
{
}

bool
Slice::CsGenerator::MetaDataVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    validate(p);
    return true;
}

void
Slice::CsGenerator::MetaDataVisitor::visitExceptionEnd(const ExceptionPtr&)
{
}

bool
Slice::CsGenerator::MetaDataVisitor::visitStructStart(const StructPtr& p)
{
    validate(p);
    return true;
}

void
Slice::CsGenerator::MetaDataVisitor::visitStructEnd(const StructPtr&)
{
}

void
Slice::CsGenerator::MetaDataVisitor::visitOperation(const OperationPtr& p)
{
    validate(p);

    ParamDeclList params = p->parameters();
    for(ParamDeclList::const_iterator i = params.begin(); i != params.end(); ++i)
    {
        visitParamDecl(*i);
    }
}

void
Slice::CsGenerator::MetaDataVisitor::visitParamDecl(const ParamDeclPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitDataMember(const DataMemberPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitSequence(const SequencePtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitDictionary(const DictionaryPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitEnum(const EnumPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitConst(const ConstPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::validate(const ContainedPtr& cont)
{
    const string msg = "ignoring invalid metadata";

    StringList localMetaData = cont->getMetaData();
    StringList newLocalMetaData;

    const UnitPtr ut = cont->unit();
    const DefinitionContextPtr dc = ut->findDefinitionContext(cont->file());
    assert(dc);

    for(StringList::iterator p = localMetaData.begin(); p != localMetaData.end(); ++p)
    {
        string& s = *p;
        string oldS = s;

        const string csPrefix = "cs:";
        const string clrPrefix = "clr:";

        if(s.find(clrPrefix) == 0)
        {
            s.replace(0, clrPrefix.size(), csPrefix);
        }

        if(s.find(csPrefix) == 0)
        {
            SequencePtr seq = SequencePtr::dynamicCast(cont);
            if(seq)
            {
                static const string csGenericPrefix = csPrefix + "generic:";
                if(s.find(csGenericPrefix) == 0)
                {
                    string type = s.substr(csGenericPrefix.size());
                    if(!type.empty())
                    {
                        newLocalMetaData.push_back(s);
                        continue; // Custom type or List<T>
                    }
                }
                static const string csSerializablePrefix = csPrefix + "serializable:";
                if(s.find(csSerializablePrefix) == 0)
                {
                    string meta;
                    if(cont->findMetaData(csPrefix + "generic:", meta))
                    {
                        dc->warning(InvalidMetaData, cont->file(), cont->line(), msg + " `" + meta + "':\n" +
                                    "serialization can only be used with the array mapping for byte sequences");
                        continue;
                    }
                    string type = s.substr(csSerializablePrefix.size());
                    BuiltinPtr builtin = BuiltinPtr::dynamicCast(seq->type());
                    if(!type.empty() && builtin && builtin->kind() == Builtin::KindByte)
                    {
                        newLocalMetaData.push_back(s);
                        continue;
                    }
                }
            }
            else if(StructPtr::dynamicCast(cont))
            {
                if(s.substr(csPrefix.size()) == "property")
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
                if(s.substr(csPrefix.size()) == "readonly")
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
            }
            else if(ClassDefPtr::dynamicCast(cont))
            {
                if(s.substr(csPrefix.size()) == "property")
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
            }
            else if(DictionaryPtr::dynamicCast(cont))
            {
                static const string csGenericPrefix = csPrefix + "generic:";
                if(s.find(csGenericPrefix) == 0)
                {
                    string type = s.substr(csGenericPrefix.size());
                    if(type == "SortedDictionary" ||  type == "SortedList")
                    {
                        newLocalMetaData.push_back(s);
                        continue;
                    }
                }
            }
            else if(ModulePtr::dynamicCast(cont))
            {
                static const string csNamespacePrefix = csPrefix + "namespace:";
                if(s.find(csNamespacePrefix) == 0 && s.size() > csNamespacePrefix.size())
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
            }

            static const string csAttributePrefix = csPrefix + "attribute:";
            if(s.find(csAttributePrefix) == 0 && s.size() > csAttributePrefix.size())
            {
                newLocalMetaData.push_back(s);
                continue;
            }

            dc->warning(InvalidMetaData, cont->file(), cont->line(), msg + " `" + oldS + "'");
            continue;
        }
        newLocalMetaData.push_back(s);
    }

    cont->setMetaData(newLocalMetaData);
}
