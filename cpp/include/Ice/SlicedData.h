//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_SLICED_DATA_H
#define ICE_SLICED_DATA_H

#include "Config.h"
#include "SlicedDataF.h"
#include "Value.h"

#include <string>

namespace Ice
{
    /**
     * Encapsulates the details of a slice for an unknown class or exception type.
     * \headerfile Ice/Ice.h
     */
    struct SliceInfo
    {
        /**
         * The Slice type ID for this slice.
         */
        std::string typeId;

        /**
         * The Slice compact type ID for this slice.
         */
        int compactId;

        /**
         * The encoded bytes for this slice, including the leading size integer.
         */
        std::vector<std::byte> bytes;

        /**
         * The class instances referenced by this slice.
         */
        std::vector<ValuePtr> instances;

        /**
         * Whether or not the slice contains optional members.
         */
        bool hasOptionalMembers;

        /**
         * Whether or not this is the last slice.
         */
        bool isLastSlice;
    };

    /**
     * Holds the slices of unknown types.
     * \headerfile Ice/Ice.h
     */
    class ICE_API SlicedData
    {
    public:
        SlicedData(const SliceInfoSeq&);

        /** The slices of unknown types. */
        const SliceInfoSeq slices;

        /**
         * Clears the slices to break potential cyclic references.
         */
        void clear();
    };

    /**
     * Represents an instance of an unknown type.
     * \headerfile Ice/Ice.h
     */
    class ICE_API UnknownSlicedValue : public Value
    {
    public:
        /**
         * Constructs the placeholder instance.
         * @param unknownTypeId The Slice type ID of the unknown value.
         */
        UnknownSlicedValue(const std::string& unknownTypeId);

        /**
         * Determine the Slice type ID associated with this instance.
         * @return The type ID supplied to the constructor.
         */
        std::string ice_id() const override;

        /**
         * Clones this object.
         * @return A new instance.
         */
        UnknownSlicedValuePtr ice_clone() const
        {
            return std::static_pointer_cast<UnknownSlicedValue>(_iceCloneImpl());
        }

    protected:
        /// \cond INTERNAL
        ValuePtr _iceCloneImpl() const override;
        /// \endcond

    private:
        const std::string _unknownTypeId;
    };
}

#endif
