from __future__ import absolute_import
import numpy as np

class NumpyObject:
	def __init__(self):
		self.arr1 = np.arange(5.0).astype(np.float32)

	def add(self):
		return self.arr1 