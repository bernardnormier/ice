//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Current } from "./Current.js";
import { Ice as Ice_Context } from "./Context.js";
const { Context } = Ice_Context;
import { Ice as Ice_OperationMode } from "./OperationMode.js";
const { OperationMode } = Ice_OperationMode;
import { Ice as Ice_Identity } from "./Identity.js";
const { Identity } = Ice_Identity;
import { identityToString } from "./IdentityUtil.js";
import { FormatType } from "./FormatType.js";
import { LocalException, UserException } from "./Exception.js";
import {
    FacetNotExistException,
    MarshalException,
    ObjectNotExistException,
    OperationNotExistException,
    RequestFailedException,
    UnknownException,
    UnknownLocalException,
    UnknownUserException,
} from "./LocalException.js";

import { Protocol } from "./Protocol.js";
import { OutputStream } from "./Stream.js";
import { StringUtil } from "./StringUtil.js";
import { IPConnectionInfo } from "./Connection.js";
import { Ice as Ice_Version } from "./Version.js";
const { EncodingVersion } = Ice_Version;
import { Ice as Ice_BuiltinSequences } from "./BuiltinSequences.js";
const { StringSeqHelper } = Ice_BuiltinSequences;
import { Debug } from "./Debug.js";

export class IncomingAsync {
    constructor(instance, connection, adapter, response, requestId) {
        this._instance = instance;
        this._response = response;
        this._connection = connection;
        this._format = FormatType.DefaultFormat;

        this._current = new Current();
        this._current.id = new Identity();
        this._current.adapter = adapter;
        this._current.con = this._connection;
        this._current.requestId = requestId;

        this._servant = null;
        this._locator = null;
        this._cookie = { value: null };

        this._os = null;
        this._is = null;
    }

    startWriteParams() {
        if (!this._response) {
            throw new MarshalException("can't marshal out parameters for oneway dispatch");
        }

        Debug.assert(this._current.encoding !== null); // Encoding for reply is known.
        this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
        this._os.writeBlob(Protocol.replyHdr);
        this._os.writeInt(this._current.requestId);
        this._os.writeByte(0);
        this._os.startEncapsulation(this._current.encoding, this._format);
        return this._os;
    }

    endWriteParams() {
        if (this._response) {
            this._os.endEncapsulation();
        }
    }

    writeEmptyParams() {
        if (this._response) {
            Debug.assert(this._current.encoding !== null); // Encoding for reply is known.
            this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
            this._os.writeBlob(Protocol.replyHdr);
            this._os.writeInt(this._current.requestId);
            this._os.writeByte(Protocol.replyOK);
            this._os.writeEmptyEncapsulation(this._current.encoding);
        }
    }

    writeParamEncaps(v, ok) {
        if (this._response) {
            Debug.assert(this._current.encoding !== null); // Encoding for reply is known.
            this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
            this._os.writeBlob(Protocol.replyHdr);
            this._os.writeInt(this._current.requestId);
            this._os.writeByte(ok ? Protocol.replyOK : Protocol.replyUserException);
            if (v === null || v.length === 0) {
                this._os.writeEmptyEncapsulation(this._current.encoding);
            } else {
                this._os.writeEncapsulation(v);
            }
        }
    }

    setFormat(format) {
        this._format = format;
    }

    warning(ex) {
        Debug.assert(this._instance !== null);

        const s = [];
        s.push("dispatch exception:");
        s.push("\nidentity: " + identityToString(this._current.id, this._instance.toStringMode()));
        s.push("\nfacet: " + StringUtil.escapeString(this._current.facet, "", this._instance.toStringMode()));
        s.push("\noperation: " + this._current.operation);
        if (this._connection !== null) {
            try {
                for (let p = this._connection.getInfo(); p; p = p.underlying) {
                    if (p instanceof IPConnectionInfo) {
                        s.push("\nremote host: " + p.remoteAddress + " remote port: " + p.remotePort);
                    }
                }
            } catch (exc) {
                // Ignore.
            }
        }
        if (ex.stack) {
            s.push("\n");
            s.push(ex.stack);
        }
        this._instance.initializationData().logger.warning(s.join(""));
    }

