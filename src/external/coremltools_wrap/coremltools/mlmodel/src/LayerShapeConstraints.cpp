//
//  LayerShapeConstraints.cpp
//  mlmodel
//
//  Created by William March on 12/5/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#include "LayerShapeConstraints.hpp"
#include <sstream>

using namespace CoreML;

RangeValue::RangeValue()
:
_isUnbound(true)
{}

RangeValue::RangeValue(size_t val)
:
_isUnbound(false),
_val(val)
{}

size_t RangeValue::value() const {
    if (_isUnbound)
        throw std::runtime_error("Attempting to access unbound size_t val from RangeVal.");
    return _val;
}

bool RangeValue::isUnbound() const {
    return _isUnbound;
}

RangeValue RangeValue::operator+ (size_t other) const {
    RangeValue retval;
    if (!_isUnbound)
        retval.set(_val + other);
    return retval;
}

RangeValue RangeValue::operator+ (int other) const {
    if (other < 0)
        return (*this)-static_cast<size_t>(-1*other);
    else
        return (*this)+static_cast<size_t>(other);
}

RangeValue RangeValue::operator+ (const RangeValue& other) const {
    if (other.isUnbound())
        return RangeValue();
    else
        return (*this)+other.value();
}

RangeValue RangeValue::operator* (size_t other) const {
    RangeValue retval;
    if (!_isUnbound)
        retval.set(_val * other);
    return retval;
}

RangeValue RangeValue::operator* (const RangeValue& other) const {
    RangeValue retval;
    if (other.isUnbound())
        return retval;
    else {
        return (*this)*other.value();
    }
}

RangeValue RangeValue::operator- (size_t other) const {
    RangeValue retval;
    if (_isUnbound)
        return retval;
    else {
        if (other > _val)
            retval.set(0);
        else
            retval.set(_val - other);
        return retval;
    }
}

RangeValue RangeValue::operator- (const RangeValue& other) const {
    if (other.isUnbound() && !(this->isUnbound())) {
        std::stringstream ss;
        ss << "Subtracting unbound range " << other << " from bound range " << *this;
        throw std::runtime_error(ss.str());
    }
    else if (other.isUnbound()) {
        // both unbound
        return RangeValue();
    }
    else {
        return (*this)-other.value();
    }
}

RangeValue RangeValue::operator- (int other) const {
    RangeValue retval;
    if (_isUnbound)
        return retval;
    else if (other < 0) {
        return (*this) + static_cast<size_t>(-1*other);
    }
    else
        return (*this) - static_cast<size_t>(other);
}

RangeValue RangeValue::operator/ (size_t other) const {
    RangeValue retval;
    if (!_isUnbound) {
        if (other == 0) {
            std::stringstream ss;
            ss << "Dividing range " << *this << " by 0.";
            throw std::runtime_error(ss.str());
        }
        retval.set(_val / other);
    }
    return retval;
}

RangeValue RangeValue::operator/ (const RangeValue& other) const {
    if (other.isUnbound()) {
        std::stringstream ss;
        ss << "Dividing range " << *this << " by unbound value.";
        throw std::runtime_error(ss.str());
    }
    else
        return (*this)/other.value();
}

RangeValue RangeValue::divideAndRoundUp(const RangeValue& other) const {

    if (_isUnbound || other.isUnbound()) {
        return *this / other;
    }
    else if (_val == 0) {
        return RangeValue(0);
    }
    else {
        return ((_val - 1) / other.value()) + 1;
    }

}

RangeValue RangeValue::divideAndRoundUp(size_t other) const {
    if (_isUnbound || other == 0) {
        return RangeValue();
    }
    else if (_val == 0) {
        return RangeValue(0);
    }
    else {
        return ((_val - 1) / other) + 1;
    }
}

bool RangeValue::operator< (size_t other) const {
    if (_isUnbound)
        return false;
    else
        return (_val < other);
}

bool RangeValue::operator< (const RangeValue& other) const {
    if (_isUnbound)
        return false;
    else if (other.isUnbound())
        return true;
    else
        return (_val < other.value());
}


