//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "Ice/OutgoingAsync.h"
#include "CollocatedRequestHandler.h"
#include "ConnectionFactory.h"
#include "ConnectionI.h"
#include "Ice/ImplicitContext.h"
#include "Ice/LocalExceptions.h"
#include "Ice/LoggerUtil.h"
#include "Instance.h"
#include "ObjectAdapterFactory.h"
#include "Reference.h"
#include "ReplyStatus.h"
#include "RequestHandlerCache.h"
#include "RetryQueue.h"
#include "ThreadPool.h"

using namespace std;
using namespace Ice;
using namespace IceInternal;

const unsigned char OutgoingAsyncBase::OK = 0x1;
const unsigned char OutgoingAsyncBase::Sent = 0x2;

OutgoingAsyncCompletionCallback::~OutgoingAsyncCompletionCallback()
{
    // Out of line to avoid weak vtable
}

bool
OutgoingAsyncBase::sent()
{
    return sentImpl(true);
}

bool
OutgoingAsyncBase::exception(std::exception_ptr ex)
{
    return exceptionImpl(ex);
}

bool
OutgoingAsyncBase::response()
{
    assert(false); // Must be overridden by request that can handle responses
    return false;
}

void
OutgoingAsyncBase::invokeSentAsync()
{
    //
    // This is called when it's not safe to call the sent callback
    // synchronously from this thread. Instead the exception callback
    // is called asynchronously from the client thread pool.
    //
    try
    {
        _instance->clientThreadPool()->execute(
            [self = shared_from_this()]() { self->invokeSent(); },
            _cachedConnection);
    }
    catch (const CommunicatorDestroyedException&)
    {
    }
}

void
OutgoingAsyncBase::invokeExceptionAsync()
{
    //
    // CommunicatorDestroyedException is the only exception that can propagate directly from this method.
    //
    _instance->clientThreadPool()->execute(
        [self = shared_from_this()]() { self->invokeException(); },
        _cachedConnection);
}

void
OutgoingAsyncBase::invokeResponseAsync()
{
    //
    // CommunicatorDestroyedException is the only exception that can propagate directly from this method.
    //
    _instance->clientThreadPool()->execute(
        [self = shared_from_this()]() { self->invokeResponse(); },
        _cachedConnection);
}

void
OutgoingAsyncBase::invokeSent()
{
    try
    {
        handleInvokeSent(_sentSynchronously, this);
    }
    catch (...)
    {
        warning("sent", current_exception());
    }

    if (_observer && _doneInSent)
    {
        _observer.detach();
    }
}

void
OutgoingAsyncBase::invokeException()
{
    try
    {
        handleInvokeException(_ex, this);
    }
    catch (...)
    {
        warning("exception", current_exception());
    }

    _observer.detach();
}

void
OutgoingAsyncBase::invokeResponse()
{
    if (_ex)
    {
        invokeException();
        return;
    }

    try
    {
        handleInvokeResponse(_state & OK, this);
    }
    catch (...)
    {
        // With the lambda async API, lambdaInvokeResponse throws _before_ reaching the application's response when the
        // unmarshaling fails or when the response contains a user exception. We want to call handleInvokeException
        // in this situation.
        if (handleException(current_exception()))
        {
            try
            {
                handleInvokeException(current_exception(), this);
            }
            catch (...)
            {
                warning("exception", current_exception());
            }
        }
    }

    _observer.detach();
}

void
OutgoingAsyncBase::cancelable(const CancellationHandlerPtr& handler)
{
    Lock sync(_m);
    if (_cancellationException)
    {
        try
        {
            rethrow_exception(_cancellationException);
        }
        catch (const LocalException&)
        {
            _cancellationException = nullptr;
            throw;
        }
    }
    _cancellationHandler = handler;
}

void
OutgoingAsyncBase::cancel()
{
    cancel(make_exception_ptr(InvocationCanceledException(__FILE__, __LINE__)));
}

void
OutgoingAsyncBase::attachRemoteObserver(const ConnectionInfoPtr& c, const EndpointPtr& endpt, std::int32_t requestId)
{
    const std::int32_t size = static_cast<std::int32_t>(_os.b.size() - headerSize - 4);
    _childObserver.attach(getObserver().getRemoteObserver(c, endpt, requestId, size));
}

