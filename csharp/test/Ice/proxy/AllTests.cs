//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Threading;
using System.Collections.Generic;
using Test;

namespace ZeroC.Ice.Test.Proxy
{
    public class AllTests
    {
        public static IMyClassPrx Run(TestHelper helper)
        {
            Communicator? communicator = helper.Communicator();
            TestHelper.Assert(communicator != null);
            System.IO.TextWriter output = helper.GetWriter();
            output.Write("testing proxy parsing... ");
            output.Flush();

            // ice1 proxies
            string[] ice1ProxyArray =
            {
                "ice -t:tcp -p 10000",
                "ice+tcp:ssl -p 10000",
            };

            // ice2 proxies
            string[] ice2ProxyArray =
            {
                "ice+tcp://host.zeroc.com/identity#facet",
                "ice+tcp://host.zeroc.com:1000/category/name",
                "ice+tcp://host.zeroc.com/category/name%20with%20space",
                "ice+ws://host.zeroc.com//identity",
                "ice+ws://host.zeroc.com//identity?alt-endpoint=host2.zeroc.com",
                "ice+ws://host.zeroc.com//identity?alt-endpoint=host2.zeroc.com:10000",
                "ice+tcp://[::1]:10000/identity?alt-endpoint=host1:10000,host2,host3,host4",
                "ice+ssl://[::1]:10000/identity?alt-endpoint=host1:10000&alt-endpoint=host2,host3&alt-endpoint=[::2]",
                "ice:location//identity#facet",
                "ice+tcp://host.zeroc.com//identity",
                "ice+tcp://host.zeroc.com:/identity", // another syntax for empty port
                "ice+universal://com.zeroc.ice/identity?transport=iaps&option=a,b%2Cb,c&option=d",
                "ice+universal://host.zeroc.com/identity?transport=100",
                "ice+universal://[::ab:cd:ef:00]/identity?transport=bt", // leading :: to make the address IPv6-like
                "ice+ws://host.zeroc.com/identity?resource=/foo%2Fbar?/xyz",
                "ice+universal://host.zeroc.com:10000/identity?transport=tcp",
                "ice+universal://host.zeroc.com/identity?transport=ws&option=/foo%2520/bar",
                "ice:tcp -p 10000", // a valid URI
            };

            // ice3 proxies
            string[] ice3ProxyArray =
            {
                "ice+universal://host.zeroc.com/identity?transport=ws&option=/foo%2520/bar&protocol=3"
            };

            foreach (string str in ice1ProxyArray)
            {
                var prx = IObjectPrx.Parse(str, communicator);
                TestHelper.Assert(prx.Protocol == Protocol.Ice1);
                // output.WriteLine($"{str} = {prx}");
                var prx2 = IObjectPrx.Parse(prx.ToString()!, communicator);
                TestHelper.Assert(prx.Equals(prx2)); // round-trip works
            }

            foreach (string str in ice2ProxyArray)
            {
                var prx = IObjectPrx.Parse(str, communicator);
                TestHelper.Assert(prx.Protocol == Protocol.Ice2);
                // output.WriteLine($"{str} = {prx}");
                var prx2 = IObjectPrx.Parse(prx.ToString()!, communicator);
                TestHelper.Assert(prx.Equals(prx2)); // round-trip works
            }

            foreach (string str in ice3ProxyArray)
            {
                var prx = IObjectPrx.Parse(str, communicator);
                TestHelper.Assert(prx.Protocol == (Protocol)3);
                // output.WriteLine($"{str} = {prx}");
                var prx2 = IObjectPrx.Parse(prx.ToString()!, communicator);
                TestHelper.Assert(prx.Equals(prx2)); // round-trip works
            }

            string[] badProxyArray =
            {
                "ice+tcp://host.zeroc.com:foo",     // missing host
                "ice+tcp:identity?protocol=invalid", // invalid protocol
                "ice+universal://host.zeroc.com", // missing transport
                "ice+universal://host.zeroc.com?transport=100&protocol=ice1", // invalid protocol
                "ice://host:1000/identity", // host not allowed
                "ice+universal:/identity", // missing host
                "ice+tcp://host.zeroc.com/identity?protocol=3", // unknown protocol (must use universal)
                "ice+ws://host.zeroc.com//identity?protocol=ice1", // invalid protocol
                "ice+tcp://host.zeroc.com/identity?alt-endpoint=host2?protocol=ice2", // protocol option in alt-endpoint
                "ice+tcp://host.zeroc.com/identity?foo=bar", // unknown option

                "",
                "\"\"",
                "\"\" test", // invalid trailing characters
                "test:", // missing endpoint
                "id@adapter test",
                "id -f \"facet x",
                "id -f \'facet x",
                "test -f facet@test @test",
                "test -p 2.0",
                "test:tcp@adapterId",
                "test: :tcp",
                "id:opaque -t 99 -v abcd -x abc", // invalid x option
                "id:opaque", // missing -t and -v
                "id:opaque -t 1 -t 1 -v abcd", // repeated -t
                "id:opaque -t 1 -v abcd -v abcd",
                "id:opaque -v abcd",
                "id:opaque -t 1",
                "id:opaque -t -v abcd",
                "id:opaque -t 1 -v",
                "id:opaque -t x -v abcd",
                "id:opaque -t -1 -v abcd", // -t must be >= 0
                "id:opaque -t 99 -v x?c", // invalid char in v
                "id:opaque -t 99 -v xc", // invalid length for base64 input
            };

            foreach (string str in badProxyArray)
            {
                try
                {
                    _ = IObjectPrx.Parse(str, communicator);
                    TestHelper.Assert(false);
                }
                catch (FormatException)
                {
                    // expected
                }
            }

            string rf = helper.GetTestProxy("test");
            var baseProxy = IObjectPrx.Parse(rf, communicator);
            TestHelper.Assert(baseProxy != null);

            var b1 = IObjectPrx.Parse("ice:test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                              b1.AdapterId.Length == 0 && b1.Facet.Length == 0);

            b1 = IObjectPrx.Parse("ice:test ", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet.Length == 0);

            b1 = IObjectPrx.Parse(" ice:test ", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                              b1.Facet.Length == 0);

            b1 = IObjectPrx.Parse(" ice:test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet.Length == 0);

            b1 = IObjectPrx.Parse("test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                              b1.AdapterId.Length == 0 && b1.Facet.Length == 0);

            b1 = IObjectPrx.Parse("test ", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet.Length == 0);

            b1 = IObjectPrx.Parse(" test ", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                              b1.Facet.Length == 0);

            b1 = IObjectPrx.Parse(" test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet.Length == 0);

            // The following tests are only relevant to the ice1 format
            b1 = IObjectPrx.Parse("'test -f facet'", communicator);
            TestHelper.Assert(b1.Identity.Name == "test -f facet" && b1.Identity.Category.Length == 0 &&
                    b1.Facet.Length == 0);
            try
            {
                b1 = IObjectPrx.Parse("\"test -f facet'", communicator);
                TestHelper.Assert(false);
            }
            catch (FormatException)
            {
            }
            b1 = IObjectPrx.Parse("\"test -f facet\"", communicator);
            TestHelper.Assert(b1.Identity.Name == "test -f facet" && b1.Identity.Category.Length == 0 &&
                    b1.Facet.Length == 0);
            b1 = IObjectPrx.Parse("\"test -f facet@test\"", communicator);
            TestHelper.Assert(b1.Identity.Name == "test -f facet@test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet.Length == 0);
            b1 = IObjectPrx.Parse("\"test -f facet@test @test\"", communicator);
            TestHelper.Assert(b1.Identity.Name == "test -f facet@test @test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet.Length == 0);
            try
            {
                b1 = IObjectPrx.Parse("test test", communicator);
                TestHelper.Assert(false);
            }
            catch (FormatException)
            {
            }
            b1 = IObjectPrx.Parse("test\\040test", communicator);

            TestHelper.Assert(b1.Identity.Name == "test test" && b1.Identity.Category.Length == 0);
            try
            {
                b1 = IObjectPrx.Parse("test\\777", communicator);
                TestHelper.Assert(false);
            }
            catch (FormatException)
            {
            }
            b1 = IObjectPrx.Parse("test\\40test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test test");

            // Test some octal and hex corner cases.
            b1 = IObjectPrx.Parse("test\\4test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test\u0004test");
            b1 = IObjectPrx.Parse("test\\04test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test\u0004test");
            b1 = IObjectPrx.Parse("test\\004test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test\u0004test");
            b1 = IObjectPrx.Parse("test\\1114test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test\u00494test");

            b1 = IObjectPrx.Parse("test\\b\\f\\n\\r\\t\\'\\\"\\\\test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test\b\f\n\r\t\'\"\\test" && b1.Identity.Category.Length == 0);

            // End of ice1 format-only tests

            b1 = IObjectPrx.Parse("ice:category/test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category" &&
                    b1.AdapterId.Length == 0);

            b1 = IObjectPrx.Parse("ice+tcp://host:10000/test?source-address=::1", communicator);
            TestHelper.Assert(b1.Equals(IObjectPrx.Parse(b1.ToString()!, communicator)));

            b1 = IObjectPrx.Parse("ice:adapter//test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.AdapterId.Equals("adapter"));

            b1 = IObjectPrx.Parse("ice:adapter/category/test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category" &&
                    b1.AdapterId.Equals("adapter"));

            b1 = IObjectPrx.Parse("ice:adapter:tcp/category/test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category" &&
                    b1.AdapterId.Equals("adapter:tcp"));
            // preferred syntax with escape:
            TestHelper.Assert(b1.Equals(IObjectPrx.Parse("ice:adapter%3Atcp/category/test", communicator)));

            b1 = IObjectPrx.Parse("category/test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category" &&
                    b1.AdapterId.Length == 0);

            b1 = IObjectPrx.Parse("test:tcp -h host -p 10000 --sourceAddress \"::1\"", communicator);
            TestHelper.Assert(b1.Equals(IObjectPrx.Parse(b1.ToString()!, communicator)));

            b1 = IObjectPrx.Parse(
                "test:udp -h host -p 10000 --sourceAddress \"::1\" --interface \"0:0:0:0:0:0:0:1%lo\"",
                communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.AdapterId.Length == 0);

            b1 = IObjectPrx.Parse("test@adapter", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.AdapterId.Equals("adapter"));

            b1 = IObjectPrx.Parse("category/test@adapter", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category" &&
                    b1.AdapterId.Equals("adapter"));

            b1 = IObjectPrx.Parse("category/test@adapter:tcp", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category" &&
                    b1.AdapterId.Equals("adapter:tcp"));

            // The following tests are only for the ice1 format:
            b1 = IObjectPrx.Parse("'category 1/test'@adapter", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category 1" &&
                    b1.AdapterId.Equals("adapter"));
            b1 = IObjectPrx.Parse("'category/test 1'@adapter", communicator);
            TestHelper.Assert(b1.Identity.Name == "test 1" && b1.Identity.Category == "category" &&
                    b1.AdapterId.Equals("adapter"));
            b1 = IObjectPrx.Parse("'category/test'@'adapter 1'", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category" &&
                    b1.AdapterId.Equals("adapter 1"));
            b1 = IObjectPrx.Parse("\"category \\/test@foo/test\"@adapter", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category /test@foo" &&
                    b1.AdapterId.Equals("adapter"));
            b1 = IObjectPrx.Parse("\"category \\/test@foo/test\"@\"adapter:tcp\"", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category == "category /test@foo" &&
                    b1.AdapterId.Equals("adapter:tcp"));
            // End of ice1 format-only tests.

            b1 = IObjectPrx.Parse("ice:id#facet", communicator);
            TestHelper.Assert(b1.Identity.Name == "id" && b1.Identity.Category.Length == 0 && b1.Facet == "facet");

            b1 = IObjectPrx.Parse("ice:id#facet%20x", communicator);
            TestHelper.Assert(b1.Identity.Name == "id" && b1.Identity.Category.Length == 0 && b1.Facet == "facet x");

            b1 = IObjectPrx.Parse("id -f facet", communicator);
            TestHelper.Assert(b1.Identity.Name == "id" && b1.Identity.Category.Length == 0 && b1.Facet == "facet");

            b1 = IObjectPrx.Parse("id -f 'facet x'", communicator);
            TestHelper.Assert(b1.Identity.Name == "id" && b1.Identity.Category.Length == 0 && b1.Facet == "facet x");

            // The following tests are only for the ice1 format:
            b1 = IObjectPrx.Parse("id -f \"facet x\"", communicator);
            TestHelper.Assert(b1.Identity.Name == "id" && b1.Identity.Category.Length == 0 && b1.Facet == "facet x");
            b1 = IObjectPrx.Parse("test -f facet:tcp", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet == "facet" && b1.AdapterId.Length == 0);
            b1 = IObjectPrx.Parse("test -f \"facet:tcp\"", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet == "facet:tcp" && b1.AdapterId.Length == 0);
            b1 = IObjectPrx.Parse("test -f facet@test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet == "facet" && b1.AdapterId.Equals("test"));
            b1 = IObjectPrx.Parse("test -f 'facet@test'", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet == "facet@test" && b1.AdapterId.Length == 0);
            b1 = IObjectPrx.Parse("test -f 'facet@test'@test", communicator);
            TestHelper.Assert(b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                    b1.Facet == "facet@test" && b1.AdapterId.Equals("test"));
             // End of ice1 format-only tests.

            b1 = IObjectPrx.Parse("ice:test", communicator);
            TestHelper.Assert(b1.InvocationMode == InvocationMode.Twoway);
            TestHelper.Assert(!b1.IsOneway);

             b1 = IObjectPrx.Parse("test", communicator);
            TestHelper.Assert(b1.InvocationMode == InvocationMode.Twoway);
            TestHelper.Assert(!b1.IsOneway);

            b1 = IObjectPrx.Parse("test -t", communicator);
            TestHelper.Assert(b1.InvocationMode == InvocationMode.Twoway);

            b1 = IObjectPrx.Parse("test -o", communicator);
            TestHelper.Assert(b1.InvocationMode == InvocationMode.Oneway);

            b1 = IObjectPrx.Parse("test -O", communicator);
            TestHelper.Assert(b1.InvocationMode == InvocationMode.BatchOneway);

            b1 = IObjectPrx.Parse("test -d", communicator);
            TestHelper.Assert(b1.InvocationMode == InvocationMode.Datagram);

            b1 = IObjectPrx.Parse("test -D", communicator);
            TestHelper.Assert(b1.InvocationMode == InvocationMode.BatchDatagram);

            b1 = IObjectPrx.Parse("ice:test", communicator);
            TestHelper.Assert(b1.Protocol == Protocol.Ice2 && b1.Encoding == Encoding.V2_0);
            b1 = IObjectPrx.Parse("test", communicator);
            TestHelper.Assert(b1.Protocol == Protocol.Ice1 && b1.Encoding == Encoding.V1_1);

            b1 = IObjectPrx.Parse("ice:test?encoding=6.5", communicator);
            TestHelper.Assert(b1.Encoding.Major == 6 && b1.Encoding.Minor == 5);
            b1 = IObjectPrx.Parse("test -e 6.5", communicator);
            TestHelper.Assert(b1.Encoding.Major == 6 && b1.Encoding.Minor == 5);

            b1 = IObjectPrx.Parse("ice:test?encoding=2.1&protocol=6", communicator);
            TestHelper.Assert(b1.Protocol == (Protocol)6 && b1.Encoding.Major == 2 && b1.Encoding.Minor == 1);

            // Test invalid endpoint syntax
            // TODO: why are we testing this here?
            try
            {
                communicator.CreateObjectAdapterWithEndpoints("BadAdapter", " : ");
                TestHelper.Assert(false);
            }
            catch (FormatException)
            {
            }

            try
            {
                communicator.CreateObjectAdapterWithEndpoints("BadAdapter", "tcp: ");
                TestHelper.Assert(false);
            }
            catch (FormatException)
            {
            }

            try
            {
                communicator.CreateObjectAdapterWithEndpoints("BadAdapter", ":tcp");
                TestHelper.Assert(false);
            }
            catch (FormatException)
            {
            }

            // Test for bug ICE-5543: escaped escapes in Identity.Parse
            var id = new Identity("test", ",X2QNUAzSBcJ_e$AV;E\\");
            var id2 = Identity.Parse(id.ToString(communicator.ToStringMode), uriFormat: false);
            var id3 = Identity.Parse(id.ToString()); // new URI style
            TestHelper.Assert(id == id2);
            TestHelper.Assert(id == id3);

            id = new Identity("test", ",X2QNUAz\\SB\\/cJ_e$AV;E\\\\");
            id2 = Identity.Parse(id.ToString(communicator.ToStringMode), uriFormat: false);
            id3 = Identity.Parse(id.ToString());
            TestHelper.Assert(id == id2);
            TestHelper.Assert(id == id3);

            id = new Identity("/test", "cat/");
            string idStr = id.ToString(communicator.ToStringMode);
            TestHelper.Assert(idStr == "cat\\//\\/test");
            id2 = Identity.Parse(idStr, uriFormat: false);
            id3 = Identity.Parse(id.ToString());
            TestHelper.Assert(id == id2);
            TestHelper.Assert(id == id3);

            // Input string in ice1 format with various pitfalls
            idStr = "\\342\\x82\\254\\60\\x9\\60\\";
            id = Identity.Parse(idStr, uriFormat: false);
            TestHelper.Assert(id.Name == "€0\t0\\" && id.Category.Length == 0);

            try
            {
                // Illegal character < 32
                _ = Identity.Parse("xx\01FooBar", uriFormat: false);
                TestHelper.Assert(false);
            }
            catch (FormatException)
            {
            }

            try
            {
                // Illegal surrogate
                _ = Identity.Parse("xx\\ud911", uriFormat: false);
                TestHelper.Assert(false);
            }
            catch (FormatException)
            {
            }

            // Testing bytes 127(\x7F, \177) and €
            // € is encoded as 0x20AC (UTF-16) and 0xE2 0x82 0xAC (UTF-8)
            id = new Identity("test", "\x7f€");

            idStr = id.ToString();
            TestHelper.Assert(idStr == "%7F%E2%82%AC/test");
            id2 = Identity.Parse(idStr);
            TestHelper.Assert(id == id2);

            idStr = id.ToString(ToStringMode.Unicode);
            TestHelper.Assert(idStr == "\\u007f€/test");
            id2 = Identity.Parse(idStr, uriFormat: false);
            TestHelper.Assert(id == id2);

            idStr = id.ToString(ToStringMode.ASCII);
            TestHelper.Assert(idStr == "\\u007f\\u20ac/test");
            id2 = Identity.Parse(idStr, uriFormat: false);
            TestHelper.Assert(id == id2);

            idStr = id.ToString(ToStringMode.Compat);
            TestHelper.Assert(idStr == "\\177\\342\\202\\254/test");
            id2 = Identity.Parse(idStr, uriFormat: false);
            TestHelper.Assert(id == id2);

            // More unicode character
            id = new Identity("banana \x0E-\ud83c\udf4c\u20ac\u00a2\u0024", "greek \ud800\udd6a");
            idStr = id.ToString();
            TestHelper.Assert(idStr == "greek%20%F0%90%85%AA/banana%20%0E-%F0%9F%8D%8C%E2%82%AC%C2%A2%24");
            id2 = Identity.Parse(idStr);
            TestHelper.Assert(id == id2);

            idStr = id.ToString(ToStringMode.Unicode);
            TestHelper.Assert(idStr == "greek \ud800\udd6a/banana \\u000e-\ud83c\udf4c\u20ac\u00a2$");
            id2 = Identity.Parse(idStr, uriFormat: false);
            TestHelper.Assert(id == id2);

            idStr = id.ToString(ToStringMode.ASCII);
            TestHelper.Assert(idStr == "greek \\U0001016a/banana \\u000e-\\U0001f34c\\u20ac\\u00a2$");
            id2 = Identity.Parse(idStr, uriFormat: false);
            TestHelper.Assert(id == id2);

            idStr = id.ToString(ToStringMode.Compat);
            id2 = Identity.Parse(idStr, uriFormat: false);
            TestHelper.Assert(idStr == "greek \\360\\220\\205\\252/banana \\016-\\360\\237\\215\\214\\342\\202\\254\\302\\242$");
            TestHelper.Assert(id == id2);

            output.WriteLine("ok");

            output.Write("testing proxyToString... ");
            output.Flush();
            b1 = IObjectPrx.Parse(rf, communicator);

            if (b1.GetConnection() is Connection connection) // not coloc-optimized target
            {
                IObjectPrx b2 = connection.CreateProxy(Identity.Parse("fixed"), IObjectPrx.Factory);
                if (connection.Endpoint.Protocol == Protocol.Ice1)
                {
                    TestHelper.Assert(b2.ToString() == "fixed -t -e 1.1");
                }
                else
                {
                    TestHelper.Assert(b2.ToString() == "ice:fixed");
                }
            }
            output.WriteLine("ok");

            output.Write("testing propertyToProxy... ");
            output.Flush();

            string propertyPrefix = "Foo.Proxy";
            communicator.SetProperty(propertyPrefix, helper.GetTestProxy("test", 0));
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(
                b1.Identity.Name == "test" && b1.Identity.Category.Length == 0 &&
                b1.AdapterId.Length == 0 && b1.Facet.Length == 0);

            string property;

            property = propertyPrefix + ".Locator";
            TestHelper.Assert(b1.Locator == null);
            communicator.SetProperty(property, "ice+tcp://host:10000/locator");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.Locator != null && b1.Locator.Identity.Name == "locator");
            communicator.SetProperty(property, "");
            property = propertyPrefix + ".LocatorCacheTimeout";
            TestHelper.Assert(b1.LocatorCacheTimeout == Timeout.InfiniteTimeSpan);
            communicator.SetProperty(property, "1s");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.LocatorCacheTimeout == TimeSpan.FromSeconds(1));
            communicator.SetProperty(property, "");

            // Now retest with an indirect proxy.
            communicator.SetProperty(propertyPrefix, "test");
            property = propertyPrefix + ".Locator";
            communicator.SetProperty(property, "ice+tcp://host:10000/locator");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.Locator != null && b1.Locator.Identity.Name == "locator");
            communicator.SetProperty(property, "");

            property = propertyPrefix + ".LocatorCacheTimeout";
            TestHelper.Assert(b1.LocatorCacheTimeout == Timeout.InfiniteTimeSpan);
            communicator.SetProperty(property, "1s");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.LocatorCacheTimeout == TimeSpan.FromSeconds(1));
            communicator.SetProperty(property, "");

            // This cannot be tested so easily because the property is cached
            // on communicator initialization.
            //
            //communicator.SetProperty("Default.LocatorCacheTimeout", "60");
            //b1 = communicator.propertyToProxy(propertyPrefix);
            //TestHelper.Assert(b1.LocatorCacheTimeout == 60);
            //communicator.SetProperty("Default.LocatorCacheTimeout", "");

            communicator.SetProperty(propertyPrefix, helper.GetTestProxy("test", 0));

            property = propertyPrefix + ".Router";
            TestHelper.Assert(b1.Router == null);
            communicator.SetProperty(property, "ice+tcp://host:10000/router");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.Router != null && b1.Router.Identity.Name == "router");
            communicator.RemoveProperty(property);

            property = propertyPrefix + ".PreferNonSecure";
            TestHelper.Assert(b1.PreferNonSecure);
            communicator.SetProperty(property, "0");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(!b1.PreferNonSecure);
            communicator.RemoveProperty(property);

            property = propertyPrefix + ".ConnectionCached";
            TestHelper.Assert(b1.IsConnectionCached);
            communicator.SetProperty(property, "0");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(!b1.IsConnectionCached);
            communicator.RemoveProperty(property);

            property = propertyPrefix + ".InvocationTimeout";
            TestHelper.Assert(b1.InvocationTimeout == -1);
            communicator.SetProperty(property, "1000");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.InvocationTimeout == 1000);
            communicator.RemoveProperty(property);

            property = propertyPrefix + ".EndpointSelection";
            TestHelper.Assert(b1.EndpointSelection == EndpointSelectionType.Random);
            communicator.SetProperty(property, "Random");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.EndpointSelection == EndpointSelectionType.Random);
            communicator.SetProperty(property, "Ordered");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.EndpointSelection == EndpointSelectionType.Ordered);
            communicator.RemoveProperty(property);

            property = propertyPrefix + ".Context.c1";
            TestHelper.Assert(!b1.Context.ContainsKey("c1"));
            communicator.SetProperty(property, "TEST");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.Context["c1"].Equals("TEST"));

