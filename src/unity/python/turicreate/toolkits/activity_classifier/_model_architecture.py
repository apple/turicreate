# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import mxnet as _mx

_net_params = {
    'conv_h': 64,
    'lstm_h': 200,
    'dense_h': 128
}


def _define_model(features, target_map, pred_win, seq_len, context):
    n_classes = len(target_map)

    # Vars
    data = _mx.sym.Variable('data')  # NTC
    weights = _mx.sym.Variable('weights')  # NT1
    target = _mx.sym.Variable('target')  # NT1

    # Conv
    conv = _mx.sym.swapaxes(data, 1, 2)  # NTC to NCT
    conv = _conv1d_layer(conv, kernel=pred_win, stride=pred_win,
                         num_filters=_net_params['conv_h'])

    # RNN
    lstm = _mx.sym.swapaxes(conv, 1, 2)  # NCT to NTC
    lstm = _lstm_layer(lstm, num_hidden=_net_params['lstm_h'],
                       seq_len=seq_len, dropout=0.2)

    # Dense
    dense = _mx.sym.Reshape(data=lstm, shape=(-1, _net_params['lstm_h']))  # NTC to NxTC
    dense = _mx.sym.FullyConnected(data=dense, name='dense0',
                                   num_hidden=_net_params['dense_h'])

    dense = _mx.sym.BatchNorm(data=dense, name='bn')
    dense = _mx.sym.Activation(dense, 'relu')
    dense = _mx.sym.Dropout(data=dense, p=0.5)

    # Output
    out = _mx.sym.FullyConnected(data=dense, num_hidden=n_classes, name='dense1')
    out = _mx.sym.Reshape(data=out,
                          shape=(-1, seq_len, n_classes))  # NxTC to NTC

    probs = _mx.sym.softmax(data=out, axis=-1, name='softmax')

    # Weights
    seq_sum_weights = _mx.sym.sum(weights, axis=1)
    binary_seq_sum_weights = _mx.sym.sum(seq_sum_weights > 0)

    # Loss
    softmax_ce = _mx.gluon.loss.SoftmaxCrossEntropyLoss()
    loss_per_seq = softmax_ce(out, target, weights)
    loss = _mx.sym.sum(loss_per_seq) / (binary_seq_sum_weights + 1e-5)
    loss = _mx.sym.make_loss(loss)

    # Accuracy
    pred_class = _mx.sym.argmax(data=probs, axis=-1, keepdims=True)
    weighted_correct = (target == pred_class) * weights
    acc_per_seq = _mx.sym.sum(weighted_correct, axis=1) / (seq_sum_weights + 1e-5)
    acc = _mx.sym.sum(acc_per_seq) / (binary_seq_sum_weights + 1e-5)

    metrics = _mx.sym.Group([loss,
                             _mx.sym.BlockGrad(acc),
                             _mx.sym.BlockGrad(loss_per_seq),
                             _mx.sym.BlockGrad(acc_per_seq)])

    # Models
    loss_model = _mx.mod.Module(symbol=metrics,
                                data_names=['data'],
                                label_names=['target', 'weights'],
                                fixed_param_names=['lstm_i2h_bias'],
                                context=context)

    pred_model = _mx.mod.Module(symbol=probs,
                                data_names=['data'],
                                label_names=None,
                                context=context)
    return loss_model, pred_model


def _conv1d_layer(data, kernel, stride, num_filters):
    conv = _mx.sym.Convolution(data=data, name='conv',
                               kernel=(kernel,), stride=(stride,),
                               num_filter=num_filters, layout='NCW')
    conv = _mx.sym.Activation(conv, 'relu')
    return conv


def _lstm_layer(data, num_hidden, seq_len, dropout):
    lstm = _mx.rnn.SequentialRNNCell()
    lstm.add(_mx.rnn.DropoutCell(dropout))
    lstm.add(_mx.rnn.LSTMCell(num_hidden=num_hidden, prefix='lstm_', forget_bias=0.0))
    output, state = lstm.unroll(length=seq_len,
                                inputs=data,
                                layout='NTC',
                                merge_outputs=True)
    return output


def _fit_model(model, data_iter, valid_iter, max_iterations, num_gpus, verbose):
    from time import time as _time

    model.bind(data_shapes=data_iter.provide_data,
               label_shapes=data_iter.provide_label)
    model.init_params(initializer=_mx.init.Xavier())
    model.init_optimizer(optimizer='adam',
                         optimizer_params={'learning_rate': 1e-3,
                                           'rescale_grad': 1.0})

    begin = _time()
    for iteration in range(max_iterations):
        log = {
            'train_loss': 0.,
            'train_acc': 0.
        }

        # Training iteration
        data_iter.reset()
        train_batches = float(data_iter.num_batches) * max(num_gpus, 1)
        for batch in data_iter:
            model.forward_backward(batch)
            loss, acc, loss_per_seq, acc_per_seq = model.get_outputs()
            log['train_loss'] += _mx.nd.sum(loss).asscalar() / train_batches
            log['train_acc'] += _mx.nd.sum(acc).asscalar() / train_batches
            model.update()

        if verbose:
            print('Iteration: %04d' % (iteration + 1))
            print('\tTrain loss    : {:.9f}'.format(log['train_loss']), end=' ')
            print('\tTrain accuracy: {:.9f}'.format(log['train_acc']))

        # Validation iteration
        if valid_iter is not None:
            valid_num_seq = valid_iter.num_rows
            valid_metrics = model.iter_predict(valid_iter)
            valid_metrics = [(_mx.nd.sum(m[0][2]).asscalar(),
                              _mx.nd.sum(m[0][3]).asscalar())
                             for m in valid_metrics]
            valid_loss, valid_acc = zip(*valid_metrics)
            log['valid_loss'] = sum(valid_loss) / valid_num_seq
            log['valid_acc'] = sum(valid_acc) / valid_num_seq

            if verbose:
                print('\tValid loss    : {:.9f}'.format(log['valid_loss']), end=' ')
                print('\tValid accuracy: {:.9f}'.format(log['valid_acc']))

    if verbose:
        print('Training complete')
        end = _time()
        print('Total Time Spent: %gs' % (end - begin))

    return log
