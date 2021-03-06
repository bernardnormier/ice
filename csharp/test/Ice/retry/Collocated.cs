//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Threading.Tasks;
using System.Collections.Generic;
using Test;
using ZeroC.Ice.Instrumentation;

namespace ZeroC.Ice.Test.Retry
{
    public class Collocated : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            ICommunicatorObserver? observer = Instrumentation.GetObserver();
            Dictionary<string, string>? properties = CreateTestProperties(ref args);
            properties["Ice.RetryIntervals"] = "0 1 10 1";
            properties["Ice.Warn.Dispatch"] = "0";
            //
            // This test kills connections, so we don't want warnings.
            //
            properties["Ice.Warn.Connections"] = "0";
            await using Communicator communicator = Initialize(properties, observer: observer);
            //
            // Configure a second communicator for the invocation timeout
            // + retry test, we need to configure a large retry interval
            // to avoid time-sensitive failures.
            //
            properties["Ice.RetryIntervals"] = "0 1 10000";
            await using Communicator? communicator2 = Initialize(properties, observer: observer);
            communicator.CreateObjectAdapter().Add("retry", new Retry());
            communicator2.CreateObjectAdapter().Add("retry", new Retry());
            await AllTests.Run(this, communicator, communicator2, "retry").ShutdownAsync();
        }
        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Collocated>(args);
    }
}
