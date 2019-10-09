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
			image = _np.array(image, dtype=uint8, copy=False)
			print(image)
		print("image done")
		for ann in annotations:
			if len(ann) > 1:
				print(ann)


	def get_augmented_data(self):
		image = _np.reshape(_np.arange(32*416*416*3),(32,416,416,3))
		L = _np.reshape(_np.arange(32* 1* 6), (32, 1, 6))
		result = {"images" : image, "annotations" : L}
		return result



