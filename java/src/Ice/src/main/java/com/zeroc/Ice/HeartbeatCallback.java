//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `Connection.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice;

/**
 * An application can implement this interface to receive notifications when a connection receives a
 * heartbeat message.
 *
 * @see Connection#setHeartbeatCallback
 */
@FunctionalInterface
public interface HeartbeatCallback {
  /**
   * This method is called by the connection when a heartbeat is received from the peer.
   *
   * @param con The connection on which a heartbeat was received.
   */
  void heartbeat(Connection con);
}