            property = propertyPrefix + ".Context.c2";
            TestHelper.Assert(!b1.Context.ContainsKey("c2"));
            communicator.SetProperty(property, "TEST");
            b1 = communicator.GetPropertyAsProxy(propertyPrefix, IObjectPrx.Factory)!;
            TestHelper.Assert(b1.Context["c2"].Equals("TEST"));

            communicator.SetProperty(propertyPrefix + ".Context.c1", "");
            communicator.SetProperty(propertyPrefix + ".Context.c2", "");

            output.WriteLine("ok");

            output.Write("testing IObjectPrx.ToProperty... ");
            output.Flush();

            IRouterPrx router = IRouterPrx.Parse("ice:router?encoding=1.1", communicator).Clone(
                cacheConnection: true,
                preferNonSecure: true,
                endpointSelection: EndpointSelectionType.Random,
                locatorCacheTimeout: TimeSpan.FromSeconds(200),
                invocationTimeout: 1500);

            ILocatorPrx? locator = ILocatorPrx.Parse("ice:locator", communicator).Clone(
                cacheConnection: false,
                preferNonSecure: true,
                endpointSelection: EndpointSelectionType.Random,
                locatorCacheTimeout: TimeSpan.FromSeconds(300),
                invocationTimeout: 1500,
                router: router);

