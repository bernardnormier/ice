// Copyright (c) ZeroC, Inc.

#nullable enable

using Ice.Internal;
using System.Diagnostics;

namespace Ice;

/// <summary>
/// Provides extension methods for <see cref="Current"/>.
/// </summary>
public static class CurrentExtensions
{
    /// <summary>
    /// Creates an outgoing response with reply status <see cref="ReplyStatus.Ok"/>.
    /// </summary>
    /// <typeparam name="TResult">The type of result.</typeparam>
    /// <param name="current">The current object of the corresponding incoming request.</param>
    /// <param name="result">The result to marshal into the response payload.</param>
    /// <param name="marshal">The action that marshals result into an output stream.</param>
    /// <param name="formatType">The class format.</param>
    /// <returns>A new outgoing response.</returns>
    public static OutgoingResponse createOutgoingResponse<TResult>(
        this Current current,
        TResult result,
        Action<OutputStream, TResult> marshal,
        FormatType? formatType = null)
    {
        OutputStream ostr = current.startReplyStream();
        if (current.requestId != 0)
        {
            try
            {
                ostr.startEncapsulation(current.encoding, formatType);
                marshal(ostr, result);
                ostr.endEncapsulation();
                return new OutgoingResponse(ostr);
            }
            catch (System.Exception exception)
            {
                return current.createOutgoingResponse(exception);
            }
        }
        else
        {
            Debug.Fail("A one-way request cannot return a response");
            return new OutgoingResponse(ostr);
        }
    }

    /// <summary>
    /// Creates an empty outgoing response with reply status <see cref="ReplyStatus.Ok"/>.
    /// </summary>
    /// <param name="current">The current object of the corresponding incoming request.</param>
    /// <returns>An outgoing response with an empty payload.</returns>
    public static OutgoingResponse createEmptyOutgoingResponse(this Current current)
    {
        OutputStream ostr = current.startReplyStream();
        if (current.requestId != 0)
        {
            try
            {
                ostr.writeEmptyEncapsulation(current.encoding);
            }
            catch (System.Exception ex)
            {
                return current.createOutgoingResponse(ex);
            }
        }
        return new OutgoingResponse(ostr);
    }

    /// <summary>
    /// Creates an outgoing response with the specified payload.
    /// </summary>
    /// <param name="current">The current object of the corresponding incoming request.</param>
    /// <param name="ok">When true, the reply status of the response is <see cref="ReplyStatus.Ok" />; otherwise, it's
    /// <see cref="ReplyStatus.UserException" />.</param>
    /// <param name="encapsulation">The payload of the response.</param>
    /// <returns>A new outgoing response.</returns>
    public static OutgoingResponse createOutgoingResponse(this Current current, bool ok, byte[] encapsulation)
    {
        // For compatibility with the Ice 3.7 and earlier.
        encapsulation ??= [];

        OutputStream ostr = current.startReplyStream(ok ? ReplyStatus.Ok : ReplyStatus.UserException);

        if (current.requestId != 0)
        {
            try
            {
                if (encapsulation.Length > 0)
                {
                    ostr.writeEncapsulation(encapsulation);
                }
                else
                {
                    ostr.writeEmptyEncapsulation(current.encoding);
                }
            }
            catch (System.Exception ex)
            {
                return current.createOutgoingResponse(ex);
            }
        }
        return new OutgoingResponse(ostr);
    }

