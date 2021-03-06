//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Net.Sockets;
using System.Text;

namespace ZeroC.Ice
{
    internal sealed class UdpTransceiver : ITransceiver
    {
        internal System.Net.IPEndPoint? McastAddress { get; private set; }

        public Socket? Fd() => _fd;

        public int Initialize(ref ArraySegment<byte> readBuffer, IList<ArraySegment<byte>> writeBuffer)
        {
            Debug.Assert(_fd != null);
            if (_state == StateNeedConnect)
            {
                _state = StateConnectPending;
                try
                {
                    if (_sourceAddr != null)
                    {
                        _fd.Bind(_sourceAddr);
                    }
                    _fd.Connect(_addr);
                }
                catch (SocketException ex)
                {
                    if (Network.WouldBlock(ex))
                    {
                        return SocketOperation.Connect;
                    }
                    throw new ConnectFailedException(ex);
                }
                catch (Exception ex)
                {
                    throw new ConnectFailedException(ex);
                }
                _state = StateConnected;
            }

            Debug.Assert(_state >= StateConnected);
            return SocketOperation.None;
        }

        // Nothing to do.
        public int Closing(bool initiator, Exception? ex) => SocketOperation.None;

        public void Close()
        {
            if (_fd != null)
            {
                try
                {
                    _fd.Close();
                }
                catch (System.IO.IOException)
                {
                }
            }
        }

        public Endpoint Bind()
        {
            Debug.Assert(_fd != null);
            if (Network.IsMulticast((System.Net.IPEndPoint)_addr))
            {
                Network.SetReuseAddress(_fd, true);
                McastAddress = (System.Net.IPEndPoint)_addr;
                if (AssemblyUtil.IsWindows)
                {
                    //
                    // Windows does not allow binding to the mcast address itself
                    // so we bind to INADDR_ANY (0.0.0.0) instead. As a result,
                    // bi-directional connection won't work because the source
                    // address won't the multicast address and the client will
                    // therefore reject the datagram.
                    //
                    if (_addr.AddressFamily == AddressFamily.InterNetwork)
                    {
                        _addr = new System.Net.IPEndPoint(System.Net.IPAddress.Any, _port);
                    }
                    else
                    {
                        _addr = new System.Net.IPEndPoint(System.Net.IPAddress.IPv6Any, _port);
                    }
                }

                _addr = Network.DoBind(_fd, _addr);
                if (_port == 0)
                {
                    McastAddress.Port = ((System.Net.IPEndPoint)_addr).Port;
                }
                Debug.Assert(_mcastInterface != null);
                Network.SetMcastGroup(_fd, McastAddress.Address, _mcastInterface);
            }
            else
            {
                _addr = Network.DoBind(_fd, _addr);
            }
            _bound = true;
            Debug.Assert(_endpoint != null);
            _endpoint = (UdpEndpoint)_endpoint.NewPort(EffectivePort());
            return _endpoint;
        }

        public void Destroy()
        {
            _readEventArgs.Dispose();
            Debug.Assert(_writeEventArgs != null);
            _writeEventArgs.Dispose();
        }

        public int Write(IList<ArraySegment<byte>> buffer, ref int offset)
        {
            int count = buffer.GetByteCount();
            int remaining = count - offset;
            if (remaining == 0)
            {
                return SocketOperation.None;
            }

            Debug.Assert(_fd != null && _state >= StateConnected);

            // The caller is supposed to check the send size before by calling checkSendSize
            Debug.Assert(Math.Min(MaxPacketSize, _sndSize - UdpOverhead) >= count);

            int ret;
            while (true)
            {
                try
                {
                    if (_state == StateConnected)
                    {
                        ret = _fd.Send(buffer, SocketFlags.None);
                    }
                    else
                    {
                        if (_peerAddr == null)
                        {
                            throw new TransportException("cannot send datagram to undefined peer");
                        }

                        ArraySegment<byte> data = buffer.GetSegment(0, count);
                        ret = _fd.SendTo(data.Array, 0, data.Count, SocketFlags.None, _peerAddr);
                    }
                    Debug.Assert(ret == count);
                    offset += ret;
                    break;
                }
                catch (SocketException ex)
                {
                    if (Network.Interrupted(ex))
                    {
                        continue;
                    }

                    if (Network.WouldBlock(ex))
                    {
                        return SocketOperation.Write;
                    }

                    if (Network.ConnectionLost(ex))
                    {
                        throw new ConnectionLostException(ex);
                    }
                    else
                    {
                        throw new TransportException(ex);
                    }
                }
            }
            return SocketOperation.None;
        }