void
OutgoingAsyncBase::attachCollocatedObserver(const ObjectAdapterPtr& adapter, std::int32_t requestId)
{
    const std::int32_t size = static_cast<std::int32_t>(_os.b.size() - headerSize - 4);
    _childObserver.attach(getObserver().getCollocatedObserver(adapter, requestId, size));
}

OutgoingAsyncBase::OutgoingAsyncBase(const InstancePtr& instance)
    : _instance(instance),
      _sentSynchronously(false),
      _doneInSent(false),
      _state(0),
      _os(instance.get(), currentProtocolEncoding),
      _is(instance.get(), currentProtocolEncoding)
{
}

bool
OutgoingAsyncBase::sentImpl(bool done)
{
    Lock sync(_m);
    bool alreadySent = (_state & Sent) > 0;
    _state |= Sent;
    if (done)
    {
        _doneInSent = true;
        _childObserver.detach();
        _cancellationHandler = 0;
    }

    bool invoke = handleSent(done, alreadySent);
    if (!invoke && _doneInSent)
    {
        _observer.detach();
    }
    return invoke;
}

bool
OutgoingAsyncBase::exceptionImpl(std::exception_ptr ex)
{
    Lock sync(_m);
    _ex = ex;
    if (_childObserver)
    {
        _childObserver.failed(getExceptionId(ex));
        _childObserver.detach();
    }
    _cancellationHandler = 0;
    _observer.failed(getExceptionId(ex));

    bool invoke = handleException(ex);
    if (!invoke)
    {
        _observer.detach();
    }
    return invoke;
}

bool
OutgoingAsyncBase::responseImpl(bool ok, bool invoke)
{
    Lock sync(_m);
    if (ok)
    {
        _state |= OK;
    }

    _cancellationHandler = 0;

    try
    {
        invoke &= handleResponse(ok);
    }
    catch (const Exception&)
    {
        _ex = current_exception();
        invoke = handleException(_ex);
    }
    if (!invoke)
    {
        _observer.detach();
    }
    return invoke;
}

void
OutgoingAsyncBase::cancel(std::exception_ptr ex)
{
    CancellationHandlerPtr handler;
    {
        Lock sync(_m);
        if (!_cancellationHandler)
        {
            _cancellationException = ex;
            return;
        }
        handler = _cancellationHandler;
    }
    handler->asyncRequestCanceled(shared_from_this(), ex);
}

void
OutgoingAsyncBase::warning(string_view callbackName, std::exception_ptr eptr) const
{
    if (_instance->initializationData().properties->getIcePropertyAsInt("Ice.Warn.AMICallback") > 0)
    {
        Warning out(_instance->initializationData().logger);
        try
        {
            rethrow_exception(eptr);
        }
        catch (const Exception& ex)
        {
            out << "Ice::Exception raised by " << callbackName << " callback:\n" << ex;
        }
        catch (const std::exception& ex)
        {
            out << "std::exception raised by " << callbackName << " callback:\n" << ex.what();
        }
        catch (...)
        {
            out << "unknown exception raised by " << callbackName << " callback";
        }
    }
}

bool
ProxyOutgoingAsyncBase::exception(std::exception_ptr exc)
{
    if (_childObserver)
    {
        _childObserver.failed(getExceptionId(exc));
        _childObserver.detach();
    }

    _cachedConnection = nullptr;

    //
    // NOTE: at this point, synchronization isn't needed, no other threads should be
    // calling on the callback.
    //
    try
    {
        //
        // It's important to let the retry queue do the retry even if
        // the retry interval is 0. This method can be called with the
        // connection locked so we can't just retry here.
        //
        _instance->retryQueue()->add(
            shared_from_this(),
            _proxy._getRequestHandlerCache()->handleException(exc, _handler, _mode, _sent, _cnt));

        return false;
    }
    catch (const Exception&)
    {
        return exceptionImpl(current_exception()); // No retries, we're done
    }
}

void
ProxyOutgoingAsyncBase::retryException()
{
    try
    {
        // It's important to let the retry queue do the retry. This is
        // called from the connect request handler and the retry might
        // require could end up waiting for the flush of the
        // connection to be done.

        // Clear cached request handler and always retry.
        _proxy._getRequestHandlerCache()->clearCachedRequestHandler(_handler);
        _instance->retryQueue()->add(shared_from_this(), 0);
    }
    catch (const Exception&)
    {
        if (exception(current_exception()))
        {
            invokeExceptionAsync();
        }
    }
}

