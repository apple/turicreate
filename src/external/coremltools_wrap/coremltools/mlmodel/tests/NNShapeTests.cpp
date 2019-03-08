//
//  NNShapeTests.cpp
//  mlmodel
//
//  Created by William March on 12/8/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#include "MLModelTests.hpp"
#include "../src/Format.hpp"
#include "../src/Model.hpp"
#include "../src/LayerShapeConstraints.hpp"

#include "framework/TestUtils.hpp"

using namespace CoreML;

int testRangeVal() {

    // Unbound constructor
    RangeValue test1 = RangeValue();
    ML_ASSERT(test1.isUnbound());

    // Relations between set and unbound
    RangeValue test2 = RangeValue(2);
    ML_ASSERT(test1 > test2);
    ML_ASSERT(test1 >= test2);
    ML_ASSERT(test2 < test1);
    ML_ASSERT(test2 <= test1);

    ML_ASSERT(test1 > 2);
    ML_ASSERT(test1 >= 2);

    ML_ASSERT(!(test1 < 6));
    ML_ASSERT(!(test1 <= 6));

    // Relations between two set values
    RangeValue test3 = RangeValue(3);
    ML_ASSERT(test3 > test2);
    ML_ASSERT(test3 >= test2);
    ML_ASSERT(test2 < test3);
    ML_ASSERT(test2 <= test3);

    ML_ASSERT(test2 < 3);
    ML_ASSERT(test2 <= 3);
    ML_ASSERT(test2 > 1);
    ML_ASSERT(test2 >= 1);
    ML_ASSERT(test2 >= 2);


    // Test two unbound values
    RangeValue test4 = RangeValue();
    ML_ASSERT(test4 > test1);
    ML_ASSERT(test1 > test4);
    ML_ASSERT(!(test4 < test1));
    ML_ASSERT(!(test1 < test4));

    ML_ASSERT(test4 >= test1);
    ML_ASSERT(test1 >= test4);
    ML_ASSERT(test4 <= test1);
    ML_ASSERT(test1 <= test4);


    ML_ASSERT(test2.value() == 2);
    bool catch_me = false;
    try {
        test1.value();
        ML_ASSERT(0);
    }
    catch (std::runtime_error& err) {
        // this should throw, so we're ok -- this is a pass
        ML_ASSERT(1);
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    // Test arithmetic
    RangeValue add1 = test1 + test2;
    ML_ASSERT(add1.isUnbound());
    RangeValue add2 = test2 + test3;
    ML_ASSERT(add2.value() == 5);
    RangeValue add3 = test1 + 4;
    ML_ASSERT(add3.isUnbound());
    RangeValue add4 = test2 + 4;
    ML_ASSERT(add4.value() == 6);

    RangeValue mul1 = test1 * test2;
    ML_ASSERT(mul1.isUnbound());
    RangeValue mul2 = test2 * test3;
    ML_ASSERT(mul2.value() == 6);
    RangeValue mul3 = test1 * 4;
    ML_ASSERT(mul3.isUnbound());
    RangeValue mul4 = test2 * 4;
    ML_ASSERT(mul4.value() == 8);

    RangeValue sub1 = test1 - test2;
    ML_ASSERT(sub1.isUnbound());
    RangeValue sub2 = test2 - test3;
    ML_ASSERT(sub2.value() == 0);
    RangeValue sub3 = test1 - 4;
    ML_ASSERT(sub3.isUnbound());
    RangeValue sub4 = test2 - 4;
    ML_ASSERT(sub4.value() == 0);
    RangeValue sub5 = test3 - test2;
    ML_ASSERT(sub5.value() == 1);
    RangeValue sub6 = test3 - 1;
    ML_ASSERT(sub6.value() == 2);

    catch_me = false;
    try {
        RangeValue sub7 = test2 - test1;
        sub7.value();
        ML_ASSERT(0);
    }
    catch (std::runtime_error& err) {
        ML_ASSERT(1);
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    catch_me = false;
    try {
        RangeValue sub8 = test4 - test1;
        sub8.value();
        ML_ASSERT(0);
    }
    catch (std::runtime_error& err) {
        ML_ASSERT(1);
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    RangeValue sub9 = test1 - -5;
    ML_ASSERT(sub9.isUnbound());
    RangeValue sub10 = test2 - -1;
    ML_ASSERT(sub10.value() == 3);
    RangeValue sub11 = test1 - 50;
    ML_ASSERT(sub11.isUnbound());
    RangeValue sub12 = test2 - 50;
    ML_ASSERT(sub12.value() == 0);

    RangeValue div1 = test1 / test2;
    ML_ASSERT(div1.isUnbound());
    RangeValue div2 = test2 / test3;
    ML_ASSERT(div2.value() == 0);
    RangeValue div3 = test1 / 4;
    ML_ASSERT(div3.isUnbound());
    RangeValue div4 = RangeValue(10) / 2;
    ML_ASSERT(div4.value() == 5);
    RangeValue div5 = test3 / test2;
    ML_ASSERT(div5.value() == 1);
    RangeValue div6 = test3 / 1;
    ML_ASSERT(div6.value() == 3);

    catch_me = false;
    try {
        RangeValue div7 = test2 / test1;
        div7.value();
        ML_ASSERT(0);
    }
    catch (std::runtime_error& err) {
        ML_ASSERT(1);
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    catch_me = false;
    try {
        RangeValue div8 = test4 / test1;
        div8.value();
        ML_ASSERT(0);
    }
    catch (std::runtime_error& err) {
        ML_ASSERT(1);
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    test1.set(10);
    ML_ASSERT(!test1.isUnbound());
    ML_ASSERT(test1.value() == 10);
    test2.set(test3);
    ML_ASSERT(!test2.isUnbound());
    ML_ASSERT(test2.value() == 3);

    return 0;
}

int testRangeValDivide() {

    RangeValue r1;
    RangeValue r2(1);
    RangeValue r3(5);
    RangeValue r4(12);

    RangeValue r5 = r1.divideAndRoundUp(r2);
    ML_ASSERT(r5.isUnbound());

    bool catch_me = false;
    try {
        RangeValue r6 = r2.divideAndRoundUp(r1);
        ML_ASSERT(r6.isUnbound()); // just to quiet the compiler
    }
    catch (std::runtime_error& err) {
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    RangeValue r7 = r2.divideAndRoundUp(r3);
    ML_ASSERT(r7.value() == 1);

    RangeValue r8 = r3.divideAndRoundUp(r2);
    ML_ASSERT(r8.value() == 5);

    RangeValue r9 = r3.divideAndRoundUp(r4);
    ML_ASSERT(r9.value() == 1);

    RangeValue r10 = r4.divideAndRoundUp(r3);
    ML_ASSERT(r10.value() == 3);

    return 0;

}

int testShapeRange() {

    ShapeRange r1 = ShapeRange();
    ShapeRange r2 = ShapeRange(10);
    ShapeRange r3 = ShapeRange(8, 20);
    ShapeRange r4 = ShapeRange(RangeValue(9), RangeValue());

    bool catch_me = false;
    try {
        ShapeRange r5 = ShapeRange(RangeValue(), RangeValue());
        r5.isValid(2);
        ML_ASSERT(0);
    }
    catch (std::runtime_error& err) {
        ML_ASSERT(1);
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    ML_ASSERT(r1.isValid(5));
    ML_ASSERT(r1.isValid(RangeValue()));
    ML_ASSERT(!(r3.isValid(5)));
    ML_ASSERT(r4.isValid(RangeValue()));

    ML_ASSERT(r1.minimum().value() == 0);
    ML_ASSERT(r2.maximum().isUnbound());

    ShapeRange r5 = r1 + 2;
    ML_ASSERT(r5.minimum().value() == 2);
    ML_ASSERT(r5.maximum().isUnbound());

    ShapeRange r6 = r3 + 3;
    ML_ASSERT(r6.minimum().value() == 11);
    ML_ASSERT(r6.maximum().value() == 23);

    ShapeRange r7 = r1 * 5;
    ML_ASSERT(r7.minimum().value() == 0);
    ML_ASSERT(r7.maximum().isUnbound());

    ShapeRange r8 = r3 * 4;
    ML_ASSERT(r8.minimum().value() == 32);
    ML_ASSERT(r8.maximum().value() == 80);

    // These can throw for making the range invalid
    ShapeRange r9 = r1 - 10;
    ML_ASSERT(r9.minimum().value() == 0);
    ML_ASSERT(r9.maximum().isUnbound());

    ShapeRange r10 = r2 - 9;
    ML_ASSERT(r10.minimum().value() == 1);
    ML_ASSERT(r10.maximum().isUnbound());

    ShapeRange r11 = r3 - 22;
    ML_ASSERT(r11.minimum().value() == 0);
    ML_ASSERT(r11.maximum().value() == 0);
    ML_ASSERT(!r11.isValid(10));

    ShapeRange r12 = r2 - (-3);
    ML_ASSERT(r12.minimum().value() == 13);
    ML_ASSERT(r12.maximum().isUnbound());

    ShapeRange r13 = r1 / size_t(10);
    ML_ASSERT(r13.minimum().value() == 0);
    ML_ASSERT(r13.maximum().isUnbound());

    // integer division
    ShapeRange r14 = r3 / size_t(3);
    ML_ASSERT(r14.minimum().value() == 2);
    ML_ASSERT(r14.maximum().value() == 6);

    catch_me = false;
    try {
        ShapeRange r15 = r3 / size_t(0);
        ML_ASSERT(0);
        ML_ASSERT(r15.isValid(1));
    }
    catch (std::runtime_error& err) {
        ML_ASSERT(1);
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    ML_ASSERT(r1.isValid(0));
    ML_ASSERT(r1.isValid(1000));
    ML_ASSERT(r1.isValid(RangeValue()));
    ML_ASSERT(r3.isValid(20));
    ML_ASSERT(!r3.isValid(21));
    ML_ASSERT(r3.isValid(8));
    ML_ASSERT(!r3.isValid(7));

    ShapeRange r16 = r1 + r3;
    ML_ASSERT(r16.minimum().value() == 8);
    ML_ASSERT(r16.maximum().isUnbound());

    ShapeRange r17 = r3 + r3;
    ML_ASSERT(r17.minimum().value() == 16);
    ML_ASSERT(r17.maximum().value() == 40);

    ShapeRange r18 = r1 - r3;
    ML_ASSERT(r18.minimum().value() == 0);
    ML_ASSERT(r18.maximum().isUnbound());

    catch_me = false;
    try {
        ShapeRange r19 = r3 - r1;
        ML_ASSERT(r19.isValid(0));
    }
    catch (const std::runtime_error& err) {
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    ShapeRange r20 = ShapeRange(4, 7);
    ShapeRange r21 = r3 - r20;
    ML_ASSERT(r21.minimum().value() == 1);
    ML_ASSERT(r21.maximum().value() == 16);

    ShapeRange r22 = r1 * r3;
    ML_ASSERT(r22.minimum().value() == 0);
    ML_ASSERT(r22.maximum().isUnbound());

    ShapeRange r23 = r1 / r3;
    ML_ASSERT(r23.minimum().value() == 0);
    ML_ASSERT(r23.maximum().isUnbound());

    ShapeRange r24 = r3 / r20;
    ML_ASSERT(r24.minimum().value() == 1);
    ML_ASSERT(r24.maximum().value() == 5);

    catch_me = false;
    try {
        ShapeRange r25 = r3 / r1;
        ML_ASSERT(r25.minimum().isUnbound());
    }
    catch (std::runtime_error& err) {
        catch_me = true;
    }
    ML_ASSERT(catch_me);

//    ShapeRange intersect(const ShapeRange& other) const;

    ShapeRange r26 = r1.intersect(r2);
    ML_ASSERT(r26.minimum().value() == 10);
    ML_ASSERT(r26.maximum().isUnbound());

    ShapeRange r27 = r2.intersect(r3);
    ML_ASSERT(r27.minimum().value() == 10);
    ML_ASSERT(r27.maximum().value() == 20);

    catch_me = false;
    try {
        ShapeRange r28 = r3.intersect(ShapeRange(3, 6));
        ML_ASSERT(r28.minimum().isUnbound());
    }
    catch (std::runtime_error& err) {
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    ShapeRange r29 = r1.unify(r2);
    ML_ASSERT(r29.minimum().value() == 0);
    ML_ASSERT(r29.maximum().isUnbound());

    ShapeRange r30 = r3.unify(ShapeRange(25, 30));
    ML_ASSERT(r30.minimum().value() == 8);
    ML_ASSERT(r30.maximum().value() == 30);

    ShapeRange r31 = r1;
    r31.setValue(10);
    ML_ASSERT(r31.minimum().value() == 10);
    ML_ASSERT(r31.maximum().value() == 10);

    ShapeRange r32 = r1;
    catch_me = false;
    try {
        r32.setValue(RangeValue());
    }
    catch (std::runtime_error& err) {
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    ShapeRange r33 = r3;
    catch_me = false;
    try {
        r33.setLower(2);
    }
    catch (std::runtime_error& err) {
        catch_me = true;
    }
    ML_ASSERT(catch_me);

    ShapeRange r34 = ShapeRange().intersect(ShapeRange());
    ML_ASSERT(r34.minimum().value() == 0);
    ML_ASSERT(r34.maximum().isUnbound());

    ShapeRange r35 = ShapeRange(1, 1).intersect(ShapeRange());
    ML_ASSERT(r35.minimum().value() == 1);
    ML_ASSERT(r35.maximum().value() == 1);

    ShapeRange r36 = ShapeRange().intersect(ShapeRange(10));
    ML_ASSERT(r36.minimum().value() == 10);
    ML_ASSERT(r36.maximum().isUnbound());

    return 0;

}
