//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

// Test: use DoubleModuleWithPackage types from (same) double module with (different) package definitions

#include "DoubleModuleWithPackage.ice"

[["java:package:dmwpTest10"]]

module M1
{
module M2
{

const dmwpEnum dmwpTest10Constant = dmwpE1;

struct dmwpTest10Struct
{
    dmwpEnum e;
    dmwpStruct s;
    dmwpStructSeq seq;
    dmwpStringStructDict dict;
    dmwpClass c;
    dmwpInterface i;
}

sequence<dmwpStruct> dmwpTest10StructSeq;

dictionary<dmwpStruct, dmwpBaseClass> dmwpTest10StructClassSeq;

interface dmwpTest10Interface extends dmwpInterface {}

exception dmwpTest10Exception extends dmwpException
{
    dmwpEnum e1;
    dmwpStruct s1;
    dmwpStructSeq seq1;
    dmwpStringStructDict dict1;
    dmwpClass c1;
    dmwpInterface i1;
}

class dmwpTest10Class extends dmwpBaseClass implements dmwpBaseInterface
{
    dmwpStruct
    dmwpTest10Op1(dmwpEnum i1,
                  dmwpStruct i2,
                  dmwpStructSeq i3,
                  dmwpStringStructDict i4,
                  dmwpInterface i5,
                  dmwpClass i6,
                  out dmwpEnum o1,
                  out dmwpStruct o2,
                  out dmwpStructSeq o3,
                  out dmwpStringStructDict o4,
                  out dmwpInterface o5,
                  out dmwpClass o6)
        throws dmwpException;

    ["amd"]
    dmwpStruct
    dmwpTest10Op3(dmwpEnum i1,
                  dmwpStruct i2,
                  dmwpStructSeq i3,
                  dmwpStringStructDict i4,
                  dmwpInterface i5,
                  dmwpClass i6,
                  out dmwpEnum o1,
                  out dmwpStruct o2,
                  out dmwpStructSeq o3,
                  out dmwpStringStructDict o4,
                  out dmwpInterface o5,
                  out dmwpClass o6)
        throws dmwpException;
}

}
}