bool RangeValue::operator<= (size_t other) const {
    if (_isUnbound)
        return false;
    else
        return (_val <= other);
}

bool RangeValue::operator<= (const RangeValue& other) const {
    if (other.isUnbound())
        return true;
    else if (_isUnbound)
        return false;
    else
        return (_val <= other.value());
}


bool RangeValue::operator> (size_t other) const {
    if (_isUnbound)
        return true;
    else
        return _val > other;
}

bool RangeValue::operator> (const RangeValue& other) const {
    if (_isUnbound)
        return true;
    else if (other.isUnbound())
        return false;
    else
        return _val > other.value();
}


bool RangeValue::operator>= (size_t other) const {
    if (_isUnbound)
        return true;
    else
        return _val >= other;
}

bool RangeValue::operator>= (const RangeValue& other) const {
    if (_isUnbound)
        return true;
    else if (other.isUnbound())
        return false;
    else
        return _val >= other.value();
}

void RangeValue::set(size_t inval) {
    _val = inval;
    _isUnbound = false;
}

void RangeValue::set(const RangeValue& val) {
    if (val.isUnbound())
        _isUnbound = true;
    else {
        _val = val.value();
        _isUnbound = false;
    }
}

std::ostream& operator<<(std::ostream& out, const RangeValue& shape) {
    if (shape.isUnbound()) {
        out << std::string("inf");
    }
    else {
        out << shape.value();
    }
    return out;
}

#pragma mark ShapeRange

ShapeRange::ShapeRange()
:
_minimum(0),
_maximum()
{}

ShapeRange::ShapeRange(size_t min)
:
_minimum(min),
_maximum()
{}

ShapeRange::ShapeRange(size_t min, size_t max)
:
_minimum(min),
_maximum(max)
{
    if (min > max) {
        std::stringstream ss;
        ss << "Constructing invalid ShapeRange with " << min << ", " << max;
        throw std::runtime_error(ss.str());
    }
}

ShapeRange::ShapeRange(const RangeValue& min, const RangeValue& max)
:
_minimum(min),
_maximum(max)
{
    if (min > max)
    {
        std::stringstream ss;
        ss << "Constructing invalid ShapeRange with " << min << ", " << max;
        throw std::runtime_error(ss.str());
    }
    if (min.isUnbound()) {
        std::stringstream ss;
        ss << "Constructing invalid ShapeRange unbound minimum value.";
        throw std::runtime_error(ss.str());
    }
}


ShapeRange::ShapeRange(const Specification::SizeRange& range) {
    _minimum = static_cast<size_t>(range.lowerbound());
    int64_t max = range.upperbound();
    if (max < 0) {
        _maximum = RangeValue();
    }
    else {
        _maximum = static_cast<size_t>(max);
    }
}

RangeValue ShapeRange::minimum() const {
    return _minimum;
}

RangeValue ShapeRange::maximum() const {
    return _maximum;
}

ShapeRange ShapeRange::operator+ (size_t val) const {
    ShapeRange out;
    out.setLower(_minimum + val);
    out.setUpper(_maximum + val);
    return out;
}

ShapeRange ShapeRange::operator- (size_t val) const {
    ShapeRange out;
    out.setLower(_minimum - val);
    out.setUpper(_maximum - val);
    return out;
}

ShapeRange ShapeRange::operator* (size_t val) const {
    ShapeRange out;
    out.setLower(_minimum * val);
    out.setUpper(_maximum * val);
    return out;
}

ShapeRange ShapeRange::operator/ (size_t val) const {
    ShapeRange out;
    out.setLower(_minimum / val);
    out.setUpper(_maximum / val);
    return out;
}

ShapeRange ShapeRange::divideAndRoundUp(size_t val) const {
    ShapeRange out;
    out.setLower(_minimum.divideAndRoundUp(val));
    out.setUpper(_maximum.divideAndRoundUp(val));
    return out;
}

ShapeRange ShapeRange::operator+ (const ShapeRange& other) const {
    ShapeRange out;
    out.setLower(_minimum + other.minimum());
    out.setUpper(_maximum + other.maximum());
    return out;
}

