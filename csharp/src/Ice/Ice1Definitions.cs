//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace ZeroC.Ice
{
    // Definitions for the ice1 protocol.
    internal static class Ice1Definitions
    {
        // The encoding of the header for ice1 frames. It is nominally 1.0, but in practice it is identical to 1.1
        // for the subset of the encoding used by the ice1 headers.
        internal static readonly Encoding Encoding = Encoding.V1_1;

        // Size of an ice1 frame header:
        // Magic number (4 bytes)
        // Protocol bytes (4 bytes)
        // Frame type (Byte)
        // Compression status (Byte)
        // Frame size (Int - 4 bytes)
        internal const int HeaderSize = 14;

        // The magic number at the front of each frame.
        internal static readonly byte[] Magic = new byte[] { 0x49, 0x63, 0x65, 0x50 }; // 'I', 'c', 'e', 'P'

        // 4-bytes after magic that provide the protocol version (always 1.0 for an ice1 frame) and the
        // encoding of the frame header (always set to 1.0 with the an ice1 frame, even though we use 1.1).
        internal static readonly byte[] ProtocolBytes = new byte[] { 1, 0, 1, 0 };

        // ice1 frame types:
        internal enum FrameType : byte
        {
            Request = 0,
            RequestBatch = 1,
            Reply = 2,
            ValidateConnection = 3,
            CloseConnection = 4
        }

        internal static readonly byte[] RequestHeader = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte) FrameType.Request,
            0, // Compression status.
            0, 0, 0, 0, // Frame size (placeholder).
            0, 0, 0, 0 // Request ID (placeholder).
        };

        internal static readonly byte[] RequestBatchHeader = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.RequestBatch,
            0, // Compression status.
            0, 0, 0, 0, // Frame size (placeholder).
            0, 0, 0, 0 // Number of requests in batch (placeholder).
        };

        internal static readonly byte[] ReplyHeader = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.Reply,
            0, // Compression status.
            0, 0, 0, 0 // Frame size (placeholder).
        };

        internal static readonly byte[] ValidateConnectionFrame = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.ValidateConnection,
            0, // Compression status.
            HeaderSize, 0, 0, 0 // Frame size.
        };

        internal static readonly byte[] CloseConnectionFrame = new byte[]
        {
            Magic[0], Magic[1], Magic[2], Magic[3],
            ProtocolBytes[0], ProtocolBytes[1], ProtocolBytes[2], ProtocolBytes[3],
            (byte)FrameType.CloseConnection,
            0, // Compression status.
            HeaderSize, 0, 0, 0 // Frame size.
        };

        // Verify that the first 8 bytes correspond to Magic + ProtocolBytes
        internal static void CheckHeader(Span<byte> header)
        {
            Debug.Assert(header.Length >= 8);
            if (header[0] != Magic[0] || header[1] != Magic[1] || header[2] != Magic[2] || header[3] != Magic[3])
            {
                throw new InvalidDataException(
                    $"received incorrect magic bytes in header of ice1 frame: {BytesToString(header.Slice(0, 4))}");
            }

            header = header.Slice(4);

            if (header[0] != ProtocolBytes[0] || header[1] != ProtocolBytes[1])
            {
                throw new InvalidDataException(
                    $"received ice1 protocol frame with protocol set to {header[0]}.{header[1]}");
            }

            if (header[2] != ProtocolBytes[2] || header[3] != ProtocolBytes[3])
            {
                throw new InvalidDataException(
                    $"received ice1 protocol frame with protocol encoding set to {header[2]}.{header[3]}");
            }
        }

        internal static List<ArraySegment<byte>> GetRequestData(OutgoingRequestFrame frame, int requestId)
        {
            byte[] headerData = new byte[HeaderSize + 4];
            RequestHeader.CopyTo(headerData.AsSpan());

            OutputStream.WriteInt(frame.Size + HeaderSize + 4, headerData.AsSpan(10, 4));
            OutputStream.WriteInt(requestId, headerData.AsSpan(HeaderSize, 4));

            var data = new List<ArraySegment<byte>>() { headerData };
            data.AddRange(frame.Data);
            return data;
        }

        internal static List<ArraySegment<byte>> GetResponseData(OutgoingResponseFrame frame, int requestId)
        {
            byte[] headerData = new byte[HeaderSize + 4];
            ReplyHeader.CopyTo(headerData.AsSpan());

            OutputStream.WriteInt(frame.Size + HeaderSize + 4, headerData.AsSpan(10, 4));
            OutputStream.WriteInt(requestId, headerData.AsSpan(HeaderSize, 4));

            var data = new List<ArraySegment<byte>>() { headerData };
            data.AddRange(frame.Data);
            return data;
        }

        private static string BytesToString(Span<byte> bytes) => BitConverter.ToString(bytes.ToArray());
    }
}