            b1 = IObjectPrx.Parse("ice:test", communicator).Clone(
                cacheConnection: true,
                preferNonSecure: false,
                endpointSelection: EndpointSelectionType.Ordered,
                locatorCacheTimeout: TimeSpan.FromSeconds(100),
                invocationTimeout: 1234,
                locator: locator);

            Dictionary<string, string> proxyProps = b1.ToProperty("Test");
            TestHelper.Assert(proxyProps.Count == 18);

            TestHelper.Assert(proxyProps["Test"] == "ice:test");
            TestHelper.Assert(proxyProps["Test.ConnectionCached"] == "1");
            TestHelper.Assert(proxyProps["Test.PreferNonSecure"] == "0");
            TestHelper.Assert(proxyProps["Test.EndpointSelection"] == "Ordered");
            TestHelper.Assert(proxyProps["Test.LocatorCacheTimeout"] == "100s");
            TestHelper.Assert(proxyProps["Test.InvocationTimeout"] == "1234");

            TestHelper.Assert(proxyProps["Test.Locator"] == "ice:locator"); // strange test with an indirect locator!
            TestHelper.Assert(proxyProps["Test.Locator.ConnectionCached"] == "0");
            TestHelper.Assert(proxyProps["Test.Locator.PreferNonSecure"] == "1");
            TestHelper.Assert(proxyProps["Test.Locator.EndpointSelection"] == "Random");
            TestHelper.Assert(proxyProps["Test.Locator.LocatorCacheTimeout"] == "5m");
            TestHelper.Assert(proxyProps["Test.Locator.InvocationTimeout"] == "1500");

