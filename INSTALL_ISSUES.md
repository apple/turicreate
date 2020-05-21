# Common Installation Errors and How to Fix

`ImportError /lib64/libstdc++.so.6: version GLIBCXX_3.4.21 not found`

You need an updated libstdc++.so. See [LINUX\_INSTALL.md](LINUX_INSTALL.md) for details.

___

`ImportError ..  version GLIBCXX\_3.4.21 not defined in libstdc++.so.6 with link time reference`

You need an updated libstdc++.so. See [LINUX\_INSTALL.md](LINUX_INSTALL.md) for details.
(Strictly, speaking you have an updated libstdc++.so, but it was not compiled with the cxx11 abi)

___

`ImportError: libblas.so.3: Cannot open shared object file: No such file or directory`

You need to install Blas and Lapack. See [LINUX\_INSTALL.md](LINUX_INSTALL.md) for details.

___

`ImportError: liblapack.so.3: Cannot open shared object file: No such file or directory`

You need to install Blas and Lapack. See [LINUX\_INSTALL.md](LINUX_INSTALL.md) for details.

___

`ModuleNotFoundError: No module named 'turicreate'` with Jupyter Notebook
This is for Ubuntu>= 17.10. If you're using another distro, your command will be slightly different according to your package manager.

For python2:\
sudo apt install python-pip python-setuptools\
sudo pip install virtualenv notebook jupyter\
cd $HOME\
virtualenv venv\
cd venv/\
source bin/activate\
pip install --upgrade pip\
pip install jupyter notebook\
pip install -U turicreate\
jupyter notebook\

For python3:

sudo apt install python3-pip python-setuptools\
sudo pip3 install virtualenv notebook jupyter\
cd $HOME\
virtualenv venv\
cd venv/\
source bin/activate\
pip3 install jupyter notebook\
pip3 install -U turicreate\
jupyter notebook
___
