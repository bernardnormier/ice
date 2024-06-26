//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Glacier2/Glacier2.h"
#include "Ice/Ice.h"

#include "TestHelper.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <list>
#include <thread>

#include "Callback.h"

using namespace std;
using namespace std::chrono_literals;
using namespace Test;

class Client;
void notifyClient();

namespace
{
    Client* instance = nullptr;

    class Executor final
    {
    public:
        void execute(std::function<void()> call, const shared_ptr<Ice::Connection>&)
        {
            bool wasEmpty = false;
            {
                lock_guard<mutex> lg(_mutex);
                wasEmpty = _queue.empty();
                _queue.push_back(std::move(call));
            }
            if (wasEmpty)
            {
                _condVar.notify_one();
            }
        }

        void destroy()
        {
            {
                lock_guard<mutex> lg(_mutex);
                _destroyed = true;
            }
            _condVar.notify_one();
        }

        void run()
        {
            for (;;)
            {
                auto item = nextItem();
                if (!item)
                {
                    break;
                }
                item();
            }
        }

    private:
        std::function<void()> nextItem()
        {
            unique_lock<mutex> lock(_mutex);
            while (_queue.empty())
            {
                if (_destroyed)
                {
                    return nullptr;
                }
                _condVar.wait(lock);
            }
            auto item = _queue.front();
            _queue.pop_front();
            return item;
        }

        list<std::function<void()>> _queue;
        bool _destroyed = false;
        mutex _mutex;
        condition_variable _condVar;
    };

    class SuccessSessionCallback final : public Glacier2::SessionCallback
    {
    public:
        void connected(const shared_ptr<Glacier2::SessionHelper>&) override
        {
            cout << "ok" << endl;
            notifyClient();
        }

        void disconnected(const shared_ptr<Glacier2::SessionHelper>&) override
        {
            cout << "ok" << endl;
            notifyClient();
        }

        void connectFailed(const shared_ptr<Glacier2::SessionHelper>&, std::exception_ptr) override { test(false); }

        void createdCommunicator(const shared_ptr<Glacier2::SessionHelper>& session) override
        {
            test(session->communicator());
        }
    };

    class AfterShutdownSessionCallback final : public Glacier2::SessionCallback
    {
    public:
        void connected(const shared_ptr<Glacier2::SessionHelper>&) override { test(false); }

        void disconnected(const shared_ptr<Glacier2::SessionHelper>&) override { test(false); }

        void connectFailed(const shared_ptr<Glacier2::SessionHelper>&, std::exception_ptr ex) override
        {
            try
            {
                rethrow_exception(ex);
            }
            catch (const Ice::ConnectFailedException&)
            {
                cout << "ok" << endl;
                notifyClient();
            }
            catch (...)
            {
                test(false);
            }
        }

        void createdCommunicator(const shared_ptr<Glacier2::SessionHelper>& session) override
        {
            test(session->communicator());
        }
    };

    class FailSessionCallback final : public Glacier2::SessionCallback
    {
    public:
        void connected(const shared_ptr<Glacier2::SessionHelper>&) override { test(false); }

        void disconnected(const shared_ptr<Glacier2::SessionHelper>&) override { test(false); }

        void connectFailed(const shared_ptr<Glacier2::SessionHelper>&, std::exception_ptr ex) override
        {
            try
            {
                rethrow_exception(ex);
            }
            catch (const Glacier2::PermissionDeniedException&)
            {
                cout << "ok" << endl;
                notifyClient();
            }
            catch (const Ice::LocalException&)
            {
                test(false);
            }
        }

        void createdCommunicator(const shared_ptr<Glacier2::SessionHelper>& session) override
        {
            test(session->communicator());
        }
    };

    class InterruptConnectCallback final : public Glacier2::SessionCallback
    {
    public:
        void connected(const shared_ptr<Glacier2::SessionHelper>&) override { test(false); }

        void disconnected(const shared_ptr<Glacier2::SessionHelper>&) override { test(false); }

        void connectFailed(const shared_ptr<Glacier2::SessionHelper>&, std::exception_ptr ex) override
        {
            try
            {
                rethrow_exception(ex);
            }
            catch (const Ice::CommunicatorDestroyedException&)
            {
                cout << "ok" << endl;
                notifyClient();
            }
            catch (...)
            {
                test(false);
            }
        }

        void createdCommunicator(const shared_ptr<Glacier2::SessionHelper>& session) override
        {
            test(session->communicator());
        }
    };

} // Anonymous namespace end

class Client final : public Test::TestHelper
{
public:
    void run(int, char**) override;

    void notify() { _condVar.notify_one(); }

private:
    shared_ptr<Glacier2::SessionHelper> _session;
    shared_ptr<Glacier2::SessionFactoryHelper> _factory;
    Ice::InitializationData _initData;
    mutex _mutex;
    condition_variable _condVar;
};

void
notifyClient()
{
    instance->notify();
}