            TestHelper.Assert(proxyProps["Test.Locator.Router"] == "ice:router?encoding=1.1"); // also very strange
            TestHelper.Assert(proxyProps["Test.Locator.Router.ConnectionCached"] == "1");
            TestHelper.Assert(proxyProps["Test.Locator.Router.PreferNonSecure"] == "1");
            TestHelper.Assert(proxyProps["Test.Locator.Router.EndpointSelection"] == "Random");
            TestHelper.Assert(proxyProps["Test.Locator.Router.LocatorCacheTimeout"] == "200s");
            TestHelper.Assert(proxyProps["Test.Locator.Router.InvocationTimeout"] == "1500");

            output.WriteLine("ok");

            output.Write("testing IObjectPrx.Communicator... ");
            output.Flush();
            TestHelper.Assert(baseProxy.Communicator == communicator);
            output.WriteLine("ok");

            output.Write("testing proxy Clone... ");

            TestHelper.Assert(baseProxy.Clone(facet: "facet", IObjectPrx.Factory).Facet == "facet");
            TestHelper.Assert(baseProxy.Clone(adapterId: "id").AdapterId.Equals("id"));
            TestHelper.Assert(!baseProxy.Clone(invocationMode: InvocationMode.Twoway).IsOneway);
            TestHelper.Assert(baseProxy.Clone(invocationMode: InvocationMode.Oneway).IsOneway);
            TestHelper.Assert(baseProxy.Clone(invocationMode: InvocationMode.Datagram).IsOneway);
            TestHelper.Assert(baseProxy.Clone(invocationMode: InvocationMode.BatchOneway).InvocationMode ==
                InvocationMode.BatchOneway);
            TestHelper.Assert(baseProxy.Clone(invocationMode: InvocationMode.Datagram).InvocationMode ==
                InvocationMode.Datagram);
            TestHelper.Assert(baseProxy.Clone(invocationMode: InvocationMode.BatchDatagram).InvocationMode ==
                InvocationMode.BatchDatagram);
            TestHelper.Assert(baseProxy.Clone(preferNonSecure: true).PreferNonSecure);
            TestHelper.Assert(!baseProxy.Clone(preferNonSecure: false).PreferNonSecure);

