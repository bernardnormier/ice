//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef TEST_I_H
#define TEST_I_H

#include "Test.h"

class TestI : public ::Test::TestIntf
{
public:
    TestI(const Ice::PropertiesPtr&);

    virtual std::string getReplicaId(const Ice::Current&);
    virtual std::string getReplicaIdAndShutdown(const Ice::Current&);

private:
    Ice::PropertiesPtr _properties;
};

#endif