    handleException(ex, amd) {
        Debug.assert(this._connection !== null);

        const props = this._instance.initializationData().properties;
        if (ex instanceof RequestFailedException) {
            if (ex.id === null) {
                ex.id = this._current.id;
            }

            if (ex.facet === null) {
                ex.facet = this._current.facet;
            }

            if (ex.operation === null || ex.operation.length === 0) {
                ex.operation = this._current.operation;
            }

            if (props.getPropertyAsIntWithDefault("Ice.Warn.Dispatch", 1) > 1) {
                this.warning(ex);
            }

            if (this._response) {
                this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
                this._os.writeBlob(Protocol.replyHdr);
                this._os.writeInt(this._current.requestId);
                if (ex instanceof ObjectNotExistException) {
                    this._os.writeByte(Protocol.replyObjectNotExist);
                } else if (ex instanceof FacetNotExistException) {
                    this._os.writeByte(Protocol.replyFacetNotExist);
                } else if (ex instanceof OperationNotExistException) {
                    this._os.writeByte(Protocol.replyOperationNotExist);
                } else {
                    Debug.assert(false);
                }
                ex.id._write(this._os);

                //
                // For compatibility with the old FacetPath.
                //
                if (ex.facet === null || ex.facet.length === 0) {
                    StringSeqHelper.write(this._os, null);
                } else {
                    StringSeqHelper.write(this._os, [ex.facet]);
                }

                this._os.writeString(ex.operation);

                this._connection.sendResponse(this._os);
            } else {
                this._connection.sendNoResponse();
            }
        } else if (ex instanceof UnknownLocalException) {
            if (props.getPropertyAsIntWithDefault("Ice.Warn.Dispatch", 1) > 0) {
                this.warning(ex);
            }

            if (this._response) {
                this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
                this._os.writeBlob(Protocol.replyHdr);
                this._os.writeInt(this._current.requestId);
                this._os.writeByte(Protocol.replyUnknownLocalException);
                this._os.writeString(ex.unknown);
                this._connection.sendResponse(this._os);
            } else {
                this._connection.sendNoResponse();
            }
        } else if (ex instanceof UnknownUserException) {
            if (props.getPropertyAsIntWithDefault("Ice.Warn.Dispatch", 1) > 0) {
                this.warning(ex);
            }

            if (this._response) {
                this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
                this._os.writeBlob(Protocol.replyHdr);
                this._os.writeInt(this._current.requestId);
                this._os.writeByte(Protocol.replyUnknownUserException);
                this._os.writeString(ex.unknown);
                this._connection.sendResponse(this._os);
            } else {
                this._connection.sendNoResponse();
            }
        } else if (ex instanceof UnknownException) {
            if (props.getPropertyAsIntWithDefault("Ice.Warn.Dispatch", 1) > 0) {
                this.warning(ex);
            }

            if (this._response) {
                this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
                this._os.writeBlob(Protocol.replyHdr);
                this._os.writeInt(this._current.requestId);
                this._os.writeByte(Protocol.replyUnknownException);
                this._os.writeString(ex.unknown);
                this._connection.sendResponse(this._os);
            } else {
                this._connection.sendNoResponse();
            }
        } else if (ex instanceof LocalException) {
            if (props.getPropertyAsIntWithDefault("Ice.Warn.Dispatch", 1) > 0) {
                this.warning(ex);
            }

            if (this._response) {
                this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
                this._os.writeBlob(Protocol.replyHdr);
                this._os.writeInt(this._current.requestId);
                this._os.writeByte(Protocol.replyUnknownLocalException);
                // this._os.writeString(ex.toString());
                const s = [ex.ice_id()];
                if (ex.stack) {
                    s.push("\n");
                    s.push(ex.stack);
                }
                this._os.writeString(s.join(""));
                this._connection.sendResponse(this._os);
            } else {
                this._connection.sendNoResponse();
            }
        } else if (ex instanceof UserException) {
            if (this._response) {
                this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
                this._os.writeBlob(Protocol.replyHdr);
                this._os.writeInt(this._current.requestId);
                this._os.writeByte(Protocol.replyUserException);
                this._os.startEncapsulation(this._current.encoding, this._format);
                this._os.writeException(ex);
                this._os.endEncapsulation();
                this._connection.sendResponse(this._os);
            } else {
                this._connection.sendNoResponse();
            }
        } else {
            if (props.getPropertyAsIntWithDefault("Ice.Warn.Dispatch", 1) > 0) {
                this.warning(ex);
            }

            if (this._response) {
                this._os = new OutputStream(this._instance, Protocol.currentProtocolEncoding);
                this._os.writeBlob(Protocol.replyHdr);
                this._os.writeInt(this._current.requestId);
                this._os.writeByte(Protocol.replyUnknownException);
                this._os.writeString(ex.toString() + (ex.stack ? "\n" + ex.stack : ""));
                this._connection.sendResponse(this._os);
            } else {
                this._connection.sendNoResponse();
            }
        }

        this._connection = null;
    }

