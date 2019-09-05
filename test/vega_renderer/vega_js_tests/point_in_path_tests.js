/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

function checkInPath(ctx, x, y) {
    check(ctx.isPointInPath(x, y) == true, `(${x}, ${y}) should be in path`);
}

function checkNotInPath(ctx, x, y) {
    check(!ctx.isPointInPath(x, y), `(${x}, ${y}) should NOT be in path`);
}


let canvas = document.createElement("canvas");
let ctx = canvas.getContext("2d");
assert(ctx != null, "rendering context must not be null");


// Test `isPointInPath` without translations
ctx.beginPath();
ctx.moveTo(0, 0);
ctx.lineTo(0, 100);
ctx.lineTo(100, 100);
ctx.lineTo(100, 0);
ctx.lineTo(0, 0);

checkInPath(ctx, 0, 0);
checkInPath(ctx, 50, 50);
checkInPath(ctx, 0, 100);

checkNotInPath(ctx, 0, 150);
checkNotInPath(ctx, 200, 200);

// Test `isPointInPath` with translations
ctx.translate(100, 100);
ctx.beginPath();
ctx.moveTo(0, 0);
ctx.lineTo(0, 100);
ctx.lineTo(100, 100);
ctx.lineTo(100, 0);
ctx.lineTo(0, 0);
ctx.closePath();

checkInPath(ctx, 100, 100);
checkInPath(ctx, 200, 100);
checkInPath(ctx, 150, 150);

checkNotInPath(ctx, 0, 0);
checkNotInPath(ctx, 300, 300);
checkNotInPath(ctx, 400, 300);
