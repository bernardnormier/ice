//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Collections.Generic;
using System.Threading.Tasks;
using Test;
using ZeroC.Ice;

namespace ZeroC.IceBox.Test.Admin
{
    public class Client : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            Dictionary<string, string>? properties = CreateTestProperties(ref args);
            properties["Ice.Default.Host"] = "127.0.0.1";
            properties["Ice.Default.Protocol"] = "ice1";
            await using Communicator communicator = Initialize(properties);
            AllTests.Run(this);
            // Shutdown the IceBox server.
            await IProcessPrx.Parse("DemoIceBox/admin -f Process:default -p 9996", communicator).ShutdownAsync();
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