        public int Read(ref ArraySegment<byte> buffer, ref int offset)
        {
            if (buffer.Count - offset == 0)
            {
                return SocketOperation.None;
            }

            Debug.Assert(offset == 0);
            Debug.Assert(_fd != null);

            int packetSize = Math.Min(MaxPacketSize, _rcvSize - UdpOverhead);
            Debug.Assert(buffer.Count == 0);
            buffer = new byte[packetSize];

            int ret;
            while (true)
            {
                try
                {
                    System.Net.EndPoint? peerAddr = _peerAddr;
                    if (peerAddr == null)
                    {
                        if (_addr.AddressFamily == AddressFamily.InterNetwork)
                        {
                            peerAddr = new System.Net.IPEndPoint(System.Net.IPAddress.Any, 0);
                        }
                        else
                        {
                            Debug.Assert(_addr.AddressFamily == AddressFamily.InterNetworkV6);
                            peerAddr = new System.Net.IPEndPoint(System.Net.IPAddress.IPv6Any, 0);
                        }
                    }

                    // TODO: Workaround for https://github.com/dotnet/corefx/issues/31182
                    if (_state == StateConnected ||
                       (AssemblyUtil.IsMacOS && _fd.AddressFamily == AddressFamily.InterNetworkV6 && _fd.DualMode))
                    {
                        ret = _fd.Receive(buffer.Array, 0, packetSize, SocketFlags.None);
                    }
                    else
                    {
                        ret = _fd.ReceiveFrom(buffer.Array, 0, packetSize, SocketFlags.None, ref peerAddr);
                        _peerAddr = (System.Net.IPEndPoint)peerAddr;
                    }
                    break;
                }
                catch (SocketException e)
                {
                    if (Network.RecvTruncated(e))
                    {
                        // The frame was truncated and the whole buffer is filled. We ignore
                        // this error here, it will be detected at the connection level when
                        // the Ice frame size is checked against the buffer size.
                        ret = buffer.Count;
                        break;
                    }

                    if (Network.Interrupted(e))
                    {
                        continue;
                    }

                    if (Network.WouldBlock(e))
                    {
                        return SocketOperation.Read;
                    }

                    if (Network.ConnectionLost(e))
                    {
                        throw new ConnectionLostException();
                    }
                    else
                    {
                        throw new TransportException(e);
                    }
                }
            }

            if (ret == 0)
            {
                throw new ConnectionLostException();
            }

            if (_state == StateNeedConnect)
            {
                Debug.Assert(_incoming);

                //
                // If we must connect, then we connect to the first peer that sends us a packet.
                //
                Debug.Assert(_peerAddr != null);
                bool connected = Network.DoConnect(_fd, _peerAddr, null);
                Debug.Assert(connected);
                _state = StateConnected; // We're connected now

                if (_communicator.TraceLevels.Network >= 1)
                {
                    _communicator.Logger.Trace(_communicator.TraceLevels.NetworkCategory,
                        $"connected udp socket\n{this}");
                }
            }

            buffer = buffer.Slice(0, ret);
            offset = ret;
            return SocketOperation.None;
        }

