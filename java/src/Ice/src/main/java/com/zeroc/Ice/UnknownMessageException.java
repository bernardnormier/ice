//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `LocalException.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice;

/** This exception indicates that an unknown protocol message has been received. */
public class UnknownMessageException extends ProtocolException {
  public UnknownMessageException() {
    super();
  }

  public UnknownMessageException(Throwable cause) {
    super(cause);
  }

  public UnknownMessageException(String reason) {
    super(reason);
  }

  public UnknownMessageException(String reason, Throwable cause) {
    super(reason, cause);
  }

  public String ice_id() {
    return "::Ice::UnknownMessageException";
  }

  private static final long serialVersionUID = 1625154579332341724L;
}
