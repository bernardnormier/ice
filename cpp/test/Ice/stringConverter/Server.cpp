//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/Ice.h"
#include "Test.h"
#include "TestHelper.h"

using namespace std;

//
// Server side is pure unicode
//
class MyObjectI : public Test::MyObject
{
public:
    virtual wstring widen(string msg, const Ice::Current&)
    {
        return stringToWstring(msg, Ice::getProcessStringConverter(), Ice::getProcessWstringConverter());
    }

    virtual string narrow(wstring wmsg, const Ice::Current&)
    {
        return wstringToString(wmsg, Ice::getProcessStringConverter(), Ice::getProcessWstringConverter());
    }

    virtual void shutdown(const Ice::Current& current) { current.adapter->getCommunicator()->shutdown(); }
};

class Server : public Test::TestHelper
{
public:
    void run(int, char**);
};

void
Server::run(int argc, char** argv)
{
    Ice::CommunicatorHolder communicator = initialize(argc, argv);
    communicator->getProperties()->setProperty("TestAdapter.Endpoints", getTestEndpoint());
    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("TestAdapter");
    adapter->add(std::make_shared<MyObjectI>(), Ice::stringToIdentity("test"));
    adapter->activate();
    serverReady();
    communicator->waitForShutdown();
}

DEFINE_TEST(Server)
