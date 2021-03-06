//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Threading.Tasks;
using Test;

namespace ZeroC.IceDiscovery.Test.Simple
{
    public class Client : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            var properties = CreateTestProperties(ref args);
            // TODO: see server
            properties["Ice.Default.Protocol"] = "ice1";
            await using Ice.Communicator communicator = Initialize(properties);
            int num;
            try
            {
                num = args.Length == 1 ? int.Parse(args[0]) : 0;
            }
            catch (FormatException)
            {
                num = 0;
            }
            AllTests.Run(this, num);
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
