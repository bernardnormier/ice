// Copyright (c) ZeroC, Inc.

#include "Gen.h"
#include "../Ice/FileUtil.h"
#include "../Slice/FileTracker.h"
#include "../Slice/Util.h"
#include "CsMetadataValidator.h"
#include "CsUtil.h"
#include "Ice/StringUtil.h"
#include "IceRpcVisitors.h"
#include "IceVisitors.h"

using namespace std;
using namespace Slice;
using namespace Slice::Csharp;
using namespace IceInternal;

Slice::Gen::Gen(const string& base, const string& dir, GenMode genMode, bool enableAnalysis)
    : _genMode(genMode), _enableAnalysis(enableAnalysis)
{
    string fileBase = base;
    string::size_type pos = base.find_last_of("/\\");
    if (pos != string::npos)
    {
        fileBase = base.substr(pos + 1);
    }
    string file = fileBase + ".cs";
    string iceRpcFile = fileBase + ".IceRpc.cs";

    if (!dir.empty())
    {
        file = dir + '/' + file;
        iceRpcFile = dir + '/' + iceRpcFile;
    }

    _out.open(file.c_str());
    if (!_out)
    {
        ostringstream os;
        os << "cannot open '" << file << "': " << IceInternal::errorToString(errno);
        throw FileException(os.str());
    }
    FileTracker::instance()->addFile(file);

    if (_genMode == GenMode::IceRpc)
    {
        _iceRpcOut.open(iceRpcFile.c_str());
        if (!_iceRpcOut)
        {
            ostringstream os;
            os << "cannot open '" << iceRpcFile << "': " << IceInternal::errorToString(errno);
            throw FileException(os.str());
        }
        FileTracker::instance()->addFile(iceRpcFile);
    }

    printHeader();

    if (!_enableAnalysis)
    {
        printGeneratedHeader(_out, fileBase + ".ice");
        if (_genMode == GenMode::IceRpc)
        {
            printGeneratedHeader(_iceRpcOut, fileBase + ".ice");
        }
    }

    _out << sp;
    _out << nl << "#nullable enable";
    _out << sp;

    if (_enableAnalysis)
    {
        // Disable some warnings when auto-generated is removed from the header. See printGeneratedHeader above.
        _out << nl << "#pragma warning disable SA1403 // File may only contain a single namespace";
        _out << nl << "#pragma warning disable SA1611 // The documentation for parameter x is missing";

        _out << nl << "#pragma warning disable CA1041 // Provide a message for the ObsoleteAttribute that marks ...";

        _out << nl << "#pragma warning disable CA1068 // Cancellation token as last parameter";
        _out << nl << "#pragma warning disable CA1725 // Change parameter name istr_ to istr in order to match ...";

        // Missing doc - only necessary for the tests.
        _out << nl << "#pragma warning disable SA1602";
        _out << nl << "#pragma warning disable SA1604";
        _out << nl << "#pragma warning disable SA1605";
    }

    _out << nl << "#pragma warning disable CS1591 // Missing XML Comment";
    _out << nl << "#pragma warning disable CS1573 // Parameter has no matching param tag in the XML comment";
    _out << nl << "#pragma warning disable CS0612 // Type or member is obsolete";
    _out << nl << "#pragma warning disable CS0618 // Type or member is obsolete";
    _out << nl << "#pragma warning disable CS0619 // Type or member is obsolete";

    if (_genMode == GenMode::Ice)
    {
        _out << nl << "[assembly:Ice.Slice(\"" << fileBase << ".ice\")]";
    }
    else
    {
        _out << sp;
        _out << nl << "using ZeroC.Slice;";
        _out << sp;
        _out << nl << "[assembly:Slice(\"" << fileBase << ".ice\")]";

        if (_genMode == GenMode::IceRpc)
        {
            _iceRpcOut << sp;
            _iceRpcOut << nl << "using ZeroC.Slice;";
            _iceRpcOut << nl << "using IceRpc.Slice;";
            _iceRpcOut << sp;
            _iceRpcOut << nl << "[assembly:Slice(\"" << fileBase << ".ice\")]";
        }
    }
}

Slice::Gen::~Gen()
{
    if (_out.isOpen())
    {
        _out << nl;
        _out.close();
    }
    if (_iceRpcOut.isOpen())
    {
        _iceRpcOut << nl;
        _iceRpcOut.close();
    }
}

void
Slice::Gen::generate(const UnitPtr& p)
{
    Slice::validateCsMetadata(p);

    if (_genMode == GenMode::Ice)
    {
        Slice::Ice::TypesVisitor typesVisitor(_out);
        p->visit(&typesVisitor);

        Slice::Ice::ResultVisitor resultVisitor(_out);
        p->visit(&resultVisitor);

        // Default skeleton.
        Slice::Ice::SkeletonVisitor skeletonVisitor(_out, false);
        p->visit(&skeletonVisitor);

        // Async skeleton.
        Slice::Ice::SkeletonVisitor asyncSkeletonVisitor(_out, true);
        p->visit(&asyncSkeletonVisitor);
    }
    else
    {
        Slice::IceRpc::TypesVisitor typesVisitor(_out);
        p->visit(&typesVisitor);

        if (_genMode == GenMode::IceRpc)
        {
            Slice::IceRpc::ProxyVisitor proxyVisitor(_iceRpcOut);
            p->visit(&proxyVisitor);

            Slice::IceRpc::SkeletonVisitor skeletonVisitor(_iceRpcOut);
            p->visit(&skeletonVisitor);

        }
    }
}

void
Slice::Gen::printHeader()
{
    _out << "// Copyright (c) ZeroC, Inc.";
    _out << sp;
    _out << nl << "// slice2cs version " << ICE_STRING_VERSION;
}