ShapeRange ShapeRange::operator+ (int val) const {
    ShapeRange out;
    out.setLower(_minimum + val);
    out.setUpper(_maximum + val);
    return out;
}

ShapeRange ShapeRange::operator- (int val) const {
    ShapeRange out;
    out.setLower(_minimum - val);
    out.setUpper(_maximum - val);
    return out;
}

ShapeRange ShapeRange::operator- (const ShapeRange& other) const {
    ShapeRange out;
    if (other.isUnbound() && this->isUnbound()) {
        return out;
    }
    out.setLower(_minimum - other.maximum());
    out.setUpper(_maximum - other.minimum());
    return out;
}

ShapeRange ShapeRange::operator* (const ShapeRange& other) const {
    ShapeRange out;
    out.setLower(_minimum * other.minimum());
    out.setUpper(_maximum * other.maximum());
    return out;
}

ShapeRange ShapeRange::operator/ (const ShapeRange& other) const {
    ShapeRange out;
    out.setLower(_minimum / other.maximum());
    out.setUpper(_maximum / other.minimum());
    return out;
}

// Just checks for negative values -- keeps from doing it in every layer
ShapeRange ShapeRange::operator/ (int val) const {
    if (val <= 0) {
        std::stringstream ss;
        ss << "Dividing ShapeRange " << *this << " by negative or zero value " << val;
        throw std::runtime_error(ss.str());
    }
    else {
        return (*this)/(static_cast<size_t>(val));
    }
}



bool ShapeRange::isValid(size_t val) const {
    return (_minimum <= val) && (_maximum >= val);
}

bool ShapeRange::isValid(const RangeValue& val) const {
    return (_minimum <= val) && (_maximum >= val);
}


void ShapeRange::setLower(size_t val) {
    if (isValid(val)) {
        _minimum.set(val);
    }
    else {
        std::stringstream ss;
        ss << "Invalid setLower " << val << " for range: " << *this << "\n";
        throw std::runtime_error(ss.str());
    }
}

void ShapeRange::setLower(const RangeValue& val) {

    if (isValid(val)) {
        _minimum.set(val);
    }
    else {
        std::stringstream ss;
        ss << "Invalid setLower " << val << " for range: " << *this << "\n";
        throw std::runtime_error(ss.str());
    }

}

void ShapeRange::setUpper(size_t val) {
    if (isValid(val)) {
        _maximum.set(val);
    }
    else {
        std::stringstream ss;
        ss << "Invalid setUpper " << val << " for range: " << *this << "\n";
        throw std::runtime_error(ss.str());
    }
}

void ShapeRange::setUpper(const RangeValue& val) {
    if (isValid(val)) {
        _maximum.set(val);
    }
    else {
        std::stringstream ss;
        ss << "Invalid setUpper " << val << " for range: " << *this << "\n";
        throw std::runtime_error(ss.str());
    }
}

void ShapeRange::setValue(size_t val) {
    if (isValid(val)) {
        _minimum.set(val);
        _maximum.set(val);
    }
    else {
        std::stringstream ss;
        ss << "Invalid setValue " << val << " for range: " << *this << "\n";
        throw std::runtime_error(ss.str());
    }
}

void ShapeRange::setValue(const RangeValue& val) {
    if (val.isUnbound())
        throw std::runtime_error("Can't set shape range to have value 'unbound'.");

    if (isValid(val)) {
        _minimum.set(val);
        _maximum.set(val);
    }
    else {
        std::stringstream ss;
        ss << "Invalid setValue " << val << " for range: " << *this << "\n";
        throw std::runtime_error(ss.str());
    }
}
ShapeRange ShapeRange::intersect(const ShapeRange& other) const {

    // These routines will throw if it's invalid (i.e. max < min)
    ShapeRange out;
    if (_minimum <= other.minimum())
        out.setLower(other.minimum());
    else {
        out.setLower(_minimum);
    }

    if (_maximum >= other.maximum())
        out.setUpper(other.maximum());
    else {
        out.setUpper(_maximum);
    }

    if (_minimum > _maximum || _minimum.isUnbound()) {
        std::stringstream ss;
        ss << "Invalid intersection between " << *this << " and " << other;
        throw std::runtime_error(ss.str());
    }

    return out;

}

