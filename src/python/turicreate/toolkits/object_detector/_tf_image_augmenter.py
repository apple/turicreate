# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import turicreate.toolkits._tf_utils as _utils
import numpy as _np

class DataAugmenter(object):
	def __init__(self, images, annotations, predictions):
		for image in images:
			image = _utils.convert_shared_float_array_to_numpy(image)
			print(image)
		print("image done")
		for ann in annotations:
			ann = _utils.convert_shared_float_array_to_numpy(ann)
			print(ann)
		print("annotation done")
		for pred in predictions:
			pred = _utils.convert_shared_float_array_to_numpy(pred)
			print(pred)
		print("predictions done")


	def get_augmented_images(self):
		image = _np.reshape(_np.arange(32*416*416*3),(32,416,416,3))
		return image

	def get_augmented_annotations(self):
		L = _np.reshape(_np.arange(32* 1* 6), (32, 1, 6))
		return L


