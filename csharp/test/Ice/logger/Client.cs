//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.IO;
using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.Logger
{
    public class Client : TestHelper
    {
        public override async Task RunAsync(string[] args)
        {
            Console.Out.Write("testing Ice.LogFile... ");
            Console.Out.Flush();
            if (File.Exists("log.txt"))
            {
                File.Delete("log.txt");
            }

            {
                var properties = CreateTestProperties(ref args);
                properties["Ice.LogFile"] = "log.txt";
                await using var communicator = Initialize(properties);
                communicator.Logger.Trace("info", "my logger");
            }
            Assert(File.Exists("log.txt"));
            Assert(File.ReadAllText("log.txt").Contains("my logger"));
            File.Delete("log.txt");
            Console.Out.WriteLine("ok");
        }

        public static Task<int> Main(string[] args) => TestDriver.RunTestAsync<Client>(args);
    }
}
