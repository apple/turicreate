Turi Create **does not require a GPU**, but certain models can be accelerated by the use of a GPU. 
To enable GPU support in linux after installation of the `turicreate` package, please perform the following steps:

 * Install CUDA 8.0 ([instructions](http://docs.nvidia.com/cuda/cuda-installation-guide-linux/))
 * Install cuDNN 5 for CUDA 8.0 ([instructions](https://developer.nvidia.com/cudnn))

Make sure to add the CUDA library path to your `LD_LIBRARY_PATH` environment
variable.  In the typical case, this means adding the following line to your
`~/.bashrc` file:

```shell
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
```

If you installed the cuDNN files into a separate directory, make sure to
separately add it as well. Next step is to uninstall `mxnet` and install the
CUDA-enabled `mxnet-cu80` package:

```
(venv) pip uninstall -y mxnet
(venv) pip install mxnet-cu80==1.1.0
```

Make sure you install the same version of MXNet as the one `turicreate` recommends
(currently `1.1.0`). If you have trouble setting up the GPU, the [MXNet
installation instructions](https://mxnet.incubator.apache.org/get_started/install.html) may
offer additional help.