void
ProxyOutgoingAsyncBase::retry()
{
    invokeImpl(false);
}

void
ProxyOutgoingAsyncBase::abort(std::exception_ptr ex)
{
    assert(!_childObserver);

    if (exceptionImpl(ex))
    {
        invokeExceptionAsync();
    }
    else
    {
        try
        {
            rethrow_exception(ex);
        }
        catch (const CommunicatorDestroyedException&)
        {
            //
            // If it's a communicator destroyed exception, don't swallow
            // it but instead notify the user thread. Even if no callback
            // was provided.
            //
            throw;
        }
        catch (...)
        {
            // ignored.
        }
    }
}

ProxyOutgoingAsyncBase::ProxyOutgoingAsyncBase(ObjectPrx proxy)
    : OutgoingAsyncBase(proxy->_getReference()->getInstance()),
      _proxy(std::move(proxy)),
      _mode(OperationMode::Normal),
      _cnt(0),
      _sent(false)
{
}

ProxyOutgoingAsyncBase::~ProxyOutgoingAsyncBase() {}

void
ProxyOutgoingAsyncBase::invokeImpl(bool userThread)
{
    try
    {
        if (userThread)
        {
            int invocationTimeout = _proxy._getReference()->getInvocationTimeout();
            if (invocationTimeout > 0)
            {
                _instance->timer()->schedule(shared_from_this(), chrono::milliseconds(invocationTimeout));
            }
        }
        else
        {
            _observer.retried();
        }

        while (true)
        {
            try
            {
                _sent = false;
                _handler = _proxy._getRequestHandlerCache()->getRequestHandler();
                AsyncStatus status = _handler->sendAsyncRequest(shared_from_this());
                if (status & AsyncStatusSent)
                {
                    if (userThread)
                    {
                        _sentSynchronously = true;
                        if (status & AsyncStatusInvokeSentCallback)
                        {
                            invokeSent(); // Call the sent callback from the user thread.
                        }
                    }
                    else
                    {
                        if (status & AsyncStatusInvokeSentCallback)
                        {
                            invokeSentAsync(); // Call the sent callback from a client thread pool thread.
                        }
                    }
                }
                return; // We're done!
            }
            catch (const RetryException&)
            {
                // Clear request handler and always retry.
                _proxy._getRequestHandlerCache()->clearCachedRequestHandler(_handler);
            }
            catch (const Exception& ex)
            {
                if (_childObserver)
                {
                    _childObserver.failed(ex.ice_id());
                    _childObserver.detach();
                }
                int interval = _proxy._getRequestHandlerCache()
                                   ->handleException(current_exception(), _handler, _mode, _sent, _cnt);

                if (interval > 0)
                {
                    _instance->retryQueue()->add(shared_from_this(), interval);
                    return;
                }
                else
                {
                    _observer.retried();
                }
            }
        }
    }
    catch (const Exception&)
    {
        //
        // If called from the user thread we re-throw, the exception
        // will be catch by the caller and abort(ex) will be called.
        //
        if (userThread)
        {
            throw;
        }
        else if (exceptionImpl(current_exception())) // No retries, we're done
        {
            invokeExceptionAsync();
        }
    }
}

bool
ProxyOutgoingAsyncBase::sentImpl(bool done)
{
    _sent = true;
    if (done)
    {
        if (_proxy._getReference()->getInvocationTimeout() != -1)
        {
            _instance->timer()->cancel(shared_from_this());
        }
    }
    return OutgoingAsyncBase::sentImpl(done);
}

bool
ProxyOutgoingAsyncBase::exceptionImpl(std::exception_ptr ex)
{
    if (_proxy._getReference()->getInvocationTimeout() != -1)
    {
        _instance->timer()->cancel(shared_from_this());
    }
    return OutgoingAsyncBase::exceptionImpl(ex);
}

bool
ProxyOutgoingAsyncBase::responseImpl(bool ok, bool invoke)
{
    if (_proxy._getReference()->getInvocationTimeout() != -1)
    {
        _instance->timer()->cancel(shared_from_this());
    }
    return OutgoingAsyncBase::responseImpl(ok, invoke);
}

