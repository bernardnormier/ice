# Copyright (c) ZeroC, Inc.

require './Twoways'
require './Oneways'
require './BatchOneways'

def allTests(helper, communicator)
    ref = "test:#{helper.getTestEndpoint()}"
    cl = Test::MyInterfacePrx.new(communicator, ref)
    derived = Test::MyDerivedInterfacePrx.checkedCast(cl)

    print "testing twoway operations... "
    STDOUT.flush
    twoways(helper, communicator, cl)
    twoways(helper, communicator, derived)
    derived.opDerived()
    puts "ok"

    print "testing oneway operations... "
    STDOUT.flush
    oneways(helper, cl)
    puts "ok"

    print "testing batch oneway operations... "
    STDOUT.flush
    batchOneways(cl)
    batchOneways(derived)
    puts "ok"

    print "testing communicator shutdown from another thread... "
    STDOUT.flush
    initData = Ice::InitializationData.new
    initData.properties = communicator.getProperties().clone()
    ic = Ice::initialize(initData)
    thread = Thread.new do
        sleep(0.2)
        ic.shutdown()
    end
    ic.waitForShutdown()
    thread.join()
    ic.destroy()
    puts "ok"

    return cl
end
