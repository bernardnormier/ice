// Copyright (c) ZeroC, Inc.

using System.Diagnostics;

namespace Ice.timeout;

public class AllTests : global::Test.AllTests
{
    public static async Task allTests(global::Test.TestHelper helper)
    {
        Test.ControllerPrx controller =
            Test.ControllerPrxHelper.createProxy(helper.communicator(), "controller:" + helper.getTestEndpoint(1));

        try
        {
            await allTestsWithController(helper, controller);
        }
        catch (Exception)
        {
            // Ensure the adapter is not in the holding state when an unexpected exception occurs to prevent
            // the test from hanging on exit in case a connection which disables timeouts is still opened.
            controller.resumeAdapter();
            throw;
        }
    }

    public static async Task allTestsWithController(global::Test.TestHelper helper, Test.ControllerPrx controller)
    {
        var communicator = helper.communicator();
        string sref = "timeout:" + helper.getTestEndpoint(0);

        Test.TimeoutPrx timeout = Test.TimeoutPrxHelper.createProxy(communicator, sref);

        var output = helper.getWriter();

        output.Write("testing connect after hold-induced connect timeout... ");
        {
            var p = timeout;
            controller.holdAdapter(-1);
            p = Test.TimeoutPrxHelper.uncheckedCast(p.ice_connectionId("connection-10"));
            try
            {
                await p.opAsync();
                test(false);
            }
            catch (ConnectTimeoutException)
            {
                // Expected.
            }

            // Second attempt to trigger ipv6 bug.
            p = Test.TimeoutPrxHelper.uncheckedCast(p.ice_connectionId("connection-11"));
            try
            {
                await p.opAsync();
                test(false);
            }
            catch (ConnectTimeoutException)
            {
                // Expected.
            }

            controller.resumeAdapter();

            try
            {
                Console.WriteLine("retrying with new proxy");
                await Task.Delay(1000);
                p = Test.TimeoutPrxHelper.uncheckedCast(p.ice_connectionId("connection-12")); // new proxy
                await p.opAsync(); // expected to work
            }
            catch (System.Exception exception)
            {
                Console.WriteLine($"!!!! failed with: {exception}");
            }
        }
        output.WriteLine("ok");

        controller.shutdown();
    }
}
