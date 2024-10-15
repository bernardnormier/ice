//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.Ice;

import com.zeroc.Ice.IceMX.CollocatedMetrics;
import com.zeroc.Ice.Instrumentation.CollocatedObserver;

/**
 * @hidden Public because it's used by IceMX (via reflection).
 */
public class CollocatedObserverI
        extends com.zeroc.Ice.IceMX.ObserverWithDelegate<CollocatedMetrics, CollocatedObserver>
        implements CollocatedObserver {
    @Override
    public void reply(final int size) {
        forEach(
                new MetricsUpdate<CollocatedMetrics>() {
                    @Override
                    public void update(CollocatedMetrics v) {
                        v.replySize += size;
                    }
                });
        if (_delegate != null) {
            _delegate.reply(size);
        }
    }
}