import turicreate as tc
import unittest
import tempfile
import shutil


class SparseNNTest(unittest.TestCase):
    def test_sparse_nn(self):
        X = tc.util.generate_random_sframe(100, "ssszzz")
        Y = X.copy()
        X = X.add_row_number()

        m = tc.extensions._sparse_nn()

        m.train(X, "id")

        for i, row in enumerate(Y):
            res = m.query(Y[i], 1)
            self.assertEqual(res, {i: 1.0})

        # Save and load
        model_file = tempfile.gettempdir() + "/sparse_nn.model"
        m.save(model_file)
        m2 = tc.load_model(model_file)

        for i, row in enumerate(Y):
            res = m2.query(Y[i], 1)
            self.assertEqual(res, {i: 1.0})

        shutil.rmtree(model_file)
