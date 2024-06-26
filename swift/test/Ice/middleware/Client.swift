// Copyright (c) ZeroC, Inc.

import Ice
import TestCommon

class Client: TestHelperI {
    override public func run(args: [String]) throws {
        let communicator = try initialize(args)
        defer {
            communicator.destroy()
        }
        try allTests(self)
    }
}