void
Client::run(int argc, char** argv)
{
    Ice::CommunicatorHolder ich = initialize(argc, argv);
    auto communicator = ich.communicator();
    instance = this;
    string protocol = getTestProtocol();
    string host = getTestHost();
    _initData.properties = Ice::createProperties(argc, argv, communicator->getProperties());
    _initData.properties->setProperty("Ice.Default.Router", "Glacier2/router:" + getTestEndpoint(50));

    Executor executor;
    auto executorFuture = std::async(launch::async, [&executor] { executor.run(); });

    _initData.executor = [&executor](std::function<void()> call, const Ice::ConnectionPtr& conn)
    { executor.execute(call, conn); };
    _factory = make_shared<Glacier2::SessionFactoryHelper>(_initData, make_shared<FailSessionCallback>());

    //
    // Test to create a session with wrong userid/password
    //

    {
        unique_lock<mutex> lock(_mutex);

        cout << "testing SessionHelper connect with wrong userid/password... " << flush;

        _session = _factory->connect("userid", "xxx");
        //
        // Wait for connectFailed callback
        //
        _condVar.wait_for(lock, 30s);
        test(!_session->isConnected());
    }
    _factory->destroy();

    //
    // Test to interrupt connection establishment
    //

    _initData.properties->setProperty("Ice.Default.Router", "");
    _factory = make_shared<Glacier2::SessionFactoryHelper>(_initData, make_shared<InterruptConnectCallback>());

    {
        unique_lock<mutex> lock(_mutex);
        cout << "testing SessionHelper connect interrupt... " << flush;
        _factory->setRouterHost(host);
        _factory->setPort(getTestPort(_initData.properties, 1));
        _factory->setProtocol(protocol);
        _session = _factory->connect("userid", "abc123");

        this_thread::sleep_for(100ms);
        _session->destroy();

        //
        // Wait for connectFailed callback
        //
        _condVar.wait_for(lock, 30s);
        test(!_session->isConnected());
    }
    _factory->destroy();

    _factory = make_shared<Glacier2::SessionFactoryHelper>(_initData, make_shared<SuccessSessionCallback>());

    {
        unique_lock<mutex> lock(_mutex);
        cout << "testing SessionHelper connect... " << flush;
        _factory->setRouterHost(host);
        _factory->setPort(getTestPort(_initData.properties, 50));
        _factory->setProtocol(protocol);
        _session = _factory->connect("userid", "abc123");

        //
        // Wait for connect callback
        //
        _condVar.wait_for(lock, 30s);

        cout << "testing SessionHelper isConnected after connect... " << flush;
        test(_session->isConnected());
        cout << "ok" << endl;

        cout << "testing SessionHelper categoryForClient after connect... " << flush;
        try
        {
            test(!_session->categoryForClient().empty());
        }
        catch (const Glacier2::SessionNotExistException&)
        {
            test(false);
        }
        cout << "ok" << endl;

        test(!_session->session());

        cout << "testing stringToProxy for server object... " << flush;
        auto base = _session->communicator()->stringToProxy(
            "callback:" + getTestEndpoint(_session->communicator()->getProperties()));
        cout << "ok" << endl;

        cout << "pinging server after session creation... " << flush;
        base->ice_ping();
        cout << "ok" << endl;

        cout << "testing checked cast for server object... " << flush;
        auto twoway = Ice::checkedCast<CallbackPrx>(base);
        test(twoway);
        cout << "ok" << endl;

        cout << "testing server shutdown... " << flush;
        twoway->shutdown();
        cout << "ok" << endl;

        test(_session->communicator());
        cout << "testing SessionHelper destroy... " << flush;
        _session->destroy();

        //
        // Wait for disconnected callback
        //
        _condVar.wait(lock);

        cout << "testing SessionHelper isConnected after destroy... " << flush;
        test(_session->isConnected() == false);
        cout << "ok" << endl;

        cout << "testing SessionHelper categoryForClient after destroy... " << flush;
        try
        {
            test(!_session->categoryForClient().empty());
            test(false);
        }
        catch (const Glacier2::SessionNotExistException&)
        {
        }
        cout << "ok" << endl;

        cout << "testing SessionHelper session after destroy... " << flush;
        test(_session->session() == nullopt);
        cout << "ok" << endl;

        cout << "testing SessionHelper communicator after destroy... " << flush;
        try
        {
            test(_session->communicator());
            _session->communicator()->stringToProxy("dummy");
            test(false);
        }
        catch (const Ice::CommunicatorDestroyedException&)
        {
        }
        cout << "ok" << endl;

        cout << "uninstalling router with communicator... " << flush;
        communicator->setDefaultRouter(nullopt);
        cout << "ok" << endl;

        cout << "testing Glacier2 shutdown... " << flush;
        Ice::ProcessPrx process(communicator, "Glacier2/admin -f Process:" + getTestEndpoint(51));
        process->shutdown();
        try
        {
            process->ice_ping();
            test(false);
        }
        catch (const Ice::LocalException&)
        {
            cout << "ok" << endl;
        }
    }

    _factory->destroy();

    _factory = make_shared<Glacier2::SessionFactoryHelper>(_initData, make_shared<AfterShutdownSessionCallback>());

    //
    // Wait a bit to ensure glaci2router has been shutdown.
    //
    this_thread::sleep_for(100ms);

    {
        unique_lock<mutex> lock(_mutex);
        cout << "testing SessionHelper connect after router shutdown... " << flush;
        _factory->setRouterHost(host);
        _factory->setPort(getTestPort(_initData.properties, 50));
        _factory->setProtocol(protocol);
        _session = _factory->connect("userid", "abc123");

        //
        // Wait for connectFailed callback
        //
        _condVar.wait(lock);

        cout << "testing SessionHelper isConnect after connect failure... " << flush;
        test(_session->isConnected() == false);
        cout << "ok" << endl;

        cout << "testing SessionHelper communicator after connect failure... " << flush;
        try
        {
            test(_session->communicator());
            _session->communicator()->stringToProxy("dummy");
            test(false);
        }
        catch (const Ice::CommunicatorDestroyedException&)
        {
        }
        cout << "ok" << endl;

        cout << "testing SessionHelper destroy after connect failure... " << flush;
        _session->destroy();
        cout << "ok" << endl;
    }

    _factory->destroy();

    // Wait for std::async thread to complete:
    executor.destroy();
    executorFuture.get();
}

DEFINE_TEST(Client)
