/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/style_transfer/mps_style_transfer.h>
#import <ml/neural_net/mps_layer_helper.h>

@implementation TCMPSStyleTransfer
- (instancetype) initWithParameters:(NSDictionary<NSString *, NSData *> *)weights {
  self = [super init];
  if (self) {
    _batch_size = 6;
    _content_loss_multiplier = [NSNumber numberWithFloat:1.0];
    _style_loss_multiplier = [NSNumber numberWithFloat:1e-4];
    _finetune_all_params = YES;
    _pretrained_weights = NO;
  }
  return self;
}

- (NSDictionary<NSString *, NSData *> *)exportWeights {
  // TODO: export weights
  NSDictionary<NSString *, NSData *> *weights;
  return weights;
}

- (NSDictionary<NSString *, NSData *> *)predict:(NSDictionary<NSString *, NSData *> *)inputs {
  // TODO: export weights
  NSDictionary<NSString *, NSData *> *weights;
  return weights;
}

- (void) setLearningRate:(NSNumber *)lr {

}

- (NSDictionary<NSString *, NSData *> *) train:(NSDictionary<NSString *, NSData *> *)inputs {
  // TODO: export weights
  NSDictionary<NSString *, NSData *> *weights;
  return weights;
}

@end