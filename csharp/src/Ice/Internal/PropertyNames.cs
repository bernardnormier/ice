// Copyright (c) ZeroC, Inc.

// Generated by makeprops.py from PropertyNames.xml, Thu Sep 26 14:57:25 2024

// IMPORTANT: Do not edit this file -- any edits made here will be lost!

namespace Ice.Internal;

public sealed class PropertyNames
{
    public static Property[] IceProps =
    {
    new(@"Ice.AcceptClassCycles", false, "0", false),
    new(@"Ice.Admin.AdapterId", false, "", false),
    new(@"Ice.Admin.Connection.CloseTimeout", false, "10", false),
    new(@"Ice.Admin.Connection.ConnectTimeout", false, "10", false),
    new(@"Ice.Admin.Connection.EnableIdleCheck", false, "1", false),
    new(@"Ice.Admin.Connection.IdleTimeout", false, "60", false),
    new(@"Ice.Admin.Connection.InactivityTimeout", false, "300", false),
    new(@"Ice.Admin.Connection.MaxDispatches", false, "100", false),
    new(@"Ice.Admin.Endpoints", false, "", false),
    new(@"Ice.Admin.Locator.EndpointSelection", false, "", false),
    new(@"Ice.Admin.Locator.ConnectionCached", false, "", false),
    new(@"Ice.Admin.Locator.PreferSecure", false, "", false),
    new(@"Ice.Admin.Locator.LocatorCacheTimeout", false, "", false),
    new(@"Ice.Admin.Locator.InvocationTimeout", false, "", false),
    new(@"Ice.Admin.Locator.Locator", false, "", false),
    new(@"Ice.Admin.Locator.Router", false, "", false),
    new(@"Ice.Admin.Locator.CollocationOptimized", false, "", false),
    new(@"^Ice\.Admin\.Locator\.Context\.[^\s]+$", true, "", false),
    new(@"Ice.Admin.Locator", false, "", false),
    new(@"Ice.Admin.PublishedEndpoints", false, "", false),
    new(@"Ice.Admin.ReplicaGroupId", false, "", false),
    new(@"Ice.Admin.Router.EndpointSelection", false, "", false),
    new(@"Ice.Admin.Router.ConnectionCached", false, "", false),
    new(@"Ice.Admin.Router.PreferSecure", false, "", false),
    new(@"Ice.Admin.Router.LocatorCacheTimeout", false, "", false),
    new(@"Ice.Admin.Router.InvocationTimeout", false, "", false),
    new(@"Ice.Admin.Router.Locator", false, "", false),
    new(@"Ice.Admin.Router.Router", false, "", false),
    new(@"Ice.Admin.Router.CollocationOptimized", false, "", false),
    new(@"^Ice\.Admin\.Router\.Context\.[^\s]+$", true, "", false),
    new(@"Ice.Admin.Router", false, "", false),
    new(@"Ice.Admin.ProxyOptions", false, "", false),
    new(@"Ice.Admin.ThreadPool.Size", false, "1", false),
    new(@"Ice.Admin.ThreadPool.SizeMax", false, "", false),
    new(@"Ice.Admin.ThreadPool.SizeWarn", false, "0", false),
    new(@"Ice.Admin.ThreadPool.StackSize", false, "0", false),
    new(@"Ice.Admin.ThreadPool.Serialize", false, "0", false),
    new(@"Ice.Admin.ThreadPool.ThreadIdleTime", false, "60", false),
    new(@"Ice.Admin.ThreadPool.ThreadPriority", false, "", false),
    new(@"Ice.Admin.MessageSizeMax", false, "", false),
    new(@"Ice.Admin.DelayCreation", false, "0", false),
    new(@"Ice.Admin.Enabled", false, "", false),
    new(@"Ice.Admin.Facets", false, "", false),
    new(@"Ice.Admin.InstanceName", false, "", false),
    new(@"Ice.Admin.Logger.KeepLogs", false, "100", false),
    new(@"Ice.Admin.Logger.KeepTraces", false, "100", false),
    new(@"Ice.Admin.Logger.Properties", false, "", false),
    new(@"Ice.Admin.ServerId", false, "", false),
    new(@"Ice.BackgroundLocatorCacheUpdates", false, "0", false),
    new(@"Ice.BatchAutoFlush", false, "", true),
    new(@"Ice.BatchAutoFlushSize", false, "1024", false),
    new(@"Ice.ClassGraphDepthMax", false, "10", false),
    new(@"Ice.Compression.Level", false, "1", false),
    new(@"Ice.Config", false, "", false),
    new(@"Ice.Connection.Client.CloseTimeout", false, "10", false),
    new(@"Ice.Connection.Client.ConnectTimeout", false, "10", false),
    new(@"Ice.Connection.Client.EnableIdleCheck", false, "1", false),
    new(@"Ice.Connection.Client.IdleTimeout", false, "60", false),
    new(@"Ice.Connection.Client.InactivityTimeout", false, "300", false),
    new(@"Ice.Connection.Client.MaxDispatches", false, "100", false),
    new(@"Ice.Connection.Server.CloseTimeout", false, "10", false),
    new(@"Ice.Connection.Server.ConnectTimeout", false, "10", false),
    new(@"Ice.Connection.Server.EnableIdleCheck", false, "1", false),
    new(@"Ice.Connection.Server.IdleTimeout", false, "60", false),
    new(@"Ice.Connection.Server.InactivityTimeout", false, "300", false),
    new(@"Ice.Connection.Server.MaxDispatches", false, "100", false),
    new(@"Ice.ConsoleListener", false, "1", false),
    new(@"Ice.Default.CollocationOptimized", false, "1", false),
    new(@"Ice.Default.EncodingVersion", false, "1.1", false),
    new(@"Ice.Default.EndpointSelection", false, "Random", false),
    new(@"Ice.Default.Host", false, "", false),
    new(@"Ice.Default.Locator.EndpointSelection", false, "", false),
    new(@"Ice.Default.Locator.ConnectionCached", false, "", false),
    new(@"Ice.Default.Locator.PreferSecure", false, "", false),
    new(@"Ice.Default.Locator.LocatorCacheTimeout", false, "", false),
    new(@"Ice.Default.Locator.InvocationTimeout", false, "", false),
    new(@"Ice.Default.Locator.Locator", false, "", false),
    new(@"Ice.Default.Locator.Router", false, "", false),
    new(@"Ice.Default.Locator.CollocationOptimized", false, "", false),
    new(@"^Ice\.Default\.Locator\.Context\.[^\s]+$", true, "", false),
    new(@"Ice.Default.Locator", false, "", false),
    new(@"Ice.Default.LocatorCacheTimeout", false, "-1", false),
    new(@"Ice.Default.InvocationTimeout", false, "-1", false),
    new(@"Ice.Default.PreferSecure", false, "0", false),
    new(@"Ice.Default.Protocol", false, "tcp", false),
    new(@"Ice.Default.Router.EndpointSelection", false, "", false),
    new(@"Ice.Default.Router.ConnectionCached", false, "", false),
    new(@"Ice.Default.Router.PreferSecure", false, "", false),
    new(@"Ice.Default.Router.LocatorCacheTimeout", false, "", false),
    new(@"Ice.Default.Router.InvocationTimeout", false, "", false),
    new(@"Ice.Default.Router.Locator", false, "", false),
    new(@"Ice.Default.Router.Router", false, "", false),
    new(@"Ice.Default.Router.CollocationOptimized", false, "", false),
    new(@"^Ice\.Default\.Router\.Context\.[^\s]+$", true, "", false),
    new(@"Ice.Default.Router", false, "", false),
    new(@"Ice.Default.SlicedFormat", false, "0", false),
    new(@"Ice.Default.SourceAddress", false, "", false),
    new(@"Ice.HTTPProxyHost", false, "", false),
    new(@"Ice.HTTPProxyPort", false, "1080", false),
    new(@"Ice.ImplicitContext", false, "None", false),
    new(@"Ice.InitPlugins", false, "1", false),
    new(@"Ice.IPv4", false, "1", false),
    new(@"Ice.IPv6", false, "1", false),
    new(@"Ice.LogFile", false, "", false),
    new(@"Ice.MessageSizeMax", false, "1024", false),
    new(@"Ice.Override.Compress", false, "", false),
    new(@"Ice.Override.Secure", false, "", false),
    new(@"^Ice\.Plugin\.[^\s]+$", true, "", false),
    new(@"Ice.PluginLoadOrder", false, "", false),
    new(@"Ice.PreferIPv6Address", false, "0", false),
    new(@"Ice.PreloadAssemblies", false, "0", false),
    new(@"Ice.PrintAdapterReady", false, "", false),
    new(@"Ice.PrintProcessId", false, "", false),
    new(@"Ice.ProgramName", false, "", false),
    new(@"Ice.RetryIntervals", false, "0", false),
    new(@"Ice.ServerIdleTime", false, "0", false),
    new(@"Ice.SOCKSProxyHost", false, "", false),
    new(@"Ice.SOCKSProxyPort", false, "1080", false),
    new(@"Ice.StdErr", false, "", false),
    new(@"Ice.StdOut", false, "", false),
    new(@"Ice.ThreadPool.Client.Size", false, "1", false),
    new(@"Ice.ThreadPool.Client.SizeMax", false, "", false),
    new(@"Ice.ThreadPool.Client.SizeWarn", false, "0", false),
    new(@"Ice.ThreadPool.Client.StackSize", false, "0", false),
    new(@"Ice.ThreadPool.Client.Serialize", false, "0", false),
    new(@"Ice.ThreadPool.Client.ThreadIdleTime", false, "60", false),
    new(@"Ice.ThreadPool.Client.ThreadPriority", false, "", false),
    new(@"Ice.ThreadPool.Server.Size", false, "1", false),
    new(@"Ice.ThreadPool.Server.SizeMax", false, "", false),
    new(@"Ice.ThreadPool.Server.SizeWarn", false, "0", false),
    new(@"Ice.ThreadPool.Server.StackSize", false, "0", false),
    new(@"Ice.ThreadPool.Server.Serialize", false, "0", false),
    new(@"Ice.ThreadPool.Server.ThreadIdleTime", false, "60", false),
    new(@"Ice.ThreadPool.Server.ThreadPriority", false, "", false),
    new(@"Ice.ThreadPriority", false, "", false),
    new(@"Ice.ToStringMode", false, "Unicode", false),
    new(@"Ice.Trace.Admin.Properties", false, "0", false),
    new(@"Ice.Trace.Admin.Logger", false, "0", false),
    new(@"Ice.Trace.Locator", false, "0", false),
    new(@"Ice.Trace.Network", false, "0", false),
    new(@"Ice.Trace.Protocol", false, "0", false),
    new(@"Ice.Trace.Retry", false, "0", false),
    new(@"Ice.Trace.Slicing", false, "0", false),
    new(@"Ice.Trace.ThreadPool", false, "0", false),
    new(@"Ice.UDP.RcvSize", false, "", false),
    new(@"Ice.UDP.SndSize", false, "", false),
    new(@"Ice.TCP.Backlog", false, "", false),
    new(@"Ice.TCP.RcvSize", false, "", false),
    new(@"Ice.TCP.SndSize", false, "", false),
    new(@"Ice.Warn.AMICallback", false, "1", false),
    new(@"Ice.Warn.Connections", false, "0", false),
    new(@"Ice.Warn.Datagrams", false, "0", false),
    new(@"Ice.Warn.Dispatch", false, "1", false),
    new(@"Ice.Warn.Endpoints", false, "1", false),
    new(@"Ice.Warn.UnknownProperties", false, "1", false),
    new(@"Ice.Warn.UnusedProperties", false, "0", false),
    new(@"Ice.CacheMessageBuffers", false, "2", false),
    };