        public bool StartRead(ref ArraySegment<byte> buffer, ref int offset, AsyncCallback callback, object state)
        {
            Debug.Assert(_fd != null);
            Debug.Assert(offset == 0, $"offset: {offset}\n{Environment.StackTrace}");

            int packetSize = Math.Min(MaxPacketSize, _rcvSize - UdpOverhead);
            Debug.Assert(buffer.Count == 0);
            buffer = new byte[packetSize];

            try
            {
                // TODO: Workaround for https://github.com/dotnet/corefx/issues/31182
                _readCallback = callback;
                _readEventArgs.UserToken = state;
                _readEventArgs.SetBuffer(buffer.Array, 0, packetSize);
                if (_state == StateConnected ||
                   (AssemblyUtil.IsMacOS && _fd.AddressFamily == AddressFamily.InterNetworkV6 && _fd.DualMode))
                {
                    return !_fd.ReceiveAsync(_readEventArgs);
                }
                else
                {
                    Debug.Assert(_incoming);

                    return !_fd.ReceiveFromAsync(_readEventArgs);
                }
            }
            catch (SocketException ex)
            {
                if (Network.RecvTruncated(ex))
                {
                    // Nothing to do
                    return true;
                }
                else
                {
                    if (Network.ConnectionLost(ex))
                    {
                        throw new ConnectionLostException(ex);
                    }
                    else
                    {
                        throw new TransportException(ex);
                    }
                }
            }
        }

        public void FinishRead(ref ArraySegment<byte> buffer, ref int offset)
        {
            if (_fd == null)
            {
                return;
            }

            int ret;
            try
            {
                if (_readEventArgs.SocketError != SocketError.Success)
                {
                    throw new SocketException((int)_readEventArgs.SocketError);
                }
                ret = _readEventArgs.BytesTransferred;
                // TODO: Workaround for https://github.com/dotnet/corefx/issues/31182
                if (_state != StateConnected &&
                   !(AssemblyUtil.IsMacOS && _fd.AddressFamily == AddressFamily.InterNetworkV6 && _fd.DualMode))
                {
                    _peerAddr = _readEventArgs.RemoteEndPoint;
                }
            }
            catch (SocketException ex)
            {
                if (Network.RecvTruncated(ex))
                {
                    // The frame was truncated and the whole buffer is filled. We ignore
                    // this error here, it will be detected at the connection level when
                    // the Ice frame size is checked against the buffer size.
                    ret = buffer.Count;
                }
                else
                {
                    if (Network.ConnectionLost(ex))
                    {
                        throw new ConnectionLostException(ex);
                    }
                    else if (Network.ConnectionRefused(ex))
                    {
                        throw new ConnectionRefusedException(ex);
                    }
                    else
                    {
                        throw new TransportException(ex);
                    }
                }
            }

            if (ret == 0)
            {
                throw new ConnectionLostException();
            }

            Debug.Assert(ret > 0);

            if (_state == StateNeedConnect)
            {
                Debug.Assert(_incoming);

                //
                // If we must connect, then we connect to the first peer that
                // sends us a packet.
                //
                bool connected = !_fd.ConnectAsync(_readEventArgs);
                Debug.Assert(connected);
                _state = StateConnected; // We're connected now

                if (_communicator.TraceLevels.Network >= 1)
                {
                    _communicator.Logger.Trace(_communicator.TraceLevels.NetworkCategory,
                        $"connected udp socket\n{this}");
                }
            }

            buffer = buffer.Slice(0, ret);
            offset = ret;
        }

        public bool
        StartWrite(IList<ArraySegment<byte>> buffer, int offset, AsyncCallback callback, object state, out bool completed)
        {
            Debug.Assert(_fd != null);
            Debug.Assert(_writeEventArgs != null);
            Debug.Assert(offset == 0);
            bool completedSynchronously;
            if (!_incoming && _state < StateConnected)
            {
                Debug.Assert(_addr != null);
                completed = false;
                if (_sourceAddr != null)
                {
                    _fd.Bind(_sourceAddr);
                }
                _writeEventArgs.UserToken = state;
                return !_fd.ConnectAsync(_writeEventArgs);
            }

            // The caller is supposed to check the send size before by calling checkSendSize
            Debug.Assert(Math.Min(MaxPacketSize, _sndSize - UdpOverhead) >= buffer.GetByteCount());

            try
            {
                _writeCallback = callback;

                if (_state == StateConnected)
                {
                    _writeEventArgs.UserToken = state;
                    _writeEventArgs.BufferList = buffer;
                    completedSynchronously = !_fd.SendAsync(_writeEventArgs);
                }
                else
                {
                    if (_peerAddr == null)
                    {
                        throw new TransportException("cannot send datagram to undefined peer");
                    }
                    _writeEventArgs.RemoteEndPoint = _peerAddr;
                    _writeEventArgs.UserToken = state;
                    ArraySegment<byte> data = buffer.GetSegment(0, buffer.GetByteCount());
                    _writeEventArgs.SetBuffer(data.Array, 0, data.Count);

                    completedSynchronously = !_fd.SendToAsync(_writeEventArgs);
                }
            }
            catch (SocketException ex)
            {
                if (Network.ConnectionLost(ex))
                {
                    throw new ConnectionLostException(ex);
                }
                else
                {
                    throw new TransportException(ex);
                }
            }
            completed = true;
            return completedSynchronously;
        }

