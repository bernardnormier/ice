//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Ice;

final class TcpTransceiver implements Transceiver {
    @Override
    public java.nio.channels.SelectableChannel fd() {
        assert (_stream != null);
        return _stream.fd();
    }

    @Override
    public void setReadyCallback(ReadyCallback callback) {
        // No need of the callback
    }

    @Override
    public int initialize(Buffer readBuffer, Buffer writeBuffer) {
        return _stream.connect(readBuffer, writeBuffer);
    }

    @Override
    public int closing(boolean initiator, LocalException ex) {
        // If we are initiating the connection closure, wait for the peer
        // to close the TCP/IP connection. Otherwise, close immediately.
        return initiator ? SocketOperation.Read : SocketOperation.None;
    }

    @Override
    public void close() {
        _stream.close();
    }

    @Override
    public EndpointI bind() {
        assert (false);
        return null;
    }

    @Override
    public int write(Buffer buf) {
        return _stream.write(buf);
    }

    @Override
    public int read(Buffer buf) {
        return _stream.read(buf);
    }

    @Override
    public String protocol() {
        return _instance.protocol();
    }

    @Override
    public String toString() {
        return _stream.toString();
    }

    @Override
    public String toDetailedString() {
        return toString();
    }

    @Override
    public ConnectionInfo getInfo() {
        TCPConnectionInfo info = new TCPConnectionInfo();
        if (_stream.fd() != null) {
            java.net.Socket socket = _stream.fd().socket();
            info.localAddress = socket.getLocalAddress().getHostAddress();
            info.localPort = socket.getLocalPort();
            if (socket.getInetAddress() != null) {
                info.remoteAddress = socket.getInetAddress().getHostAddress();
                info.remotePort = socket.getPort();
            }
            if (!socket.isClosed()) {
                info.rcvSize = Network.getRecvBufferSize(_stream.fd());
                info.sndSize = Network.getSendBufferSize(_stream.fd());
            }
        }
        return info;
    }

    @Override
    public void checkSendSize(Buffer buf) {}

    @Override
    public void setBufferSize(int rcvSize, int sndSize) {
        _stream.setBufferSize(rcvSize, sndSize);
    }

    TcpTransceiver(ProtocolInstance instance, StreamSocket stream) {
        _instance = instance;
        _stream = stream;
    }

    private final ProtocolInstance _instance;
    private final StreamSocket _stream;
}
