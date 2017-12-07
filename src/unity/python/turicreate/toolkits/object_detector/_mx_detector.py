# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import mxnet as _mx
import numpy as _np


@_mx.init.register
class ConstantArray(_mx.init.Initializer):
    def __init__(self, value):
        super(ConstantArray, self).__init__(value=value)
        self.value = value

    def _init_weight(self, _, arr):
        arr[:] = _mx.nd.array(self.value)


class YOLOLoss(_mx.gluon.loss.Loss):
    def __init__(self, input_shape, output_shape, batch_size, num_classes,
                 anchors, parameters, batch_axis=0, **kwargs):
        super(YOLOLoss, self).__init__(1.0, batch_axis, **kwargs)
        self._batch_axis = batch_axis
        self._weight = 1.0
        self.input_shape = input_shape
        self.output_shape = output_shape
        self.batch_size = batch_size
        self.num_classes = num_classes
        self.anchors = anchors
        self.parameters = parameters

    def hybrid_forward(self, F, s_model, s_ymap):
        output_shape = self.output_shape
        batch_size = self.batch_size
        num_classes = self.num_classes
        anchors = self.anchors
        params = self.parameters
        num_anchors = len(anchors)

        ypred = s_model
        ymap = s_ymap

        lmb_coord_xy = params['lmb_coord_xy']
        lmb_coord_wh = params['lmb_coord_wh']
        lmb_obj = params['lmb_obj']
        lmb_noobj = params['lmb_noobj']
        lmb_class = params['lmb_class']
        rescore = bool(params['rescore'])

        f_axis = 4
        raw_xy = F.slice_axis(ypred, axis=f_axis, begin=0, end=2)
        raw_wh = F.slice_axis(ypred, axis=f_axis, begin=2, end=4)
        raw_conf1 = F.slice_axis(ypred, axis=f_axis, begin=4, end=5)
        raw_conf = raw_conf1.reshape((-1,) + tuple(output_shape) + (num_anchors,))
        class_scores = F.slice_axis(ypred, axis=f_axis, begin=5, end=5 + num_classes)

        if F == _mx.sym:
            mx_anchors = F.Variable('anchors', shape=_np.shape(anchors),
                                    init=ConstantArray(anchors))
        else:
            mx_anchors = F.array(anchors)

        gt_xy = F.slice_axis(ymap, axis=f_axis, begin=0, end=2)
        gt_wh = F.slice_axis(ymap, axis=f_axis, begin=2, end=4)

        gt_raw_wh = F.log(F.broadcast_div(gt_wh, mx_anchors) + 1e-5)
        gt_conf1 = F.slice_axis(ymap, axis=f_axis, begin=4, end=5)
        gt_conf = gt_conf1.reshape((-1,) + tuple(output_shape) + (num_anchors,))
        gt_class = F.slice_axis(ymap, axis=f_axis, begin=5, end=5+num_classes)
        gt_class_index = F.argmax(gt_class, axis=-1)

        xy = F.sigmoid(raw_xy)
        wh = F.broadcast_mul(F.exp(raw_wh), mx_anchors)

        lo = xy - wh / 2
        hi = xy + wh / 2

        gt_area = F.prod(gt_wh, axis=-1)
        gt_lo = gt_xy - gt_wh / 2
        gt_hi = gt_xy + gt_wh / 2

        c_inter = F.maximum(2 * F.broadcast_minimum(mx_anchors/2, gt_wh/2), 0)
        c_area = F.prod(mx_anchors, axis=-1)
        c_inter_area = F.prod(c_inter, axis=-1)
        c_iou = c_inter_area / (F.broadcast_add(gt_area, c_area) - c_inter_area)

        inter = F.maximum(F.minimum(hi, gt_hi) - F.maximum(lo, gt_lo), 0)
        area = F.prod(wh, axis=-1)
        inter_area = F.prod(inter, axis=-1)
        iou = inter_area / (area + gt_area - inter_area)

        max_iou = F.max(c_iou, axis=3, keepdims=True)

        POS_IOU = 0.7
        NEG_IOU = 0.3


        cond_gt = gt_conf == 1.0
        cond_max = F.broadcast_equal(c_iou, max_iou)
        cond_above = c_iou > POS_IOU

        def logical_or(x, y):
            return F.clip(x + y, 0.0, 1.0)

        def logical_and(x, y):
            return F.clip(x * y, 0.0, 1.0)

        def broadcast_logical_or(x, y):
            return F.clip(F.broadcast_add(x, y), 0.0, 1.0)

        def broadcast_logical_and(x, y):
            return F.clip(F.broadcast_mul(x, y), 0.0, 1.0)

        def logical_not(x):
            return 1 - x

        cond_obj = logical_or(logical_and(cond_gt, cond_max), cond_above)

        kr_obj_ij = F.stop_gradient(cond_obj)

        cond_below = c_iou < NEG_IOU
        cond_noobj = logical_and(cond_below, logical_not(cond_obj))

        kr_noobj_ij = F.stop_gradient(cond_noobj)

        count = F.sum(kr_obj_ij)
        eps_count = count + 1e-4

        scale_conf = 1 / (batch_size * output_shape[0] * output_shape[1])

        kr_obj_ij_plus1 = F.expand_dims(kr_obj_ij, axis=4)
        if rescore:
            obj_gt_conf = kr_obj_ij * F.stop_gradient(iou)
        else:
            obj_gt_conf = kr_obj_ij

        obj_w = kr_obj_ij * lmb_obj + kr_noobj_ij * lmb_noobj

        loss_xy = F.broadcast_div(lmb_coord_xy * F.sum(
                F.broadcast_mul(kr_obj_ij_plus1, F.square(gt_xy - xy))
            ), eps_count)

        def huber_loss(x, delta):
            abs_x = F.abs(x)
            inside = abs_x < delta
            value_i = F.square(x) / 2
            value_o = delta * (abs_x - delta / 2)
            return value_i * inside + value_o * (1 - inside)

        each_loss_wh = huber_loss(gt_raw_wh - raw_wh, delta=1.0)

        scale_wh = lmb_coord_wh / 2 / eps_count
        loss_wh = scale_wh * F.sum(F.broadcast_mul(each_loss_wh, kr_obj_ij_plus1))

        # Numerically stable sigmoid cross entropy
        pos = raw_conf >= 0
        each_loss_conf = (F.log(1 + F.exp(raw_conf - 2 * pos * raw_conf)) +
                          raw_conf * (pos - obj_gt_conf))

        loss_conf = scale_conf * F.sum(obj_w * each_loss_conf)

        class_scores_flat = F.reshape(class_scores, [-1, num_classes])
        gt_class_index_flat = F.reshape(gt_class_index, [-1])
        kr_obj_ij_flat = F.reshape(kr_obj_ij, [-1, 1])

        x_loss = _mx.gluon.loss.SoftmaxCrossEntropyLoss(axis=-1,
                                                        sparse_label=True,
                                                        from_logits=False)

        weights = lmb_class * F.broadcast_div(kr_obj_ij_flat, eps_count)
        loss_cls = F.sum(x_loss(class_scores_flat, gt_class_index_flat, weights))

        loss = loss_xy + loss_wh + loss_conf + loss_cls
        return loss