        public void FinishWrite(IList<ArraySegment<byte>> buffer, ref int offset)
        {
            Debug.Assert(_writeEventArgs != null);
            Debug.Assert(offset == 0);
            if (_fd == null)
            {
                int count = buffer.GetByteCount(); // Assume all the data was sent for at-most-once semantics.
                _writeEventArgs = null;
                offset = count;
                return;
            }

            if (!_incoming && _state < StateConnected)
            {
                if (_writeEventArgs.SocketError != SocketError.Success)
                {
                    var ex = new SocketException((int)_writeEventArgs.SocketError);
                    if (Network.ConnectionRefused(ex))
                    {
                        throw new ConnectionRefusedException(ex);
                    }
                    else
                    {
                        throw new ConnectFailedException(ex);
                    }
                }
                return;
            }

            int ret;
            try
            {
                if (_writeEventArgs.SocketError != SocketError.Success)
                {
                    throw new SocketException((int)_writeEventArgs.SocketError);
                }
                ret = _writeEventArgs.BytesTransferred;
                _writeEventArgs.SetBuffer(null, 0, 0);
                if (_writeEventArgs.BufferList != null && _writeEventArgs.BufferList != buffer)
                {
                    _writeEventArgs.BufferList.Clear();
                }
                _writeEventArgs.BufferList = null;
            }
            catch (SocketException ex)
            {
                if (Network.ConnectionLost(ex))
                {
                    throw new ConnectionLostException(ex);
                }
                else
                {
                    throw new TransportException(ex);
                }
            }

            if (ret == 0)
            {
                throw new ConnectionLostException();
            }

            Debug.Assert(ret > 0);
            Debug.Assert(ret == buffer.GetByteCount());
            offset = ret;
            return;
        }

        public void CheckSendSize(int size)
        {
            // The maximum packetSize is either the maximum allowable UDP packet size, or the UDP send buffer size
            // (which ever is smaller).
            int packetSize = Math.Min(MaxPacketSize, _sndSize - UdpOverhead);
            if (packetSize < size)
            {
                throw new DatagramLimitException($"cannot send more than {packetSize} bytes with UDP");
            }
        }

        public void SetBufferSize(int rcvSize, int sndSize) => SetBufSize(rcvSize, sndSize);

        public override string ToString()
        {
            try
            {
                string s;
                if (_incoming && !_bound)
                {
                    s = "local address = " + Network.AddrToString(_addr);
                }
                else if (_state == StateNotConnected)
                {
                    s = "local address = " + Network.LocalAddrToString(Network.GetLocalAddress(_fd));
                    if (_peerAddr != null)
                    {
                        s += "\nremote address = " + Network.AddrToString(_peerAddr);
                    }
                }
                else
                {
                    s = Network.FdToString(_fd);
                }

                if (McastAddress != null)
                {
                    s += "\nmulticast address = " + Network.AddrToString(McastAddress);
                }
                return s;
            }
            catch (ObjectDisposedException)
            {
                return "<closed>";
            }
        }

