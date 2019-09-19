import os

# general data folder
if not os.path.exists("./data"):
	os.makedirs("data")

# create block 1 folder
if not os.path.exists("./data/block1"):
	os.makedirs("./data/block1")

# create block 1 test folder
if not os.path.exists("./data/block1/test_1"):
	os.makedirs("./data/block1/test_1")

# create block 2 folder
if not os.path.exists("./data/block2"):
	os.makedirs("./data/block2")

# create block 2 test folder
if not os.path.exists("./data/block2/test_1"):
	os.makedirs("./data/block2/test_1")

# create vgg 16 folder
if not os.path.exists("./data/vgg16"):
	os.makedirs("./data/vgg16")

# create vgg 16 test folder
if not os.path.exists("./data/vgg16/test_1"):
	os.makedirs("./data/vgg16/test_1")

# create encode folder
if not os.path.exists("./data/encode"):
	os.makedirs("./data/encode")

# create encode test folder
if not os.path.exists("./data/encode/test_1"):
	os.makedirs("./data/encode/test_1")

# create residual folder
if not os.path.exists("./data/residual"):
	os.makedirs("./data/residual")

# create residual test folder
if not os.path.exists("./data/residual/test_1"):
	os.makedirs("./data/residual/test_1")

# create decode folder
if not os.path.exists("./data/decode"):
	os.makedirs("./data/decode")

# create decode test folder
if not os.path.exists("./data/decode/test_1"):
	os.makedirs("./data/decode/test_1")

# create resnet folder
if not os.path.exists("./data/resnet"):
	os.makedirs("./data/resnet")

# create resnet test folder
if not os.path.exists("./data/resnet/test_1"):
	os.makedirs("./data/resnet/test_1")