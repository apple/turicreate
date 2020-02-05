# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import numpy as _np

from .._mps_utils import (
    MpsLowLevelAPI as _MpsLowLevelAPI,
    MpsLowLevelNetworkType as _MpsLowLevelNetworkType,
    MpsLowLevelMode as _MpsLowLevelMode,
)


def _define_model_mps(
    batch_size, num_features, num_classes, pred_win, seq_len, is_prediction_model
):

    config = {
        "mode": _MpsLowLevelMode.kLowLevelModeTrain,
        "ac_pred_window": pred_win,
        "ac_seq_len": seq_len,
    }

    input_width = pred_win * seq_len

    if is_prediction_model:
        config["mode"] = _MpsLowLevelMode.kLowLevelModeInference

    model = _MpsLowLevelAPI(network_id=_MpsLowLevelNetworkType.kActivityClassifierNet)
    model.init(
        batch_size,
        num_features,
        1,
        input_width,
        num_classes,
        1,
        seq_len,
        updater=2,
        config=config,
    )

    return model


def _calc_batch_metrics(
    output, labels, weights, actual_seq_len, actual_batch_len, loss_per_sec
):
    prediction = _np.argmax(output, axis=-1)
    prediction = _np.expand_dims(prediction, 2)
    accuracy = (prediction == labels) * (weights > 0)
    acc_per_seq = _np.sum(accuracy, axis=1).astype(float) / (actual_seq_len + 1e-5)
    batch_accuracy = _np.sum(acc_per_seq).astype(float) / (actual_batch_len + 1e-5)
    batch_loss = _np.sum(loss_per_sec)

    return batch_loss, batch_accuracy, acc_per_seq


def _fit_model_mps(model, data_iter, valid_iter, max_iterations, verbose):
    from time import time as _time

    model.initalize_weights()

    if verbose:
        # Print progress table header
        column_names = ["Iteration", "Train Accuracy", "Train Loss"]
        if valid_iter:
            column_names += ["Validation Accuracy", "Validation Loss"]
        column_names.append("Elapsed Time")
        num_columns = len(column_names)
        column_width = max(map(lambda x: len(x), column_names)) + 2
        hr = "+" + "+".join(["-" * column_width] * num_columns) + "+"
        print(hr)
        print(
            ("| {:<{width}}" * num_columns + "|").format(
                *column_names, width=column_width - 1
            )
        )
        print(hr)

    begin = _time()
    for iteration in range(max_iterations):
        log = {"train_loss": 0.0, "train_acc": 0.0, "valid_loss": 0.0, "valid_acc": 0.0}

        # Training iteration
        data_iter.reset()
        train_batches = float(data_iter.num_batches)

        # Encapsulates the work for feeding a batch into the model.
        def start_batch(batch, batch_idx, is_train):
            input_data = batch.data
            labels = batch.labels
            weights = batch.weights
            actual_seq_len = _np.sum(weights, axis=1)
            actual_batch_len = _np.sum((actual_seq_len > 0))
            if is_train and actual_batch_len > 0:
                weights /= actual_batch_len

            # MPS model requires 4-dimensional NHWC input
            model_fn = model.train if is_train else model.predict_with_loss
            (fwd_out, loss_out) = model_fn(
                _np.expand_dims(input_data, 1),
                _np.expand_dims(labels, 1),
                _np.expand_dims(weights, 1),
            )

            return {
                "labels": labels,
                "weights": weights,
                "actual_seq_len": actual_seq_len,
                "actual_batch_len": actual_batch_len,
                "fwd_out": fwd_out,
                "loss_out": loss_out,
            }

        # Encapsulates the work for processing a response from the model.
        def finish_batch(
            batch_idx,
            is_train,
            labels,
            weights,
            actual_seq_len,
            actual_batch_len,
            fwd_out,
            loss_out,
        ):
            # MPS yields 4-dimensional NHWC output. Collapse the H dimension,
            # which should have size 1.
            forward_output = _np.squeeze(fwd_out.asnumpy(), axis=1)
            loss_per_sequence = _np.squeeze(loss_out.asnumpy(), axis=1)

            batch_loss, batch_accuracy, acc_per_sequence = _calc_batch_metrics(
                forward_output,
                labels,
                weights,
                actual_seq_len,
                actual_batch_len,
                loss_per_sequence,
            )
            if is_train:
                log["train_loss"] += batch_loss / train_batches
                log["train_acc"] += batch_accuracy / train_batches
            else:
                log["valid_loss"] += _np.sum(loss_per_sequence) / valid_num_seq_in_epoch
                log["valid_acc"] += _np.sum(acc_per_sequence) / valid_num_seq_in_epoch

        # Perform the following sequence of calls, effectively double buffering:
        # start_batch(1)
        # start_batch(2)    # Two outstanding batches
        # finish_batch(1)
        # start_batch(3)
        # finish_batch(2)
        # ...
        # start_batch(n)
        # finish_batch(n-1)
        # finish_batch(n)
        def perform_batches(data_iter, is_train=True):
            batch_count = 0
            prev_batch_info = None
            last_batch_info = None
            for batch in data_iter:
                (prev_batch_info, last_batch_info) = (
                    last_batch_info,
                    start_batch(batch, batch_count, is_train),
                )
                if batch_count > 0:
                    finish_batch(batch_count - 1, is_train, **prev_batch_info)
                batch_count += 1
            if batch_count > 0:
                finish_batch(batch_count - 1, is_train, **last_batch_info)

        perform_batches(data_iter, is_train=True)

        # Validation iteration
        if valid_iter is not None:
            valid_iter.reset()
            valid_num_seq_in_epoch = valid_iter.num_rows
            perform_batches(valid_iter, is_train=False)

        if verbose:
            elapsed_time = _time() - begin
            if valid_iter is None:
                # print progress row without validation info
                print(
                    "| {cur_iter:<{width}}| {train_acc:<{width}.3f}| {train_loss:<{width}.3f}| {time:<{width}.1f}|".format(
                        cur_iter=iteration + 1,
                        train_acc=log["train_acc"],
                        train_loss=log["train_loss"],
                        time=elapsed_time,
                        width=column_width - 1,
                    )
                )
            else:
                # print progress row with validation info
                print(
                    "| {cur_iter:<{width}}| {train_acc:<{width}.3f}| {train_loss:<{width}.3f}"
                    "| {valid_acc:<{width}.3f}| {valid_loss:<{width}.3f}| {time:<{width}.1f}| ".format(
                        cur_iter=iteration + 1,
                        train_acc=log["train_acc"],
                        train_loss=log["train_loss"],
                        valid_acc=log["valid_acc"],
                        valid_loss=log["valid_loss"],
                        time=elapsed_time,
                        width=column_width - 1,
                    )
                )

    if verbose:
        print(hr)
        print("Training complete")
        end = _time()
        print("Total Time Spent: %gs" % (end - begin))

    return log


def _predict_mps(pred_model, data_iter):
    data_iter.reset()

    output_list = []

    for batch in data_iter:
        input_data = batch.data
        pad = batch.pad

        # Feed the input into the model, converting to and from the
        # 4-dimensional NHWC shape that MPS expects.
        raw_output = pred_model.predict(_np.expand_dims(input_data, 1))
        output = _np.squeeze(raw_output.asnumpy(), axis=1)

        trimmed_output = output[0 : output.shape[0] - pad].copy()
        output_list.append(trimmed_output)

    prediction_output = _np.concatenate([output for output in output_list])

    return prediction_output