    public static Property[] IceMXProps =
    {
    new(@"^IceMX\.Metrics\.[^\s]+\.GroupBy$", true, "", false),
    new(@"^IceMX\.Metrics\.[^\s]+\.Map$", true, "", false),
    new(@"^IceMX\.Metrics\.[^\s]+\.RetainDetached$", true, "10", false),
    new(@"^IceMX\.Metrics\.[^\s]+\.Accept$", true, "", false),
    new(@"^IceMX\.Metrics\.[^\s]+\.Reject$", true, "", false),
    new(@"^IceMX\.Metrics\.[^\s]+$", true, "", false),
    };

    public static Property[] IceDiscoveryProps =
    {
    new(@"IceDiscovery.Multicast.AdapterId", false, "", false),
    new(@"IceDiscovery.Multicast.Connection.CloseTimeout", false, "10", false),
    new(@"IceDiscovery.Multicast.Connection.ConnectTimeout", false, "10", false),
    new(@"IceDiscovery.Multicast.Connection.EnableIdleCheck", false, "1", false),
    new(@"IceDiscovery.Multicast.Connection.IdleTimeout", false, "60", false),
    new(@"IceDiscovery.Multicast.Connection.InactivityTimeout", false, "300", false),
    new(@"IceDiscovery.Multicast.Connection.MaxDispatches", false, "100", false),
    new(@"IceDiscovery.Multicast.Endpoints", false, "", false),
    new(@"IceDiscovery.Multicast.Locator.EndpointSelection", false, "", false),
    new(@"IceDiscovery.Multicast.Locator.ConnectionCached", false, "", false),
    new(@"IceDiscovery.Multicast.Locator.PreferSecure", false, "", false),
    new(@"IceDiscovery.Multicast.Locator.LocatorCacheTimeout", false, "", false),
    new(@"IceDiscovery.Multicast.Locator.InvocationTimeout", false, "", false),
    new(@"IceDiscovery.Multicast.Locator.Locator", false, "", false),
    new(@"IceDiscovery.Multicast.Locator.Router", false, "", false),
    new(@"IceDiscovery.Multicast.Locator.CollocationOptimized", false, "", false),
    new(@"^IceDiscovery\.Multicast\.Locator\.Context\.[^\s]+$", true, "", false),
    new(@"IceDiscovery.Multicast.Locator", false, "", false),
    new(@"IceDiscovery.Multicast.PublishedEndpoints", false, "", false),
    new(@"IceDiscovery.Multicast.ReplicaGroupId", false, "", false),
    new(@"IceDiscovery.Multicast.Router.EndpointSelection", false, "", false),
    new(@"IceDiscovery.Multicast.Router.ConnectionCached", false, "", false),
    new(@"IceDiscovery.Multicast.Router.PreferSecure", false, "", false),
    new(@"IceDiscovery.Multicast.Router.LocatorCacheTimeout", false, "", false),
    new(@"IceDiscovery.Multicast.Router.InvocationTimeout", false, "", false),
    new(@"IceDiscovery.Multicast.Router.Locator", false, "", false),
    new(@"IceDiscovery.Multicast.Router.Router", false, "", false),
    new(@"IceDiscovery.Multicast.Router.CollocationOptimized", false, "", false),
    new(@"^IceDiscovery\.Multicast\.Router\.Context\.[^\s]+$", true, "", false),
    new(@"IceDiscovery.Multicast.Router", false, "", false),
    new(@"IceDiscovery.Multicast.ProxyOptions", false, "", false),
    new(@"IceDiscovery.Multicast.ThreadPool.Size", false, "1", false),
    new(@"IceDiscovery.Multicast.ThreadPool.SizeMax", false, "", false),
    new(@"IceDiscovery.Multicast.ThreadPool.SizeWarn", false, "0", false),
    new(@"IceDiscovery.Multicast.ThreadPool.StackSize", false, "0", false),
    new(@"IceDiscovery.Multicast.ThreadPool.Serialize", false, "0", false),
    new(@"IceDiscovery.Multicast.ThreadPool.ThreadIdleTime", false, "60", false),
    new(@"IceDiscovery.Multicast.ThreadPool.ThreadPriority", false, "", false),
    new(@"IceDiscovery.Multicast.MessageSizeMax", false, "", false),
    new(@"IceDiscovery.Reply.AdapterId", false, "", false),
    new(@"IceDiscovery.Reply.Connection.CloseTimeout", false, "10", false),
    new(@"IceDiscovery.Reply.Connection.ConnectTimeout", false, "10", false),
    new(@"IceDiscovery.Reply.Connection.EnableIdleCheck", false, "1", false),
    new(@"IceDiscovery.Reply.Connection.IdleTimeout", false, "60", false),
    new(@"IceDiscovery.Reply.Connection.InactivityTimeout", false, "300", false),
    new(@"IceDiscovery.Reply.Connection.MaxDispatches", false, "100", false),
    new(@"IceDiscovery.Reply.Endpoints", false, "", false),
    new(@"IceDiscovery.Reply.Locator.EndpointSelection", false, "", false),
    new(@"IceDiscovery.Reply.Locator.ConnectionCached", false, "", false),
    new(@"IceDiscovery.Reply.Locator.PreferSecure", false, "", false),
    new(@"IceDiscovery.Reply.Locator.LocatorCacheTimeout", false, "", false),
    new(@"IceDiscovery.Reply.Locator.InvocationTimeout", false, "", false),
    new(@"IceDiscovery.Reply.Locator.Locator", false, "", false),
    new(@"IceDiscovery.Reply.Locator.Router", false, "", false),
    new(@"IceDiscovery.Reply.Locator.CollocationOptimized", false, "", false),
    new(@"^IceDiscovery\.Reply\.Locator\.Context\.[^\s]+$", true, "", false),
    new(@"IceDiscovery.Reply.Locator", false, "", false),
    new(@"IceDiscovery.Reply.PublishedEndpoints", false, "", false),
    new(@"IceDiscovery.Reply.ReplicaGroupId", false, "", false),
    new(@"IceDiscovery.Reply.Router.EndpointSelection", false, "", false),
    new(@"IceDiscovery.Reply.Router.ConnectionCached", false, "", false),
    new(@"IceDiscovery.Reply.Router.PreferSecure", false, "", false),
    new(@"IceDiscovery.Reply.Router.LocatorCacheTimeout", false, "", false),
    new(@"IceDiscovery.Reply.Router.InvocationTimeout", false, "", false),
    new(@"IceDiscovery.Reply.Router.Locator", false, "", false),
    new(@"IceDiscovery.Reply.Router.Router", false, "", false),
    new(@"IceDiscovery.Reply.Router.CollocationOptimized", false, "", false),
    new(@"^IceDiscovery\.Reply\.Router\.Context\.[^\s]+$", true, "", false),
    new(@"IceDiscovery.Reply.Router", false, "", false),
    new(@"IceDiscovery.Reply.ProxyOptions", false, "", false),
    new(@"IceDiscovery.Reply.ThreadPool.Size", false, "1", false),
    new(@"IceDiscovery.Reply.ThreadPool.SizeMax", false, "", false),
    new(@"IceDiscovery.Reply.ThreadPool.SizeWarn", false, "0", false),
    new(@"IceDiscovery.Reply.ThreadPool.StackSize", false, "0", false),
    new(@"IceDiscovery.Reply.ThreadPool.Serialize", false, "0", false),
    new(@"IceDiscovery.Reply.ThreadPool.ThreadIdleTime", false, "60", false),
    new(@"IceDiscovery.Reply.ThreadPool.ThreadPriority", false, "", false),
    new(@"IceDiscovery.Reply.MessageSizeMax", false, "", false),
    new(@"IceDiscovery.Locator.AdapterId", false, "", false),
    new(@"IceDiscovery.Locator.Connection.CloseTimeout", false, "10", false),
    new(@"IceDiscovery.Locator.Connection.ConnectTimeout", false, "10", false),
    new(@"IceDiscovery.Locator.Connection.EnableIdleCheck", false, "1", false),
    new(@"IceDiscovery.Locator.Connection.IdleTimeout", false, "60", false),
    new(@"IceDiscovery.Locator.Connection.InactivityTimeout", false, "300", false),
    new(@"IceDiscovery.Locator.Connection.MaxDispatches", false, "100", false),
    new(@"IceDiscovery.Locator.Endpoints", false, "", false),
    new(@"IceDiscovery.Locator.Locator.EndpointSelection", false, "", false),
    new(@"IceDiscovery.Locator.Locator.ConnectionCached", false, "", false),
    new(@"IceDiscovery.Locator.Locator.PreferSecure", false, "", false),
    new(@"IceDiscovery.Locator.Locator.LocatorCacheTimeout", false, "", false),
    new(@"IceDiscovery.Locator.Locator.InvocationTimeout", false, "", false),
    new(@"IceDiscovery.Locator.Locator.Locator", false, "", false),
    new(@"IceDiscovery.Locator.Locator.Router", false, "", false),
    new(@"IceDiscovery.Locator.Locator.CollocationOptimized", false, "", false),
    new(@"^IceDiscovery\.Locator\.Locator\.Context\.[^\s]+$", true, "", false),
    new(@"IceDiscovery.Locator.Locator", false, "", false),
    new(@"IceDiscovery.Locator.PublishedEndpoints", false, "", false),
    new(@"IceDiscovery.Locator.ReplicaGroupId", false, "", false),
    new(@"IceDiscovery.Locator.Router.EndpointSelection", false, "", false),
    new(@"IceDiscovery.Locator.Router.ConnectionCached", false, "", false),
    new(@"IceDiscovery.Locator.Router.PreferSecure", false, "", false),
    new(@"IceDiscovery.Locator.Router.LocatorCacheTimeout", false, "", false),
    new(@"IceDiscovery.Locator.Router.InvocationTimeout", false, "", false),
    new(@"IceDiscovery.Locator.Router.Locator", false, "", false),
    new(@"IceDiscovery.Locator.Router.Router", false, "", false),
    new(@"IceDiscovery.Locator.Router.CollocationOptimized", false, "", false),
    new(@"^IceDiscovery\.Locator\.Router\.Context\.[^\s]+$", true, "", false),
    new(@"IceDiscovery.Locator.Router", false, "", false),
    new(@"IceDiscovery.Locator.ProxyOptions", false, "", false),
    new(@"IceDiscovery.Locator.ThreadPool.Size", false, "1", false),
    new(@"IceDiscovery.Locator.ThreadPool.SizeMax", false, "", false),
    new(@"IceDiscovery.Locator.ThreadPool.SizeWarn", false, "0", false),
    new(@"IceDiscovery.Locator.ThreadPool.StackSize", false, "0", false),
    new(@"IceDiscovery.Locator.ThreadPool.Serialize", false, "0", false),
    new(@"IceDiscovery.Locator.ThreadPool.ThreadIdleTime", false, "60", false),
    new(@"IceDiscovery.Locator.ThreadPool.ThreadPriority", false, "", false),
    new(@"IceDiscovery.Locator.MessageSizeMax", false, "", false),
    new(@"IceDiscovery.Lookup", false, "", false),
    new(@"IceDiscovery.Timeout", false, "300", false),
    new(@"IceDiscovery.RetryCount", false, "3", false),
    new(@"IceDiscovery.LatencyMultiplier", false, "1", false),
    new(@"IceDiscovery.Address", false, "", false),
    new(@"IceDiscovery.Port", false, "4061", false),
    new(@"IceDiscovery.Interface", false, "", false),
    new(@"IceDiscovery.DomainId", false, "", false),
    };

