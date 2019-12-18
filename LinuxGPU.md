Turi Create **does not require a GPU**, but certain models can be accelerated by the use of a GPU. 
To enable GPU support in Linux after installation of the `turicreate` package, first make sure both
[CUDA](http://docs.nvidia.com/cuda/cuda-installation-guide-linux/) and [cuDNN](https://developer.nvidia.com/cudnn) are installed.

Make sure to add the CUDA library path to your `LD_LIBRARY_PATH` environment
variable.  In the typical case, this means adding the following line to your
`~/.bashrc` file:

```shell
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
```
If you installed the cuDNN files into a separate directory, make sure to
separately add those as well.

The next step is to uninstall `tensorflow` and install the
CUDA-enabled `tensorflow-gpu` package:

```
(venv) pip uninstall -y tensorflow
(venv) pip install tensorflow-gpu
```

If you have trouble setting up the GPU, the [TensorFlow
installation instructions](https://www.tensorflow.org/install/gpu) may
offer additional help.

