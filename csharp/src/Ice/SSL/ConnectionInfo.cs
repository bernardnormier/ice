// Copyright (c) ZeroC, Inc.

#nullable enable

using System.Security.Cryptography.X509Certificates;

namespace Ice.SSL;

public class ConnectionInfo : Ice.ConnectionInfo
{
    public string cipher;

    public X509Certificate2[] certs;
    public bool verified;

    public ConnectionInfo()
    {
        cipher = "";
        certs = [];
    }

    public ConnectionInfo(
        Ice.ConnectionInfo underlying,
        bool incoming,
        string adapterName,
        string connectionId,
        string cipher,
        X509Certificate2[] certs,
        bool verified)
        : base(underlying, incoming, adapterName, connectionId)
    {
        this.cipher = cipher;
        this.certs = certs;
        this.verified = verified;
    }
}
