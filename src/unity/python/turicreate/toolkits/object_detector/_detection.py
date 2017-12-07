# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import numpy as _np


def multi_range(*args):
    import itertools
    return itertools.product(*[range(a) for a in args])


def bbox_to_ybox(bbox):
    """Convert from corner bounding box to center/shape"""
    return [
            (bbox[1] + bbox[3]) / 2,
            (bbox[0] + bbox[2]) / 2,
            (bbox[3] - bbox[1]),
            (bbox[2] - bbox[0]),
    ]


def softmax(z):
    zz = _np.exp(z - z.max())
    return zz / zz.sum(-1, keepdims=True)


def sigmoid(z):
    return 1 / (1 + _np.exp(-z))


def intersection_over_union(bbs1, bbs2):
    assert bbs1.ndim == 2 and bbs2.ndim == 2
    bbs1_bc = bbs1[:, _np.newaxis]
    bbs2_bc = bbs2[_np.newaxis]

    inter = (_np.maximum(_np.minimum(bbs1_bc[..., 2:], bbs2_bc[..., 2:]) -
             _np.maximum(bbs1_bc[..., :2], bbs2_bc[..., :2]), 0))
    inter_area = inter[..., 0] * inter[..., 1]
    area1 = (bbs1_bc[..., 2] - bbs1_bc[..., 0]) * (bbs1_bc[..., 3] - bbs1_bc[..., 1])
    area2 = (bbs2_bc[..., 2] - bbs2_bc[..., 0]) * (bbs2_bc[..., 3] - bbs2_bc[..., 1])

    return inter_area / (area1 + area2 - inter_area + 1e-5)


# Class-independent NMS
def non_maximum_suppression(boxes, classes, scores, num_classes, threshold=0.5,
                            limit=None):
    np_scores = _np.array(scores)
    np_boxes = _np.array(boxes)
    np_classes = _np.array(classes)

    new_boxes = []
    new_classes = []
    new_scores = []
    for c in range(num_classes):
        c_scores = np_scores[np_classes == c]
        c_boxes = np_boxes[np_classes == c]
        ii = _np.argsort(c_scores)[::-1]
        c_scores = c_scores[ii]
        c_boxes = c_boxes[ii]
        keep = _np.ones(len(c_scores)).astype(_np.bool)
        for i in range(len(c_scores)):
            if keep[i]:
                ious = intersection_over_union(c_boxes[[i]], c_boxes[i+1:])[0]
                keep[i + 1:] &= ious <= threshold

        c_scores = c_scores[keep]
        c_boxes = c_boxes[keep]

        new_boxes.extend(c_boxes)
        new_classes.extend(_np.full(len(c_boxes), c, dtype=_np.int32))
        new_scores.extend(c_scores)

    new_boxes = _np.array(new_boxes).reshape(-1, 4)
    new_scores = _np.array(new_scores)
    new_classes = _np.array(new_classes)

    ii = _np.argsort(-new_scores)[:limit]

    new_boxes = new_boxes[ii]
    new_scores = new_scores[ii]
    new_classes = new_classes[ii]

    return new_boxes, new_classes, new_scores


def yolo_map_to_bounding_boxes(output, anchors, confidence_threshold=0.3,
                               block_size=32, nms_thresh=0.5, limit=None):
    assert output.shape[0] == 1, "For now works on single images"
    num_anchors = output.shape[-2]
    num_classes = output.shape[-1] - 5

    boxes = []
    classes = []
    scores = []
    for cy, cx, b in multi_range(output.shape[1], output.shape[2], num_anchors):
        tx, ty, tw, th, tc = output[0, cy, cx, b, :5]

        x = (cx + sigmoid(tx)) * block_size
        y = (cy + sigmoid(ty)) * block_size

        w = _np.exp(tw) * anchors[b][0] * block_size
        h = _np.exp(th) * anchors[b][1] * block_size

        confidence = sigmoid(tc)

        classpreds = softmax(output[0, cy, cx, b, 5:])

        for i in range(num_classes):
            class_score = classpreds[i]

            confidence_in_class = class_score * confidence
            if confidence_in_class > confidence_threshold:
                boxes.append([y - h/2, x - w/2, y + h/2, x + w/2])
                classes.append(int(i))
                scores.append(confidence_in_class)

    if nms_thresh is not None:
        boxes, classes, scores = non_maximum_suppression(
                boxes, classes, scores,
                num_classes=num_classes, threshold=nms_thresh,
                limit=None)

    return boxes, classes, scores


def yolo_boxes_to_yolo_map(gt_mxboxes, input_shape, output_shape,
                           num_classes, anchors):

    num_anchors = len(anchors)

    # The 5 are: y, x, h, w, class id
    ymap_shape = tuple(output_shape) + (num_anchors, 5 + num_classes)
    ymap = _np.zeros(ymap_shape, dtype=_np.float32)

    for gt_mxbox, gt_cls in zip(gt_mxboxes[:, 1:], gt_mxboxes[:, 0]):
        gt_cls = int(gt_cls)
        if gt_cls < 0:
            break

        x = output_shape[1] * (gt_mxbox[0] + gt_mxbox[2]) / 2
        y = output_shape[0] * (gt_mxbox[1] + gt_mxbox[3]) / 2
        w = output_shape[1] * (gt_mxbox[2] - gt_mxbox[0])
        h = output_shape[0] * (gt_mxbox[3] - gt_mxbox[1])

        # Skip boxes with zero area (this usually means that they have been
        # transformed to lie outside the image and then clipped to the edge
        if w * h < 1e-3:
            continue

        iy = int(_np.floor(y))
        ix = int(_np.floor(x))

        if 0 <= iy < output_shape[0] and 0 <= ix < output_shape[1]:
            # YOLO uses (x, y)/(w, h) order and not (y, x)/(h, w)
            ymap[iy, ix, :, 0] = x - _np.floor(x)
            ymap[iy, ix, :, 1] = y - _np.floor(y)
            ymap[iy, ix, :, 2] = w
            ymap[iy, ix, :, 3] = h
            ymap[iy, ix, :, 4] = 1.0
            ymap[iy, ix, :, 5+gt_cls] = 1

    return ymap
