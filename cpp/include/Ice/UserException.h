//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_USER_EXCEPTION_H
#define ICE_USER_EXCEPTION_H

#include "Exception.h"

#include <string_view>

namespace Ice
{
    class InputStream;
    class OutputStream;

    /**
     * Abstract base class for all Ice exceptions defined in Slice.
     * \headerfile Ice/Ice.h
     */
    class ICE_API UserException : public Exception
    {
    public:
        /**
         * Default constructor.
         */
        UserException() : Exception(nullptr, 0) {}

        /**
         * Obtains the Slice type ID of this exception.
         * @return The fully-scoped type ID.
         */
        static std::string_view ice_staticId() noexcept;

        /**
         * Throws this exception.
         */
        virtual void ice_throw() const = 0;

        /// \cond STREAM
        // _write and _read are virtual for the Python, Ruby etc. mappings.
        virtual void _write(OutputStream*) const;
        virtual void _read(InputStream*);

        virtual bool _usesClasses() const;
        /// \endcond

    protected:
        /// \cond STREAM
        virtual void _writeImpl(OutputStream*) const = 0;
        virtual void _readImpl(InputStream*) = 0;
        /// \endcond
    };
}

#endif
