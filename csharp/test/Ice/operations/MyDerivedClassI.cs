//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Linq;
using System.Collections.Generic;
using Test;

namespace ZeroC.Ice.Test.Operations
{
    public sealed class MyDerivedClass : IMyDerivedClass, IObject
    {
        private readonly object _mutex = new object();

        // Override the Object "pseudo" operations to verify the operation mode.
        public bool IceIsA(string id, Current current)
        {
            TestHelper.Assert(current.IsIdempotent);
            return typeof(IMyDerivedClass).GetAllIceTypeIds().Contains(id);
        }

        public void IcePing(Current current) => TestHelper.Assert(current.IsIdempotent);

        public IEnumerable<string> IceIds(Current current)
        {
            TestHelper.Assert(current.IsIdempotent);
            return typeof(IMyDerivedClass).GetAllIceTypeIds();
        }

        public string IceId(Current current)
        {
            TestHelper.Assert(current.IsIdempotent);
            return typeof(IMyDerivedClass).GetIceTypeId()!;
        }

        public void Shutdown(Current current) => current.Adapter.Communicator.ShutdownAsync();

        // TODO check if compress is supported.
        public bool SupportsCompress(Current current) => false;

        public void OpVoid(Current current) => TestHelper.Assert(!current.IsIdempotent);

        public (bool, bool) OpBool(bool p1, bool p2, Current current) => (p2, p1);

