//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;

namespace ZeroC.Ice
{
    /// <summary>Creates endpoints from strings or encoded bytes. A single factory object can support multiple
    /// transports. This is a publicly visible Ice-internal interface implemented by Ice transports.</summary>
    public interface IEndpointFactory
    {
        /// <summary>Creates a new endpoint from an already parsed endpoint string. Only used for protocol ice1 with the
        /// old string format.</summary>
        /// <param name="transport">The transport of the new endpoint.</param>
        /// <param name="protocol">The protocol of the new endpoint.</param>
        /// <param name="options">The options of the new endpoint. The factory removes any option it understands from
        /// this dictionary, leaving only unknown options.</param>
        /// <param name="oaEndpoint">When true, the new endpoints corresponds to an object adapter's endpoint
        /// configuration; when false, endpointString represents a proxy endpoint.</param>
        /// <param name="endpointString">The original endpoint string, for error messages and tracing.</param>
        /// <returns>The new endpoint.</returns>
        /// <exception cref="NotSupportedException">Thrown when transport and protocol are not compatible.</exception>
        Endpoint Create(
            Transport transport,
            Protocol protocol, // TODO: temporary, remove once URI parsing is implemented
            Dictionary<string, string?> options,
            bool oaEndpoint,
            string endpointString);

        /// <summary>Creates a new endpoint from a parsed URI.</summary>
        /// <param name="transport">The transport of the new endpoint.</param>
        /// <param name="protocol">The protocol of the new endpoint.</param>
        /// <param name="host">The host name or IP address.</param>
        /// <param name="port">The port number.</param>
        /// <param name="options">The options of the new endpoint. The factory removes any option it understands from
        /// this dictionary, leaving only unknown options.</param>
        /// <param name="oaEndpoint">When true, the new endpoints corresponds to an object adapter's endpoint
        /// configuration; when false, endpointString represents a proxy endpoint.</param>
        /// <param name="endpointString">The original endpoint string, for error messages and tracing.</param>
        /// <returns>The new endpoint.</returns>
        /// <exception cref="NotSupportedException">Thrown when transport and protocol are not compatible.</exception>
        Endpoint Create(
            Transport transport,
            Protocol protocol,
            string host,
            ushort port,
            Dictionary<string, string> options,
            bool oaEndpoint,
            string endpointString);

        /// <summary>Reads an endpoint from the given input stream.</summary>
        /// <param name="istr">The input stream to read from.</param>
        /// <param name="transport">The transport of the endpoint.</param>
        /// <param name="protocol">The protocol of the enclosing proxy.</param>
        /// <returns>Returns the endpoint read from the stream</returns>
        Endpoint Read(InputStream istr, Transport transport, Protocol protocol);
    }
}
