//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System.Diagnostics;

using ZeroC.Ice.Instrumentation;

namespace ZeroC.Ice
{
    internal static class Incoming
    {
        internal static void ReportException(RemoteException ex,
                                             IDispatchObserver? dispatchObserver,
                                             Current current)
        {
            bool unhandledException = ex is UnhandledException;

            if (unhandledException && current.Adapter.Communicator.WarnDispatch)
            {
                Warning((UnhandledException)ex, current);
            }

            if (dispatchObserver != null)
            {
                if (unhandledException)
                {
                    dispatchObserver.Failed((ex.InnerException ?? ex).GetType().FullName ?? "System.Exception");
                }
                else
                {
                    dispatchObserver.RemoteException();
                }
            }
        }

        private static void Warning(UnhandledException ex, Current current)
        {
            Debug.Assert(current.Adapter.Communicator != null);

            var output = new System.Text.StringBuilder();

            output.Append("dispatch exception:");
            output.Append("\nidentity: ").Append(current.Identity.ToString(current.Communicator.ToStringMode));
            output.Append("\nfacet: ").Append(
                StringUtil.EscapeString(current.Facet, current.Communicator.ToStringMode));
            output.Append("\noperation: ").Append(current.Operation);

            if ((current.Connection as IPConnection)?.RemoteEndpoint is System.Net.IPEndPoint remoteEndpoint)
            {
                output.Append("\nremote address: ").Append(remoteEndpoint);
            }
            output.Append('\n');
            output.Append(ex.ToString());
            current.Adapter.Communicator.Logger.Warning(output.ToString());
        }
    }
}
