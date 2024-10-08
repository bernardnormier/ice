// Copyright (c) ZeroC, Inc.

#nullable enable

namespace Ice.inactivityTimeout;

internal class AllTests : global::Test.AllTests
{
    internal static async Task allTests(global::Test.TestHelper helper)
    {
        Communicator communicator = helper.communicator();
        string proxyString = $"test: {helper.getTestEndpoint()}";
        Test.TestIntfPrx p = Test.TestIntfPrxHelper.createProxy(communicator, proxyString);

        string proxyString3s = $"test: {helper.getTestEndpoint(1)}";

        await testClientInactivityTimeout(p, helper.getWriter());
        await testServerInactivityTimeout(proxyString3s, communicator.getProperties(), helper.getWriter());
        await testWithOutstandingRequest(p, oneway: false, helper.getWriter());
        await testWithOutstandingRequest(p, oneway: true, helper.getWriter());

        await p.shutdownAsync();
    }
    private static async Task testClientInactivityTimeout(Test.TestIntfPrx p, TextWriter output)
    {
        output.Write("testing that the client side inactivity timeout shuts down the connection... ");
        output.Flush();

        await p.ice_pingAsync();
        Connection? connection = p.ice_getConnection();
        test(connection is not null);

        // The inactivity timeout is 3s on the client side and 5s on the server side. 4 seconds tests the client side.
        await Task.Delay(4000);
        await p.ice_pingAsync();
        Connection? connection2 = p.ice_getConnection();
        test(connection2 != connection);
        output.WriteLine("ok");
    }

    private static async Task testServerInactivityTimeout(string proxyString, Properties properties, TextWriter output)
    {
        output.Write("testing that the server side inactivity timeout shuts down the connection... ");
        output.Flush();

        // Create a new communicator with the desired properties.
        properties = properties.Clone();
        properties.setProperty("Ice.Connection.Client.InactivityTimeout", "5");
        Communicator communicator = Util.initialize(new InitializationData { properties = properties });
        Test.TestIntfPrx p = Test.TestIntfPrxHelper.createProxy(communicator, proxyString);

        await p.ice_pingAsync();
        Connection? connection = p.ice_getConnection();
        test(connection is not null);

        // The inactivity timeout is 5s on the client side and 3s on the server side. 4 seconds tests the server side.
        await Task.Delay(4000);
        await p.ice_pingAsync();
        Connection? connection2 = p.ice_getConnection();
        test(connection2 != connection);
        output.WriteLine("ok");
    }

    private static async Task testWithOutstandingRequest(Test.TestIntfPrx p, bool oneway, TextWriter output)
    {
        string onewayString = oneway ? "one-way" : "two-way";
        output.Write($"testing the inactivity timeout with an outstanding {onewayString} request... ");
        output.Flush();

        if (oneway)
        {
            p = (Test.TestIntfPrx)p.ice_oneway();
        }

        await p.ice_pingAsync();
        Connection? connection = p.ice_getConnection();
        test(connection is not null);

        // The inactivity timeout is 3s on the client side and 5s on the server side; 4 seconds tests only the
        // client-side.
        await p.sleepAsync(4000); // two-way blocks for 4 seconds; one-way is non-blocking
        if (oneway)
        {
            await Task.Delay(4000);
        }
        await p.ice_pingAsync();
        Connection? connection2 = p.ice_getConnection();

        if (oneway)
        {
            // With a oneway invocation, the inactivity timeout on the client side shut down the first connection.
            test(connection2 != connection);
        }
        else
        {
            // With a two-way invocation, the inactivity timeout should not shutdown any connection.
            test(connection2 == connection);
        }
        output.WriteLine("ok");
    }
}