            try
            {
                baseProxy.Clone(invocationTimeout: 0);
                TestHelper.Assert(false);
            }
            catch (ArgumentException)
            {
            }

            try
            {
                baseProxy.Clone(invocationTimeout: -1);
            }
            catch (ArgumentException)
            {
                TestHelper.Assert(false);
            }

            try
            {
                baseProxy.Clone(invocationTimeout: -2);
                TestHelper.Assert(false);
            }
            catch (ArgumentException)
            {
            }

            try
            {
                baseProxy.Clone(locatorCacheTimeout: TimeSpan.Zero);
            }
            catch (ArgumentException)
            {
                TestHelper.Assert(false);
            }

            try
            {
                baseProxy.Clone(locatorCacheTimeout: Timeout.InfiniteTimeSpan);
            }
            catch (ArgumentException)
            {
                TestHelper.Assert(false);
            }

            try
            {
                baseProxy.Clone(locatorCacheTimeout: TimeSpan.FromSeconds(-2));
                TestHelper.Assert(false);
            }
            catch (ArgumentException)
            {
            }

            output.WriteLine("ok");

            output.Write("testing proxy comparison... ");
            output.Flush();

            TestHelper.Assert(IObjectPrx.Parse("ice:foo", communicator).Equals(IObjectPrx.Parse("ice:foo", communicator)));
            TestHelper.Assert(!IObjectPrx.Parse("ice:foo", communicator).Equals(IObjectPrx.Parse("ice:foo2", communicator)));

