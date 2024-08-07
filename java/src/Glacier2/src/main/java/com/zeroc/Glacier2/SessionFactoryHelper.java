//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Glacier2;

import com.zeroc.Ice.InitializationData;
import com.zeroc.Ice.InitializationException;
import com.zeroc.Ice.Properties;
import com.zeroc.Ice.Util;

/**
 * A helper class for using Glacier2 with GUI applications.
 *
 * <p>Applications should create a session factory for each Glacier2 router to which the application
 * will connect. To connect with the Glacier2 router, call {@link SessionFactoryHelper#connect}. The
 * callback object is notified of the various life cycle events. Once the session is torn down for
 * whatever reason, the application can use the session factory to create another connection.
 */
public class SessionFactoryHelper {
  /**
   * Creates a SessionFactory object.
   *
   * @param callback The callback object for notifications.
   * @throws com.zeroc.Ice.InitializationException If a failure occurred while initializing the
   *     communicator.
   */
  public SessionFactoryHelper(SessionCallback callback) throws InitializationException {
    initialize(callback, new InitializationData(), new Properties());
  }

  /**
   * Creates a SessionFactory object.
   *
   * @param initData The initialization data to use when creating the communicator.
   * @param callback The callback object for notifications.
   * @throws com.zeroc.Ice.InitializationException If a failure occurred while initializing the
   *     communicator.
   */
  public SessionFactoryHelper(InitializationData initData, SessionCallback callback)
      throws InitializationException {
    initialize(callback, initData, initData.properties);
  }

  /**
   * Creates a SessionFactory object.
   *
   * @param properties The properties to use when creating the communicator.
   * @param callback The callback object for notifications.
   * @throws com.zeroc.Ice.InitializationException If a failure occurred while initializing the
   *     communicator.
   */
  public SessionFactoryHelper(Properties properties, SessionCallback callback)
      throws InitializationException {
    initialize(callback, new InitializationData(), properties);
  }

  private void initialize(
      SessionCallback callback, InitializationData initData, Properties properties)
      throws InitializationException {
    if (callback == null) {
      throw new InitializationException(
          "Attempt to create a SessionFactoryHelper with a null " + "SessionCallback argument");
    }

    if (initData == null) {
      throw new InitializationException(
          "Attempt to create a SessionFactoryHelper with a null " + "InitializationData argument");
    }

    if (properties == null) {
      throw new InitializationException(
          "Attempt to create a SessionFactoryHelper with a null Properties " + "argument");
    }

    _callback = callback;
    _initData = initData;
    _initData.properties = properties;

    //
    // Set default properties;
    //
    _initData.properties.setProperty("Ice.RetryIntervals", "-1");
  }

  /**
   * Set the router object identity.
   *
   * @param identity The Glacier2 router's identity.
   */
  public synchronized void setRouterIdentity(com.zeroc.Ice.Identity identity) {
    _identity = identity;
  }

  /**
   * Returns the object identity of the Glacier2 router.
   *
   * @return The Glacier2 router's identity.
   */
  public synchronized com.zeroc.Ice.Identity getRouterIdentity() {
    return _identity;
  }

  /**
   * Sets the host on which the Glacier2 router runs.
   *
   * @param hostname The host name (or IP address) of the router host.
   */
  public synchronized void setRouterHost(String hostname) {
    _routerHost = hostname;
  }

  /**
   * Returns the host on which the Glacier2 router runs.
   *
   * @return The Glacier2 router host.
   */
  public synchronized String getRouterHost() {
    return _routerHost;
  }

  /**
   * Sets whether to connect with the Glacier2 router securely.
   *
   * @param secure If <code>true</code>, the client connects to the router via SSL; otherwise, the
   *     client connects via TCP.
   * @deprecated deprecated, use SessionFactoryHelper.setProtocol instead
   */
  @Deprecated
  public void setSecure(boolean secure) {
    setProtocol(secure ? "ssl" : "tcp");
  }

  /**
   * Returns whether the session factory will establish a secure connection to the Glacier2 router.
   *
   * @return The secure flag.
   * @deprecated deprecated, use SessionFactoryHelper.getProtocol instead
   */
  @Deprecated
  public boolean getSecure() {
    return getProtocol().equals("ssl");
  }

  /**
   * Sets the protocol that will be used by the session factory to establish the connection.
   *
   * @param protocol The communication protocol.
   */
  public synchronized void setProtocol(String protocol) {
    if (protocol == null) {
      throw new IllegalArgumentException("You must use a valid protocol");
    }

    if (!protocol.equals("tcp")
        && !protocol.equals("ssl")
        && !protocol.equals("wss")
        && !protocol.equals("ws")) {
      throw new IllegalArgumentException("Unknown protocol `" + protocol + "'");
    }

    _protocol = protocol;
  }

