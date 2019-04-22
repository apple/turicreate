import random as _random
import turicreate as _tc
from turicreate import extensions as _extensions
from turicreate.toolkits._model import CustomModel as _CustomModel

def create(dataset, target, feature=None, batch_size=0, max_iterations=0,
           seed=None, verbose=True):
    model = _extensions.one_shot_object_detector()
    if seed is None: seed = _random.randint(0, (1<<31)-1)
    model.train(dataset, target, _tc.SFrame(), seed, {'mlmodel_path':'/Users/schhabra/Desktop/apple/turicreate/darknet.mlmodel', 'max_iterations' : 25})
    return OneShotObjectDetector(model)

class OneShotObjectDetector(_CustomModel):
    _PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION = 1

    def __init__(self, model):
        self.__proxy__ = model

    @classmethod
    def _native_name(cls):
        return "one_shot_object_detector"

    def _get_native_state(self):
        pass
        # state = self.__proxy__.get_state()
        # mxnet_params = state['_model'].collect_params()
        # state['_model'] = _mxnet_utils.get_gluon_net_params_state(mxnet_params)
        # return state

    def _get_version(self):
        return self._PYTHON_ONE_SHOT_OBJECT_DETECTOR_VERSION

    @classmethod
    def _load_version(cls, state, version):
        pass
        # _tkutl._model_version_check(version, 
        #     cls._PYTHON_DRAWING_CLASSIFIER_VERSION)
        # from ._model_architecture import Model as _Model
        # net = _Model(num_classes = len(state['classes']), prefix = 'drawing_')
        # ctx = _mxnet_utils.get_mxnet_context(max_devices=state['batch_size'])
        # net_params = net.collect_params()
        # _mxnet_utils.load_net_params_from_state(
        #     net_params, state['_model'], ctx=ctx 
        #     )
        # state['_model'] = net
        # # For a model trained on integer classes, when saved and loaded back,
        # # the classes are loaded as floats. The following if statement casts
        # # the loaded "float" classes back to int.
        # if len(state['classes']) > 0 and isinstance(state['classes'][0], float):
        #     state['classes'] = list(map(int, state['classes']))
        # return DrawingClassifier(state)

    def predict(self, dataset):
        return self.__proxy__.predict(dataset)

    def evaluate(self, dataset, metric="auto"):
        return self.__proxy__.evaluate(dataset, metric)

    def export_coreml(self, filename, verbose=False):
        self.__proxy__.export_to_coreml(filename)