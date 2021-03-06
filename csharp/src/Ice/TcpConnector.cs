//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Net;

namespace ZeroC.Ice
{
    internal sealed class TcpConnector : IConnector
    {
        public ITransceiver Connect() =>
            _endpoint.CreateTransceiver(new StreamSocket(_endpoint.Communicator, _proxy, _addr, _endpoint.SourceAddress),
                                        null);

        internal TcpConnector(TcpEndpoint endpoint, EndPoint addr, INetworkProxy? proxy)
        {
            _endpoint = endpoint;
            _addr = addr;
            _proxy = proxy;

            var hash = new System.HashCode();
            hash.Add(_endpoint.Protocol);
            hash.Add(_endpoint.Transport);
            hash.Add(_addr);
            if (_endpoint.SourceAddress != null)
            {
                hash.Add(_endpoint.SourceAddress);
            }
            _hashCode = hash.ToHashCode();
        }

        public override bool Equals(object? obj)
        {
            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj is TcpConnector tcpConnector)
            {
                if (_endpoint.Protocol != tcpConnector._endpoint.Protocol)
                {
                    return false;
                }

                if (_endpoint.Transport != tcpConnector._endpoint.Transport)
                {
                    return false;
                }

                if (!Equals(_endpoint.SourceAddress, tcpConnector._endpoint.SourceAddress))
                {
                    return false;
                }

                return _addr.Equals(tcpConnector._addr);
            }
            else
            {
                return false;
            }
        }

        public override string ToString() => Network.AddrToString(_proxy == null ? _addr : _proxy.GetAddress());

        public override int GetHashCode() => _hashCode;

        private readonly TcpEndpoint _endpoint;
        private readonly EndPoint _addr;
        private readonly INetworkProxy? _proxy;
        private readonly int _hashCode;
    }
}