  /**
   * Returns the protocol that will be used by the session factory to establish the connection.
   *
   * @return The protocol.
   */
  public synchronized String getProtocol() {
    return _protocol;
  }

  /**
   * Sets the Glacier2 router port to connect to.
   *
   * @param port The port. If 0, then the default port (4063 for TCP or 4064 for SSL) is used.
   */
  public synchronized void setPort(int port) {
    _port = port;
  }

  /**
   * Returns the Glacier2 router port to connect to.
   *
   * @return The port.
   */
  public synchronized int getPort() {
    return getPortInternal();
  }

  private int getPortInternal() {
    return _port == 0
        ? ((_protocol.equals("ssl") || _protocol.equals("wss"))
            ? GLACIER2_SSL_PORT
            : GLACIER2_TCP_PORT)
        : _port;
  }

  /**
   * Returns the initialization data used to initialize the communicator.
   *
   * @return The initialization data.
   */
  public synchronized InitializationData getInitializationData() {
    return _initData;
  }

  /**
   * Sets the request context to use while establishing a connection to the Glacier2 router.
   *
   * @param context The request context.
   */
  public synchronized void setConnectContext(final java.util.Map<String, String> context) {
    _context = context;
  }

  /**
   * Determines whether the session should create an object adapter that the client can use for
   * receiving callbacks.
   *
   * @param useCallbacks True if the session should create an object adapter.
   */
  public synchronized void setUseCallbacks(boolean useCallbacks) {
    _useCallbacks = useCallbacks;
  }

  /**
   * Indicates whether a newly-created session will also create an object adapter that the client
   * can use for receiving callbacks.
   *
   * @return True if the session will create an object adapter.
   */
  public synchronized boolean getUseCallbacks() {
    return _useCallbacks;
  }

  /**
   * Connects to the Glacier2 router using the associated SSL credentials.
   *
   * <p>Once the connection is established, {@link SessionCallback#connected} is called on the
   * callback object; upon failure, {@link SessionCallback#connectFailed} is called with the
   * exception.
   *
   * @return The connected session.
   */
  public synchronized SessionHelper connect() {
    SessionHelper session =
        new SessionHelper(_callback, createInitData(), getRouterFinderStr(), _useCallbacks);
    session.connect(_context);
    return session;
  }

  /**
   * Connect the Glacier2 session using user name and password credentials.
   *
   * <p>Once the connection is established, {@link SessionCallback#connected} is called on the
   * callback object; upon failure, {@link SessionCallback#connectFailed} is called with the
   * exception.
   *
   * @param username The user name.
   * @param password The password.
   * @return The connected session.
   */
  public synchronized SessionHelper connect(final String username, final String password) {
    SessionHelper session =
        new SessionHelper(_callback, createInitData(), getRouterFinderStr(), _useCallbacks);
    session.connect(username, password, _context);
    return session;
  }

  private InitializationData createInitData() {
    //
    // Clone the initialization data and properties.
    //
    InitializationData initData = _initData.clone();
    initData.properties = initData.properties._clone();

    if (initData.properties.getProperty("Ice.Default.Router").isEmpty() && _identity != null) {
      initData.properties.setProperty("Ice.Default.Router", getProxyStr(_identity));
    }
    return initData;
  }

  private String getRouterFinderStr() {
    com.zeroc.Ice.Identity ident = new com.zeroc.Ice.Identity("RouterFinder", "Ice");
    return getProxyStr(ident);
  }

  private String getProxyStr(com.zeroc.Ice.Identity ident) {
    StringBuilder sb = new StringBuilder();
    sb.append("\"");
    sb.append(Util.identityToString(ident, com.zeroc.Ice.ToStringMode.Unicode));
    sb.append("\":");
    sb.append(_protocol + " -p ");
    sb.append(getPortInternal());
    sb.append(" -h \"");
    sb.append(_routerHost);
    sb.append("\"");
    return sb.toString();
  }

  private SessionCallback _callback;
  private String _routerHost = "localhost";
  private InitializationData _initData;
  private com.zeroc.Ice.Identity _identity = null;
  private String _protocol = "ssl";
  private int _port = 0;
  private java.util.Map<String, String> _context;
  private boolean _useCallbacks = true;
  private static final int GLACIER2_SSL_PORT = 4064;
  private static final int GLACIER2_TCP_PORT = 4063;
}
