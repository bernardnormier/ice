//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "WstringI.h"

::std::wstring
Test1::WstringClassI::opString(::std::wstring s1, ::std::wstring& s2, const Ice::Current&)
{
    s2 = s1;
    return s1;
}

::Test1::WstringStruct
Test1::WstringClassI::opStruct(::Test1::WstringStruct s1, ::Test1::WstringStruct& s2, const Ice::Current&)
{
    s2 = s1;
    return s1;
}

void
Test1::WstringClassI::throwExcept(::std::wstring reason, const Ice::Current&)
{
    Test1::WstringException ex;
    ex.reason = reason;
    throw ex;
}

::std::wstring
Test2::WstringClassI::opString(::std::wstring s1, ::std::wstring& s2, const Ice::Current&)
{
    s2 = s1;
    return s1;
}

::Test2::WstringStruct
Test2::WstringClassI::opStruct(::Test2::WstringStruct s1, ::Test2::WstringStruct& s2, const Ice::Current&)
{
    s2 = s1;
    return s1;
}

void
Test2::WstringClassI::throwExcept(::std::wstring reason, const Ice::Current&)
{
    Test2::WstringException ex;
    ex.reason = reason;
    throw ex;
}