    invoke(servantManager, stream) {
        this._is = stream;

        //
        // Read the current.
        //
        this._current.id._read(this._is);

        //
        // For compatibility with the old FacetPath.
        //
        const facetPath = StringSeqHelper.read(this._is);
        if (facetPath.length > 0) {
            if (facetPath.length > 1) {
                throw new MarshalException();
            }
            this._current.facet = facetPath[0];
        } else {
            this._current.facet = "";
        }

        this._current.operation = this._is.readString();
        this._current.mode = OperationMode.valueOf(this._is.readByte());
        this._current.ctx = new Context();
        let sz = this._is.readSize();
        while (sz-- > 0) {
            this._current.ctx.set(this._is.readString(), this._is.readString());
        }

        //
        // Don't put the code above into the try block below. Exceptions
        // in the code above are considered fatal, and must propagate to
        // the caller of this operation.
        //
        if (servantManager !== null) {
            this._servant = servantManager.findServant(this._current.id, this._current.facet);
            if (this._servant === null) {
                this._locator = servantManager.findServantLocator(this._current.id.category);
                if (this._locator === null && this._current.id.category.length > 0) {
                    this._locator = servantManager.findServantLocator("");
                }

                if (this._locator !== null) {
                    try {
                        this._servant = this._locator.locate(this._current, this._cookie);
                    } catch (ex) {
                        this.skipReadParams(); // Required for batch requests.
                        this.handleException(ex, false);
                        return;
                    }
                }
            }
        }

        if (this._servant === null) {
            try {
                if (servantManager !== null && servantManager.hasServant(this._current.id)) {
                    throw new FacetNotExistException(this._current.id, this._current.facet, this._current.operation);
                } else {
                    throw new ObjectNotExistException(this._current.id, this._current.facet, this._current.operation);
                }
            } catch (ex) {
                this.skipReadParams(); // Required for batch requests.
                this.handleException(ex, false);
                return;
            }
        }

        try {
            Debug.assert(this._servant !== null);
            const promise = this._servant._iceDispatch(this, this._current);
            if (promise !== null) {
                promise.then(
                    () => this.completed(null, true),
                    (ex) => this.completed(ex, true),
                );
                return;
            }

            Debug.assert(!this._response || this._os !== null);
            this.completed(null, false);
        } catch (ex) {
            this.completed(ex, false);
        }
    }

    startReadParams() {
        //
        // Remember the encoding used by the input parameters, we'll
        // encode the response parameters with the same encoding.
        //
        this._current.encoding = this._is.startEncapsulation();
        return this._is;
    }

    endReadParams() {
        this._is.endEncapsulation();
    }

    readEmptyParams() {
        this._current.encoding = this._is.skipEmptyEncapsulation();
    }

    readParamEncaps() {
        this._current.encoding = new EncodingVersion();
        return this._is.readEncapsulation(this._current.encoding);
    }

    skipReadParams() {
        this._current.encoding = this._is.skipEncapsulation();
    }

    completed(exc, amd) {
        try {
            if (this._locator !== null) {
                Debug.assert(this._locator !== null && this._servant !== null);
                try {
                    this._locator.finished(this._current, this._servant, this._cookie.value);
                } catch (ex) {
                    this.handleException(ex, amd);
                    return;
                }
            }

            Debug.assert(this._connection !== null);

            if (exc !== null) {
                this.handleException(exc, amd);
            } else if (this._response) {
                this._connection.sendResponse(this._os);
            } else {
                this._connection.sendNoResponse();
            }
        } catch (ex) {
            if (ex instanceof LocalException) {
                this._connection.invokeException(ex, 1);
            } else {
                throw ex;
            }
        }
        this._connection = null;
    }
}