        public (ReadOnlyMemory<bool>, ReadOnlyMemory<bool>) OpBoolS(bool[] p1, bool[] p2, Current current)
        {
            bool[] p3 = new bool[p1.Length + p2.Length];
            Array.Copy(p1, p3, p1.Length);
            Array.Copy(p2, 0, p3, p1.Length, p2.Length);

            bool[] r = new bool[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<bool[]>, IEnumerable<bool[]>) OpBoolSS(bool[][] p1, bool[][] p2, Current current)
        {
            bool[][] p3 = new bool[p1.Length + p2.Length][];
            Array.Copy(p1, p3, p1.Length);
            Array.Copy(p2, 0, p3, p1.Length, p2.Length);

            bool[][] r = new bool[p1.Length][];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (byte, byte) OpByte(byte p1, byte p2, Current current) => (p1, (byte)(p1 ^ p2));

        public (IReadOnlyDictionary<byte, bool>, IReadOnlyDictionary<byte, bool>) OpByteBoolD(
            Dictionary<byte, bool> p1, Dictionary<byte, bool> p2, Current current)
        {
            var r = new Dictionary<byte, bool>();
            foreach (KeyValuePair<byte, bool> e in p1)
            {
                r[e.Key] = e.Value;
            }
            foreach (KeyValuePair<byte, bool> e in p2)
            {
                r[e.Key] = e.Value;
            }
            return (r, p1);
        }

        public (ReadOnlyMemory<byte>, ReadOnlyMemory<byte>) OpByteS(byte[] p1, byte[] p2, Current current)
        {
            byte[] p3 = new byte[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                p3[i] = p1[^(i + 1)];
            }

            byte[] r = new byte[p1.Length + p2.Length];
            Array.Copy(p1, r, p1.Length);
            Array.Copy(p2, 0, r, p1.Length, p2.Length);
            return (r, p3);
        }

        public (IEnumerable<byte[]>, IEnumerable<byte[]>) OpByteSS(byte[][] p1, byte[][] p2, Current current)
        {
            byte[][] p3 = new byte[p1.Length][];
            for (int i = 0; i < p1.Length; i++)
            {
                p3[i] = p1[^(i + 1)];
            }

            byte[][] r = new byte[p1.Length + p2.Length][];
            Array.Copy(p1, r, p1.Length);
            Array.Copy(p2, 0, r, p1.Length, p2.Length);
            return (r, p3);
        }

        public (double, float, double) OpFloatDouble(float p1, double p2, Current current) => (p2, p1, p2);

        public (ReadOnlyMemory<double>, ReadOnlyMemory<float>, ReadOnlyMemory<double>) OpFloatDoubleS(float[] p1,
            double[] p2, Current current)
        {
            double[] p4 = new double[p2.Length];
            for (int i = 0; i < p2.Length; i++)
            {
                p4[i] = p2[^(i + 1)];
            }

            double[] r = new double[p2.Length + p1.Length];
            Array.Copy(p2, r, p2.Length);
            for (int i = 0; i < p1.Length; i++)
            {
                r[p2.Length + i] = p1[i];
            }
            return (r, p1, p4);
        }

        public (IEnumerable<double[]>, IEnumerable<float[]>, IEnumerable<double[]>) OpFloatDoubleSS(float[][] p1,
            double[][] p2, Current current)
        {
            double[][] p4 = new double[p2.Length][];
            for (int i = 0; i < p2.Length; i++)
            {
                p4[i] = p2[^(i + 1)];
            }

            double[][] r = new double[p2.Length + p2.Length][];
            Array.Copy(p2, r, p2.Length);
            for (int i = 0; i < p2.Length; i++)
            {
                r[p2.Length + i] = new double[p2[i].Length];
                for (int j = 0; j < p2[i].Length; j++)
                {
                    r[p2.Length + i][j] = p2[i][j];
                }
            }
            return (r, p1, p4);
        }

        public (IReadOnlyDictionary<long, float>, IReadOnlyDictionary<long, float>) OpLongFloatD(
            Dictionary<long, float> p1, Dictionary<long, float> p2, Current current)
        {
            var r = new Dictionary<long, float>(p1);
            foreach ((long key, float value) in p2)
            {
                r[key] = value;
            }
            return (r, p1);
        }

        public (IReadOnlyDictionary<ulong, float>, IReadOnlyDictionary<ulong, float>) OpULongFloatD(
            Dictionary<ulong, float> p1,
            Dictionary<ulong, float> p2,
            Current current)
        {
            var r = new Dictionary<ulong, float>(p1);
            foreach ((ulong key, float value) in p2)
            {
                r[key] = value;
            }
            return (r, p1);
        }

        public (IMyClassPrx?, IMyClassPrx?, IMyClassPrx?) OpMyClass(IMyClassPrx? p1, Current current) =>
            (current.Adapter.CreateProxy(current.Identity, IMyClassPrx.Factory),
             p1,
             current.Adapter.CreateProxy("noSuchIdentity", IMyClassPrx.Factory));

        public (MyEnum, MyEnum) OpMyEnum(MyEnum p1, Current current) => (MyEnum.enum3, p1);

        public (IReadOnlyDictionary<short, int>, IReadOnlyDictionary<short, int>) OpShortIntD(
            Dictionary<short, int> p1,
            Dictionary<short, int> p2,
            Current current)
        {
            var r = new Dictionary<short, int>(p1);
            foreach ((short key, int value) in p2)
            {
                r[key] = value;
            }
            return (r, p1);
        }

        public (IReadOnlyDictionary<ushort, uint>, IReadOnlyDictionary<ushort, uint>) OpUShortUIntD(
            Dictionary<ushort, uint> p1,
            Dictionary<ushort, uint> p2,
            Current current)
        {
            var r = new Dictionary<ushort, uint>(p1);
            foreach ((ushort key, uint value) in p2)
            {
                r[key] = value;
            }
            return (r, p1);
        }

        public (long, short, int, long) OpShortIntLong(short p1, int p2, long p3, Current current) => (p3, p1, p2, p3);

        public (ulong, ushort, uint, ulong) OpUShortUIntULong(ushort p1, uint p2, ulong p3, Current current) =>
            (p3, p1, p2, p3);

        public int OpVarInt(int v, Current current) => v;
        public uint OpVarUInt(uint v, Current current) => v;
        public long OpVarLong(long v, Current current) => v;
        public ulong OpVarULong(ulong v, Current current) => v;

        public (ReadOnlyMemory<long>,
                ReadOnlyMemory<short>,
                ReadOnlyMemory<int>,
                ReadOnlyMemory<long>) OpShortIntLongS(
            short[] p1,
            int[] p2,
            long[] p3,
            Current current)
        {
            int[] p5 = new int[p2.Length];
            for (int i = 0; i < p2.Length; i++)
            {
                p5[i] = p2[^(i + 1)];
            }

            long[] p6 = new long[p3.Length + p3.Length];
            Array.Copy(p3, p6, p3.Length);
            Array.Copy(p3, 0, p6, p3.Length, p3.Length);

            return (p3, p1, p5, p6);
        }

        public (ReadOnlyMemory<ulong>, ReadOnlyMemory<ushort>, ReadOnlyMemory<uint>, ReadOnlyMemory<ulong>)
        OpUShortUIntULongS(ushort[] p1, uint[] p2, ulong[] p3, Current current)
        {
            uint[] p5 = new uint[p2.Length];
            for (int i = 0; i < p2.Length; i++)
            {
                p5[i] = p2[^(i + 1)];
            }

            ulong[] p6 = new ulong[p3.Length + p3.Length];
            Array.Copy(p3, p6, p3.Length);
            Array.Copy(p3, 0, p6, p3.Length, p3.Length);

            return (p3, p1, p5, p6);
        }

        public (IEnumerable<long>, IEnumerable<int>, IEnumerable<long>)
        OpVarIntVarLongS(int[] p1, long[] p2, Current current)
        {
            int[] p4 = new int[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                p4[i] = p1[^(i + 1)];
            }

            long[] p5 = new long[p2.Length + p2.Length];
            Array.Copy(p2, p5, p2.Length);
            Array.Copy(p2, 0, p5, p2.Length, p2.Length);

            return (p2, p4, p5);
        }

        public (IEnumerable<ulong>, IEnumerable<uint>, IEnumerable<ulong>)
        OpVarUIntVarULongS(uint[] p1, ulong[] p2, Current current)
        {
            uint[] p4 = new uint[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                p4[i] = p1[^(i + 1)];
            }

            ulong[] p5 = new ulong[p2.Length + p2.Length];
            Array.Copy(p2, p5, p2.Length);
            Array.Copy(p2, 0, p5, p2.Length, p2.Length);

            return (p2, p4, p5);
        }

        public (IEnumerable<long[]>, IEnumerable<short[]>, IEnumerable<int[]>, IEnumerable<long[]>) OpShortIntLongSS(
            short[][] p1, int[][] p2, long[][] p3, Current current)
        {
            int[][] p5 = new int[p2.Length][];
            for (int i = 0; i < p2.Length; i++)
            {
                p5[i] = p2[^(i + 1)];
            }

            long[][] p6 = new long[p3.Length + p3.Length][];
            Array.Copy(p3, p6, p3.Length);
            Array.Copy(p3, 0, p6, p3.Length, p3.Length);

            return (p3, p1, p5, p6);
        }

        public (IEnumerable<ulong[]>, IEnumerable<ushort[]>, IEnumerable<uint[]>, IEnumerable<ulong[]>)
        OpUShortUIntULongSS(ushort[][] p1, uint[][] p2, ulong[][] p3, Current current)
        {
            uint[][] p5 = new uint[p2.Length][];
            for (int i = 0; i < p2.Length; i++)
            {
                p5[i] = p2[^(i + 1)];
            }

            ulong[][] p6 = new ulong[p3.Length + p3.Length][];
            Array.Copy(p3, p6, p3.Length);
            Array.Copy(p3, 0, p6, p3.Length, p3.Length);

            return (p3, p1, p5, p6);
        }

        public (string, string) OpString(string p1, string p2, Current current) => (p1 + " " + p2, p2 + " " + p1);

        public (IReadOnlyDictionary<string, MyEnum>, IReadOnlyDictionary<string, MyEnum>) OpStringMyEnumD(
            Dictionary<string, MyEnum> p1, Dictionary<string, MyEnum> p2, Current current)
        {
            var r = new Dictionary<string, MyEnum>();
            foreach (KeyValuePair<string, MyEnum> e in p1)
            {
                r[e.Key] = e.Value;
            }
            foreach (KeyValuePair<string, MyEnum> e in p2)
            {
                r[e.Key] = e.Value;
            }
            return (r, p1);
        }

        public (IReadOnlyDictionary<MyEnum, string>, IReadOnlyDictionary<MyEnum, string>) OpMyEnumStringD(
            Dictionary<MyEnum, string> p1, Dictionary<MyEnum, string> p2, Current current)
        {
            var r = new Dictionary<MyEnum, string>(p1);
            foreach ((MyEnum key, string value) in p2)
            {
                r[key] = value;
            }
            return (r, p1);
        }

        public (IReadOnlyDictionary<MyStruct, MyEnum>, IReadOnlyDictionary<MyStruct, MyEnum>) OpMyStructMyEnumD(
            Dictionary<MyStruct, MyEnum> p1,
            Dictionary<MyStruct, MyEnum> p2,
            Current current)
        {
            var r = new Dictionary<MyStruct, MyEnum>(p1);
            foreach ((MyStruct key, MyEnum value) in p2)
            {
                r[key] = value;
            }
            return (r, p1);
        }

        public (IEnumerable<Dictionary<byte, bool>>, IEnumerable<Dictionary<byte, bool>>) OpByteBoolDS(
            Dictionary<byte, bool>[] p1,
            Dictionary<byte, bool>[] p2,
            Current current)
        {
            var p3 = new Dictionary<byte, bool>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<byte, bool>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<Dictionary<short, int>>, IEnumerable<Dictionary<short, int>>) OpShortIntDS(
            Dictionary<short, int>[] p1,
            Dictionary<short, int>[] p2,
            Current current)
        {
            var p3 = new Dictionary<short, int>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<short, int>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<Dictionary<ushort, uint>>, IEnumerable<Dictionary<ushort, uint>>) OpUShortUIntDS(
            Dictionary<ushort, uint>[] p1,
            Dictionary<ushort, uint>[] p2,
            Current current)
        {
            var p3 = new Dictionary<ushort, uint>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<ushort, uint>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<Dictionary<long, float>>, IEnumerable<Dictionary<long, float>>) OpLongFloatDS(
            Dictionary<long, float>[] p1,
            Dictionary<long, float>[] p2,
            Current current)
        {
            var p3 = new Dictionary<long, float>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<long, float>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<Dictionary<ulong, float>>, IEnumerable<Dictionary<ulong, float>>) OpULongFloatDS(
            Dictionary<ulong, float>[] p1,
            Dictionary<ulong, float>[] p2,
            Current current)
        {
            var p3 = new Dictionary<ulong, float>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<ulong, float>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<Dictionary<string, string>>, IEnumerable<Dictionary<string, string>>) OpStringStringDS(
            Dictionary<string, string>[] p1,
            Dictionary<string, string>[] p2,
            Current current)
        {
            var p3 = new Dictionary<string, string>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<string, string>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<Dictionary<string, MyEnum>>, IEnumerable<Dictionary<string, MyEnum>>) OpStringMyEnumDS(
            Dictionary<string, MyEnum>[] p1,
            Dictionary<string, MyEnum>[] p2,
            Current current)
        {
            var p3 = new Dictionary<string, MyEnum>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<string, MyEnum>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<Dictionary<MyEnum, string>>, IEnumerable<Dictionary<MyEnum, string>>) OpMyEnumStringDS(
            Dictionary<MyEnum, string>[] p1,
            Dictionary<MyEnum, string>[] p2,
            Current current)
        {
            var p3 = new Dictionary<MyEnum, string>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<MyEnum, string>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<Dictionary<MyStruct, MyEnum>>,
                IEnumerable<Dictionary<MyStruct, MyEnum>>) OpMyStructMyEnumDS(
            Dictionary<MyStruct, MyEnum>[] p1,
            Dictionary<MyStruct, MyEnum>[] p2,
            Current current)
        {
            var p3 = new Dictionary<MyStruct, MyEnum>[p1.Length + p2.Length];
            Array.Copy(p2, p3, p2.Length);
            Array.Copy(p1, 0, p3, p2.Length, p1.Length);

            var r = new Dictionary<MyStruct, MyEnum>[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IReadOnlyDictionary<byte, byte[]>, IReadOnlyDictionary<byte, byte[]>) OpByteByteSD(
            Dictionary<byte, byte[]> p1,
            Dictionary<byte, byte[]> p2,
            Current current)
        {
            var r = new Dictionary<byte, byte[]>(p1);
            foreach ((byte key, byte[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<bool, bool[]>, IReadOnlyDictionary<bool, bool[]>) OpBoolBoolSD(
            Dictionary<bool, bool[]> p1,
            Dictionary<bool, bool[]> p2,
            Current current)
        {
            var r = new Dictionary<bool, bool[]>(p1);
            foreach ((bool key, bool[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<short, short[]>, IReadOnlyDictionary<short, short[]>) OpShortShortSD(
            Dictionary<short, short[]> p1,
            Dictionary<short, short[]> p2,
            Current current)
        {
            var r = new Dictionary<short, short[]>(p1);
            foreach ((short key, short[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<ushort, ushort[]>, IReadOnlyDictionary<ushort, ushort[]>) OpUShortUShortSD(
            Dictionary<ushort, ushort[]> p1,
            Dictionary<ushort, ushort[]> p2,
            Current current)
        {
            var r = new Dictionary<ushort, ushort[]>(p1);
            foreach ((ushort key, ushort[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<int, int[]>, IReadOnlyDictionary<int, int[]>) OpIntIntSD(
            Dictionary<int, int[]> p1,
            Dictionary<int, int[]> p2,
            Current current)
        {
            var r = new Dictionary<int, int[]>(p1);
            foreach ((int key, int[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<uint, uint[]>, IReadOnlyDictionary<uint, uint[]>) OpUIntUIntSD(
            Dictionary<uint, uint[]> p1,
            Dictionary<uint, uint[]> p2,
            Current current)
        {
            var r = new Dictionary<uint, uint[]>(p1);
            foreach ((uint key, uint[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<long, long[]>, IReadOnlyDictionary<long, long[]>) OpLongLongSD(
            Dictionary<long, long[]> p1, Dictionary<long, long[]> p2, Current current)
        {
            var r = new Dictionary<long, long[]>(p1);
            foreach ((long key, long[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<ulong, ulong[]>, IReadOnlyDictionary<ulong, ulong[]>) OpULongULongSD(
            Dictionary<ulong, ulong[]> p1,
            Dictionary<ulong, ulong[]> p2,
            Current current)
        {
            var r = new Dictionary<ulong, ulong[]>(p1);
            foreach ((ulong key, ulong[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<string, float[]>, IReadOnlyDictionary<string, float[]>) OpStringFloatSD(
            Dictionary<string, float[]> p1,
            Dictionary<string, float[]> p2,
            Current current)
        {
            var r = new Dictionary<string, float[]>(p1);
            foreach ((string key, float[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<string, double[]>, IReadOnlyDictionary<string, double[]>) OpStringDoubleSD(
            Dictionary<string, double[]> p1,
            Dictionary<string, double[]> p2,
            Current current)
        {
            var r = new Dictionary<string, double[]>(p1);
            foreach ((string key, double[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<string, string[]>, IReadOnlyDictionary<string, string[]>) OpStringStringSD(
            Dictionary<string, string[]> p1,
            Dictionary<string, string[]> p2,
            Current current)
        {
            var r = new Dictionary<string, string[]>(p1);
            foreach ((string key, string[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public (IReadOnlyDictionary<MyEnum, MyEnum[]>, IReadOnlyDictionary<MyEnum, MyEnum[]>) OpMyEnumMyEnumSD(
            Dictionary<MyEnum, MyEnum[]> p1,
            Dictionary<MyEnum, MyEnum[]> p2,
            Current ice)
        {
            var r = new Dictionary<MyEnum, MyEnum[]>(p1);
            foreach ((MyEnum key, MyEnum[] value) in p2)
            {
                r[key] = value;
            }
            return (r, p2);
        }

        public ReadOnlyMemory<int> OpIntS(int[] s, Current current)
        {
            int[] r = new int[s.Length];
            for (int i = 0; i < s.Length; ++i)
            {
                r[i] = -s[i];
            }
            return r;
        }

        public void OpByteSOneway(byte[] s, Current current)
        {
            lock (_mutex)
            {
                ++_opByteSOnewayCallCount;
            }
        }

        public int OpByteSOnewayCallCount(Current current)
        {
            lock (_mutex)
            {
                int count = _opByteSOnewayCallCount;
                _opByteSOnewayCallCount = 0;
                return count;
            }
        }

        public IReadOnlyDictionary<string, string> OpContext(Current current) =>
            current.Context == null ?
                new Dictionary<string, string>() :
                new Dictionary<string, string>(current.Context);

        public void OpDoubleMarshaling(double p1, double[] p2, Current current)
        {
            double d = 1278312346.0 / 13.0;
            TestHelper.Assert(p1 == d);
            for (int i = 0; i < p2.Length; ++i)
            {
                TestHelper.Assert(p2[i] == d);
            }
        }

        public (IEnumerable<string>, IEnumerable<string>) OpStringS(string[] p1, string[] p2, Current current)
        {
            string[] p3 = new string[p1.Length + p2.Length];
            Array.Copy(p1, p3, p1.Length);
            Array.Copy(p2, 0, p3, p1.Length, p2.Length);

            string[] r = new string[p1.Length];
            for (int i = 0; i < p1.Length; i++)
            {
                r[i] = p1[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<string[]>, IEnumerable<string[]>) OpStringSS(string[][] p1, string[][] p2, Current current)
        {
            string[][] p3 = new string[p1.Length + p2.Length][];
            Array.Copy(p1, p3, p1.Length);
            Array.Copy(p2, 0, p3, p1.Length, p2.Length);

            string[][] r = new string[p2.Length][];
            for (int i = 0; i < p2.Length; i++)
            {
                r[i] = p2[^(i + 1)];
            }
            return (r, p3);
        }

        public (IEnumerable<string[][]>, IEnumerable<string[][]>) OpStringSSS(string[][][] p1, string[][][] p2,
            Current current)
        {
            string[][][]? p3 = new string[p1.Length + p2.Length][][];
            Array.Copy(p1, p3, p1.Length);
            Array.Copy(p2, 0, p3, p1.Length, p2.Length);

            string[][][] r = new string[p2.Length][][];
            for (int i = 0; i < p2.Length; i++)
            {
                r[i] = p2[^(i + 1)];
            }
            return (r, p3);
        }

        public (IReadOnlyDictionary<string, string>, IReadOnlyDictionary<string, string>) OpStringStringD(
            Dictionary<string, string> p1,
            Dictionary<string, string> p2,
            Current current)
        {
            var r = new Dictionary<string, string>(p1);
            foreach ((string key, string value) in p2)
            {
                r[key] = value;
            }
            return (r, p1);
        }

        public (Structure, Structure) OpStruct(Structure p1, Structure p2, Current current)
        {
            Structure p3 = p1;
            p3.S.S = "a new string";
            return (p2, p3);
        }

        public void OpIdempotent(Current current) => TestHelper.Assert(current.IsIdempotent);

        // "return" exception when called two-way, otherwise succeeds.
        public void OpOneway(Current current) => throw new SomeException();

        // "return" exception when called two-way, otherwise succeeds.
        public void OpOnewayMetadata(Current current) => throw new SomeException();

        public void OpDerived(Current current)
        {
        }

        public byte OpByte1(byte opByte1, Current current) => opByte1;

        public short OpShort1(short opShort1, Current current) => opShort1;
        public ushort OpUShort1(ushort opUShort1, Current current) => opUShort1;

        public int OpInt1(int opInt1, Current current) => opInt1;
        public uint OpUInt1(uint opUInt1, Current current) => opUInt1;

        public long OpLong1(long opLong1, Current current) => opLong1;
        public ulong OpULong1(ulong opULong1, Current current) => opULong1;

        public float OpFloat1(float opFloat1, Current current) => opFloat1;

        public double OpDouble1(double opDouble1, Current current) => opDouble1;

        public string OpString1(string opString1, Current current) => opString1;

        public IEnumerable<string> OpStringS1(string[] opStringS1, Current current) => opStringS1;

        public IReadOnlyDictionary<byte, bool> OpByteBoolD1(Dictionary<byte, bool> opByteBoolD1, Current current) =>
            opByteBoolD1;

        public IEnumerable<string> OpStringS2(string[] opStringS2, Current current) => opStringS2;

        public IReadOnlyDictionary<byte, bool> OpByteBoolD2(Dictionary<byte, bool> opByteBoolD2, Current current) =>
            opByteBoolD2;

        public MyClass1? OpMyClass1(MyClass1? c, Current current) => c;

        public MyStruct1 OpMyStruct1(MyStruct1 s, Current current) => s;

        public IEnumerable<string> OpStringLiterals(Current current)
        {
            return new string[]
                {
                        Constants.s0,
                        Constants.s1,
                        Constants.s2,
                        Constants.s3,
                        Constants.s4,
                        Constants.s5,
                        Constants.s6,
                        Constants.s7,
                        Constants.s8,
                        Constants.s9,
                        Constants.s10,

                        Constants.sw0,
                        Constants.sw1,
                        Constants.sw2,
                        Constants.sw3,
                        Constants.sw4,
                        Constants.sw5,
                        Constants.sw6,
                        Constants.sw7,
                        Constants.sw8,
                        Constants.sw9,
                        Constants.sw10,

                        Constants.ss0,
                        Constants.ss1,
                        Constants.ss2,
                        Constants.ss3,
                        Constants.ss4,
                        Constants.ss5,

                        Constants.su0,
                        Constants.su1,
                        Constants.su2
                };
        }

        public IEnumerable<string> OpWStringLiterals(Current current) => OpStringLiterals(current);

        public IMyClass.OpMStruct1MarshaledReturnValue OpMStruct1(Current current) =>
            new IMyClass.OpMStruct1MarshaledReturnValue(
                new Structure(null, MyEnum.enum1, new AnotherStruct("")), current);

        public IMyClass.OpMStruct2MarshaledReturnValue OpMStruct2(Structure p1, Current current) =>
            new IMyClass.OpMStruct2MarshaledReturnValue(p1, p1, current);

        public IMyClass.OpMSeq1MarshaledReturnValue OpMSeq1(Current current) =>
            new IMyClass.OpMSeq1MarshaledReturnValue(Array.Empty<string>(), current);

        public IMyClass.OpMSeq2MarshaledReturnValue OpMSeq2(string[] p1, Current current) =>
            new IMyClass.OpMSeq2MarshaledReturnValue(p1, p1, current);

        public IMyClass.OpMDict1MarshaledReturnValue OpMDict1(Current current) =>
            new IMyClass.OpMDict1MarshaledReturnValue(new Dictionary<string, string>(), current);

        public IMyClass.OpMDict2MarshaledReturnValue OpMDict2(Dictionary<string, string> p1, Current current) =>
            new IMyClass.OpMDict2MarshaledReturnValue(p1, p1, current);

        private int _opByteSOnewayCallCount;
    }
}