ShapeRange ShapeRange::unify(const ShapeRange& other) const {
    ShapeRange out;

    if (_minimum <= other.minimum())
        out.setLower(_minimum);
    else
        out.setLower(other.minimum());

    if (_maximum >= other.maximum())
        out.setUpper(_maximum);
    else
        out.setUpper(other.maximum());

    return out;
}

bool ShapeRange::isUnbound() const {
    return _maximum.isUnbound();
}

bool ShapeRange::equals(size_t val) const {
    return (_minimum.value() == val) && (!_maximum.isUnbound()) && (_maximum.value() == val);
}

size_t ShapeRange::minimumValue() const {
    return _minimum.value();
}

RangeValue ShapeRange::maximumValue() const {
    return _maximum;
}

bool ShapeRange::isFixed() const {
    return (!_maximum.isUnbound() && (_maximum.value() == _minimum.value()));
}

std::ostream& operator<<(std::ostream& out, const ShapeRange& shape) {
    out << "[" << shape.minimum() << ", " << shape.maximum() << "]";
    return out;
}


# pragma mark ShapeConstraint

ShapeConstraint::ShapeConstraint(const std::string& name)
:
_name(name) {}

ShapeConstraint::ShapeConstraint() {}

void ShapeConstraint::updateConstraint(const Specification::FeatureType& type) {

    if (type.Type_case() == Specification::FeatureType::kImageType) {

        // Handle the number of channels first
        if (type.imagetype().colorspace() == Specification::ImageFeatureType_ColorSpace_GRAYSCALE)
            setChannel(1);
        else {
            setChannel(3);
        }

        switch (type.imagetype().SizeFlexibility_case()) {

            case CoreML::Specification::ImageFeatureType::kEnumeratedSizes: {

                size_t min_width = SIZE_MAX;
                size_t max_width = 0;
                size_t min_height = SIZE_MAX;
                size_t max_height = 0;
                for (int i = 0; i < type.imagetype().enumeratedsizes().sizes_size(); i++) {
                    size_t width = (size_t)type.imagetype().enumeratedsizes().sizes(i).width();
                    if (width > max_width)
                        max_width = width;
                    if (width < min_width)
                        min_width = width;

                    size_t height = (size_t)type.imagetype().enumeratedsizes().sizes(i).height();
                    if (height > max_height)
                        max_height = height;
                    if (height < min_height)
                        min_height = height;
                }

                // This is a hack, in that it loses the numerated nature of the constraint
                ShapeRange widthRange = ShapeRange(min_width, max_width);
                updateWidthRange(widthRange);

                ShapeRange heightRange = ShapeRange(min_height, max_height);
                updateHeightRange(heightRange);

                break;
            }
            case CoreML::Specification::ImageFeatureType::kImageSizeRange: {

                ShapeRange allowedWidths(type.imagetype().imagesizerange().widthrange());
                updateWidthRange(allowedWidths);

                ShapeRange allowedHeights(type.imagetype().imagesizerange().heightrange());
                updateHeightRange(allowedHeights);

                break;
            }
            case CoreML::Specification::ImageFeatureType::SIZEFLEXIBILITY_NOT_SET: {
                // the back compatible portion -- if the shape isn't set, then we'll use the old fields
                setHeight(static_cast<size_t>(type.imagetype().height()));
                setWidth(static_cast<size_t>(type.imagetype().width()));

                break;
            }

        } // switch

    } // is an image
    else if (type.Type_case() == Specification::FeatureType::kMultiArrayType) {

        std::vector<ShapeRange> ranges;

        switch (type.multiarraytype().ShapeFlexibility_case()) {

            case CoreML::Specification::ArrayFeatureType::kEnumeratedShapes: {
                int maxDims = 0;
                for (const auto &shape : type.multiarraytype().enumeratedshapes().shapes()) {
                    if (shape.shape_size() > maxDims) {
                        maxDims = shape.shape_size();
                    }
                }
                for (int d=0; d<maxDims; d++) {
                    size_t minSize = SIZE_MAX;
                    size_t maxSize = 0;
                    for (int i = 0; i < type.multiarraytype().enumeratedshapes().shapes_size(); i++) {
                        size_t size = static_cast<size_t>(type.multiarraytype().enumeratedshapes().shapes(i).shape(d));
                        if (minSize > size)
                            minSize = size;
                        if (maxSize < size)
                            maxSize = size;
                    }
                    ranges.push_back(ShapeRange(minSize,maxSize));
                }
                break;
            }
            case CoreML::Specification::ArrayFeatureType::kShapeRange: {
                for (int i = 0; i < type.multiarraytype().shaperange().sizeranges_size(); i++) {
                    ranges.push_back(ShapeRange(type.multiarraytype().shaperange().sizeranges(i)));
                }
                break;
            }
            case  CoreML::Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                break;
        }

        if (ranges.size() == 1) {
            updateChannelRange(ranges[0]);
        }
        else if (ranges.size() == 3) {
            updateChannelRange(ranges[0]);
            updateHeightRange(ranges[1]);
            updateWidthRange(ranges[2]);
        }
        else if (ranges.size() == 0) {
            // legacy case for older models.
            setChannel(static_cast<size_t>(type.multiarraytype().shape(0)));
            if (type.multiarraytype().shape_size() == 3) {
                setHeight(static_cast<size_t>(type.multiarraytype().shape(1)));
                setWidth(static_cast<size_t>(type.multiarraytype().shape(2)));
            }
            else {
                setHeight(1);
                setWidth(1);
            }
        }
        else {
            throw std::runtime_error("Attempting to constrain an input or output feature \"" + _name + "\" with an invalid array shape constraint.");
        }

    }
    else {
        throw std::runtime_error("Attempting to update feature constraint " + _name + " with a type description which is not a multi array or image.");
    }

}