    /// <summary>
    /// Creates an outgoing response that marshals an exception.
    /// </summary>
    /// <param name="current">The current object of the corresponding incoming request.</param>
    /// <param name="exception">The exception to marshal into the response payload.</param>
    /// <returns>A new outgoing response.</returns>
    public static OutgoingResponse createOutgoingResponse(this Current current, System.Exception exception)
    {
        try
        {
            return createOutgoingResponseCore(exception);
        }
        catch (System.Exception ex)
        {
            // Try a second time with the marshal exception. This should not fail.
            return createOutgoingResponseCore(ex);
        }

        OutgoingResponse createOutgoingResponseCore(System.Exception exc)
        {
            OutputStream ostr;

            if (current.requestId != 0)
            {
                // The default class format doesn't matter since we always encode user exceptions in Sliced format.
                ostr = new OutputStream(Util.currentProtocolEncoding);
                ostr.writeBlob(Protocol.replyHdr);
                ostr.writeInt(current.requestId);
            }
            else
            {
                ostr = new OutputStream();
            }

            ReplyStatus replyStatus;
            string exceptionId;
            string? exceptionDetails = null;
            string? unknownExceptionMessage = null;

            switch (exc)
            {
                case RequestFailedException rfe:
                    exceptionId = rfe.ice_id();

                    replyStatus = rfe switch
                    {
                        ObjectNotExistException _ => ReplyStatus.ObjectNotExist,
                        FacetNotExistException _ => ReplyStatus.FacetNotExist,
                        OperationNotExistException _ => ReplyStatus.OperationNotExist,
                        _ => throw new MarshalException("Unexpected exception type")
                    };

                    Identity id = rfe.id;
                    string facet = rfe.facet;
                    if (id.name.Length == 0)
                    {
                        id = current.id;
                        facet = current.facet;
                    }
                    string operation = rfe.operation.Length == 0 ? current.operation : rfe.operation;

                    exceptionDetails = RequestFailedException.createMessage(rfe.GetType().Name, id, facet, operation);

                    if (current.requestId != 0)
                    {
                        ostr.writeByte((byte)replyStatus);
                        Identity.ice_write(ostr, id);

                        if (facet.Length == 0)
                        {
                            ostr.writeStringSeq([]);
                        }
                        else
                        {
                            ostr.writeStringSeq([facet]);
                        }

                        ostr.writeString(operation);
                    }
                    break;

                case UserException ex:
                    exceptionId = ex.ice_id();
                    replyStatus = ReplyStatus.UserException;

                    if (current.requestId != 0)
                    {
                        ostr.writeByte((byte)replyStatus);
                        ostr.startEncapsulation(current.encoding, FormatType.SlicedFormat);
                        ostr.writeException(ex);
                        ostr.endEncapsulation();
                    }
                    break;

                case UnknownLocalException ex:
                    exceptionId = ex.ice_id();
                    replyStatus = ReplyStatus.UnknownLocalException;
                    unknownExceptionMessage = ex.Message;
                    break;

                case UnknownUserException ex:
                    exceptionId = ex.ice_id();
                    replyStatus = ReplyStatus.UnknownUserException;
                    unknownExceptionMessage = ex.Message;
                    break;

                case UnknownException ex:
                    exceptionId = ex.ice_id();
                    replyStatus = ReplyStatus.UnknownException;
                    unknownExceptionMessage = ex.Message;
                    break;

                case LocalException ex:
                    exceptionId = ex.ice_id();
                    replyStatus = ReplyStatus.UnknownLocalException;
                    break;

                case Ice.Exception ex:
                    exceptionId = ex.ice_id();
                    replyStatus = ReplyStatus.UnknownException;
                    break;

                default:
                    replyStatus = ReplyStatus.UnknownException;
                    exceptionId = exc.GetType().FullName ?? "System.Exception";
                    break;
            }

            if (current.requestId != 0 &&
                (replyStatus is
                    ReplyStatus.UnknownUserException or
                    ReplyStatus.UnknownLocalException or
                    ReplyStatus.UnknownException))
            {
                ostr.writeByte((byte)replyStatus);
                // If the exception is an UnknownXxxException, we keep its message as-is; otherwise, we create a custom
                // message. This message doesn't include the stack trace.
                unknownExceptionMessage ??= $"Dispatch failed with {exceptionId}: {exc.Message}";
                ostr.writeString(unknownExceptionMessage);
            }

            return new OutgoingResponse(replyStatus, exceptionId, exceptionDetails ?? exc.ToString(), ostr);
        }
    }

    /// <summary>
    /// Starts the output stream for a reply, with everything up to and including the reply status. When the request ID
    /// is 0 (one-way request), the returned output stream is empty.
    /// </summary>
    /// <param name="current">The current object of the corresponding incoming request.</param>
    /// <param name="replyStatus">The reply status.</param>
    /// <returns>The new output stream.</returns>
    public static OutputStream startReplyStream(this Current current, ReplyStatus replyStatus = ReplyStatus.Ok)
    {
        if (current.requestId == 0)
        {
            return new OutputStream();
        }
        else
        {
            var ostr = new OutputStream(
                Util.currentProtocolEncoding,
                current.adapter.getCommunicator().instance.defaultsAndOverrides().defaultFormat);

            ostr.writeBlob(Protocol.replyHdr);
            ostr.writeInt(current.requestId);
            ostr.writeByte((byte)replyStatus);
            return ostr;
        }
    }
}