            var compObj = IObjectPrx.Parse("ice:foo", communicator);

            TestHelper.Assert(compObj.Clone(facet: "facet", IObjectPrx.Factory).Equals(
                              compObj.Clone(facet: "facet", IObjectPrx.Factory)));
            TestHelper.Assert(!compObj.Clone(facet: "facet", IObjectPrx.Factory).Equals(
                              compObj.Clone(facet: "facet1", IObjectPrx.Factory)));

            TestHelper.Assert(compObj.Clone(invocationMode: InvocationMode.Oneway).Equals(
                              compObj.Clone(invocationMode: InvocationMode.Oneway)));
            TestHelper.Assert(!compObj.Clone(invocationMode: InvocationMode.Oneway).Equals(
                              compObj.Clone(invocationMode: InvocationMode.Twoway)));

            TestHelper.Assert(compObj.Clone(cacheConnection: true).Equals(compObj.Clone(cacheConnection: true)));
            TestHelper.Assert(!compObj.Clone(cacheConnection: false).Equals(compObj.Clone(cacheConnection: true)));

            TestHelper.Assert(compObj.Clone(endpointSelection: EndpointSelectionType.Random).Equals(
                              compObj.Clone(endpointSelection: EndpointSelectionType.Random)));
            TestHelper.Assert(!compObj.Clone(endpointSelection: EndpointSelectionType.Random).Equals(
                              compObj.Clone(endpointSelection: EndpointSelectionType.Ordered)));

            TestHelper.Assert(compObj.Clone(connectionId: "id2").Equals(compObj.Clone(connectionId: "id2")));
            TestHelper.Assert(!compObj.Clone(connectionId: "id1").Equals(compObj.Clone(connectionId: "id2")));
            TestHelper.Assert(compObj.Clone(connectionId: "id1").ConnectionId.Equals("id1"));
            TestHelper.Assert(compObj.Clone(connectionId: "id2").ConnectionId.Equals("id2"));

            var loc1 = ILocatorPrx.Parse("ice+tcp://host:10000/loc1", communicator);
            var loc2 = ILocatorPrx.Parse("ice+tcp://host:10000/loc2", communicator);
            TestHelper.Assert(compObj.Clone(clearLocator: true).Equals(compObj.Clone(clearLocator: true)));
            TestHelper.Assert(compObj.Clone(locator: loc1).Equals(compObj.Clone(locator: loc1)));
            TestHelper.Assert(!compObj.Clone(locator: loc1).Equals(compObj.Clone(clearLocator: true)));
            TestHelper.Assert(!compObj.Clone(clearLocator: true).Equals(compObj.Clone(locator: loc2)));
            TestHelper.Assert(!compObj.Clone(locator: loc1).Equals(compObj.Clone(locator: loc2)));