void ShapeConstraint::setName(const std::string& name) {
    _name = name;
}

const ShapeRange& ShapeConstraint::sequenceRange() const {
    return _sequenceRange;
}

const ShapeRange& ShapeConstraint::batchRange() const {
    return _batchRange;
}

const ShapeRange& ShapeConstraint::channelRange() const {
    return _channelRange;
}

const ShapeRange& ShapeConstraint::heightRange() const {
    return _heightRange;
}

const ShapeRange& ShapeConstraint::widthRange() const {
    return _widthRange;
}

void ShapeConstraint::updateSequenceRange(const ShapeRange& other) {
    try {
        _sequenceRange = _sequenceRange.intersect(other);
    }
    catch (std::runtime_error& e) {
        std::string err = "Invalid sequence range in blob " + _name + ". " + e.what();
        throw std::runtime_error(err);
    }
}

void ShapeConstraint::updateBatchRange(const ShapeRange& other) {
    try {
        _batchRange = _batchRange.intersect(other);
    }
    catch (std::runtime_error& e) {
        std::string err = "Invalid batch range in blob " + _name + ". " + e.what();
        throw std::runtime_error(err);
    }
}

void ShapeConstraint::updateChannelRange(const ShapeRange& other) {
    try {
        _channelRange = _channelRange.intersect(other);
    }
    catch (std::runtime_error& e) {
        std::string err = "Invalid channel range in blob " + _name + ". " + e.what();
        throw std::runtime_error(err);
    }
}

void ShapeConstraint::updateHeightRange(const ShapeRange& other) {
    try {
        _heightRange = _heightRange.intersect(other);
    }
    catch (std::runtime_error& e) {
        std::string err = "Invalid height range in blob " + _name + ". " + e.what();
        throw std::runtime_error(err);
    }
}

void ShapeConstraint::updateWidthRange(const ShapeRange& other) {
    try {
        _widthRange = _widthRange.intersect(other);
    }
    catch (std::runtime_error& e) {
        std::string err = "Invalid width range in blob " + _name + ". " + e.what();
        throw std::runtime_error(err);
    }
}