void
ProxyOutgoingAsyncBase::runTimerTask()
{
    cancel(make_exception_ptr(InvocationTimeoutException(__FILE__, __LINE__)));
}

OutgoingAsync::OutgoingAsync(ObjectPrx proxy, bool synchronous)
    : ProxyOutgoingAsyncBase(std::move(proxy)),
      _encoding(_proxy->_getReference()->getEncoding()),
      _synchronous(synchronous)
{
}

void
OutgoingAsync::prepare(string_view operation, OperationMode mode, const Context& context)
{
    if (_proxy._getReference()->getProtocol().major != currentProtocol.major)
    {
        throw FeatureNotSupportedException{
            __FILE__,
            __LINE__,
            "cannot send request using protocol version " +
                Ice::protocolVersionToString(_proxy._getReference()->getProtocol())};
    }

    _mode = mode;

    _observer.attach(_proxy, operation, context);

    // We need to check isBatch() and not if getBatchRequestQueue() is not null: for a fixed proxy,
    // getBatchRequestQueue() always returns a non null value.
    if (_proxy._getReference()->isBatch())
    {
        _proxy._getReference()->getBatchRequestQueue()->prepareBatchRequest(&_os);
    }
    else
    {
        _os.writeBlob(requestHdr, sizeof(requestHdr));
    }

    Reference* ref = _proxy._getReference().get();

    _os.write(ref->getIdentity());

    //
    // For compatibility with the old FacetPath.
    //
    if (ref->getFacet().empty())
    {
        _os.write(static_cast<string*>(0), static_cast<string*>(0));
    }
    else
    {
        string facet = ref->getFacet();
        _os.write(&facet, &facet + 1);
    }

    _os.write(operation, false);

    _os.write(static_cast<uint8_t>(_mode));

    if (&context != &noExplicitContext)
    {
        //
        // Explicit context
        //
        _os.write(context);
    }
    else
    {
        //
        // Implicit context
        //
        const ImplicitContextPtr& implicitContext = ref->getInstance()->getImplicitContext();
        const Context& prxContext = ref->getContext()->getValue();
        if (implicitContext)
        {
            implicitContext->write(prxContext, &_os);
        }
        else
        {
            _os.write(prxContext);
        }
    }
}

bool
OutgoingAsync::sent()
{
    return ProxyOutgoingAsyncBase::sentImpl(
        !_proxy._getReference()->isTwoway()); // done = true if it's not a two-way proxy
}

bool
OutgoingAsync::response()
{
    //
    // NOTE: this method is called from ConnectionI.parseMessage
    // with the connection locked. Therefore, it must not invoke
    // any user callbacks.
    //
    assert(_proxy._getReference()->isTwoway()); // Can only be called for twoways.

    if (_childObserver)
    {
        _childObserver->reply(static_cast<int32_t>(_is.b.size() - headerSize - 4));
        _childObserver.detach();
    }

    uint8_t replyStatus;
    try
    {
        _is.read(replyStatus);

        switch (replyStatus)
        {
            case replyOK:
            {
                break;
            }
            case replyUserException:
            {
                _observer.userException();
                break;
            }

            case replyObjectNotExist:
            case replyFacetNotExist:
            case replyOperationNotExist:
            {
                Identity ident;
                _is.read(ident);

                //
                // For compatibility with the old FacetPath.
                //
                vector<string> facetPath;
                _is.read(facetPath);
                string facet;
                if (!facetPath.empty())
                {
                    if (facetPath.size() > 1)
                    {
                        throw MarshalException{__FILE__, __LINE__, "received facet path with more than one element"};
                    }
                    facet.swap(facetPath[0]);
                }

                string operation;
                _is.read(operation, false);
                switch (replyStatus)
                {
                    case replyObjectNotExist:
                    {
                        throw ObjectNotExistException{
                            __FILE__,
                            __LINE__,
                            std::move(ident),
                            std::move(facet),
                            std::move(operation)};
                        break;
                    }

                    case replyFacetNotExist:
                    {
                        throw FacetNotExistException{
                            __FILE__,
                            __LINE__,
                            std::move(ident),
                            std::move(facet),
                            std::move(operation)};
                        break;
                    }

                    case replyOperationNotExist:
                    {
                        throw OperationNotExistException{
                            __FILE__,
                            __LINE__,
                            std::move(ident),
                            std::move(facet),
                            std::move(operation)};
                        break;
                    }

                    default:
                    {
                        assert(false);
                        break;
                    }
                }
                break;
            }

            case replyUnknownException:
            case replyUnknownLocalException:
            case replyUnknownUserException:
            {
                string message;
                _is.read(message, false);

                switch (replyStatus)
                {
                    case replyUnknownException:
                    {
                        throw UnknownException{__FILE__, __LINE__, std::move(message)};
                        break;
                    }

                    case replyUnknownLocalException:
                    {
                        throw UnknownLocalException{__FILE__, __LINE__, std::move(message)};
                        break;
                    }

                    case replyUnknownUserException:
                    {
                        throw UnknownUserException{__FILE__, __LINE__, std::move(message)};
                        break;
                    }

                    default:
                    {
                        assert(false);
                        break;
                    }
                }
                break;
            }

            default:
            {
                throw ProtocolException{
                    __FILE__,
                    __LINE__,
                    "received unknown reply status in Reply message" + to_string(replyStatus)};
            }
        }

        return responseImpl(replyStatus == replyOK, true);
    }
    catch (const Exception&)
    {
        return exception(current_exception());
    }
}

