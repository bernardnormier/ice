//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Collections.Concurrent;

namespace ZeroC.Ice
{
    /// <summary>Represents a response protocol frame sent by the application.</summary>
    public sealed class OutgoingResponseFrame
    {
        /// <summary>The encoding of the frame payload</summary>
        public Encoding Encoding { get; }

        /// <summary>True for a sealed frame, false otherwise. Once sealed, a frame is read-only.</summary>
        public bool IsSealed { get; private set; }

        /// <summary>Returns a list of array segments with the contents of the frame payload.</summary>
        public IList<ArraySegment<byte>> Payload => Data;

        /// <summary>The Ice protocol of this frame.</summary>
        public Protocol Protocol { get; }

        /// <summary>The frame reply status <see cref="ReplyStatus"/>.</summary>
        public ReplyStatus ReplyStatus => (ReplyStatus) Data[0][0];

        /// <summary>The frame byte count.</summary>
        public int Size { get; private set; }

        // The compression status from the incoming request.
        internal byte CompressionStatus { get; }
        // Contents of the Frame
        internal List<ArraySegment<byte>> Data { get; private set; }

        private static readonly ConcurrentDictionary<(Protocol Protocol, Encoding Encoding), OutgoingResponseFrame>
            _cachedVoidReturnValueFrames =
                new ConcurrentDictionary<(Protocol Protocol, Encoding Encoding), OutgoingResponseFrame>();

        /// <summary>Creates a new outgoing response frame with an OK reply status and a void return value.</summary>
        /// <param name="current">The Current object for the corresponding incoming request.</param>
        /// <returns>A new OutgoingResponseFrame.</returns>
        public static OutgoingResponseFrame WithVoidReturnValue(Current current) =>
            _cachedVoidReturnValueFrames.GetOrAdd((current.Protocol, current.Encoding), key =>
            {
                var data = new List<ArraySegment<byte>>();
                var ostr = new OutputStream(key.Protocol.GetEncoding(), data);
                ostr.WriteByte((byte)ReplyStatus.OK);
                _ = ostr.WriteEmptyEncapsulation(key.Encoding);
                return new OutgoingResponseFrame(current.IncomingRequestFrame, data);
            });

        /// <summary>Creates a new outgoing response frame with an OK reply status and a return value.</summary>
        /// <param name="current">The Current object for the corresponding incoming request.</param>
        /// <param name="format">The format type used to marshal classes and exceptions, when this parameter is null
        /// the communicator's default format is used.</param>
        /// <param name="value">The return value to marshal.</param>
        /// <param name="writer">A delegate that must write the value to the frame.</param>
        /// <returns>A new OutgoingResponseFrame.</returns>
        public static OutgoingResponseFrame WithReturnValue<T>(Current current,
                                                               FormatType? format,
                                                               T value,
                                                               OutputStreamWriter<T> writer)
        {
            var response = new OutgoingResponseFrame(current.IncomingRequestFrame);
            byte[] buffer = new byte[256];
            buffer[0] = (byte)ReplyStatus.OK;
            response.Data.Add(buffer);
            var ostr = new OutputStream(current.Protocol.GetEncoding(), response.Data, new OutputStream.Position(0, 1),
                response.Encoding, format ?? current.Adapter.Communicator.DefaultFormat);
            writer(ostr, value);
            ostr.Save();
            response.Finish();
            return response;
        }

        /// <summary>Creates a new outgoing response frame with an OK reply status and a return value.</summary>
        /// <param name="current">The Current object for the corresponding incoming request.</param>
        /// <param name="format">The format type used to marshal classes and exceptions, when this parameter is null
        /// the communicator's default format is used.</param>
        /// <param name="value">The return value to marshal, when the response frame contains multiple return
        /// values they must be passed in a tuple.</param>
        /// <param name="writer">A delegate that must write the value to the frame.</param>
        /// <returns>A new OutgoingResponseFrame.</returns>
        public static OutgoingResponseFrame WithReturnValue<T>(Current current,
                                                               FormatType? format,
                                                               in T value,
                                                               OutputStreamValueWriter<T> writer)
            where T : struct
        {
            var response = new OutgoingResponseFrame(current.IncomingRequestFrame);
            byte[] buffer = new byte[256];
            buffer[0] = (byte)ReplyStatus.OK;
            response.Data.Add(buffer);
            var ostr = new OutputStream(current.Protocol.GetEncoding(),
                                        response.Data,
                                        new OutputStream.Position(0, 1),
                                        response.Encoding,
                                        format ?? current.Adapter.Communicator.DefaultFormat);
            writer(ostr, value);
            ostr.Save();
            response.Finish();
            return response;
        }

