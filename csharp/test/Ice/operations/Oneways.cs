// Copyright (c) ZeroC, Inc.

namespace Ice
{
    namespace operations
    {
        internal class Oneways
        {
            private static void test(bool b)
            {
                if (!b)
                {
                    throw new System.SystemException();
                }
            }

            internal static void oneways(global::Test.TestHelper helper, Test.MyClassPrx p)
            {
                Ice.Communicator communicator = helper.communicator();
                p = Test.MyClassPrxHelper.uncheckedCast(p.ice_oneway());

                {
                    p.ice_ping();
                }

                {
                    p.opVoid();
                }

                {
                    p.opIdempotent();
                }

                {
                    byte b;
                    try
                    {
                        p.opByte((byte)0xff, (byte)0x0f, out b);
                        test(false);
                    }
                    catch (Ice.TwowayOnlyException)
                    {
                    }
                }
            }
        }
    }
}
