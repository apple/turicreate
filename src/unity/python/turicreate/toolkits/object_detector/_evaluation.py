# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import numpy as _np


def average_precision(predictions,
                      groundtruth,
                      class_to_index,
                      iou_thresholds):
    aps = _np.zeros((len(iou_thresholds), len(class_to_index)))
    for classname, c in class_to_index.items():
        c_predictions = predictions[predictions['label'] == classname]
        c_groundtruth = groundtruth[groundtruth['label'] == classname]

        pred_sorted = c_predictions.sort_values('confidence', ascending=False)
        num_pred = len(pred_sorted)
        tp = _np.zeros((len(iou_thresholds), num_pred))
        fp = _np.zeros((len(iou_thresholds), num_pred))

        gts_dict = {}
        for index, (_, row) in enumerate(pred_sorted.iterrows()):
            if row['row_id'] in gts_dict:
                gts = gts_dict[row['row_id']]
            else:
                gts = c_groundtruth[c_groundtruth['row_id'] == row['row_id']].reset_index(drop=True)
                for i in range(len(iou_thresholds)):
                    gts['correct_%d' % i] = False
                gts_dict[row['row_id']] = gts

            if gts.size > 0:
                x_lo = _np.maximum(gts['x'] - gts['width'] / 2, row['x'] - row['width'] / 2)
                x_hi = _np.minimum(gts['x'] + gts['width'] / 2, row['x'] + row['width'] / 2)
                y_lo = _np.maximum(gts['y'] - gts['height'] / 2, row['y'] - row['height'] / 2)
                y_hi = _np.minimum(gts['y'] + gts['height'] / 2, row['y'] + row['height'] / 2)

                width = _np.maximum(x_hi - x_lo, 0)
                height = _np.maximum(y_hi - y_lo, 0)

                inter_area = width * height
                pred_area = row['width'] * row['height']
                gt_area = gts['width'] * gts['height']

                iou = inter_area / (pred_area + gt_area - inter_area)
                best_gt_index = _np.argmax(iou)
                best_iou = iou[best_gt_index]
            else:
                best_iou = 0.0

            for th_index, iou_threshold in enumerate(iou_thresholds):
                if best_iou > iou_threshold and not gts.ix[best_gt_index, 'correct_%d' % th_index]:
                    gts.ix[best_gt_index, 'correct_%d' % th_index] = True
                    tp[th_index, index] = 1
                else:
                    fp[th_index, index] = 1

        cum_fp = _np.cumsum(fp, axis=1)
        cum_tp = _np.cumsum(tp, axis=1)

        def pad1(x, v0, v1):
            return _np.concatenate([_np.full((x.shape[0], 1), v0, dtype=_np.float64),
                                    x,
                                    _np.full((x.shape[0], 1), v1, dtype=_np.float64)], axis=1)

        recall = pad1(cum_tp / len(c_groundtruth), 0, 1)
        precision_non_monotonic = pad1(cum_tp / _np.maximum(cum_tp + cum_fp, 1), 0, 0)

        precision = _np.maximum.accumulate(precision_non_monotonic[:, ::-1], axis=1)[:, ::-1]

        rec_diff = _np.diff(recall, axis=1)

        for th_index, (rec_diff_th, precision_th) in enumerate(zip(rec_diff, precision)):
            ii = _np.where(rec_diff_th > 0)[0]
            ap = (rec_diff_th[ii] * precision_th[ii + 1]).sum()
            aps[th_index, c] = ap

    return aps