        /// <summary>Creates a new outgoing response frame with the given payload.</summary>
        /// <param name="request">The incoming request for which this constructor creates a response.</param>
        /// <param name="payload">The payload for this response frame.</param>
        // TODO: add parameter such as "bool assumeOwnership" once we add memory pooling.
        // TODO: should we pass the payload as a list of segments, or maybe add a separate
        // ctor that accepts a list of segments instead of a single segment
        public OutgoingResponseFrame(IncomingRequestFrame request, ArraySegment<byte> payload)
            : this(request)
        {
            if (payload[0] == (byte)ReplyStatus.OK || payload[0] == (byte)ReplyStatus.UserException)
            {
                // The minimum size for the payload is 7 bytes, the reply status byte plus 6 bytes for an
                // empty encapsulation.
                if (payload.Count < 7)
                {
                    throw new ArgumentException(
                        $"{nameof(payload)} should contain at least 7 bytes, but it contains `{payload.Count}' bytes",
                        nameof(payload));
                }

                (int size, Encoding encapsEncoding) = InputStream.ReadEncapsulationHeader(
                    Protocol.GetEncoding(), payload.AsSpan(1));

                if (size + 4 + 1 != payload.Count) // 4 = size length with 1.1 encoding
                {
                    throw new ArgumentException($"invalid payload size `{size}'; expected `{payload.Count - 5}'",
                        nameof(payload));
                }

                if (encapsEncoding != Encoding)
                {
                    throw new ArgumentException(@$"the payload encoding `{encapsEncoding
                        }' must be the same as the supplied encoding `{Encoding}'",
                        nameof(payload));
                }
            }

            // No need to keep track of the compression status for Ice2, the compression is handled by
            // the encapsulation. We need it for Ice1, since the compression is handled by protocol.
            if (Protocol == Protocol.Ice1)
            {
                CompressionStatus = request.CompressionStatus;
            }

            Data.Add(payload);
            Size = Data.GetByteCount();
            IsSealed = true;
        }

        /// <summary>Creates a response frame that represents "failure" and contains an exception.</summary>
        /// <param name="request">The incoming request for which this constructor creates a response.</param>
        /// <param name="exception">The exception to store into the frame's payload.</param>
        public OutgoingResponseFrame(IncomingRequestFrame request, RemoteException exception)
            : this(request)
        {
            OutputStream ostr;
            if (exception is DispatchException dispatchException)
            {
                ostr = new OutputStream(Protocol.GetEncoding(), Data);
                if (dispatchException is PreExecutionException preExecutionException)
                {
                    ReplyStatus replyStatus = preExecutionException switch
                    {
                        ObjectNotExistException _ => ReplyStatus.ObjectNotExistException,
                        OperationNotExistException _ => ReplyStatus.OperationNotExistException,
                        _ => throw new ArgumentException("unknown PreExecutionException", nameof(exception))
                    };

                    ostr.WriteByte((byte)replyStatus);
                    preExecutionException.Id.IceWrite(ostr);
                    ostr.WriteFacet(preExecutionException.Facet);
                    ostr.WriteString(preExecutionException.Operation);
                }
                else
                {
                    ostr.WriteByte((byte)ReplyStatus.UnknownLocalException);
                    ostr.WriteString(dispatchException.Message);
                }
            }
            else
            {
                byte[] buffer = new byte[256];
                buffer[0] = (byte)ReplyStatus.UserException;
                Data.Add(buffer);
                ostr = new OutputStream(Protocol.GetEncoding(),
                                        Data,
                                        new OutputStream.Position(0, 1),
                                        Encoding,
                                        FormatType.Sliced);
                ostr.WriteException(exception);
            }

            ostr.Save();
            Size = Data.GetByteCount();
            IsSealed = true;
        }

        private OutgoingResponseFrame(IncomingRequestFrame request, List<ArraySegment<byte>>? data = null)
        {
            Protocol = request.Protocol;
            Encoding = request.Encoding;
            // No need to keep track of the compression status for Ice2, the compression is handled by
            // the encapsulation. We need it for Ice1, since the compression is handled by protocol.
            if (Protocol == Protocol.Ice1)
            {
                CompressionStatus = request.CompressionStatus;
            }
            if (data == null)
            {
                Data = new List<ArraySegment<byte>>();
            }
            else
            {
                Data = data;
                Size = Data.GetByteCount();
                IsSealed = true;
            }
        }

        private void Finish()
        {
            Size = Data.GetByteCount();
            IsSealed = true;
        }
    }
}