AsyncStatus
OutgoingAsync::invokeRemote(const ConnectionIPtr& connection, bool compress, bool response)
{
    _cachedConnection = connection;
    return connection->sendAsyncRequest(shared_from_this(), compress, response, 0);
}

AsyncStatus
OutgoingAsync::invokeCollocated(CollocatedRequestHandler* handler)
{
    return handler->invokeAsyncRequest(this, 0, _synchronous);
}

void
OutgoingAsync::abort(std::exception_ptr ex)
{
    if (_proxy._getReference()->isBatch())
    {
        //
        // If we didn't finish a batch oneway or datagram request, we
        // must notify the connection about that we give up ownership
        // of the batch stream.
        //
        _proxy._getReference()->getBatchRequestQueue()->abortBatchRequest(&_os);
    }

    ProxyOutgoingAsyncBase::abort(ex);
}

void
OutgoingAsync::invoke(string_view operation)
{
    if (_proxy._getReference()->isBatch())
    {
        _sentSynchronously = true;
        _proxy._getReference()->getBatchRequestQueue()->finishBatchRequest(&_os, _proxy, operation);
        responseImpl(true, false); // Don't call sent/completed callback for batch AMI requests
        return;
    }

    //
    // NOTE: invokeImpl doesn't throw so this can be called from the
    // try block with the catch block calling abort(ex) in case of an
    // exception.
    //
    invokeImpl(true); // userThread = true
}

void
OutgoingAsync::invoke(
    string_view operation,
    OperationMode mode,
    FormatType format,
    const Context& context,
    function<void(OutputStream*)> write)
{
    try
    {
        prepare(operation, mode, context);
        if (write)
        {
            _os.startEncapsulation(_encoding, format);
            write(&_os);
            _os.endEncapsulation();
        }
        else
        {
            _os.writeEmptyEncapsulation(_encoding);
        }
        invoke(operation);
    }
    catch (const Exception&)
    {
        abort(current_exception());
    }
}

void
OutgoingAsync::throwUserException()
{
    try
    {
        _is.startEncapsulation();
        _is.throwException();
    }
    catch (const UserException& ex)
    {
        _is.endEncapsulation();
        if (_userException)
        {
            _userException(ex);
        }
        throw UnknownUserException::fromTypeId(__FILE__, __LINE__, ex.ice_id());
    }
}

bool
LambdaInvoke::handleSent(bool, bool alreadySent) noexcept
{
    return _sent != nullptr && !alreadySent; // Invoke the sent callback only if not already invoked.
}

bool
LambdaInvoke::handleException(std::exception_ptr) noexcept
{
    return _exception != nullptr; // Invoke the callback
}

bool
LambdaInvoke::handleResponse(bool)
{
    return _response != nullptr;
}

void
LambdaInvoke::handleInvokeSent(bool sentSynchronously, OutgoingAsyncBase*) const
{
    _sent(sentSynchronously);
}

void
LambdaInvoke::handleInvokeException(std::exception_ptr ex, OutgoingAsyncBase*) const
{
    _exception(ex);
}

void
LambdaInvoke::handleInvokeResponse(bool ok, OutgoingAsyncBase*) const
{
    _response(ok);
}
