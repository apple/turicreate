//
//  LayerShapeConstraints.hpp
//  mlmodel
//
//  Created by William March on 12/5/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#ifndef LayerShapeConstraints_hpp
#define LayerShapeConstraints_hpp

#include <stdio.h>
#include <ostream>
#include <stdexcept>
#include <string>
#include "Validators.hpp"

namespace CoreML {

    // An implementation of an element of the natural numbers plus a special unbound value.
    // If subtraction would make it negative, it is clamped to zero.
    // Adding and multiplying with an unbound value is unbound.
    // Subtracting or dividing by an unbound value is undefined (will throw)
    class RangeValue {

    public:

        // Defaults to being unbound
        RangeValue();
        RangeValue(size_t val);

        void set(size_t inval);
        void set(const RangeValue& val);

        size_t value() const;
        bool isUnbound() const;

        RangeValue operator+ (size_t other) const;
        RangeValue operator+ (const RangeValue& other) const;
        RangeValue operator+ (int other) const;
        RangeValue operator* (size_t other) const;
        RangeValue operator* (const RangeValue& other) const;

        // These clamp to zero if invalid
        // Subtracting or dividing by an unbound number throws
        RangeValue operator- (size_t other) const;
        RangeValue operator- (const RangeValue& other) const;
        RangeValue operator- (int other) const;
        RangeValue operator/ (size_t other) const;
        RangeValue operator/ (const RangeValue& other) const;

        // integer division rounded up
        RangeValue divideAndRoundUp(const RangeValue& other) const;
        RangeValue divideAndRoundUp(size_t other) const;

        // Unbound is greater than everything (including itself) and less than nothing (including itself)
        // Unbound values are equal
        bool operator< (size_t other) const;
        bool operator< (const RangeValue& other) const;
        bool operator<= (size_t other) const;
        bool operator<= (const RangeValue& other) const;
        bool operator> (size_t other) const;
        bool operator> (const RangeValue& other) const;
        bool operator>= (size_t other) const;
        bool operator>= (const RangeValue& other) const;

    private:

        bool _isUnbound;
        size_t _val;

    };

    // A range of possible values for a tensor axis
    // throws if it tries to get constrained to a disallowed value
    // Also needs to figure out if the max isn't bigger than the min
    class ShapeRange {

    public:

        ShapeRange();
        ShapeRange(size_t min);
        ShapeRange(size_t min, size_t max);
        ShapeRange(const RangeValue& min, const RangeValue& max);

        // Convenience constructor from protobuf message
        ShapeRange(const Specification::SizeRange& range);

        // Checks if the given value is a valid part of this range
        bool isValid(const RangeValue& val) const;
        bool isValid(size_t val) const;

        RangeValue minimum() const;
        RangeValue maximum() const;

        // Scales both min and max independenly
        ShapeRange operator+ (size_t val) const;
        ShapeRange operator* (size_t val) const;

        // These can throw for making the range invalid -- scales both endpoints
        ShapeRange operator- (size_t val) const;
        ShapeRange operator/ (size_t val) const;

        // Checks for negatives
        ShapeRange operator+ (int val) const;
        ShapeRange operator- (int val) const;
        // Just checks for negative values -- keeps from doing it in every layer
        ShapeRange operator/ (int val) const;

        // We're defining the difference of two unbound ranges to be unbound
        ShapeRange operator+ (const ShapeRange& other) const;
        ShapeRange operator- (const ShapeRange& other) const;
        ShapeRange operator* (const ShapeRange& other) const;
        ShapeRange operator/ (const ShapeRange& other) const;

        // Integer division rounded up
        ShapeRange divideAndRoundUp(size_t val) const;

        // The intersection of the two ranges
        // Throws if the intersection is invalid (empty)
        ShapeRange intersect(const ShapeRange& other) const;

        // Computes the union of the two ranges
        // If they are disjoint, the result will include the gap between them as well
        ShapeRange unify(const ShapeRange& other) const;

        // These can throw if the value is not part of the current range
        void setLower(size_t val);
        void setUpper(size_t val);
        void setValue(size_t val);
        void setLower(const RangeValue& val);
        void setUpper(const RangeValue& val);
        void setValue(const RangeValue& val);

        bool isUnbound() const;

        bool equals(size_t val) const;

        size_t minimumValue() const;
        RangeValue maximumValue() const;

        // Returns true if the range can only accept a single value, false if there is
        // more than one valid value
        bool isFixed() const;

    private:

        RangeValue _minimum;
        RangeValue _maximum;

    };

    /**
     * Stores the constraint for a particular data blob.
     */
    class ShapeConstraint {

        // concepts we want: minimum and maximum possible size, unknown size, unbounded size
    public:

        ShapeConstraint(const std::string& name);
        // All axes unconstrained
        ShapeConstraint();

        std::string name() const;
        void setName(const std::string& name);

        // Copies all 5 constraints
        void copyFrom(const ShapeConstraint& other);
        // Copies only C, H, W -- leaves other two unchanged
        void copyFromNoBatchSeq(const ShapeConstraint& other);

        // Equivalent to intersect(Range(0, val))
        void upperBoundSequence(size_t val);
        void upperBoundBatch(size_t val);
        void upperBoundChannel(size_t val);
        void upperBoundHeight(size_t val);
        void upperBoundWidth(size_t val);

        void upperBoundSequence(const RangeValue& val);
        void upperBoundBatch(const RangeValue& val);
        void upperBoundChannel(const RangeValue& val);
        void upperBoundHeight(const RangeValue& val);
        void upperBoundWidth(const RangeValue& val);

        // Equivalent to intersect(Range(val))
        void lowerBoundSequence(size_t val);
        void lowerBoundBatch(size_t val);
        void lowerBoundChannel(size_t val);
        void lowerBoundHeight(size_t val);
        void lowerBoundWidth(size_t val);

        // Computes the intersection
        void updateSequenceRange(const ShapeRange& other);
        void updateBatchRange(const ShapeRange& other);
        void updateChannelRange(const ShapeRange& other);
        void updateHeightRange(const ShapeRange& other);
        void updateWidthRange(const ShapeRange& other);

        // Throws if the value is not valid
        void setSequence(size_t val);
        void setBatch(size_t val);
        void setChannel(size_t val);
        void setHeight(size_t val);
        void setWidth(size_t val);

        const ShapeRange& sequenceRange() const;
        const ShapeRange& batchRange() const;
        const ShapeRange& channelRange() const;
        const ShapeRange& heightRange() const;
        const ShapeRange& widthRange() const;

        bool isValid() const;

        size_t minimumSequenceLength() const;
        size_t minimumBatchSize() const;
        size_t minimumChannelSize() const;
        size_t minimumHeight() const;
        size_t minimumWidth() const;

        void updateConstraint(const Specification::FeatureType& type);

        bool hasFixedCHW() const;

    private:

        ShapeRange _sequenceRange;
        ShapeRange _batchRange;
        ShapeRange _channelRange;
        ShapeRange _heightRange;
        ShapeRange _widthRange;

        // The name of the data blob -- used to throw informative error messages
        std::string _name;

    };

}

std::ostream& operator<< (std::ostream& out, const CoreML::RangeValue& obj);
std::ostream& operator<< (std::ostream& out, const CoreML::ShapeRange& obj);
std::ostream& operator<< (std::ostream& out, const CoreML::ShapeConstraint& obj);

#endif /* LayerShapeConstraints_hpp */
