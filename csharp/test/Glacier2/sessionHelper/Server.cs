// Copyright (c) ZeroC, Inc.

using System.Reflection;

[assembly: CLSCompliant(true)]

[assembly: AssemblyTitle("IceTest")]
[assembly: AssemblyDescription("Ice test")]
[assembly: AssemblyCompany("ZeroC, Inc.")]

public class Server : Test.TestHelper
{
    public override void run(string[] args)
    {
        using (var communicator = initialize(ref args))
        {
            communicator.getProperties().setProperty("DeactivatedAdapter.Endpoints", getTestEndpoint(1));
            communicator.createObjectAdapter("DeactivatedAdapter");

            communicator.getProperties().setProperty("CallbackAdapter.Endpoints", getTestEndpoint(0));
            Ice.ObjectAdapter adapter = communicator.createObjectAdapter("CallbackAdapter");
            adapter.add(new CallbackI(), Ice.Util.stringToIdentity("callback"));
            adapter.activate();
            communicator.waitForShutdown();
        }
    }

    public static Task<int> Main(string[] args) =>
        Test.TestDriver.runTestAsync<Server>(args);
}
