// Copyright (c) ZeroC, Inc.

#nullable enable

namespace Ice;

/// <summary>
/// This class allows a proxy to be used as the key for a hashed collection.
/// The GetHashCode, Equals, and Compare methods are based on the object identity
/// of the proxy.
/// </summary>
public class ProxyIdentityKey : System.Collections.IEqualityComparer, System.Collections.IComparer
{
    /// <summary>
    /// Computes a hash value based on the object identity of the proxy.
    /// </summary>
    /// <param name="obj">The proxy whose hash value to compute.</param>
    /// <returns>The hash value for the proxy based on the identity.</returns>
    public int GetHashCode(object obj) => ((ObjectPrx)obj).ice_getIdentity().GetHashCode();

    /// Compares two proxies for equality.
    /// <param name="obj1">A proxy to compare.</param>
    /// <param name="obj2">A proxy to compare.</param>
    /// <returns>True if the passed proxies have the same object
    /// identity; false, otherwise.</returns>
    public new bool Equals(object? obj1, object? obj2)
    {
        try
        {
            return Compare(obj1, obj2) == 0;
        }
        catch (System.Exception)
        {
            return false;
        }
    }

    /// Compares two proxies using the object identity for comparison.
    /// <param name="obj1">A proxy to compare.</param>
    /// <param name="obj2">A proxy to compare.</param>
    /// <returns>&lt; 0 if obj1 is less than obj2; &gt; 0 if obj1 is greater than obj2;
    /// 0, otherwise.</returns>
    public int Compare(object? obj1, object? obj2)
    {
        ObjectPrx? proxy1 = obj1 as ObjectPrx;
        if (obj1 is not null && proxy1 is null)
        {
            throw new ArgumentException("Argument must be derived from Ice.ObjectPrx", nameof(obj1));
        }

        ObjectPrx? proxy2 = obj2 as ObjectPrx;
        if (obj2 is not null && proxy2 is null)
        {
            throw new ArgumentException("Argument must be derived from Ice.ObjectPrx", nameof(obj2));
        }
        return Util.proxyIdentityCompare(proxy1, proxy2);
    }
}

/// <summary>
/// This class allows a proxy to be used as the key for a hashed collection.
/// The GetHashCode, Equals, and Compare methods are based on the object identity and
/// the facet of the proxy.
/// </summary>
public class ProxyIdentityFacetKey : System.Collections.IEqualityComparer, System.Collections.IComparer
{
    /// <summary>
    /// Computes a hash value based on the object identity and facet of the proxy.
    /// </summary>
    /// <param name="obj">The proxy whose hash value to compute.</param>
    /// <returns>The hash value for the proxy based on the identity and facet.</returns>
    public int GetHashCode(object obj)
    {
        var o = (ObjectPrx)obj;
        return HashCode.Combine(o.ice_getIdentity(), o.ice_getFacet());
    }

    /// Compares two proxies for equality.
    /// <param name="obj1">A proxy to compare.</param>
    /// <param name="obj2">A proxy to compare.</param>
    /// <returns>True if the passed proxies have the same object
    /// identity and facet; false, otherwise.</returns>
    public new bool Equals(object? obj1, object? obj2)
    {
        try
        {
            return Compare(obj1, obj2) == 0;
        }
        catch (System.Exception)
        {
            return false;
        }
    }

    /// Compares two proxies using the object identity and facet for comparison.
    /// <param name="obj1">A proxy to compare.</param>
    /// <param name="obj2">A proxy to compare.</param>
    /// <returns>&lt; 0 if obj1 is less than obj2; &gt; 0 if obj1 is greater than obj2;
    /// 0, otherwise.</returns>
    public int Compare(object? obj1, object? obj2)
    {
        ObjectPrx? proxy1 = obj1 as ObjectPrx;
        if (obj1 is not null && proxy1 is null)
        {
            throw new ArgumentException("Argument must be derived from Ice.ObjectPrx", nameof(obj1));
        }

        ObjectPrx? proxy2 = obj2 as ObjectPrx;
        if (obj2 is not null && proxy2 is null)
        {
            throw new ArgumentException("Argument must be derived from Ice.ObjectPrx", nameof(obj2));
        }
        return Util.proxyIdentityAndFacetCompare(proxy1, proxy2);
    }
}
