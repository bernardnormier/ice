//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#pragma once

[[suppress-warning:reserved-identifier]]

module ZeroC::Ice::Test::Optional
{
    sequence<int?> OptIntSeq;
    sequence<string?> OptStringSeq;

    dictionary<int, int?> IntOptIntDict;
    dictionary<int, string?> IntOptStringDict;

    class C
    {
        int x;
    }

    struct MyStruct
    {
        Object? proxy;
        int? x;
        OptStringSeq? stringSeq;
    }

    class Base
    {
        Object? proxy;
        int? x;
        OptStringSeq? stringSeq;
    }

    class Derived : Base
    {
        AnyClass? someClass;
        string? s;
    }

    exception BaseEx
    {
        Object? proxy;
        int? x;
        OptStringSeq? stringSeq;
    }

    exception DerivedEx : BaseEx
    {
        AnyClass? someClass;
        string? s;
    }

    interface Test
    {
        void shutdown();

        void opSingleInInt(int? i1);
        void opSingleInString(string? i1);
        void opSingleOutInt(out int? o1);
        void opSingleOutString(out string? o1);
        int? opSingleReturnInt();
        string? opSingleReturnString();

        void opBasicIn(int i1, int? i2, string? i3, string i4);
        int? opBasicInOut(int i1, int? i2, string? i3, out int o1, out int? o2, out string? o3);

        Object? opObject(Object i1, Object? i2);
        Test? opTest(Test i1, Test? i2);

        AnyClass? opAnyClass(AnyClass i1, AnyClass? i2);
        C? opC(C i1, C? i2);

        OptIntSeq opOptIntSeq(OptIntSeq i1);
        tag(1) OptIntSeq? opTaggedOptIntSeq(tag(2) OptIntSeq? i1);

        OptStringSeq opOptStringSeq(OptStringSeq i1);
        tag(1) OptStringSeq? opTaggedOptStringSeq(tag(2) OptStringSeq? i1);

        IntOptIntDict opIntOptIntDict(IntOptIntDict i1);
        tag(1) IntOptIntDict? opTaggedIntOptIntDict(tag(2) IntOptIntDict? i1);

        IntOptStringDict opIntOptStringDict(IntOptStringDict i1);
        tag(1) IntOptStringDict? opTaggedIntOptStringDict(tag(2) IntOptStringDict? i1);

        MyStruct opMyStruct(MyStruct i1);
        MyStruct? opOptMyStruct(MyStruct? i1);
        Derived opDerived(Derived i1);
        Derived? opOptDerived(Derived? i1);
        void opDerivedEx();
    }
}