std::string ShapeConstraint::name() const {
    return _name;
}

void ShapeConstraint::copyFrom(const ShapeConstraint& other) {
    _sequenceRange = _sequenceRange.intersect(other.sequenceRange());
    _batchRange = _batchRange.intersect(other.batchRange());
    _channelRange = _channelRange.intersect(other.channelRange());
    _heightRange = _heightRange.intersect(other.heightRange());
    _widthRange = _widthRange.intersect(other.widthRange());
}

void ShapeConstraint::copyFromNoBatchSeq(const ShapeConstraint& other) {
    _channelRange = _channelRange.intersect(other.channelRange());
    _heightRange = _heightRange.intersect(other.heightRange());
    _widthRange = _widthRange.intersect(other.widthRange());
}

void ShapeConstraint::upperBoundSequence(size_t val) {
    _sequenceRange = _sequenceRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundBatch(size_t val) {
    _batchRange = _batchRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundChannel(size_t val) {
    _channelRange = _channelRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundHeight(size_t val) {
    _heightRange = _heightRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundWidth(size_t val) {
    _widthRange = _widthRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundSequence(const RangeValue& val) {
    if (!val.isUnbound())
        _sequenceRange = _sequenceRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundBatch(const RangeValue& val) {
    if (!val.isUnbound())
        _batchRange = _batchRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundChannel(const RangeValue& val) {
    if (!val.isUnbound())
        _channelRange = _channelRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundHeight(const RangeValue& val) {
    if (!val.isUnbound())
        _heightRange = _heightRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::upperBoundWidth(const RangeValue& val) {
    if (!val.isUnbound())
        _widthRange = _widthRange.intersect(ShapeRange(0, val));
}

void ShapeConstraint::lowerBoundSequence(size_t val) {
    _sequenceRange = _sequenceRange.intersect(ShapeRange(val));
}

void ShapeConstraint::lowerBoundBatch(size_t val) {
    _batchRange = _batchRange.intersect(ShapeRange(val));
}

void ShapeConstraint::lowerBoundChannel(size_t val) {
    _channelRange = _channelRange.intersect(ShapeRange(val));
}

void ShapeConstraint::lowerBoundHeight(size_t val) {
    _heightRange = _heightRange.intersect(ShapeRange(val));
}

void ShapeConstraint::lowerBoundWidth(size_t val) {
    _widthRange = _widthRange.intersect(ShapeRange(val));
}

void ShapeConstraint::setSequence(size_t val) {
    _sequenceRange.setValue(val);
}

void ShapeConstraint::setBatch(size_t val) {
    _batchRange.setValue(val);
}

void ShapeConstraint::setChannel(size_t val) {
    _channelRange.setValue(val);
}

void ShapeConstraint::setHeight(size_t val) {
    _heightRange.setValue(val);
}

void ShapeConstraint::setWidth(size_t val) {
    _widthRange.setValue(val);
}

bool ShapeConstraint::isValid() const {
    return true; // we can't create one that is invalid
}

size_t ShapeConstraint::minimumSequenceLength() const {
    return _sequenceRange.minimumValue();
}

size_t ShapeConstraint::minimumBatchSize() const {
    return _batchRange.minimumValue();
}

size_t ShapeConstraint::minimumChannelSize() const {
    return _channelRange.minimumValue();
}

size_t ShapeConstraint::minimumHeight() const {
    return _heightRange.minimumValue();
}

size_t ShapeConstraint::minimumWidth() const {
    return _widthRange.minimumValue();
}

bool ShapeConstraint::hasFixedCHW() const {
    return (_channelRange.isFixed() && _heightRange.isFixed() && _widthRange.isFixed());
}

std::ostream& operator<<(std::ostream& out, const ShapeConstraint& shape) {
    out << shape.name() << std::string(":") << std::endl;
    out << shape.sequenceRange() << std::endl;
    out << shape.batchRange() << std::endl;
    out << shape.channelRange() << std::endl;
    out << shape.heightRange() << std::endl;
    out << shape.widthRange() << std::endl;
    return out;
}
