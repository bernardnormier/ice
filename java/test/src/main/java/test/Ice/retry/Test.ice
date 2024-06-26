//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[["java:package:test.Ice.retry"]]
module Test
{

interface Retry
{
    void op(bool kill);

    idempotent int opIdempotent(int c);
    void opNotIdempotent();

    idempotent void sleep(int delay);

    idempotent void shutdown();
}

}