        public string ToDetailedString()
        {
            var s = new StringBuilder(ToString());
            List<string> intfs;
            if (McastAddress == null)
            {
                intfs = Network.GetHostsForEndpointExpand(Network.EndpointAddressToString(_addr),
                                                          _communicator.IPVersion, true);
            }
            else
            {
                Debug.Assert(_mcastInterface != null);
                intfs = Network.GetInterfacesForMulticast(_mcastInterface,
                                                          Network.GetIPVersion(McastAddress.Address));
            }

            if (intfs.Count != 0)
            {
                s.Append("\nlocal interfaces = ");
                s.Append(string.Join(", ", intfs));
            }
            return s.ToString();
        }

        public ushort EffectivePort() => Network.EndpointPort(_addr);

        //
        // Only for use by UdpConnector.
        //
        internal UdpTransceiver(
            Communicator communicator,
            System.Net.EndPoint addr,
            System.Net.IPAddress? sourceAddr,
            string mcastInterface,
            int mcastTtl)
        {
            _communicator = communicator;
            _addr = addr;
            if (sourceAddr != null)
            {
                _sourceAddr = new System.Net.IPEndPoint(sourceAddr, 0);
            }

            _readEventArgs = new SocketAsyncEventArgs();
            _readEventArgs.RemoteEndPoint = _addr;
            _readEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(IoCompleted);

            _writeEventArgs = new SocketAsyncEventArgs();
            _writeEventArgs.RemoteEndPoint = _addr;
            _writeEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(IoCompleted);

            _mcastInterface = mcastInterface;
            _state = StateNeedConnect;
            _incoming = false;

            _fd = Network.CreateSocket(true, _addr.AddressFamily);
            SetBufSize(-1, -1);
            Network.SetBlock(_fd, false);
            if (Network.IsMulticast((System.Net.IPEndPoint)_addr))
            {
                if (_mcastInterface.Length > 0)
                {
                    Network.SetMcastInterface(_fd, _mcastInterface, _addr.AddressFamily);
                }
                if (mcastTtl != -1)
                {
                    Network.SetMcastTtl(_fd, mcastTtl, _addr.AddressFamily);
                }
            }
        }

        //
        // Only for use by UdpEndpoint.
        //
        internal UdpTransceiver(UdpEndpoint endpoint, Communicator communicator, string host,
            ushort port, string mcastInterface, bool connect)
        {
            _endpoint = endpoint;
            _communicator = communicator;
            _state = connect ? StateNeedConnect : StateNotConnected;
            _mcastInterface = mcastInterface;
            _incoming = true;
            _port = port;

            try
            {
                _addr = Network.GetAddressForServerEndpoint(host, port, communicator.IPVersion, communicator.PreferIPv6);

                _readEventArgs = new SocketAsyncEventArgs();
                _readEventArgs.RemoteEndPoint = _addr;
                _readEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(IoCompleted);

                _writeEventArgs = new SocketAsyncEventArgs();
                _writeEventArgs.RemoteEndPoint = _addr;
                _writeEventArgs.Completed += new EventHandler<SocketAsyncEventArgs>(IoCompleted);

                _fd = Network.CreateServerSocket(true, _addr.AddressFamily, communicator.IPVersion);
                SetBufSize(-1, -1);
                Network.SetBlock(_fd, false);
            }
            catch
            {
                if (_readEventArgs != null)
                {
                    _readEventArgs.Dispose();
                }
                if (_writeEventArgs != null)
                {
                    _writeEventArgs.Dispose();
                }
                throw;
            }
        }

