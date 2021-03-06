//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

#include <Test.ice>

module Test
{

exception UnknownDerived : Base
{
    string ud;
}

exception UnknownIntermediate : Base
{
   string ui;
}

exception UnknownMostDerived1 : KnownIntermediate
{
   string umd1;
}

exception UnknownMostDerived2 : UnknownIntermediate
{
   string umd2;
}

class SPreservedClass : BaseClass
{
    string spc;
}

exception SPreserved1 : KnownPreservedDerived
{
    BaseClass p1;
}

exception SPreserved2 : SPreserved1
{
    BaseClass p2;
}

}
