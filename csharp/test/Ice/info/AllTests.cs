//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Authentication;
using Test;

namespace ZeroC.Ice.Test.Info
{
    public class AllTests
    {
        public static void Run(TestHelper helper)
        {
            Communicator? communicator = helper.Communicator();
            TestHelper.Assert(communicator != null);
            bool ice1 = communicator.DefaultProtocol == Protocol.Ice1;
            string transport = communicator.DefaultTransport;
            TextWriter output = helper.GetWriter();
            output.Write("testing proxy endpoint information... ");
            output.Flush();
            {
                string ice1Prx = @"test -t:tcp -h tcphost -p 10000 -t 1200 -z
                    --sourceAddress 10.10.10.10: udp -h udphost -p 10001 --interface eth0 --ttl 5
                    --sourceAddress 10.10.10.10:opaque -e 1.8 -t 100 -v ABCD";

                string ice2Prx = "ice+tcp://tcphost:10000/test?source-address=10.10.10.10" +
                    "&alt-endpoint=ice+universal://unihost:10000?transport=100$option=ABCD";

                var p1 = IObjectPrx.Parse(ice1 ? ice1Prx : ice2Prx, communicator);

                IReadOnlyList<Endpoint> endps = p1.Endpoints;

                Endpoint tcpEndpoint = endps[0];
                TestHelper.Assert(tcpEndpoint.Transport == Transport.TCP && !tcpEndpoint.IsSecure);
                TestHelper.Assert(tcpEndpoint.Host == "tcphost");
                TestHelper.Assert(tcpEndpoint.Port == 10000);
                TestHelper.Assert(tcpEndpoint["source-address"] == "10.10.10.10");
                if (ice1)
                {
                    TestHelper.Assert(tcpEndpoint["timeout"] == "1200");
                    TestHelper.Assert(tcpEndpoint["compress"] == "true");
                }
                TestHelper.Assert(!tcpEndpoint.IsDatagram);

                if (ice1)
                {
                    Endpoint udpEndpoint = endps[1];
                    TestHelper.Assert(udpEndpoint.Host == "udphost");
                    TestHelper.Assert(udpEndpoint.Port == 10001);
                    TestHelper.Assert(udpEndpoint["interface"] == "eth0");
                    TestHelper.Assert(udpEndpoint["ttl"] == "5");
                    TestHelper.Assert(udpEndpoint["source-address"] == "10.10.10.10");
                    TestHelper.Assert(udpEndpoint["timeout"] == null);
                    TestHelper.Assert(udpEndpoint["compress"] == null);
                    TestHelper.Assert(!udpEndpoint.IsSecure);
                    TestHelper.Assert(udpEndpoint.IsDatagram);
                    TestHelper.Assert(udpEndpoint.Transport == Transport.UDP);

                    Endpoint opaqueEndpoint = endps[2];
                    TestHelper.Assert(opaqueEndpoint["value"] == "ABCD");
                    TestHelper.Assert(opaqueEndpoint["value-encoding"] == "1.8");
                }
                else
                {
                    Endpoint universalEndpoint = endps[1];
                    TestHelper.Assert(universalEndpoint.Transport == (Transport)100);
                    TestHelper.Assert(universalEndpoint["option"] == "ABCD");
                }
            }
            output.WriteLine("ok");

            ObjectAdapter adapter;
            output.Write("test object adapter endpoint information... ");
            output.Flush();
            {
                string host = (communicator.GetPropertyAsBool("Ice.IPv6") ?? false) ? "::1" : "127.0.0.1";
                communicator.SetProperty("TestAdapter.Endpoints", "tcp -h \"" + host +
                    "\" -t 15000:udp -h \"" + host + "\"");
                adapter = communicator.CreateObjectAdapter("TestAdapter");

                IReadOnlyList<Endpoint> endpoints = adapter.GetEndpoints();
                TestHelper.Assert(endpoints.Count == 2);
                IReadOnlyList<Endpoint> publishedEndpoints = adapter.GetPublishedEndpoints();
                TestHelper.Assert(endpoints.SequenceEqual(publishedEndpoints));

                Endpoint tcpEndpoint = endpoints[0];
                TestHelper.Assert(tcpEndpoint != null);
                TestHelper.Assert(tcpEndpoint.Transport == Transport.TCP ||
                                  tcpEndpoint.Transport == Transport.SSL ||
                                  tcpEndpoint.Transport == Transport.WS ||
                                  tcpEndpoint.Transport == Transport.WSS);

                TestHelper.Assert(tcpEndpoint.Host == host);
                TestHelper.Assert(tcpEndpoint.Port > 0);
                TestHelper.Assert(tcpEndpoint["timeout"] is string value && int.Parse(value) == 15000);

                Endpoint udpEndpoint = endpoints[1];
                TestHelper.Assert(udpEndpoint.Host == host);
                TestHelper.Assert(udpEndpoint.IsDatagram);
                TestHelper.Assert(udpEndpoint.Port > 0);

                endpoints = new List<Endpoint> { endpoints[0] };

                adapter.SetPublishedEndpoints(endpoints);
                publishedEndpoints = adapter.GetPublishedEndpoints();
                TestHelper.Assert(endpoints.SequenceEqual(publishedEndpoints));

                adapter.Dispose();

                int port = helper.GetTestPort(1);
                communicator.SetProperty("TestAdapter.Endpoints",
                    ice1 ? $"default -h 0.0.0.0 -p {port}" : $"ice+{transport}://0.0.0.0:{port}");
                communicator.SetProperty("TestAdapter.PublishedEndpoints", helper.GetTestEndpoint(1));
                adapter = communicator.CreateObjectAdapter("TestAdapter");

                endpoints = adapter.GetEndpoints();
                TestHelper.Assert(endpoints.Count >= 1);
                publishedEndpoints = adapter.GetPublishedEndpoints();
                TestHelper.Assert(publishedEndpoints.Count == 1);

                foreach (Endpoint endpoint in endpoints)
                {
                    TestHelper.Assert(endpoint.Port == port);
                }

                tcpEndpoint = publishedEndpoints[0];
                TestHelper.Assert(tcpEndpoint.Host == "127.0.0.1");
                TestHelper.Assert(tcpEndpoint.Port == port);

                adapter.Dispose();
            }
            output.WriteLine("ok");

            int endpointPort = helper.GetTestPort(0);

            ITestIntfPrx testIntf;
            if (ice1)
            {
                testIntf = ITestIntfPrx.Parse("test:" + helper.GetTestEndpoint(0) + ":" +
                                              helper.GetTestEndpoint(0, "udp"), communicator);
            }
            else
            {
                testIntf = ITestIntfPrx.Parse(helper.GetTestProxy("test", 0), communicator);
            }

            string defaultHost = communicator.GetProperty("Ice.Default.Host") ?? "";

            output.Write("test connection endpoint information... ");
            output.Flush();
            {
                Endpoint tcpEndpoint = testIntf.GetConnection()!.Endpoint;
                TestHelper.Assert(tcpEndpoint.Port == endpointPort);

                TestHelper.Assert(tcpEndpoint["compress"] == null);
                TestHelper.Assert(tcpEndpoint.Host == defaultHost);

                Dictionary<string, string> ctx = testIntf.GetEndpointInfoAsContext();
                TestHelper.Assert(ctx["host"] == tcpEndpoint.Host);
                TestHelper.Assert(ctx["compress"] == "false");
                int port = int.Parse(ctx["port"]);
                TestHelper.Assert(port > 0);

                if (ice1)
                {
                    Endpoint udpEndpoint =
                        testIntf.Clone(invocationMode: InvocationMode.Datagram).GetConnection()!.Endpoint;
                    TestHelper.Assert(udpEndpoint.Port == endpointPort);
                    TestHelper.Assert(udpEndpoint.Host == defaultHost);
                }
            }
            output.WriteLine("ok");

            output.Write("testing connection information... ");
            output.Flush();
            {
                var connection = (IPConnection)testIntf.GetConnection()!;

                TestHelper.Assert(!connection.IsIncoming);
                TestHelper.Assert(connection.Adapter == null);
                TestHelper.Assert(connection.RemoteEndpoint!.Port == endpointPort);
                TestHelper.Assert(connection.LocalEndpoint!.Port > 0);
                if (defaultHost.Equals("127.0.0.1"))
                {
                    TestHelper.Assert(connection.LocalEndpoint!.Address.ToString() == defaultHost);
                    TestHelper.Assert(connection.RemoteEndpoint!.Address.ToString() == defaultHost);
                }

                if (connection.Endpoint.IsSecure)
                {
                    TestHelper.Assert(((TcpConnection)connection).IsEncrypted);
                    // WSS tests run with IceSSL.VerifyPeer set to 0 so the connection is no mutually
                    // authenticated for compatibility with web browser testing.
                    if (connection.Endpoint.Transport == Transport.SSL)
                    {
                        TestHelper.Assert(((TcpConnection)connection).IsMutuallyAuthenticated);
                        TestHelper.Assert(((TcpConnection)connection).LocalCertificate != null);
                    }
                    else
                    {
                        TestHelper.Assert(!((TcpConnection)connection).IsMutuallyAuthenticated);
                        TestHelper.Assert(((TcpConnection)connection).LocalCertificate == null);
                    }
                    TestHelper.Assert(((TcpConnection)connection).IsSigned);

                    TestHelper.Assert(((TcpConnection)connection).RemoteCertificate != null);
                    TestHelper.Assert(((TcpConnection)connection).NegotiatedApplicationProtocol != null);
                    TestHelper.Assert(((TcpConnection)connection).NegotiatedCipherSuite != null);
                    TestHelper.Assert(((TcpConnection)connection).SslProtocol == SslProtocols.Tls12 ||
                                      ((TcpConnection)connection).SslProtocol == SslProtocols.Tls13);
                }
                else
                {
                    TestHelper.Assert(!((TcpConnection)connection).IsEncrypted);
                    TestHelper.Assert(!((TcpConnection)connection).IsMutuallyAuthenticated);
                    TestHelper.Assert(!((TcpConnection)connection).IsSigned);
                    TestHelper.Assert(((TcpConnection)connection).LocalCertificate == null);
                    TestHelper.Assert(((TcpConnection)connection).RemoteCertificate == null);
                    TestHelper.Assert(((TcpConnection)connection).NegotiatedApplicationProtocol == null);
                    TestHelper.Assert(((TcpConnection)connection).NegotiatedCipherSuite == null);
                    TestHelper.Assert(((TcpConnection)connection).SslProtocol == null);
                }

                Dictionary<string, string> ctx = testIntf.GetConnectionInfoAsContext();
                TestHelper.Assert(ctx["incoming"].Equals("true"));
                TestHelper.Assert(ctx["adapterName"].Equals("TestAdapter"));
                TestHelper.Assert(ctx["remoteAddress"].Equals(connection.LocalEndpoint!.Address.ToString()));
                TestHelper.Assert(ctx["localAddress"].Equals(connection.RemoteEndpoint!.Address.ToString()));
                TestHelper.Assert(ctx["remotePort"].Equals(connection.LocalEndpoint!.Port.ToString()));
                TestHelper.Assert(ctx["localPort"].Equals(connection.RemoteEndpoint!.Port.ToString()));

                if ((connection as WSConnection)?.Headers is IReadOnlyDictionary<string, string> headers)
                {
                    TestHelper.Assert(headers["Upgrade"].Equals("websocket"));
                    TestHelper.Assert(headers["Connection"].Equals("Upgrade"));
                    TestHelper.Assert(headers["Sec-WebSocket-Protocol"].Equals("ice.zeroc.com"));
                    TestHelper.Assert(headers["Sec-WebSocket-Accept"] != null);

                    TestHelper.Assert(ctx["ws.Upgrade"].Equals("websocket"));
                    TestHelper.Assert(ctx["ws.Connection"].Equals("Upgrade"));
                    TestHelper.Assert(ctx["ws.Sec-WebSocket-Protocol"].Equals("ice.zeroc.com"));
                    TestHelper.Assert(ctx["ws.Sec-WebSocket-Version"].Equals("13"));
                    TestHelper.Assert(ctx["ws.Sec-WebSocket-Key"] != null);
                }

                if (ice1)
                {
                    connection = (IPConnection)testIntf.Clone(invocationMode: InvocationMode.Datagram).GetConnection()!;

                    var udpConnection = connection as UdpConnection;
                    TestHelper.Assert(udpConnection != null);
                    TestHelper.Assert(!udpConnection.IsIncoming);
                    TestHelper.Assert(udpConnection.Adapter == null);
                    TestHelper.Assert(udpConnection.LocalEndpoint?.Port > 0);
                    TestHelper.Assert(udpConnection.RemoteEndpoint?.Port == endpointPort);

                    if (defaultHost == "127.0.0.1")
                    {
                        TestHelper.Assert(udpConnection.RemoteEndpoint.Address.ToString().Equals(defaultHost));
                        TestHelper.Assert(udpConnection.LocalEndpoint.Address.ToString().Equals(defaultHost));
                    }
                }
            }
            output.WriteLine("ok");

            testIntf.Shutdown();

            communicator.ShutdownAsync();
            communicator.WaitForShutdownAsync();
        }
    }
}