        private void SetBufSize(int rcvSize, int sndSize)
        {
            Debug.Assert(_fd != null);

            for (int i = 0; i < 2; ++i)
            {
                bool isSnd;
                string direction;
                string prop;
                int dfltSize;
                int sizeRequested;
                if (i == 0)
                {
                    isSnd = false;
                    direction = "receive";
                    prop = "Ice.UDP.RcvSize";
                    dfltSize = Network.GetRecvBufferSize(_fd);
                    sizeRequested = rcvSize;
                    _rcvSize = dfltSize;
                }
                else
                {
                    isSnd = true;
                    direction = "send";
                    prop = "Ice.UDP.SndSize";
                    dfltSize = Network.GetSendBufferSize(_fd);
                    sizeRequested = sndSize;
                    _sndSize = dfltSize;
                }

                //
                // Get property for buffer size if size not passed in.
                //
                if (sizeRequested == -1)
                {
                    sizeRequested = _communicator.GetPropertyAsByteSize(prop) ?? dfltSize;
                }
                //
                // Check for sanity.
                //
                if (sizeRequested < (UdpOverhead + Ice1Definitions.HeaderSize))
                {
                    _communicator.Logger.Warning($"Invalid {prop} value of {sizeRequested} adjusted to {dfltSize}");
                    sizeRequested = dfltSize;
                }

                if (sizeRequested != dfltSize)
                {
                    //
                    // Try to set the buffer size. The kernel will silently adjust
                    // the size to an acceptable value. Then read the size back to
                    // get the size that was actually set.
                    //
                    int sizeSet;
                    if (i == 0)
                    {
                        Network.SetRecvBufferSize(_fd, sizeRequested);
                        _rcvSize = Network.GetRecvBufferSize(_fd);
                        sizeSet = _rcvSize;
                    }
                    else
                    {
                        Network.SetSendBufferSize(_fd, sizeRequested);
                        _sndSize = Network.GetSendBufferSize(_fd);
                        sizeSet = _sndSize;
                    }

                    //
                    // Warn if the size that was set is less than the requested size
                    // and we have not already warned
                    //
                    if (sizeSet < sizeRequested)
                    {
                        BufSizeWarnInfo winfo = _communicator.GetBufSizeWarn(Transport.UDP);
                        if ((isSnd && (!winfo.SndWarn || winfo.SndSize != sizeRequested)) ||
                           (!isSnd && (!winfo.RcvWarn || winfo.RcvSize != sizeRequested)))
                        {
                            _communicator.Logger.Warning(
                                $"UDP {direction} buffer size: requested size of {sizeRequested} adjusted to {sizeSet}");

                            if (isSnd)
                            {
                                _communicator.SetSndBufSizeWarn(Transport.UDP, sizeRequested);
                            }
                            else
                            {
                                _communicator.SetRcvBufSizeWarn(Transport.UDP, sizeRequested);
                            }
                        }
                    }
                }
            }
        }

        internal void IoCompleted(object? sender, SocketAsyncEventArgs e)
        {
            switch (e.LastOperation)
            {
                case SocketAsyncOperation.Receive:
                case SocketAsyncOperation.ReceiveFrom:
                    Debug.Assert(_readCallback != null);
                    _readCallback(e.UserToken);
                    break;
                case SocketAsyncOperation.Send:
                case SocketAsyncOperation.Connect:
                    Debug.Assert(_writeCallback != null);
                    _writeCallback(e.UserToken);
                    break;
                default:
                    throw new ArgumentException("the last operation completed on the socket was not a receive or send",
                        nameof(e.LastOperation));
            }
        }

        private UdpEndpoint? _endpoint;
        private readonly Communicator _communicator;
        private int _state;
        private readonly bool _incoming;
        private int _rcvSize;
        private int _sndSize;
        private readonly Socket _fd;
        private System.Net.EndPoint _addr;
        private readonly System.Net.IPEndPoint? _sourceAddr;
        private System.Net.EndPoint? _peerAddr;
        private readonly string? _mcastInterface;

        private readonly int _port;
        private bool _bound;

        private SocketAsyncEventArgs? _writeEventArgs;
        private readonly SocketAsyncEventArgs _readEventArgs;

        private AsyncCallback? _writeCallback;
        private AsyncCallback? _readCallback;

        private const int StateNeedConnect = 0;
        private const int StateConnectPending = 1;
        private const int StateConnected = 2;
        private const int StateNotConnected = 3;

        //
        // The maximum IP datagram size is 65535. Subtract 20 bytes for the IP header and 8 bytes for the UDP header
        // to get the maximum payload.
        //
        private const int UdpOverhead = 20 + 8;
        private const int MaxPacketSize = 65535 - UdpOverhead;
    }
}