            var rtr1 = IRouterPrx.Parse("ice+tcp://host:10000/rtr1", communicator);
            var rtr2 = IRouterPrx.Parse("ice+tcp://host:10000/rtr2", communicator);
            TestHelper.Assert(compObj.Clone(clearRouter: true).Equals(compObj.Clone(clearRouter: true)));
            TestHelper.Assert(compObj.Clone(router: rtr1).Equals(compObj.Clone(router: rtr1)));
            TestHelper.Assert(!compObj.Clone(router: rtr1).Equals(compObj.Clone(clearRouter: true)));
            TestHelper.Assert(!compObj.Clone(clearRouter: true).Equals(compObj.Clone(router: rtr2)));
            TestHelper.Assert(!compObj.Clone(router: rtr1).Equals(compObj.Clone(router: rtr2)));

            var ctx1 = new Dictionary<string, string>
            {
                ["ctx1"] = "v1"
            };
            var ctx2 = new Dictionary<string, string>
            {
                ["ctx2"] = "v2"
            };
            TestHelper.Assert(compObj.Clone(context: new Dictionary<string, string>()).Equals(
                              compObj.Clone(context: new Dictionary<string, string>())));
            TestHelper.Assert(compObj.Clone(context: ctx1).Equals(compObj.Clone(context: ctx1)));
            TestHelper.Assert(!compObj.Clone(context: ctx1).Equals(
                              compObj.Clone(context: new Dictionary<string, string>())));
            TestHelper.Assert(!compObj.Clone(context: new Dictionary<string, string>()).Equals(
                              compObj.Clone(context: ctx2)));
            TestHelper.Assert(!compObj.Clone(context: ctx1).Equals(compObj.Clone(context: ctx2)));

            TestHelper.Assert(compObj.Clone(preferNonSecure: true).Equals(compObj.Clone(preferNonSecure: true)));
            TestHelper.Assert(!compObj.Clone(preferNonSecure: true).Equals(compObj.Clone(preferNonSecure: false)));

            var compObj1 = IObjectPrx.Parse("ice+tcp://127.0.0.1:10000/foo", communicator);
            var compObj2 = IObjectPrx.Parse("ice+tcp://127.0.0.1:10001/foo", communicator);
            TestHelper.Assert(!compObj1.Equals(compObj2));

            compObj1 = IObjectPrx.Parse("ice:MyAdapter1//foo", communicator);
            compObj2 = IObjectPrx.Parse("ice:MyAdapter2//foo", communicator);
            TestHelper.Assert(!compObj1.Equals(compObj2));

            TestHelper.Assert(compObj1.Clone(locatorCacheTimeout: TimeSpan.FromSeconds(20))
                .Equals(compObj1.Clone(locatorCacheTimeout: TimeSpan.FromSeconds(20))));
            TestHelper.Assert(!compObj1.Clone(locatorCacheTimeout: TimeSpan.FromSeconds(10))
                .Equals(compObj1.Clone(locatorCacheTimeout: TimeSpan.FromSeconds(20))));

            TestHelper.Assert(compObj1.Clone(invocationTimeout: 20).Equals(compObj1.Clone(invocationTimeout: 20)));
            TestHelper.Assert(!compObj1.Clone(invocationTimeout: 10).Equals(compObj1.Clone(invocationTimeout: 20)));

            compObj1 = IObjectPrx.Parse("ice+tcp://127.0.0.1:10000/foo", communicator);
            compObj2 = IObjectPrx.Parse("ice:MyAdapter1//foo", communicator);
            TestHelper.Assert(!compObj1.Equals(compObj2));

            IReadOnlyList<Endpoint> endpts1 =
                IObjectPrx.Parse("ice+tcp://127.0.0.1:10000/foo", communicator).Endpoints;
            IReadOnlyList<Endpoint> endpts2 =
                IObjectPrx.Parse("ice+tcp://127.0.0.1:10001/foo", communicator).Endpoints;
            TestHelper.Assert(!endpts1[0].Equals(endpts2[0]));
            TestHelper.Assert(endpts1[0].Equals(
                IObjectPrx.Parse("ice+tcp://127.0.0.1:10000/foo", communicator).Endpoints[0]));

            if (baseProxy.GetConnection() is Connection baseConnection)
            {
                Connection baseConnection2 = baseProxy.Clone(connectionId: "base2").GetConnection()!;
                compObj1 = compObj1.Clone(fixedConnection: baseConnection);
                compObj2 = compObj2.Clone(fixedConnection: baseConnection2);
                TestHelper.Assert(!compObj1.Equals(compObj2));
            }
            output.WriteLine("ok");

            output.Write("testing checked cast... ");
            output.Flush();
            var cl = IMyClassPrx.CheckedCast(baseProxy);
            TestHelper.Assert(cl != null);
            var derived = IMyDerivedClassPrx.CheckedCast(cl);
            TestHelper.Assert(derived != null);
            TestHelper.Assert(cl.Equals(baseProxy));
            TestHelper.Assert(derived.Equals(baseProxy));
            TestHelper.Assert(cl.Equals(derived));
            try
            {
                IMyDerivedClassPrx.CheckedCast(cl.Clone(facet: "facet", IObjectPrx.Factory));
                TestHelper.Assert(false);
            }
            catch (ObjectNotExistException)
            {
            }
            output.WriteLine("ok");

            output.Write("testing checked cast with context... ");
            output.Flush();

            Dictionary<string, string> c = cl.GetContext();
            TestHelper.Assert(c == null || c.Count == 0);

            c = new Dictionary<string, string>
            {
                ["one"] = "hello",
                ["two"] = "world"
            };
            cl = IMyClassPrx.CheckedCast(baseProxy, c);
            Dictionary<string, string> c2 = cl!.GetContext();
            TestHelper.Assert(c.DictionaryEquals(c2));
            output.WriteLine("ok");