    public static Property[] IceLocatorDiscoveryProps =
    {
    new(@"IceLocatorDiscovery.Reply.AdapterId", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Connection.CloseTimeout", false, "10", false),
    new(@"IceLocatorDiscovery.Reply.Connection.ConnectTimeout", false, "10", false),
    new(@"IceLocatorDiscovery.Reply.Connection.EnableIdleCheck", false, "1", false),
    new(@"IceLocatorDiscovery.Reply.Connection.IdleTimeout", false, "60", false),
    new(@"IceLocatorDiscovery.Reply.Connection.InactivityTimeout", false, "300", false),
    new(@"IceLocatorDiscovery.Reply.Connection.MaxDispatches", false, "100", false),
    new(@"IceLocatorDiscovery.Reply.Endpoints", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator.EndpointSelection", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator.ConnectionCached", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator.PreferSecure", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator.LocatorCacheTimeout", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator.InvocationTimeout", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator.Locator", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator.Router", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator.CollocationOptimized", false, "", false),
    new(@"^IceLocatorDiscovery\.Reply\.Locator\.Context\.[^\s]+$", true, "", false),
    new(@"IceLocatorDiscovery.Reply.Locator", false, "", false),
    new(@"IceLocatorDiscovery.Reply.PublishedEndpoints", false, "", false),
    new(@"IceLocatorDiscovery.Reply.ReplicaGroupId", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Router.EndpointSelection", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Router.ConnectionCached", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Router.PreferSecure", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Router.LocatorCacheTimeout", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Router.InvocationTimeout", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Router.Locator", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Router.Router", false, "", false),
    new(@"IceLocatorDiscovery.Reply.Router.CollocationOptimized", false, "", false),
    new(@"^IceLocatorDiscovery\.Reply\.Router\.Context\.[^\s]+$", true, "", false),
    new(@"IceLocatorDiscovery.Reply.Router", false, "", false),
    new(@"IceLocatorDiscovery.Reply.ProxyOptions", false, "", false),
    new(@"IceLocatorDiscovery.Reply.ThreadPool.Size", false, "1", false),
    new(@"IceLocatorDiscovery.Reply.ThreadPool.SizeMax", false, "", false),
    new(@"IceLocatorDiscovery.Reply.ThreadPool.SizeWarn", false, "0", false),
    new(@"IceLocatorDiscovery.Reply.ThreadPool.StackSize", false, "0", false),
    new(@"IceLocatorDiscovery.Reply.ThreadPool.Serialize", false, "0", false),
    new(@"IceLocatorDiscovery.Reply.ThreadPool.ThreadIdleTime", false, "60", false),
    new(@"IceLocatorDiscovery.Reply.ThreadPool.ThreadPriority", false, "", false),
    new(@"IceLocatorDiscovery.Reply.MessageSizeMax", false, "", false),
    new(@"IceLocatorDiscovery.Locator.AdapterId", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Connection.CloseTimeout", false, "10", false),
    new(@"IceLocatorDiscovery.Locator.Connection.ConnectTimeout", false, "10", false),
    new(@"IceLocatorDiscovery.Locator.Connection.EnableIdleCheck", false, "1", false),
    new(@"IceLocatorDiscovery.Locator.Connection.IdleTimeout", false, "60", false),
    new(@"IceLocatorDiscovery.Locator.Connection.InactivityTimeout", false, "300", false),
    new(@"IceLocatorDiscovery.Locator.Connection.MaxDispatches", false, "100", false),
    new(@"IceLocatorDiscovery.Locator.Endpoints", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator.EndpointSelection", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator.ConnectionCached", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator.PreferSecure", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator.LocatorCacheTimeout", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator.InvocationTimeout", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator.Locator", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator.Router", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator.CollocationOptimized", false, "", false),
    new(@"^IceLocatorDiscovery\.Locator\.Locator\.Context\.[^\s]+$", true, "", false),
    new(@"IceLocatorDiscovery.Locator.Locator", false, "", false),
    new(@"IceLocatorDiscovery.Locator.PublishedEndpoints", false, "", false),
    new(@"IceLocatorDiscovery.Locator.ReplicaGroupId", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Router.EndpointSelection", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Router.ConnectionCached", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Router.PreferSecure", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Router.LocatorCacheTimeout", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Router.InvocationTimeout", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Router.Locator", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Router.Router", false, "", false),
    new(@"IceLocatorDiscovery.Locator.Router.CollocationOptimized", false, "", false),
    new(@"^IceLocatorDiscovery\.Locator\.Router\.Context\.[^\s]+$", true, "", false),
    new(@"IceLocatorDiscovery.Locator.Router", false, "", false),
    new(@"IceLocatorDiscovery.Locator.ProxyOptions", false, "", false),
    new(@"IceLocatorDiscovery.Locator.ThreadPool.Size", false, "1", false),
    new(@"IceLocatorDiscovery.Locator.ThreadPool.SizeMax", false, "", false),
    new(@"IceLocatorDiscovery.Locator.ThreadPool.SizeWarn", false, "0", false),
    new(@"IceLocatorDiscovery.Locator.ThreadPool.StackSize", false, "0", false),
    new(@"IceLocatorDiscovery.Locator.ThreadPool.Serialize", false, "0", false),
    new(@"IceLocatorDiscovery.Locator.ThreadPool.ThreadIdleTime", false, "60", false),
    new(@"IceLocatorDiscovery.Locator.ThreadPool.ThreadPriority", false, "", false),
    new(@"IceLocatorDiscovery.Locator.MessageSizeMax", false, "", false),
    new(@"IceLocatorDiscovery.Lookup", false, "", false),
    new(@"IceLocatorDiscovery.Timeout", false, "300", false),
    new(@"IceLocatorDiscovery.RetryCount", false, "3", false),
    new(@"IceLocatorDiscovery.RetryDelay", false, "2000", false),
    new(@"IceLocatorDiscovery.Address", false, "", false),
    new(@"IceLocatorDiscovery.Port", false, "4061", false),
    new(@"IceLocatorDiscovery.Interface", false, "", false),
    new(@"IceLocatorDiscovery.InstanceName", false, "IceLocatorDiscovery", false),
    new(@"IceLocatorDiscovery.Trace.Lookup", false, "0", false),
    };

    public static Property[] IceBoxProps =
    {
    new(@"IceBox.InheritProperties", false, "", false),
    new(@"IceBox.LoadOrder", false, "", false),
    new(@"IceBox.PrintServicesReady", false, "", false),
    new(@"^IceBox\.Service\.[^\s]+$", true, "", false),
    new(@"IceBox.Trace.ServiceObserver", false, "", false),
    new(@"^IceBox\.UseSharedCommunicator\.[^\s]+$", true, "", false),
    };

    public static Property[] IceSSLProps =
    {
    new(@"IceSSL.CAs", false, "", false),
    new(@"IceSSL.CertStore", false, "My", false),
    new(@"IceSSL.CertStoreLocation", false, "CurrentUser", false),
    new(@"IceSSL.CertFile", false, "", false),
    new(@"IceSSL.CheckCertName", false, "0", false),
    new(@"IceSSL.CheckCRL", false, "0", false),
    new(@"IceSSL.DefaultDir", false, "", false),
    new(@"IceSSL.FindCert", false, "", false),
    new(@"IceSSL.Password", false, "", false),
    new(@"IceSSL.Trace.Security", false, "0", false),
    new(@"IceSSL.TrustOnly", false, "", false),
    new(@"IceSSL.TrustOnly.Client", false, "", false),
    new(@"IceSSL.TrustOnly.Server", false, "", false),
    new(@"^IceSSL\.TrustOnly\.Server\.[^\s]+$", true, "", false),
    new(@"IceSSL.UsePlatformCAs", false, "0", false),
    new(@"IceSSL.VerifyPeer", false, "2", false),
    };

    public static Property[][] validProps =
    {
        IceProps,
        IceMXProps,
        IceDiscoveryProps,
        IceLocatorDiscoveryProps,
        IceBoxProps,
        IceSSLProps,
    };

    public static string[] clPropNames =
    {
        "Ice",
        "IceMX",
        "IceDiscovery",
        "IceLocatorDiscovery",
        "IceBox",
        "IceBoxAdmin",
        "IceBridge",
        "IceGridAdmin",
        "IceGrid",
        "IceSSL",
        "IceStormAdmin",
        "IceBT",
        "Glacier2",
    };
}
