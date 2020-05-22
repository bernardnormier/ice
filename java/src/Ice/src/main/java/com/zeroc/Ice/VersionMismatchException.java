//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `LocalException.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

package com.zeroc.Ice;

/**
 * This exception is raised if the Ice library version does not match the version in the Ice header files.
 **/
public class VersionMismatchException extends LocalException
{
    public VersionMismatchException()
    {
    }

    public VersionMismatchException(Throwable cause)
    {
        super(cause);
    }

    public String ice_id()
    {
        return "::Ice::VersionMismatchException";
    }

    /** @hidden */
    public static final long serialVersionUID = 3839952284604400769L;
}