            output.Write("testing ice_fixed... ");
            output.Flush();
            {
                if (cl.GetConnection() is Connection connection2)
                {
                    TestHelper.Assert(!cl.IsFixed);
                    IMyClassPrx prx = cl.Clone(fixedConnection: connection2);
                    TestHelper.Assert(prx.IsFixed);
                    prx.IcePing();
                    TestHelper.Assert(cl.Clone("facet", IObjectPrx.Factory, fixedConnection: connection2).Facet == "facet");
                    TestHelper.Assert(cl.Clone(invocationMode: InvocationMode.Oneway, fixedConnection: connection2).IsOneway);
                    var ctx = new Dictionary<string, string>
                    {
                        ["one"] = "hello",
                        ["two"] = "world"
                    };
                    TestHelper.Assert(cl.Clone(fixedConnection: connection2).Context.Count == 0);
                    TestHelper.Assert(cl.Clone(context: ctx, fixedConnection: connection2).Context.Count == 2);
                    TestHelper.Assert(cl.Clone(fixedConnection: connection2).InvocationTimeout == -1);
                    TestHelper.Assert(cl.Clone(invocationTimeout: 10, fixedConnection: connection2).InvocationTimeout == 10);
                    TestHelper.Assert(cl.Clone(fixedConnection: connection2).GetConnection() == connection2);
                    TestHelper.Assert(cl.Clone(fixedConnection: connection2).Clone(fixedConnection: connection2).GetConnection() == connection2);
                    Connection? fixedConnection = cl.Clone(connectionId: "ice_fixed").GetConnection();
                    TestHelper.Assert(cl.Clone(fixedConnection: connection2).Clone(fixedConnection: fixedConnection).GetConnection() == fixedConnection);
                    try
                    {
                        cl.Clone(invocationMode: InvocationMode.Datagram, fixedConnection: connection2);
                        TestHelper.Assert(false);
                    }
                    catch (ArgumentException)
                    {
                    }
                }
            }
            output.WriteLine("ok");

            output.Write("testing encoding versioning... ");
            string ref13 = helper.GetTestProxy("test", 0);
            IMyClassPrx cl13 = IMyClassPrx.Parse(ref13, communicator).Clone(encoding: new Encoding(1, 3));
            try
            {
                cl13.IcePing();
                TestHelper.Assert(false);
            }
            catch (NotSupportedException)
            {
                // expected
            }
            output.WriteLine("ok");

            if (communicator.DefaultProtocol == Protocol.Ice2)
            {
                output.Write("testing protocol versioning... ");
                output.Flush();
                string ref3 = helper.GetTestProxy("test", 0);
                ref3 += "?protocol=3";

                string transport = helper.GetTestTransport();
                ref3 = ref3.Replace($"ice+{transport}", "ice+universal");
                ref3 += $"&transport={transport}";
                var cl3 = IMyClassPrx.Parse(ref3, communicator);
                try
                {
                    cl3.IcePing();
                    TestHelper.Assert(false);
                }
                catch (NotSupportedException)
                {
                    // expected
                }
                output.WriteLine("ok");
            }

            output.Write("testing ice2 universal endpoints... ");
            output.Flush();

            var p1 = IObjectPrx.Parse("ice+universal://127.0.0.1:4062/test?transport=tcp", communicator);
            TestHelper.Assert(p1.ToString() == "ice+tcp://127.0.0.1/test"); // uses default port

            p1 = IObjectPrx.Parse(
                "ice+universal://127.0.0.1:4062/test?transport=tcp&alt-endpoint=host2:10000?transport=tcp",
                communicator);
            TestHelper.Assert(p1.ToString() == "ice+tcp://127.0.0.1/test?alt-endpoint=host2:10000");

            p1 = IObjectPrx.Parse(
                "ice+universal://127.0.0.1:4062/test?transport=tcp&alt-endpoint=host2:10000?transport=99$option=a",
                communicator);

            TestHelper.Assert(p1.ToString() ==
                "ice+tcp://127.0.0.1/test?alt-endpoint=ice+universal://host2:10000?transport=99$option=a");

            output.WriteLine("ok");

            output.Write("testing ice1 opaque endpoints... ");
            output.Flush();

            // Legal TCP endpoint expressed as opaque endpoint
            p1 = IObjectPrx.Parse("test:opaque -e 1.1 -t 1 -v CTEyNy4wLjAuMeouAAAQJwAAAA==",
                                      communicator);
            TestHelper.Assert(p1.ToString() == "test -t -e 1.1:tcp -h 127.0.0.1 -p 12010 -t 10000");

            // Two legal TCP endpoints expressed as opaque endpoints
            p1 = IObjectPrx.Parse(@"test:opaque -e 1.1 -t 1 -v CTEyNy4wLjAuMeouAAAQJwAAAA==:
                opaque -e 1.1 -t 1 -v CTEyNy4wLjAuMusuAAAQJwAAAA==", communicator);
            TestHelper.Assert(p1.ToString() ==
                "test -t -e 1.1:tcp -h 127.0.0.1 -p 12010 -t 10000:tcp -h 127.0.0.2 -p 12011 -t 10000");

            // Test that an SSL endpoint and an unknown-transport endpoint get written back out as an opaque
            // endpoint.
            p1 = IObjectPrx.Parse(
                "test:opaque -e 1.1 -t 2 -v CTEyNy4wLjAuMREnAAD/////AA==:opaque -e 1.1 -t 99 -v abch",
                communicator);
            TestHelper.Assert(p1.ToString() ==
                "test -t -e 1.1:ssl -h 127.0.0.1 -p 10001 -t -1:opaque -t 99 -e 1.1 -v abch");

            output.WriteLine("ok");

            output.Write("testing communicator shutdown/destroy... ");
            output.Flush();
            {
                var com = new Communicator();
                com.ShutdownAsync();
                com.WaitForShutdownAsync();
                com.Dispose();
                com.ShutdownAsync();
                com.WaitForShutdownAsync();
                com.Dispose();
            }
            output.WriteLine("ok");

            return cl;
        }
    }
}
