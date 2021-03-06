//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
using System;
using System.Diagnostics;

namespace ZeroC.Ice
{
    public sealed class Time
    {
        /// <summary>Gets the total elapsed time since the Ice run-time started as a TimeSpan object.</summary>
        public static TimeSpan Elapsed => _stopwatch.Elapsed;
        static Time() => _stopwatch.Start();

        private static readonly Stopwatch _stopwatch = new Stopwatch();
    }
}
