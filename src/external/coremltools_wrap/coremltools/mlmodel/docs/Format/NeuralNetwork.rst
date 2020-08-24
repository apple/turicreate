A neural network is defined through a collection of layers
and represents a directed acyclic graph (DAG).
Each layer has a name, a layer type,
a list of input names, a list of output names,
and a collection of parameters specific to the layer type.

The graph structure and connectivity of the neural network
is inferred from the input and output names.
A neural network starts with the layer
whose input name is equal to the value specified in
``Model.description.input.name``,
and ends with the layer
whose output name is equal to the value specified in
``Model.description.output.name``.
Layers must have unique input and output names,
and a layer may not have input or output names that
refer to layers that are not yet defined.

For Core ML specification version <=3,
all inputs are mapped to static rank 5 tensors, with axis notations
[Sequence, Batch, Channel, Height, Width].

From specification version 4 onwards (iOS >= 13, macOS >= 10.15), more options are available
(see enums ``NeuralNetworkMultiArrayShapeMapping``, ``NeuralNetworkImageShapeMapping``)
to map inputs to generic N-Dimensional (or N rank) tensors, where N >= 1.

Each layer type may have specific constraints on the ranks of its inputs and outputs.

Some of the layers (such as softmax, reduce, etc) have parameters that have been described in
terms of notational axis "Channel", "Height", "Width" or "Sequence". They can be re-interpreted easily in
the general ND setting by using the following rule:
"width" is same as axis = -1 (i.e. the last axis from the end)
"height" is same as axis = -2 (i.e. the second last axis from the end)
"channel" is same as axis = -3 (i.e. the third last axis from the end)
"sequence" is same as axis = -5 (i.e. the fifth last axis from the end)

Several layers are available in 3 different variations, with the names ending
in identifiers: ``like``, ``static`` and ``dynamic``. For instance, ``FillLike``,
``FillStatic`` and ``FillDynamic``. The ``static`` variation generally will have
a property corresponding to the shape of the output. For instance, if the
output of the ``FillStatic`` layer is desired to be of shape (10, 4), the
property ``targetShape`` will have to be set to [10, 4]. In the ``dynamic`` case,
the shape is an input, hence it can be changed at runtime. For instance, for
a ``FillDynamic`` layer, the input would have to be an array containing the
values 10 and 4, if the desired output is of shape (10, 4). Whereas in the
``like`` case, the additional input's shape is used as the output shape, ignoring
its values. For instance, for a ``FillLike`` layer, for an input with shape
(10, 4), the output generated will also be of shape (10, 4), values of the
input will be ignored.



NeuralNetwork
________________________________________________________________________________

A neural network.


.. code-block:: proto

	message NeuralNetwork {

	    repeated NeuralNetworkLayer layers = 1;
	    repeated NeuralNetworkPreprocessing preprocessing = 2;

	    // use this enum value to determine the input tensor shapes to the neural network, for multiarray inputs
	    NeuralNetworkMultiArrayShapeMapping arrayInputShapeMapping = 5;

	    // use this enum value to determine the input tensor shapes to the neural network, for image inputs
	    NeuralNetworkImageShapeMapping imageInputShapeMapping = 6;


	    NetworkUpdateParameters updateParams = 10;

	}






NeuralNetworkImageScaler
________________________________________________________________________________

A neural network preprocessor that
performs a scalar multiplication of an image
followed by addition of scalar biases to the channels.

Input: X
   An image in BGR or RGB format with shape ``[3, H, W]``
   or in grayscale format with shape ``[1, H, W]``.
Output: Y
   An image with format and shape corresponding to the input.

If the input image is in BGR format:

.. code::

    Y[0, :, :] = channelScale * X[0, :, :] + blueBias
    Y[1, :, :] = channelScale * X[1, :, :] + greenBias
    Y[2, :, :] = channelScale * X[2, :, :] + redBias

If the input image is in RGB format:

.. code::

    Y[0, :, :] = channelScale * X[0, :, :] + redBias
    Y[1, :, :] = channelScale * X[1, :, :] + greenBias
    Y[2, :, :] = channelScale * X[2, :, :] + blueBias

If the input image is in grayscale format:

.. code::

    Y[0, :, :] = channelScale * X[0, :, :] + grayBias


.. code-block:: proto

	message NeuralNetworkImageScaler {

	    float channelScale = 10;
	    float blueBias = 20;
	    float greenBias = 21;
	    float redBias = 22;
	    float grayBias = 30;

	}






NeuralNetworkMeanImage
________________________________________________________________________________

A neural network preprocessor that
subtracts the provided mean image from the input image.
The mean image is subtracted from the input named
``NeuralNetworkPreprocessing.featureName``.


.. code-block:: proto

	message NeuralNetworkMeanImage {

	    repeated float meanImage = 1;

	}






NeuralNetworkPreprocessing
________________________________________________________________________________

Preprocessing parameters for image inputs.


.. code-block:: proto

	message NeuralNetworkPreprocessing {

	    string featureName = 1;
	    oneof preprocessor {
	        NeuralNetworkImageScaler scaler = 10;
	        NeuralNetworkMeanImage meanImage = 11;
	    }

	}






ActivationReLU
________________________________________________________________________________

A rectified linear unit (ReLU) activation function.

This function has the following formula:

.. math::
    f(x) = \text{max}(0, x)


.. code-block:: proto

	message ActivationReLU {

	}






ActivationLeakyReLU
________________________________________________________________________________

A leaky rectified linear unit (ReLU) activation function.

This function has the following formula:

.. math::
    f(x) = \begin{cases}
            x      & \text{if } x \geq 0 \\
            \alpha x & \text{if } x < 0
           \end{cases}


.. code-block:: proto

	message ActivationLeakyReLU {

	    float alpha = 1; //negative slope value for leakyReLU

	}






ActivationTanh
________________________________________________________________________________

A hyperbolic tangent activation function.

This function has the following formula:

.. math::
    f(x) = \dfrac{1 - e^{-2x}}{1 + e^{-2x}}


.. code-block:: proto

	message ActivationTanh {

	}






ActivationScaledTanh
________________________________________________________________________________

A scaled hyperbolic tangent activation function.

This function has the following formula:

.. math::
    f(x) = \alpha \tanh(\beta x)


.. code-block:: proto

	message ActivationScaledTanh {

	    float alpha = 1;
	    float beta = 2;

	}






ActivationSigmoid
________________________________________________________________________________

A sigmoid activation function.

This function has the following formula:

.. math::
    f(x) = \dfrac{1}{1 + e^{-x}}


.. code-block:: proto

	message ActivationSigmoid {

	}






ActivationLinear
________________________________________________________________________________

A linear activation function.

This function has the following formula:

.. math::
    f(x) = \alpha x + \beta


.. code-block:: proto

	message ActivationLinear {

	    float alpha = 1;
	    float beta = 2;

	}






ActivationSigmoidHard
________________________________________________________________________________

A hard sigmoid activation function.

This function has the following formula:

.. math::
    f(x) = \text{min}(\text{max}(\alpha x + \beta, 0), 1)


.. code-block:: proto

	message ActivationSigmoidHard {

	    float alpha = 1;
	    float beta = 2;

	}






ActivationPReLU
________________________________________________________________________________

A parameterized rectified linear unit (PReLU) activation function.
Input must be at least rank 3. Axis = -3 is denoted by "C", or channels.
"alpha" parameter can be a vector of length C.

This function has the following formula:

.. math::
   f(x_i) = \begin{cases}
                x_i          & \text{if } x_i \geq 0 \\
                \alpha_i x_i & \text{if } x_i < 0
            \end{cases} \;,\;i=1,...,C


.. code-block:: proto

	message ActivationPReLU {

	    // parameter of length C or 1.
	    // If length is 1, same value is used for all channels
	    WeightParams alpha = 1;

	}






ActivationELU
________________________________________________________________________________

An exponential linear unit (ELU) activation function.

This function has the following formula:

.. math::
    f(x) = \begin{cases}
            x              & \text{if } x \geq 0 \\
            \alpha (e^x - 1) & \text{if } x < 0
           \end{cases}


.. code-block:: proto

	message ActivationELU {

	    float alpha = 1;

	}






ActivationThresholdedReLU
________________________________________________________________________________

A thresholded rectified linear unit (ReLU) activation function.

This function has the following formula:

.. math::
    f(x) = \begin{cases}
            x & \text{if } x \geq \alpha \\
            0 & \text{if } x < \alpha
           \end{cases}


.. code-block:: proto

	message ActivationThresholdedReLU {

	    float alpha = 1;

	}






ActivationSoftsign
________________________________________________________________________________

A softsign activation function.

This function has the following formula:

.. math::
    f(x) = \dfrac{x}{1 + |x|}


.. code-block:: proto

	message ActivationSoftsign {

	}






ActivationSoftplus
________________________________________________________________________________

A softplus activation function.

This function has the following formula:

.. math::
    f(x) = \text{log}(1 + e^x)


.. code-block:: proto

	message ActivationSoftplus {

	}






ActivationParametricSoftplus
________________________________________________________________________________

A parametric softplus activation function.
Input must be at least rank 3. axis = -3 is denoted by "C", or channels.
"alpha"/"beta" parameter can be a vector of length C.

This function has the following formula:

.. math::
    f(x_i) = \alpha_i \text{log}(1 + e^{\beta_i x_i}) \;,\;i=1,...,C


.. code-block:: proto

	message ActivationParametricSoftplus {

	    // If length is 1, same value is used for all channels
	    WeightParams alpha = 1; //parameter of length C or 1
	    WeightParams beta = 2; //parameter of length C or 1

	}






ActivationParams
________________________________________________________________________________




.. code-block:: proto

	message ActivationParams {

	    oneof NonlinearityType {
	        ActivationLinear linear = 5;

	        ActivationReLU ReLU = 10;
	        ActivationLeakyReLU leakyReLU = 15;
	        ActivationThresholdedReLU thresholdedReLU = 20;
	        ActivationPReLU PReLU = 25;

	        ActivationTanh tanh = 30;
	        ActivationScaledTanh scaledTanh = 31;

	        ActivationSigmoid sigmoid = 40;
	        ActivationSigmoidHard sigmoidHard = 41;

	        ActivationELU ELU = 50;

	        ActivationSoftsign softsign = 60;
	        ActivationSoftplus softplus = 70;
	        ActivationParametricSoftplus parametricSoftplus = 71;
	    }

	}






Tensor
________________________________________________________________________________

Representation of the intermediate tensors


.. code-block:: proto

	message Tensor {

	    // Number of dimensions in the tensor shape
	    uint32 rank = 1;
	    // actual value of the tensor shape.
	    // must be of length "rank". Can contain -1s for unknown dimensions.
	    repeated int64 dimValue = 2;

	}






NeuralNetworkLayer
________________________________________________________________________________

A single neural network layer.


.. code-block:: proto

	message NeuralNetworkLayer {

	    string name = 1; //descriptive name of the layer
	    repeated string input = 2;
	    repeated string output = 3;

	    repeated Tensor inputTensor = 4; // must be the same length as the "input" field
	    repeated Tensor outputTensor = 5; // must be the same length as the "output" field

	    // Must be set to true to mark the layer as updatable.
	    // If true, the weightParams in the layer's properties must also be set to updatable
	    // If false, the value of the isUpdatable parameter within the layer's weights are ignored
	    bool isUpdatable = 10;

	    oneof layer {

	        // Start at 100 here
	        ConvolutionLayerParams convolution = 100;

	        PoolingLayerParams pooling = 120;

	        ActivationParams activation = 130;

	        InnerProductLayerParams innerProduct = 140;
	        EmbeddingLayerParams embedding = 150;

	        // Normalization-related Layers
	        BatchnormLayerParams batchnorm = 160;
	        MeanVarianceNormalizeLayerParams mvn = 165;
	        L2NormalizeLayerParams l2normalize = 170;
	        SoftmaxLayerParams softmax = 175;
	        LRNLayerParams lrn = 180;

	        CropLayerParams crop = 190;
	        PaddingLayerParams padding = 200;
	        UpsampleLayerParams upsample = 210;

	        ResizeBilinearLayerParams resizeBilinear = 211;
	        CropResizeLayerParams cropResize = 212;

	        UnaryFunctionLayerParams unary = 220;

	        // Element-wise Operations
	        AddLayerParams add = 230;
	        MultiplyLayerParams multiply = 231;

	        AverageLayerParams average = 240;
	        ScaleLayerParams scale = 245;

	        BiasLayerParams bias = 250;
	        MaxLayerParams max = 260;
	        MinLayerParams min = 261;

	        DotProductLayerParams dot = 270;
	        ReduceLayerParams reduce = 280;
	        LoadConstantLayerParams loadConstant = 290;

	        // Data Reorganization
	        ReshapeLayerParams reshape = 300;
	        FlattenLayerParams flatten = 301;
	        PermuteLayerParams permute = 310;
	        ConcatLayerParams concat = 320;
	        SplitLayerParams split = 330;
	        SequenceRepeatLayerParams sequenceRepeat = 340;

	        ReorganizeDataLayerParams reorganizeData = 345;
	        SliceLayerParams slice = 350;

	        // Recurrent Layers
	        SimpleRecurrentLayerParams simpleRecurrent = 400;
	        GRULayerParams gru = 410;
	        UniDirectionalLSTMLayerParams uniDirectionalLSTM = 420;
	        BiDirectionalLSTMLayerParams biDirectionalLSTM = 430;

	        // Custom (user-implemented) Layer
	        CustomLayerParams custom = 500;

	        // Following layers are available only after Core ML Specification
	        // version >= 4 (iOS >= 13, macOS >= 10.15)

	        // Control Flow related Layers
	        CopyLayerParams copy = 600;
	        BranchLayerParams branch = 605;

	        LoopLayerParams loop = 615;
	        LoopBreakLayerParams loopBreak = 620;
	        LoopContinueLayerParams loopContinue = 625;

	        RangeStaticLayerParams rangeStatic = 635;
	        RangeDynamicLayerParams rangeDynamic = 640;

	        // Element-wise Unary Layers
	        ClipLayerParams clip = 660;
	        CeilLayerParams ceil = 665;
	        FloorLayerParams floor = 670;

	        SignLayerParams sign = 680;
	        RoundLayerParams round = 685;

	        Exp2LayerParams exp2 = 700;

	        SinLayerParams sin = 710;
	        CosLayerParams cos = 715;
	        TanLayerParams tan = 720;

	        AsinLayerParams asin = 730;
	        AcosLayerParams acos = 735;
	        AtanLayerParams atan = 740;

	        SinhLayerParams sinh = 750;
	        CoshLayerParams cosh = 755;
	        TanhLayerParams tanh = 760;

	        AsinhLayerParams asinh = 770;
	        AcoshLayerParams acosh = 775;
	        AtanhLayerParams atanh = 780;

	        ErfLayerParams erf = 790;
	        GeluLayerParams gelu = 795;

	        // Element-wise Binary with Broadcasting Support
	        EqualLayerParams equal = 815;
	        NotEqualLayerParams notEqual = 820;
	        LessThanLayerParams lessThan = 825;
	        LessEqualLayerParams lessEqual = 827;
	        GreaterThanLayerParams greaterThan = 830;
	        GreaterEqualLayerParams greaterEqual = 832;

	        LogicalOrLayerParams logicalOr = 840;
	        LogicalXorLayerParams logicalXor = 845;
	        LogicalNotLayerParams logicalNot = 850;
	        LogicalAndLayerParams logicalAnd = 855;

	        ModBroadcastableLayerParams modBroadcastable = 865;
	        MinBroadcastableLayerParams minBroadcastable = 870;
	        MaxBroadcastableLayerParams maxBroadcastable = 875;
	        AddBroadcastableLayerParams addBroadcastable = 880;
	        PowBroadcastableLayerParams powBroadcastable = 885;
	        DivideBroadcastableLayerParams divideBroadcastable = 890;
	        FloorDivBroadcastableLayerParams floorDivBroadcastable = 895;
	        MultiplyBroadcastableLayerParams multiplyBroadcastable = 900;
	        SubtractBroadcastableLayerParams subtractBroadcastable = 905;

	        // Tensor Manipulations
	        TileLayerParams tile = 920;
	        StackLayerParams stack = 925;
	        GatherLayerParams gather = 930;
	        ScatterLayerParams scatter = 935;
	        GatherNDLayerParams gatherND = 940;
	        ScatterNDLayerParams scatterND = 945;
	        SoftmaxNDLayerParams softmaxND = 950;
	        GatherAlongAxisLayerParams gatherAlongAxis = 952;
	        ScatterAlongAxisLayerParams scatterAlongAxis = 954;

	        ReverseLayerParams reverse = 960;
	        ReverseSeqLayerParams reverseSeq = 965;

	        SplitNDLayerParams splitND = 975;
	        ConcatNDLayerParams concatND = 980;
	        TransposeLayerParams transpose = 985;

	        SliceStaticLayerParams sliceStatic = 995;
	        SliceDynamicLayerParams sliceDynamic = 1000;
	        SlidingWindowsLayerParams slidingWindows = 1005;

	        TopKLayerParams topK = 1015;
	        ArgMinLayerParams argMin = 1020;
	        ArgMaxLayerParams argMax = 1025;

	        EmbeddingNDLayerParams embeddingND = 1040;
	        BatchedMatMulLayerParams batchedMatmul = 1045;

	        // Tensor Allocation / Reshape-related Operations
	        GetShapeLayerParams getShape = 1065;
	        LoadConstantNDLayerParams loadConstantND = 1070;

	        FillLikeLayerParams fillLike = 1080;
	        FillStaticLayerParams fillStatic = 1085;
	        FillDynamicLayerParams fillDynamic = 1090;

	        BroadcastToLikeLayerParams broadcastToLike = 1100;
	        BroadcastToStaticLayerParams broadcastToStatic = 1105;
	        BroadcastToDynamicLayerParams broadcastToDynamic = 1110;

	        SqueezeLayerParams squeeze = 1120;
	        ExpandDimsLayerParams expandDims = 1125;
	        FlattenTo2DLayerParams flattenTo2D = 1130;
	        ReshapeLikeLayerParams reshapeLike = 1135;
	        ReshapeStaticLayerParams reshapeStatic = 1140;
	        ReshapeDynamicLayerParams reshapeDynamic = 1145;
	        RankPreservingReshapeLayerParams rankPreservingReshape = 1150;

	        ConstantPaddingLayerParams constantPad = 1155;

	        // Random Distributions
	        RandomNormalLikeLayerParams randomNormalLike = 1170;
	        RandomNormalStaticLayerParams randomNormalStatic = 1175;
	        RandomNormalDynamicLayerParams randomNormalDynamic = 1180;

	        RandomUniformLikeLayerParams randomUniformLike = 1190;
	        RandomUniformStaticLayerParams randomUniformStatic = 1195;
	        RandomUniformDynamicLayerParams randomUniformDynamic = 1200;

	        RandomBernoulliLikeLayerParams randomBernoulliLike = 1210;
	        RandomBernoulliStaticLayerParams randomBernoulliStatic = 1215;
	        RandomBernoulliDynamicLayerParams randomBernoulliDynamic = 1220;

	        CategoricalDistributionLayerParams categoricalDistribution = 1230;

	        // Reduction-related Layers:
	        ReduceL1LayerParams reduceL1 = 1250;
	        ReduceL2LayerParams reduceL2 = 1255;
	        ReduceMaxLayerParams reduceMax = 1260;
	        ReduceMinLayerParams reduceMin = 1265;
	        ReduceSumLayerParams reduceSum = 1270;
	        ReduceProdLayerParams reduceProd = 1275;
	        ReduceMeanLayerParams reduceMean = 1280;
	        ReduceLogSumLayerParams reduceLogSum = 1285;
	        ReduceSumSquareLayerParams reduceSumSquare = 1290;
	        ReduceLogSumExpLayerParams reduceLogSumExp = 1295;

	        // Masking / Selection Layers
	        WhereNonZeroLayerParams whereNonZero = 1313;
	        MatrixBandPartLayerParams matrixBandPart = 1315;
	        LowerTriangularLayerParams lowerTriangular = 1320;
	        UpperTriangularLayerParams upperTriangular = 1325;
	        WhereBroadcastableLayerParams whereBroadcastable = 1330;

	        // Normalization Layers
	        LayerNormalizationLayerParams layerNormalization = 1350;

	        NonMaximumSuppressionLayerParams NonMaximumSuppression = 1400;

	        // Following layers are available only after Core ML Specification
	        // version >= 5 (iOS >= 14, macOS >= 10.16)
	        OneHotLayerParams oneHot = 1450;
	        CumSumLayerParams cumSum = 1455;
	        ClampedReLULayerParams clampedReLU = 1460;
	        ArgSortLayerParams argSort = 1461;
	        Pooling3DLayerParams pooling3d = 1465;
	        GlobalPooling3DLayerParams globalPooling3d = 1466;
	        SliceBySizeLayerParams sliceBySize = 1470;
	        Convolution3DLayerParams convolution3d = 1471;

	    }

	}






BranchLayerParams
________________________________________________________________________________

Branching Layer

A layer that provides the functionality of branching or an If-Else block.

Must have 1 input. There are no outputs as the execution is transferred to either the
if or the else branch based on the value of the input.

Input is the condition predicate. Must be a scalar (length 1 tensor).


.. code-block:: proto

	message BranchLayerParams {

	    NeuralNetwork ifBranch = 1;
	    NeuralNetwork elseBranch = 2;

	}






LoopLayerParams
________________________________________________________________________________

Loop Layer

A layer that provides the functionality of a "for" loop or a "while" loop.

There are either no inputs or 1 input. When an input is present, it corresponds to the maximum loop count,
in that case the value of the "maxLoopIterations" field is ignored. Input must be a scalar.
(For description below, maxLoopIterations is assumed to be the value of the input, when its present)

No outputs are produced. Blobs produced by the condition or the body network are visible in the scope of the overall network.

"conditionNetwork" must produce a tensor with the name specified in the "conditionVar" field.

There are 3 possible cases for determining the termination condition:

Case 1:

If there is no "conditionNetwork", in this case the layer corresponds to a pure for loop, which is run "maxLoopIterations" number of times.
Equivalent pseudo-code:

for loopIterator = 0 : maxLoopIterations
     bodyNetwork()


Case 2:

"conditionNetwork" is present, and "maxLoopIterations" is 0 and there is no input,
in this case the layer corresponds to a while loop. Equivalent pseudo-code:

conditionVar = conditionNetwork()
while conditionVar:
     bodyNetwork()
     conditionVar = conditionNetwork()


Case 3:

"conditionNetwork" is provided, and "maxLoopIterations" is positive or there is an input,
in this case the layer corresponds to a while loop with a joint condition. Equivalent pseudo-code:

loopIterator = 0
conditionVar = conditionNetwork()
while (conditionVar and loopIterator < maxLoopIterations):
     bodyNetwork()
     loopIterator = loopIterator + 1
     conditionVar = conditionNetwork()


.. code-block:: proto

	message LoopLayerParams {

	    uint64 maxLoopIterations = 1;
	    string conditionVar = 2;
	    NeuralNetwork conditionNetwork = 3;
	    NeuralNetwork bodyNetwork = 4;

	}






LoopBreakLayerParams
________________________________________________________________________________

Loop break Layer

Terminate the loop that has this layer.
If present, it should always reside in the "bodyNetwork" of the loop layer

No inputs/outputs


.. code-block:: proto

	message LoopBreakLayerParams {

	}






LoopContinueLayerParams
________________________________________________________________________________

Loop Continue Layer

Stop the current loop iteration and continue on the next iteration.
If present, it should always reside in the "bodyNetwork" of the loop layer

No inputs/outputs


.. code-block:: proto

	message LoopContinueLayerParams {

	}






CopyLayerParams
________________________________________________________________________________

Copy Layer

A layer that copies its input tensor to the output tensor.
Must have 1 input and 1 output, with distinct names.
This is the only layer that is allowed to re-generate an output that is already present in the neural network prior to this layer,
in which case it will overwrite the output tensor.


.. code-block:: proto

	message CopyLayerParams {

	}






GreaterThanLayerParams
________________________________________________________________________________

GreaterThan Layer

Either 1 or 2 inputs.
Produces 1 output.
Perform elementwise greater than operation.

Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = x1 > x2
         or
     y = x1 > alpha, if only one input is provided

Broadcasting is supported.


.. code-block:: proto

	message GreaterThanLayerParams {

	    float alpha = 2;

	}






GreaterEqualLayerParams
________________________________________________________________________________

GreaterEqual Layer

Either 1 or 2 inputs.
Produces 1 output.
Perform elementwise greater equal operation.

Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = x1 >= x2
         or
     y = x1 >= alpha, if only one input is provided

Broadcasting is supported.


.. code-block:: proto

	message GreaterEqualLayerParams {

	    float alpha = 2;

	}






LessThanLayerParams
________________________________________________________________________________

LessThan Layer

Either 1 or 2 inputs.
Produces 1 output.
Perform elementwise less than operation.

Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = x1 < x2
         or
     y = x1 < alpha, if only one input is provided

Broadcasting is supported.


.. code-block:: proto

	message LessThanLayerParams {

	    float alpha = 2;

	}






LessEqualLayerParams
________________________________________________________________________________

LessEqual Layer

Either 1 or 2 inputs.
Produces 1 output.
Perform elementwise less equal operation.

Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = x1 <= x2
         or
     y = x1 <= alpha, if only one input is provided

Broadcasting is supported.


.. code-block:: proto

	message LessEqualLayerParams {

	    float alpha = 2;

	}






EqualLayerParams
________________________________________________________________________________

Equal Layer

Either 1 or 2 inputs.
Produces 1 output.
Perform elementwise equal operation.

Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = x1 == x2
         or
     y = x1 == alpha, if only one input is provided

Broadcasting is supported.


.. code-block:: proto

	message EqualLayerParams {

	    float alpha = 1;

	}






NotEqualLayerParams
________________________________________________________________________________

NotEqual Layer

Either 1 or 2 inputs.
Produces 1 output.
Perform elementwise not equal operation.

Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = x1 != x2
         or
     y = x1 != alpha, if only one input is provided

Broadcasting is supported.


.. code-block:: proto

	message NotEqualLayerParams {

	    float alpha = 1;

	}






LogicalAndLayerParams
________________________________________________________________________________

LogicalAnd Layer

Must have 2 inputs, produces 1 output.
Perform elementwise logical AND operation.

Input is considered False if equal to 0.0f otherwise True.
Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = AND(x1, x2)

Broadcasting is supported.


.. code-block:: proto

	message LogicalAndLayerParams {

	}






LogicalOrLayerParams
________________________________________________________________________________

LogicalOr Layer

Must have 2 inputs, produces 1 output.
Perform elementwise logical OR operation.

Input is considered False if equal to 0.0f otherwise True.
Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = OR(x1, x2)

Broadcasting is supported.


.. code-block:: proto

	message LogicalOrLayerParams {

	}






LogicalXorLayerParams
________________________________________________________________________________

LogicalXor Layer

Must have 2 inputs, produces 1 output.
Perform elementwise logical XOR operation.

Input is considered False if equal to 0.0f otherwise True.
Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = XOR(x1, x2)

Broadcasting is supported.


.. code-block:: proto

	message LogicalXorLayerParams {

	}






LogicalNotLayerParams
________________________________________________________________________________

LogicalNot Layer

Must have 1 input, produces 1 output.
Perform elementwise logical NOT operation.

Input is considered False if equal to 0.0f otherwise True.
Output is 1.0f if the condition is true otherwise 0.0f.

.. code::

     y = NOT(x)


.. code-block:: proto

	message LogicalNotLayerParams {

	}






BorderAmounts
________________________________________________________________________________

Specifies the amount of spatial border to be either padded or cropped.

For padding:

.. code::

    H_out = borderAmounts[0].startEdgeSize + H_in + borderAmounts[0].endEdgeSize
    W_out = borderAmounts[1].startEdgeSize + W_in + borderAmounts[1].endEdgeSize

    topPaddingAmount == Height startEdgeSize
    bottomPaddingAmount == Height endEdgeSize
    leftPaddingAmount == Width startEdgeSize
    rightPaddingAmount == Width endEdgeSize

For cropping:

.. code::

    H_out = (-borderAmounts[0].startEdgeSize) + H_in + (-borderAmounts[0].endEdgeSize)
    W_out = (-borderAmounts[1].startEdgeSize) + W_in + (-borderAmounts[1].endEdgeSize)

    topCropAmount == Height startEdgeSize
    bottomCropAmount == Height endEdgeSize
    leftCropAmount == Width startEdgeSize
    rightCropAmount == Width endEdgeSize


.. code-block:: proto

	message BorderAmounts {

	    message EdgeSizes {
	        uint64 startEdgeSize = 1;

	        uint64 endEdgeSize = 2;
	    }

	    repeated EdgeSizes borderAmounts = 10;

	}






BorderAmounts.EdgeSizes
--------------------------------------------------------------------------------




.. code-block:: proto

	    message EdgeSizes {
	        uint64 startEdgeSize = 1;

	        uint64 endEdgeSize = 2;
	    }






ValidPadding
________________________________________________________________________________

Specifies the type of padding to be used with Convolution/Deconvolution and Pooling layers.
After padding, input spatial shape: ``[H_in, W_in]``, gets modified to the
output spatial shape ``[H_out, W_out]``.

.. code::

     topPaddingAmount == Height startEdgeSize == borderAmounts[0].startEdgeSize
     bottomPaddingAmount == Height endEdgeSize == borderAmounts[0].endEdgeSize
     leftPaddingAmount == Width startEdgeSize == borderAmounts[1].startEdgeSize
     rightPaddingAmount == Width endEdgeSize == borderAmounts[1].endEdgeSize

With Convolution or Pooling:

.. code::

   H_out = int_division_round_down((H_in + topPaddingAmount + bottomPaddingAmount - KernelSize[0]),stride[0]) + 1

which is same as:

.. code::

   H_out = int_division_round_up((H_in + topPaddingAmount + bottomPaddingAmount - KernelSize[0] + 1),stride[0])

With Deconvolution:

.. code::

   H_out = (H_in-1) * stride[0] + kernelSize[0] - (topPaddingAmount + bottomPaddingAmount)


The equivalent expressions hold true for ``W_out`` as well.


By default, the values of ``paddingAmounts`` are set to ``0``,
which results in a "true" valid padding.
If non-zero values are provided for ``paddingAmounts``,
"valid" convolution/pooling is performed within the spatially expanded input.


.. code-block:: proto

	message ValidPadding {

	    BorderAmounts paddingAmounts = 1;

	}






SamePadding
________________________________________________________________________________

Specifies the type of padding to be used with Convolution/Deconvolution and pooling layers.
After padding, input spatial shape: ``[H_in, W_in]``, gets modified to the
output spatial shape ``[H_out, W_out]``.
With Convolution or pooling:

.. code::

     H_out = int_division_round_up(H_in,stride[0])
     W_out = int_division_round_up(W_in,stride[1])

This is achieved by using the following padding amounts:

.. code::

    totalPaddingHeight = max(0,(H_out-1) * stride[0] + KernelSize[0] - Hin)
    totalPaddingWidth = max(0,(W_out-1) * stride[1] + KernelSize[1] - Win)

There are two modes of asymmetry:
``BOTTOM_RIGHT_HEAVY``, and ``TOP_LEFT_HEAVY``.

If the mode is ``BOTTOM_RIGHT_HEAVY``:

.. code::

    topPaddingAmount = floor(totalPaddingHeight / 2)
    bottomPaddingAmount = totalPaddingHeight - topPaddingAmount
    leftPaddingAmount = floor(totalPaddingWidth / 2)
    rightPaddingAmount = totalPaddingWidth - leftPaddingAmount

If the mode is ``TOP_LEFT_HEAVY``:

.. code::

    bottomPaddingAmount = floor(totalPaddingHeight / 2)
    topPaddingAmount = totalPaddingHeight - bottomPaddingAmount
    rightPaddingAmount = floor(totalPaddingWidth / 2)
    leftPaddingAmount = totalPaddingWidth - rightPaddingAmount


With Deconvolution:

.. code::

   H_out = H_in * stride[0]
   W_out = W_in * stride[1]


.. code-block:: proto

	message SamePadding {

	    enum SamePaddingMode {

	        BOTTOM_RIGHT_HEAVY = 0;
	        TOP_LEFT_HEAVY = 1;

	    }
	    SamePaddingMode asymmetryMode = 1;

	}






SamplingMode
________________________________________________________________________________

Specifies how grid points are sampled from an interval.
Without the loss of generality, assume the interval to be [0, X-1] from which N points are to be sampled.
Here X may correspond to an input image's height or width.
All the methods can be expressed in terms of numpy's linspace function, along with the constraint that grid points have to lie in the interval [0, X-1].
Note: numpy.linspace(start = start, end = end, num = N, endpoint = True) corresponds to sampling
N points uniformly from the interval [start, end], endpoints included.
The methods vary in how the ``start`` and ``end`` values are computed.


.. code-block:: proto

	message SamplingMode {

	    enum Method {

	        STRICT_ALIGN_ENDPOINTS_MODE = 0;

	        ALIGN_ENDPOINTS_MODE = 1;

	        UPSAMPLE_MODE = 2;

	        ROI_ALIGN_MODE = 3;

	    }

	    Method samplingMethod = 1;

	}






BoxCoordinatesMode
________________________________________________________________________________

Specifies the convention used to specify four bounding box coordinates for an image of size (Height, Width).
The (0,0) coordinate corresponds to the top-left corner of the image.


.. code-block:: proto

	message BoxCoordinatesMode {

	    enum Coordinates {

	        CORNERS_HEIGHT_FIRST = 0;

	        CORNERS_WIDTH_FIRST = 1;

	        CENTER_SIZE_HEIGHT_FIRST = 2;

	        CENTER_SIZE_WIDTH_FIRST = 3;

	    }

	    Coordinates boxMode = 1;

	}






WeightParams
________________________________________________________________________________

Weights for layer parameters.
Weights are stored as repeated floating point numbers
using row-major ordering
and can represent 1-, 2-, 3-, or 4-dimensional data.


.. code-block:: proto

	message WeightParams {

	    repeated float floatValue = 1;

	    bytes float16Value = 2;

	    bytes rawValue = 30;

	    bytes int8RawValue = 31;

	    QuantizationParams quantization = 40;

	    bool isUpdatable = 50;

	}






QuantizationParams
________________________________________________________________________________

Quantization parameters.


.. code-block:: proto

	message QuantizationParams {

	    uint64 numberOfBits = 1;
	    oneof QuantizationType {
	        LinearQuantizationParams linearQuantization = 101;
	        LookUpTableQuantizationParams lookupTableQuantization = 102;
	    }

	}






LinearQuantizationParams
________________________________________________________________________________




.. code-block:: proto

	message LinearQuantizationParams {

	    repeated float scale = 1;
	    repeated float bias = 2;

	}






LookUpTableQuantizationParams
________________________________________________________________________________




.. code-block:: proto

	message LookUpTableQuantizationParams {

	    (2^numberOfBits) Elements.
	    repeated float floatValue = 1;

	}






ConvolutionLayerParams
________________________________________________________________________________

A layer that performs spatial convolution or deconvolution.

.. code::

     y = ConvolutionLayer(x)

Requires 1 or 2 inputs and produces 1 output.

Input
   First Input:
     A blob with rank greater than or equal to 4.
     Rank 4 blob represents [Batch, channels, height, width].
     For ranks greater than 4, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.

    From Core ML specification version 4 onwards (iOS >= 13, macOS >= 10.15).
    convolution layer can have 2 inputs, in which case the second input is
    the blob representing the weights. This is allowed when "isDeconvolution" = False.
    The weight blob should have shape
    ``[outputChannels, kernelChannels, kernelHeight, kernelWidth]``,
    where kernelChannels == inputChannels / nGroups.

Output
  Rank is same as the input. e.g.: for rank 4 input, output shape is [B, C_out, H_out, W_out]


If ``dilationFactor`` is not 1, effective kernel size is
modified as follows:

.. code::

     KernelSize[0] <-- (kernelSize[0]-1) * dilationFactor[0] + 1
     KernelSize[1] <-- (kernelSize[1]-1) * dilationFactor[1] + 1

Type of padding can be ``valid`` or ``same``. Output spatial dimensions depend on the
the type of padding. For details, refer to the descriptions of the messages "ValidPadding"
and "SamePadding". Padded values are all zeros.

For Deconvolution, ``ConvolutionPaddingType`` (``valid`` or ``same``) is ignored when ``outputShape`` is set.


.. code-block:: proto

	message ConvolutionLayerParams {

	    uint64 outputChannels = 1;

	    uint64 kernelChannels = 2;

	    uint64 nGroups = 10;

	    repeated uint64 kernelSize = 20;

	    repeated uint64 stride = 30;

	    repeated uint64 dilationFactor = 40;

	    oneof ConvolutionPaddingType {
	        ValidPadding valid = 50;
	        SamePadding same = 51;
	    }

	    bool isDeconvolution = 60;

	    bool hasBias = 70;

	    WeightParams weights = 90;
	    WeightParams bias = 91;

	    repeated uint64 outputShape = 100;

	}






Convolution3DLayerParams
________________________________________________________________________________

A layer that performs a 3-dimensional convolution.

.. code::

     y = Convolution3DLayer(x)

Input
   A blob of rank 5.
   The input blob's shape should be ``[batch, channels, depth, height, width]``.

Fields
  The bias field, if set, should have shape of ``[channelsOut]``.

Output
  A blob of rank 5.
  The output blob's shape is ``[batch, channelsOut, depthOut, heightOut, widthOut]``.

Type of padding can be ``custom``, ``valid``, or ``same``. Padded values are all zeros.
Output spatial dimensions depend on the the type of padding. For details, refer to the
descriptions of the ``PaddingType`` field of this ``Convolution3DLayerParams`` message.

Example
  For example, given an input of size ``[1, 3, 3, 8, 8]``, a stride of 2 in each dimension,
  a kernel of 3 in each dimension, 2 output channels, and ``same`` padding, this layer will
  compute the total padding applied in the depth, height, and width dimensions to be 2, 1, and 1,
  respectively. The depth padding is even and will be applied equally to both sides of the depth
  dimension. Since the height and width padding values are odd, they'll be applied to the
  bottom/right of the height/width dimensions. Thus, the padding applied to the input will be
  ``[1, 1, 0, 1, 0, 1]`` (front, back, top, bottom, left, right). Finally, the output produced
  will have size ``[1, 2, 2, 4, 4]``.


.. code-block:: proto

	message Convolution3DLayerParams {

	    int32 outputChannels = 1;

	    int32 inputChannels = 2;

	    int32 nGroups = 10;

	    int32 kernelDepth = 20;

	    int32 kernelHeight = 21;

	    int32 kernelWidth = 22;

	    int32 strideDepth = 31;

	    int32 strideHeight = 32;

	    int32 strideWidth = 33;

	    int32 dilationDepth = 40;

	    int32 dilationHeight = 41;

	    int32 dilationWidth = 42;

	    bool hasBias = 50;

	    WeightParams weights = 60;

	    WeightParams bias = 61;


	    enum PaddingType {
	        CUSTOM = 0;
	        VALID = 1;
	        SAME = 2;
	    }
	    PaddingType paddingType = 70;

	    int32 customPaddingFront = 80;

	    int32 customPaddingBack = 81;

	    int32 customPaddingTop = 82;

	    int32 customPaddingBottom = 83;

	    int32 customPaddingLeft = 84;

	    int32 customPaddingRight = 85;

	}






InnerProductLayerParams
________________________________________________________________________________

A layer that performs a matrix-vector or matrix-matrix product.
This is equivalent to a fully-connected, or dense layer.
The weight parameters correspond to a matrix of dimensions (inputChannels, outputChannels) i.e. (C_in, C_out)

.. code::

     y = InnerProductLayer(x)

Requires 1 input and produces 1 output.

Input
     Input can have rank 1 to rank 5. This is how it is reshaped in to the matrix (for rank > 1):
     rank 1 (x1) : in this case, the layer corresponds to a matrix-vector product. x1 must be equal to C_in
     rank 2 (x1, x2): x2 must be equal to C_in
     rank 3 (x1, x2, x3) --> (x1 * x2, x3). x3 must be equal to C_in
     rank 4 (x1, x2, x3, x4) ---> (x1, x2 * x3 * x4). x2 * x3 * x4 must be equal to C_in
     rank 5 (x1, x2, x3, x4, x5) ---> (x1 * x2, x3 * x4 * x5). x3 * x4 * x5 must be equal to C_in

Output
     Output rank is same as the input rank
     rank 1: (C_out)
     rank 2: (x1, C_out)
     rank 3: (x1, x2, C_out)
     rank 4: (x1, C_out, 1, 1)
     rank 5: (x1, x2, C_out, 1, 1)


.. code-block:: proto

	message InnerProductLayerParams {

	    uint64 inputChannels = 1;
	    uint64 outputChannels = 2;

	    bool hasBias = 10;

	    WeightParams weights = 20;
	    WeightParams bias = 21;

	    bool int8DynamicQuantize = 22;

	}






EmbeddingLayerParams
________________________________________________________________________________

A layer that performs a matrix lookup and optionally adds a bias.
The weights matrix is stored with dimensions [outputChannels, inputDim].

.. code::

     y = EmbeddingLayer(x)

Requires 1 input and produces 1 output.

Input
    Input values must be in the range ``[0, inputDim - 1]``.

    Input must have rank equal to 4 or 5, such that the last 3 dimensions are all 1.
    rank 4: shape (x1, 1, 1, 1). x1 is effectively the batch/sequence length.
    rank 5: shape (x1, x2 , 1, 1, 1). x1 * x2 is effectively the combined batch/sequence length.

Output
     Output rank is same as the input rank. Please see input description above.
     rank 4: shape (x1, outputChannels, 1, 1)
     rank 5: shape (x1, x2, outputChannels, 1, 1)


.. code-block:: proto

	message EmbeddingLayerParams {

	    uint64 inputDim = 1;
	    uint64 outputChannels = 2;

	    bool hasBias = 10;

	    WeightParams weights = 20;
	    WeightParams bias = 21;

	}






EmbeddingNDLayerParams
________________________________________________________________________________

A layer that performs a matrix lookup and optionally adds a bias.
The weights matrix is stored with dimensions [embeddingSize, vocabSize].

.. code::

     y = EmbeddingNDLayer(x)

Requires 1 input and produces 1 output.

Input
    Input values must be in the range ``[0, vocabSize - 1]``.
    Input must have rank at least 2. The last dimension must always be 1.
    rank 2: shape (x1, 1). x1 is the batch/sequence length.
    rank 3: shape (x1, x2, 1). x1 * x2 is effectively the combined batch/sequence length.
    rank 4: shape (x1, x2, x3, 1). x1 * x2 * x2 is effectively the combined batch/sequence length.
    rank 5: shape (x1, x2 , x3, x4, 1). x1 * x2 * x3 * x4 is effectively the combined batch/sequence length.

Output
     Output rank is same as the input rank. Please see input description above.
     rank 2: shape (x1, embeddingSize)
     rank 3: shape (x1, x2, embeddingSize)
     rank 4: shape (x1, x2, x3, embeddingSize)
     rank 5: shape (x1, x2, x3, x4, embeddingSize)


.. code-block:: proto

	message EmbeddingNDLayerParams {

	    uint64 vocabSize = 1;
	    uint64 embeddingSize = 2;
	    bool hasBias = 3;
	    WeightParams weights = 20;
	    WeightParams bias = 21;

	}






BatchnormLayerParams
________________________________________________________________________________

A layer that performs batch normalization,
which is performed along axis = -3,
and repeated along the other axes, if present.

.. code::

     y = BatchnormLayer(x)

Requires 1 input and produces 1 output.

This operation is described by the following formula:

.. math::
    y_i = \gamma_i \dfrac{ (x_i - \mu_i)}{\sqrt{\sigma_i^2 + \epsilon}} + \beta_i \;,\;i=1,....,C

Input
    A blob with rank greater than equal to 3.
    Example: Rank 4 blob represents [Batch, channels, height, width]
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.

Output
    A blob with the same shape as the input.


.. code-block:: proto

	message BatchnormLayerParams {

	    uint64 channels = 1;

	    bool computeMeanVar = 5;
	    bool instanceNormalization = 6;

	    float epsilon = 10;

	    WeightParams gamma = 15;
	    WeightParams beta = 16;
	    WeightParams mean = 17;
	    WeightParams variance = 18;

	}






PoolingLayerParams
________________________________________________________________________________

A spatial pooling layer.

.. code::

     y = PoolingLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank greater than equal to 4.
    Rank 4 blob represents [Batch, channels, height, width]
    For ranks greater than 4, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.

Output
    Rank is same as the input. e.g.: for rank 4 input, output shape is [B, C, H_out, W_out]

Padding options are similar to ``ConvolutionLayerParams``
with the additional option of ``ValidCompletePadding`` (``includeLastPixel``),
which ensures that the last application of the kernel
always includes the last pixel of the input image, if there is padding.

.. code::

    H_out = ceil(float(H_in + 2 * paddingAmounts[0] - kernelSize[0])/float(Stride[0])) + 1
    if (paddingAmounts[0] > 0 or paddingAmounts[1] > 0)
         if ((H_out - 1) * Stride >= H_in + paddingAmounts[0]) {
             H_out = H_out - 1
         }
    }

The equivalent expressions hold true for ``W_out`` as well.
Only symmetric padding is supported with this option.


.. code-block:: proto

	message PoolingLayerParams {

	    enum PoolingType {

	        MAX = 0;
	        AVERAGE = 1;
	        L2 = 2;

	    }
	    PoolingType type = 1;

	    repeated uint64 kernelSize = 10;

	    repeated uint64 stride = 20;

	    message ValidCompletePadding {

	        repeated uint64 paddingAmounts = 10;

	    }

	    oneof PoolingPaddingType {
	        ValidPadding valid = 30;
	        SamePadding same = 31;
	        ValidCompletePadding includeLastPixel = 32;
	    }

	    bool avgPoolExcludePadding = 50;

	    bool globalPooling = 60;

	}






PoolingLayerParams.ValidCompletePadding
--------------------------------------------------------------------------------




.. code-block:: proto

	    message ValidCompletePadding {

	        repeated uint64 paddingAmounts = 10;

	    }






Pooling3DLayerParams
________________________________________________________________________________




.. code-block:: proto

	message Pooling3DLayerParams {

	    enum PoolingType3D {
	        MAX = 0;
	        AVERAGE = 1;
	    }

	    // Whether to use Max or Average
	    PoolingType3D type = 1;

	    // Depth of the pooling region.
	    int32 kernelDepth = 2;

	    // Height of the pooling region.
	    int32 kernelHeight = 3;

	    // Width of the pooling region.
	    int32 kernelWidth = 4;

	    // Stride along the depth direction
	    int32 strideDepth = 5;

	    // Stride along the height direction
	    int32 strideHeight = 6;

	    // Stride along the width direction
	    int32 strideWidth = 7;

	    enum Pooling3DPaddingType {
	        CUSTOM = 0;
	        VALID = 1;
	        SAME = 2;
	    }
	    Pooling3DPaddingType paddingType = 15;

	    // Padding before the input in the depth direction.
	    int32 customPaddingFront = 8;

	    // Padding after the input in the depth direction.
	    int32 customPaddingBack = 9;

	    // Padding before the input in the height direction.
	    int32 customPaddingTop = 10;

	    // Padding after the input in the height direction.
	    int32 customPaddingBottom = 11;

	    // Padding before the input in the width direction.
	    int32 customPaddingLeft = 12;

	    // Padding after the input in the width direction.
	    int32 customPaddingRight = 13;

	    // If true, exclude zeros from padding in Average pooling.  Meaningless in Max Pooling.
	    bool countExcludePadding = 14;
	}






GlobalPooling3DLayerParams
________________________________________________________________________________




.. code-block:: proto

	message GlobalPooling3DLayerParams {

	    enum GlobalPoolingType3D {
	        MAX = 0;
	        AVERAGE = 1;
	    }

	    // Whether to use Max or Average
	    GlobalPoolingType3D type = 1;
	}






PaddingLayerParams
________________________________________________________________________________

A layer that performs padding along spatial dimensions.

.. code::

     y = PaddingLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank at least 2.
    e.g.: blob with shape ``[H_in, W_in]``.
    For ranks greater than 2, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch
    i.e. Padding is applied on last two dimensions.

Output
    Same rank as the input.
    e.g.: blob with shape ``[H_out, W_out]``.

Output dimensions are calculated as follows:

.. code::

    H_out = H_in + topPaddingAmount + bottomPaddingAmount
    W_out = W_in + leftPaddingAmount + rightPaddingAmount

    topPaddingAmount == Height startEdgeSize == borderAmounts[0].startEdgeSize
    bottomPaddingAmount == Height endEdgeSize == borderAmounts[0].endEdgeSize
    leftPaddingAmount == Width startEdgeSize == borderAmounts[1].startEdgeSize
    rightPaddingAmount == Width endEdgeSize == borderAmounts[1].endEdgeSize

There are three types of padding:

- ``PaddingConstant``, which fills a constant value at the border.
- ``PaddingReflection``, which reflects the values at the border.
- ``PaddingReplication``, which replicates the values at the border.

Given the following input:

.. code::

    [1, 3, 4]  :  1   2   3   4
                  5   6   7   8
                  9   10  11  12

Here is the output of applying the padding
``(top=2, left=2, bottom=0, right=0)``
with each of the supported types:

- ``PaddingConstant`` (``value = 0``):
  .. code::

      [1, 5, 6]  :  0   0   0  0   0   0
                    0   0   0  0   0   0
                    0   0   1  2   3   4
                    0   0   5  6   7   8
                    0   0   9  10  11  12

- ``PaddingReflection``:
  .. code::

      [1, 5, 6]  :  11  10  9  10  11  12
                    7   6   5  6   7   8
                    3   2   1  2   3   4
                    7   6   5  6   7   8
                    11  10  9  10  11  12

- ``PaddingReplication``:
  .. code::

      [1, 5, 6]  :  1   1   1  2   3   4
                    1   1   1  2   3   4
                    1   1   1  2   3   4
                    5   5   5  6   7   8
                    9   9   9  10  11  12


.. code-block:: proto

	message PaddingLayerParams {

	    message PaddingConstant {
	        float value = 1;
	    }

	    message PaddingReflection {
	    }

	    message PaddingReplication {
	    }

	    oneof PaddingType {
	        PaddingConstant constant = 1;
	        PaddingReflection reflection = 2;
	        PaddingReplication replication = 3;
	    }

	    BorderAmounts paddingAmounts = 10;

	}






PaddingLayerParams.PaddingConstant
--------------------------------------------------------------------------------

Fill a constant value in the padded region.


.. code-block:: proto

	    message PaddingConstant {
	        float value = 1;
	    }






PaddingLayerParams.PaddingReflection
--------------------------------------------------------------------------------

Reflect the values at the border for padding.


.. code-block:: proto

	    message PaddingReflection {
	    }






PaddingLayerParams.PaddingReplication
--------------------------------------------------------------------------------

Replicate the values at the border for padding.


.. code-block:: proto

	    message PaddingReplication {
	    }






ConcatLayerParams
________________________________________________________________________________

A layer that concatenates along the axis = -3 or -5.
For general concatenation along any axis, see ConcatNDLayer.

.. code::

     y = ConcatLayer(x1,x2,....)

Requires more than 1 input and produces 1 output.

Input
  All input blobs must have same rank.
  If "sequenceConcat" = False, rank must be greater than equal to 3. In this case concatenation is along axis = -3
  If "sequenceConcat" = True, rank must be greater than equal to 5. In this case concatenation is along axis = -5

Output
  Same rank as the input.


.. code-block:: proto

	message ConcatLayerParams {

	    bool sequenceConcat = 100;

	}






LRNLayerParams
________________________________________________________________________________

A layer that performs local response normalization (LRN).

.. code::

     y = LRNLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank greater than equal to 3.
    Example: Rank 4 blob represents [Batch, channels, height, width]
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.
Output
    A blob with the same shape as the input.

This layer is described by the following formula:

.. math::
    x_i \leftarrow  \dfrac{x_i}{\left ( k + \dfrac{\alpha}{C} \sum_j x_j^2 \right )^\beta}

where the summation is done over a ``(localSize, 1, 1)`` neighborhood ---
that is, over a window "across" channels in 1x1 spatial neighborhoods.


.. code-block:: proto

	message LRNLayerParams {

	    float alpha = 1;
	    float beta = 2;
	    uint64 localSize = 3;
	    float k = 4;

	}






SoftmaxLayerParams
________________________________________________________________________________

Softmax Normalization Layer

A layer that performs softmax normalization.
Normalization is applied along axis = -3 or N-3 (where N is the rank of the input)
For softmax layer that can operate on any axis, see SoftmaxNDLayer.


.. code::

     y = SoftmaxLayer(x)

Requires 1 input and produces 1 output.

Input
    Must be a blob with rank >= 3.
Output
    A blob with the same shape as the input.

This layer is described by the following formula:

.. math::
    x_i \leftarrow \dfrac{e^{x_i}}{\sum_i{e^{x_i}}}


.. code-block:: proto

	message SoftmaxLayerParams {

	}






SplitLayerParams
________________________________________________________________________________

A layer that uniformly splits across axis = -3 to produce a specified number of outputs.
For general split operation along any axis, see SplitNDLayer.

.. code::

     (y1,y2,...yN) = SplitLayer(x), where N = nOutputs

Requires 1 input and produces multiple outputs.

Input
    A blob with rank at least 3.
    e.g.: blob with shape ``[C, H, W]``
Output
    ``nOutputs`` blobs each with same rank as the input.
    e.g.: For input that is of shape ``[C, H, W]``, output shapes will be ``[C/nOutputs, H, W]``


.. code-block:: proto

	message SplitLayerParams {

	    uint64 nOutputs = 1;

	}






AddLayerParams
________________________________________________________________________________

A layer that performs elementwise addition.
This layer has limited broadcasting support. For general broadcasting see AddBroadcastableLayer.

.. code::

     y = AddLayer(x1,x2,...)

Requires 1 or more than 1 input and produces 1 output.

Input
    In general, there are no rank constraints.
    However, only certain set of shapes are broadcastable. For example:
    [B, 1, 1, 1], [B, C, 1, 1], [B, 1, H, W], [B, C, H, W]
Output
    A blob with shape equal to the input blob.

If only one input is provided, scalar addition is performed:

.. math::
    y = x + \alpha


.. code-block:: proto

	message AddLayerParams {

	    float alpha = 1;

	}






MultiplyLayerParams
________________________________________________________________________________

A layer that performs elementwise multiplication.
This layer has limited broadcasting support. For general broadcasting see MultiplyBroadcastableLayer.

.. code::

     y = MultiplyLayer(x1,x2,...)

Requires 1 or more than 1 input and produces 1 output.

Input
    In general, there are no rank constraints.
    However, only certain set of shapes are broadcastable. For example:
    [B, 1, 1, 1], [B, C, 1, 1], [B, 1, H, W], [B, C, H, W]
Output
    A blob with shape equal to the first input blob.

If only one input is provided, scalar multiplication is performed:

.. math::
    y = \alpha x


.. code-block:: proto

	message MultiplyLayerParams {

	    float alpha = 1;

	}






UnaryFunctionLayerParams
________________________________________________________________________________

A layer that applies a unary function.

.. code::

     y = UnaryFunctionLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with no rank constraints.
Output
    A blob with the same shape as the input.

The input is first modified by shifting and scaling:

.. math::
    x \leftarrow \text{scale} \cdot x + \text{shift}


.. code-block:: proto

	message UnaryFunctionLayerParams {

	    enum Operation {
	        SQRT = 0;
	        RSQRT = 1;
	        INVERSE = 2;
	        POWER = 3;
	        EXP = 4;
	        LOG = 5;
	        ABS = 6;
	        THRESHOLD = 7;
	    }
	    Operation type = 1;

	    float alpha = 2;

	    float epsilon = 3;

	    float shift = 4;

	    float scale = 5;

	}






UpsampleLayerParams
________________________________________________________________________________

A layer that scales up spatial dimensions.
It supports two modes: nearest neighbour (default) and bilinear.

.. code::

     y = UpsampleLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank at least 3.
    e.g.: blob with shape ``[C, H, W]``.
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.

Output
    Same rank as the input.
    e.g.: blob with shape ``[C, scalingFactor[0] * H, scalingFactor[1] * W]``


.. code-block:: proto

	message UpsampleLayerParams {

	    repeated uint64 scalingFactor = 1;

	    repeated float fractionalScalingFactor = 7;

	    enum InterpolationMode {

	        NN = 0;
	        BILINEAR = 1;

	    }

	    InterpolationMode mode = 5;

	    enum LinearUpsampleMode {

	        DEFAULT = 0;
	        ALIGN_CORNERS_TRUE = 1;
	        ALIGN_CORNERS_FALSE = 2;

	    }

	    LinearUpsampleMode linearUpsampleMode = 6;

	}






ResizeBilinearLayerParams
________________________________________________________________________________

A layer that resizes the input to a pre-specified spatial size using bilinear interpolation.

.. code::

     y = ResizeBilinearLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank at least 3.
    e.g.: blob with shape ``[C, H_in, W_in]``.
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.

Output
    Same rank as the input.
    e.g.: blob with shape ``[C, H_out, W_out]``.


.. code-block:: proto

	message ResizeBilinearLayerParams {

	    repeated uint64 targetSize = 1;

	    SamplingMode mode = 2;

	}






CropResizeLayerParams
________________________________________________________________________________

A layer that extracts cropped spatial patches or RoIs (regions of interest) from the input and resizes them to a pre-specified size using
bilinear interpolation.
Note that RoI Align layer can be implemented with this layer followed by a pooling layer.

.. code::

     y = CropResizeLayer(x)

Requires 2 inputs and produces 1 output.

Input
    There are two inputs.
    First input represents an image feature map.
    Second input represents the bounding box coordinates for N patches or RoIs (region of interest).

    First input is rank 5: [1, Batch, C, H_in, W_in].
    Second input is rank 5. Its shape can be either [N, 1, 4, 1, 1] or [N, 1, 5, 1, 1].

    N: number of patches/RoIs to be extracted

    If RoI shape = ``[N, 1, 4, 1, 1]``
                   The axis=-3 corresponds to the four coordinates specifying the bounding box.
                   All the N RoIs are extracted from all the batches of the input.

    If RoI shape = ``[N, 1, 5, 1, 1]``
                    The first element of the axis=-3 specifies the input batch id from which to extract the RoI and
                              must be in the interval ``[0, Batch - 1]``. That is, n-th RoI is extracted from the RoI[n,0,0,0,0]-th
                    input batch id. The last four elements of the axis=-3 specify the bounding box coordinates.

Output
    A blob with rank 5.
          - Shape is [N, Batch, C, H_out, W_out] if input RoI shape is [N, 1, 4, 1, 1]
          - Shape is [N, 1, C, H_out, W_out] if input RoI shape is [N, 1, 5, 1, 1]


.. code-block:: proto

	message CropResizeLayerParams {

	    repeated uint64 targetSize = 1;

	    bool normalizedCoordinates = 2;

	    SamplingMode mode = 3;

	    BoxCoordinatesMode boxIndicesMode = 4;

	    float spatialScale = 5;

	}






BiasLayerParams
________________________________________________________________________________

A layer that performs elementwise addition of a bias,
which is broadcasted to match the input shape.

.. code::

     y = BiasLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank at least 3.
    e.g.: blob with shape ``[C, H, W]``.
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.
Output
    A blob with the same shape as the input.


.. code-block:: proto

	message BiasLayerParams {

	    repeated uint64 shape = 1;

	    WeightParams bias = 2;

	}






ScaleLayerParams
________________________________________________________________________________

A layer that performs elmentwise multiplication by a scale factor
and optionally adds a bias;
both the scale and bias are broadcasted to match the input shape.

.. code::

     y = ScaleLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank at least 3.
    e.g.: blob with shape ``[C, H, W]``.
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.
Output
    A blob with the same shape as the input.


.. code-block:: proto

	message ScaleLayerParams {

	    repeated uint64 shapeScale = 1;

	    WeightParams scale = 2;

	    bool hasBias = 3;

	    repeated uint64 shapeBias = 4;

	    WeightParams bias = 5;

	}






LoadConstantLayerParams
________________________________________________________________________________

A layer that loads data as a parameter and provides it as an output.
The output is rank 5. For general rank, see LoadConstantNDLayer.

.. code::

     y = LoadConstantLayer()

Requires no input and produces 1 output.

Output:
    A blob with rank 5 and shape ``[1, 1, C, H, W]``


.. code-block:: proto

	message LoadConstantLayerParams {

	    repeated uint64 shape = 1;

	    WeightParams data = 2;

	}






L2NormalizeLayerParams
________________________________________________________________________________

A layer that performs L2 normalization, i.e. divides by the
the square root of the sum of squares of all elements of input.

.. code::

     y = L2NormalizeLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank greater than equal to 3.
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.
Output
    A blob with the same shape as the input.

This layer is described by the following formula:

.. math::
    x_i \leftarrow \dfrac{x_i}{\sqrt{\sum{x_i^2} + \epsilon}}


.. code-block:: proto

	message L2NormalizeLayerParams {

	    float epsilon = 1;

	}






FlattenLayerParams
________________________________________________________________________________

A layer that flattens the input.

.. code::

     y = FlattenLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank greater than equal to 3.
    e.g.: Rank 4 blob represents [Batch, C, H, W]
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.
Output
    Same rank as the input, such that last two dimensions are both 1.
    e.g.: For rank 4 input, output shape is ``[Batch, C * H * W, 1, 1]``

There are two X orders: ``CHANNEL_FIRST`` and ``CHANNEL_LAST``.
``CHANNEL_FIRST`` does not require data to be rearranged,
because row major ordering is used by internal storage.
``CHANNEL_LAST`` requires data to be rearranged.


.. code-block:: proto

	message FlattenLayerParams {

	    enum FlattenOrder {

	        CHANNEL_FIRST = 0;
	        CHANNEL_LAST = 1;

	    }
	    FlattenOrder mode = 1;

	}






ReshapeLayerParams
________________________________________________________________________________

A layer that recasts the input into a new shape.

.. code::

     y = ReshapeLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank 5.
    e.g.: ``[1, 1, C, H, W]`` or ``[Seq, 1, C, H, W]``.
Output
    A blob with rank 5.
    e.g.: ``[1, 1, C_out, H_out, W_out]`` or ``[Seq_out, 1, C_out, H_out, W_out]``.

There are two reshape orders: ``CHANNEL_FIRST`` and ``CHANNEL_LAST``.
``CHANNEL_FIRST`` is equivalent to
flattening the input to ``[Seq, 1, C * H * W, 1, 1]`` in channel first order
and then reshaping it to the target shape;
no data rearrangement is required.
``CHANNEL_LAST`` is equivalent to
flattening the input to ``[Seq, 1, H * W * C, 1, 1]`` in channel last order,
reshaping it to ``[Seq_out, 1, H_out, W_out, C_out]`` (it is now in "H_out-major"" order),
and then permuting it to ``[C_out, H_out, W_out]``;
both the flattening and permuting requires the data to be rearranged.


.. code-block:: proto

	message ReshapeLayerParams {

	    repeated int64 targetShape = 1;

	    enum ReshapeOrder {

	        CHANNEL_FIRST = 0;
	        CHANNEL_LAST = 1;

	    }
	    ReshapeOrder mode = 2;

	}






PermuteLayerParams
________________________________________________________________________________

A layer that rearranges the dimensions and data of an input.
For generic transpose/permute operation see TransposeLayer.

.. code::

     y = PermuteLayer(x)

Requires 1 input and produces 1 output.

Input
    Must be a rank 5 blob.
    e.g.: shape ``[Seq, B, C, H, W]``.
Output
    Rank 5 blob. Transposed version of the input, such that dimensions at axis=1 or axis=-4 is unchanged.


Examples:

 Assume input shape is [Seq, B, C, H, W]

- If ``axis`` is set to ``[0, 3, 1, 2]``,
  then the output has shape ``[Seq, B, W, C, H]``

- If ``axis`` is set to ``[3, 1, 2, 0]``,
  then the output has shape ``[W, B, C, H, Seq]``

- If ``axis`` is set to ``[0, 3, 2, 1]``,
  then the output has shape ``[Seq, B, W, H, C]``

- If ``axis`` is not set, or is set to ``[0, 1, 2, 3]``,
  the output is the same as the input.


.. code-block:: proto

	message PermuteLayerParams {

	    repeated uint64 axis = 1;

	}






ReorganizeDataLayerParams
________________________________________________________________________________

A layer that reorganizes data in the input in specific ways.

.. code::

     y = ReorganizeDataLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank at least 3.
    e.g.: blob with shape ``[C, H, W]``.
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.
Output
    Same rank as the input.
    e.g.: blob with shape ``[C_out, H_out, W_out]``.

mode == SPACE_TO_DEPTH
 ``[C_out, H_out, W_out]`` : ``[C * blockSize * blockSize, H/blockSize, W/blockSize]``.
 blockSize must divide H and W.
 Data is moved from the spatial dimensions to the channel dimension. Input is spatially divided into
 non-overlapping blocks of size blockSize X blockSize and data from each block is moved into the
 channel dimension.

mode == DEPTH_TO_SPACE
 ``[C_out, H_out, W_out]`` : ``[C/(blockSize * blockSize), H * blockSize, W * blockSize]``.
 Square of blockSize must divide C.
 Reverse of SPACE_TO_DEPTH. Data is moved from the channel dimension to the spatial dimensions.

mode == PIXEL_SHUFFLE
 ``[C_out, H_out, W_out]`` : ``[C/(blockSize * blockSize), H * blockSize, W *  blockSize]``.
 Square of blockSize must divide C.
 Similar to DEPTH_TO_SPACE, but using the pixel-shuffle semantics for channel order in the output space.
 In both modes, elements along the channel dimension are collapsed into
 blocks in the spatial dimensions. The difference is in the arrangement of
 the input-channels' data in the output space. See below example for more
 detail.
 (Only available in Core ML Specification >= 5 (iOS >= 14, macOS >= 11.0)


Examples:

Assume input is the following [C = 8, H = 1, W = 2] tensor:

.. code::

   [[[1 2]] [[3 4]] [[5 6]] [[7 8]] [[9 10]] [[11 12]] [[13 14]] [[15 16]]]

If block_size == 2 and mode == DEPTH_TO_SPACE, output will be the following
[C = 2, H = 2, W = 4] tensor:

.. code::

   [[[ 1  5  2  6]
     [ 9 13 10 14]]

    [[ 3  7  4  8]
     [11 15 12 16]]]

For mode == SPACE_TO_DEPTH, the behavior is the same as mode ==
DEPTH_TO_SPACE, but with the input and output swapped.

If block_size == 2 and mode == PIXEL_SHUFFLE, output will be the following
[C = 2, H = 2, W = 4] tensor:

.. code::

   [[[ 1  3  2  4]
     [ 5  7  6  8]]

    [[ 9 11 10 12]
     [13 15 14 16]]]


.. code-block:: proto

	message ReorganizeDataLayerParams {

	    enum ReorganizationType {

	        SPACE_TO_DEPTH = 0;
	        DEPTH_TO_SPACE = 1;
	        PIXEL_SHUFFLE = 2;

	    }
	    ReorganizationType mode = 1;
	    uint64 blockSize = 2;

	}






SliceLayerParams
________________________________________________________________________________

A layer that slices the input data along axis = -1 or -2 or -3.
For general slice along any axis, please see SliceStaticLayer/SliceDynamicLayer.

.. code::

     y = SliceLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob that can, in general, have any rank. However, depending on the value of "axis" ,
    there may be additional rank constraints.
Output
    A blob with the same rank as the input.

Sliced section is taken from the interval ``[startIndex, endIndex)``, i.e.
startIndex is inclusive while endIndex is exclusive.
stride must be positive and represents the step size for slicing.
Negative indexing is supported for startIndex and endIndex.
-1 denotes N-1, -2 denotes N-2 and so on, where N is the length of the dimension to be sliced.


.. code-block:: proto

	message SliceLayerParams {

	    int64 startIndex = 1;
	    int64 endIndex = 2;
	    uint64 stride = 3;

	    enum SliceAxis {

	        CHANNEL_AXIS = 0;
	        HEIGHT_AXIS = 1;
	        WIDTH_AXIS = 2;

	    }
	    // The following mapping is used for interpreting this parameter:
	    // CHANNEL_AXIS => axis = -3, input must have rank at least 3.
	    // HEIGHT_AXIS => axis = -2, input must have rank at least 2.
	    // WIDTH_AXIS => axis = -1
	    SliceAxis axis = 4;

	}






ReduceLayerParams
________________________________________________________________________________

A layer that reduces the input using a specified operation.

.. code::

     y = ReduceLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob that can, in general, have any rank. However, depending on the value of "axis" ,
     there may be additional rank constraints.
Output
    A blob with the same rank as the input, which has 1s on the dimensions specified in the parameter "axis"

    Values supported for axis are [-1], [-2], [-3], [-2,-1], [-3,-2,-1]
    and the equivalent positive values (depending on the rank of the input)
    For mode == 'ArgMax', axis must be [-1] or [-2] or [-3].


.. code-block:: proto

	message ReduceLayerParams {

	    enum ReduceOperation {

	        SUM = 0;
	        AVG = 1;
	        PROD = 2;
	        LOGSUM = 3;
	        SUMSQUARE = 4;
	        L1 = 5;
	        L2 = 6;
	        MAX = 7;
	        MIN = 8;
	        ARGMAX = 9;

	    }
	    ReduceOperation mode = 1;

	    float epsilon = 2;

	    enum ReduceAxis {

	        CHW = 0;
	        HW = 1;
	        C = 2;
	        H = 3;
	        W = 4;

	    }

	    // The following mapping is used for interpreting this parameter:
	    // CHW = axis [-3, -2, -1], input must have rank at least 3.
	    // HW = axis [-2, -1], input must have rank at least 2.
	    // C = axis [-3]
	    // H = axis [-2]
	    // W = axis [-1]
	    ReduceAxis axis = 3;

	}






CropLayerParams
________________________________________________________________________________

A layer that crops the spatial dimensions of an input.
If two inputs are provided, the shape of the second input is used as the reference shape.

.. code::

     y = CropLayer(x1) or y = CropLayer(x1,x2)

Requires 1 or 2 inputs and produces 1 output.

Input
   1 or 2 tensors, each with rank at least 3, both inputs must have equal rank.
   Example:
    - 1 input case: A blob with shape ``[C, H_in, W_in]``.
    - 2 input case: 1st blob with shape ``[C, H_in, W_in]``, 2nd blob with shape ``[C, H_out, W_out]``.

    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.

Output
    Same rank as the inputs.
    e.g.: A blob with shape ``[C, H_out, W_out]``.

If one input is used, output is computed as follows:

.. code::

     y = x1[:, topCropAmount:H_in - bottomCropAmount, leftCropAmount:W_in - rightCropAmount]

     topCropAmount == Height startEdgeSize == borderAmounts[0].startEdgeSize
     bottomCropAmount == Height endEdgeSize == borderAmounts[0].endEdgeSize
     leftCropAmount == Width startEdgeSize == borderAmounts[1].startEdgeSize
     rightCropAmount == Width endEdgeSize == borderAmounts[1].endEdgeSize

     H_out = H_in - topCropAmount - bottomCropAmount
     W_out = W_in - leftCropAmount - rightCropAmount

If two inputs are used, output is computed as follows:

.. code::

     y = x1[:, offset[0]:offset[0] + H_out, offset[1]:offset[1] + W_out]


.. code-block:: proto

	message CropLayerParams {

	    BorderAmounts cropAmounts = 1;

	    repeated uint64 offset = 5;

	}






AverageLayerParams
________________________________________________________________________________

A layer that computes the elementwise average of the inputs.
This layer has limited broadcasting support. For general broadcasting see AddBroadcastableLayer.

.. code::

     y = AverageLayer(x1,x2,...)

Requires multiple inputs and produces 1 output.

Input
    In general, there are no rank constraints.
    However, only certain set of shapes are broadcastable. For example:
    [B, 1, 1, 1], [B, C, 1, 1], [B, 1, H, W], [B, C, H, W]
Output
    A blob with the same shape as each input.


.. code-block:: proto

	message AverageLayerParams {

	}






MaxLayerParams
________________________________________________________________________________

A layer that computes the elementwise maximum over the inputs.

.. code::

     y = MaxLayer(x1,x2,...)

Requires multiple inputs and produces 1 output.

Input
    In general, there are no rank constraints.
    However, only certain set of shapes are broadcastable. For example:
    [B, C, 1, 1], [B, C, H, W]
Output
    A blob with the same shape as each input.


.. code-block:: proto

	message MaxLayerParams {

	}






MinLayerParams
________________________________________________________________________________

A layer that computes the elementwise minimum over the inputs.

.. code::

     y = MinLayer(x1,x2,...)

Requires multiple inputs and produces 1 output.

Input
    In general, there are no rank constraints.
    However, only certain set of shapes are broadcastable. For example:
    [B, C, 1, 1], [B, C, H, W]
Output
    A blob with the same shape as each input.


.. code-block:: proto

	message MinLayerParams {

	}






DotProductLayerParams
________________________________________________________________________________

A layer that computes the dot product of two vectors.

.. code::

     y = DotProductLayer(x1,x2)

Requires 2 inputs and produces 1 output.

Input
    Two blobs with rank at least 3, such that the last two dimensions must be 1.
    e.g.: blobs with shape ``[B, C, 1, 1]``.
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.

Output
    Same rank as the input.
    e.g. for rank 4 inputs, output shape: [B, 1, 1, 1]


.. code-block:: proto

	message DotProductLayerParams {

	    bool cosineSimilarity = 1;

	}






MeanVarianceNormalizeLayerParams
________________________________________________________________________________

A layer that performs mean variance normalization, along axis = -3.

.. code::

     y = MeanVarianceNormalizeLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank greater than equal to 3.
    Example: Rank 4 blob represents [Batch, channels, height, width]
    For ranks greater than 3, the leading dimensions, starting from 0 to -4 (inclusive), are all treated as batch.

Output
    A blob with the same shape as the input.

If ``acrossChannels == true``
normalization is performed on flattened input, i.e. the input is reshaped to (Batch,C), where "Batch" contains
all dimensions from 0 to -4 (inclusive), and C contains dimensions -1, -2, -3.

If ``acrossChannels == false``
normalization is performed within a channel,
across spatial dimensions (i.e. last two dimensions).


.. code-block:: proto

	message MeanVarianceNormalizeLayerParams {

	    bool acrossChannels = 1;

	    bool normalizeVariance = 2;

	    float epsilon = 3;

	}






SequenceRepeatLayerParams
________________________________________________________________________________

A layer that repeats a sequence or the dimension sitting at axis = -5

.. code::

     y = SequenceRepeatLayer(x)

Requires 1 input and produces 1 output.

Input
    A blob with rank at least 5.
    e.g: shape ``[Seq, B, C, H, W]``
Output
    A blob with the same rank as the input.
    e.g.: for input shape ``[Seq, B, C, H, W]``, output shape is ``[nRepetitions * Seq, B, C, H, W]``.


.. code-block:: proto

	message SequenceRepeatLayerParams {

	    uint64 nRepetitions = 1;

	}






SimpleRecurrentLayerParams
________________________________________________________________________________

A simple recurrent layer.

.. code::

     y_t = SimpleRecurrentLayer(x_t, y_{t-1})

Input
   A blob of rank 5, with shape `[Seq, Batch, inputVectorSize, 1, 1]``.
   This represents a sequence of vectors of size ``inputVectorSize``.
Output
   Same rank as the input.
   Represents a vector of size ``outputVectorSize``. It is either the final output or a sequence of outputs at all time steps.

- Output Shape: ``[1, Batch, outputVectorSize, 1, 1]`` , if ``sequenceOutput == false``
- Output Shape: ``[Seq, Batch, outputVectorSize, 1, 1]`` , if ``sequenceOutput == true``

This layer is described by the following equation:

.. math::
    \boldsymbol{y_t} = f(\mathrm{clip}(W \boldsymbol{x_t} + \
                                       R \boldsymbol{y_{t-1}} + b))

- ``W`` is a 2-dimensional weight matrix
  (``[outputVectorSize, inputVectorSize]``, row-major)
- ``R`` is a 2-dimensional recursion matrix
  (``[outputVectorSize, outputVectorSize]``, row-major)
- ``b`` is a 1-dimensional bias vector (``[outputVectorSize]``)
- ``f()`` is an activation
- ``clip()`` is a function that constrains values between ``[-50.0, 50.0]``


.. code-block:: proto

	message SimpleRecurrentLayerParams {

	    uint64 inputVectorSize = 1;
	    uint64 outputVectorSize = 2;

	    ActivationParams activation = 10;

	        If false output is just the result after final state update.
	        If true, output is a sequence, containing outputs at all time steps.
	    bool sequenceOutput = 15;

	    bool hasBiasVector = 20;

	    WeightParams weightMatrix = 30;
	    WeightParams recursionMatrix = 31;
	    WeightParams biasVector = 32;

	    bool reverseInput = 100;
	    // If true, then the node processes the input sequence from right to left

	}






GRULayerParams
________________________________________________________________________________

Gated-Recurrent Unit (GRU) Layer

.. code::

     y_t = GRULayer(x_t, y_{t-1})

Input
   A blob of rank 5, with shape `[Seq, Batch, inputVectorSize, 1, 1]``.
   This represents a sequence of vectors of size ``inputVectorSize``.
Output
   Same rank as the input.
   Represents a vector of size ``outputVectorSize``. It is either the final output or a sequence of outputs at all time steps.

- Output Shape: ``[1, Batch, outputVectorSize, 1, 1]`` , if ``sequenceOutput == false``
- Output Shape: ``[Seq, Batch, outputVectorSize, 1, 1]`` , if ``sequenceOutput == true``

This layer is described by the following equations:

Update Gate
    .. math::
        \boldsymbol{z_t} = \
            f(\mathrm{clip}(W_z \boldsymbol{x_t} + \
                            R_z \boldsymbol{y_{t-1}} + b_z)

Reset Gate
    .. math::
        \boldsymbol{r_t} = \
            f(\mathrm{clip}(W_r \boldsymbol{x_t} + \
                            R_r \boldsymbol{y_{t-1}} + b_r))

Cell Memory State
    .. math::
        \boldsymbol{c_t} = \
            \boldsymbol{y_{t-1}} \odot \boldsymbol{r_t}

Output Gate
    .. math::
        \boldsymbol{o_t} = \
            g(\mathrm{clip}(W_o \boldsymbol{x_t} + \
                            R_o \boldsymbol{c_t} + b_o))

Output
    .. math::
        \boldsymbol{y_t} = \
            (1 - \boldsymbol{z_t}) \odot \boldsymbol{o_t} + \
             \boldsymbol{z_t} \odot \boldsymbol{y_{t-1}}

- ``W_z``, ``W_r``, ``W_o`` are 2-dimensional input weight matrices
  (``[outputVectorSize, inputVectorSize]``, row-major)
- ``R_z``, ``R_r``, ``R_o`` are 2-dimensional recursion matrices
  (``[outputVectorSize, outputVectorSize]``, row-major)
- ``b_z``, ``b_r``, ``b_o`` are 1-dimensional bias vectors
  (``[outputVectorSize]``)
- ``f()``, ``g()`` are activations
- ``clip()`` is a function that constrains values between ``[-50.0, 50.0]``
- ``⊙`` denotes the elementwise product of matrices


.. code-block:: proto

	message GRULayerParams {

	    uint64 inputVectorSize = 1;
	    uint64 outputVectorSize = 2;

	    repeated ActivationParams activations = 10;

	    bool sequenceOutput = 15;

	    bool hasBiasVectors = 20;

	    WeightParams updateGateWeightMatrix = 30;
	    WeightParams resetGateWeightMatrix = 31;
	    WeightParams outputGateWeightMatrix = 32;

	    WeightParams updateGateRecursionMatrix = 50;
	    WeightParams resetGateRecursionMatrix = 51;
	    WeightParams outputGateRecursionMatrix = 52;

	    WeightParams updateGateBiasVector = 70;
	    WeightParams resetGateBiasVector = 71;
	    WeightParams outputGateBiasVector = 72;

	    bool reverseInput = 100;

	}






LSTMParams
________________________________________________________________________________

Long short-term memory (LSTM) parameters.

This is described by the following equations:

Input Gate
    .. math::
        \boldsymbol{i_t} = \
            f(\mathrm{clip}(W_i \boldsymbol{x_t} + \
                            R_i \boldsymbol{y_{t-1}} + \
                            p_i \odot c_{t-1} + b_i))

Forget Gate
    .. math::
        \boldsymbol{f_t} = \
            f(\mathrm{clip}(W_f \boldsymbol{x_t} + \
                            R_f \boldsymbol{y_{t-1}} + \
                            p_f \odot c_{t-1} + b_f))

Block Input
    .. math::
        \boldsymbol{z_t} = \
            g(\mathrm{clip}(W_z \boldsymbol{x_t} + \
                            R_z \boldsymbol{y_{t-1}} + b_z))

Cell Memory State
    .. math::
        \boldsymbol{c_t} = \
            \boldsymbol{c_{t-1}} \odot \boldsymbol{f_t} + \
            \boldsymbol{i_t} \odot \boldsymbol{z_t}

Output Gate
    .. math::
        \boldsymbol{o_t} = \
            f(\mathrm{clip}(W_o \boldsymbol{x_t} + \
                            R_o \boldsymbol{y_{t-1}} + \
                            p_o \odot c_t + b_o))

Output
    .. math::
        \boldsymbol{y_t} = \
            h(\boldsymbol{c_t}) \odot \boldsymbol{o_t}

- ``W_i``, ``W_f``, ``W_z``, ``W_o`` are 2-dimensional input weight matrices
  (``[outputVectorSize, inputVectorSize]``, row-major)
- ``R_i``, ``R_f``, ``R_z``, ``R_o`` are 2-dimensional recursion matrices
  (``[outputVectorSize, outputVectorSize]``, row-major)
- ``b_i``, ``b_f``, ``b_z``, ``b_o`` are 1-dimensional bias vectors
  (``[outputVectorSize]``)
- ``p_``, ``p_f``, ``p_o`` are 1-dimensional peephole vectors
  (``[outputVectorSize]``)
- ``f()``, ``g()``, ``h()`` are activations
- ``clip()`` is a function that constrains values between ``[-50.0, 50.0]``
- ``⊙`` denotes the elementwise product of matrices


.. code-block:: proto

	message LSTMParams {

	    bool sequenceOutput = 10;

	    bool hasBiasVectors = 20;

	    bool forgetBias = 30;

	    bool hasPeepholeVectors = 40;

	    bool coupledInputAndForgetGate = 50;

	    float cellClipThreshold = 60;

	}






LSTMWeightParams
________________________________________________________________________________

Weights for long short-term memory (LSTM) layers


.. code-block:: proto

	message LSTMWeightParams {

	    WeightParams inputGateWeightMatrix = 1;
	    WeightParams forgetGateWeightMatrix = 2;
	    WeightParams blockInputWeightMatrix = 3;
	    WeightParams outputGateWeightMatrix = 4;

	    WeightParams inputGateRecursionMatrix = 20;
	    WeightParams forgetGateRecursionMatrix = 21;
	    WeightParams blockInputRecursionMatrix = 22;
	    WeightParams outputGateRecursionMatrix = 23;

	    //biases:
	    WeightParams inputGateBiasVector = 40;
	    WeightParams forgetGateBiasVector = 41;
	    WeightParams blockInputBiasVector = 42;
	    WeightParams outputGateBiasVector = 43;

	    //peepholes:
	    WeightParams inputGatePeepholeVector = 60;
	    WeightParams forgetGatePeepholeVector = 61;
	    WeightParams outputGatePeepholeVector = 62;

	}






UniDirectionalLSTMLayerParams
________________________________________________________________________________

A unidirectional long short-term memory (LSTM) layer.

.. code::

     (y_t, c_t) = UniDirectionalLSTMLayer(x_t, y_{t-1}, c_{t-1})

Input
   A blob of rank 5, with shape `[Seq, Batch, inputVectorSize, 1, 1]``.
   This represents a sequence of vectors of size ``inputVectorSize``.
Output
   Same rank as the input.
   Represents a vector of size ``outputVectorSize``. It is either the final output or a sequence of outputs at all time steps.

- Output Shape: ``[1, Batch, outputVectorSize, 1, 1]`` , if ``sequenceOutput == false``
- Output Shape: ``[Seq, Batch, outputVectorSize, 1, 1]`` , if ``sequenceOutput == true``


.. code-block:: proto

	message UniDirectionalLSTMLayerParams {

	    uint64 inputVectorSize = 1;
	    uint64 outputVectorSize = 2;

	    repeated ActivationParams activations = 10;

	    LSTMParams params = 15;

	    LSTMWeightParams weightParams = 20;

	    bool reverseInput = 100;

	}






BiDirectionalLSTMLayerParams
________________________________________________________________________________

Bidirectional long short-term memory (LSTM) layer

.. code::

     (y_t, c_t, y_t_reverse, c_t_reverse) = BiDirectionalLSTMLayer(x_t, y_{t-1}, c_{t-1}, y_{t-1}_reverse, c_{t-1}_reverse)

Input
   A blob of rank 5, with shape `[Seq, Batch, inputVectorSize, 1, 1]``.
   This represents a sequence of vectors of size ``inputVectorSize``.
Output
   Same rank as the input.
   Represents a vector of size ``2 * outputVectorSize``. It is either the final output or a sequence of outputs at all time steps.

- Output Shape: ``[1, Batch, 2 * outputVectorSize, 1, 1]`` , if ``sequenceOutput == false``
- Output Shape: ``[Seq, Batch, 2 * outputVectorSize, 1, 1]`` , if ``sequenceOutput == true``


The first LSTM operates on the input sequence in the forward direction.
The second LSTM operates on the input sequence in the reverse direction.

Example: given the input sequence ``[x_1, x_2, x_3]``,
where ``x_i`` are vectors at time index ``i``:

The forward LSTM output is ``[yf_1, yf_2, yf_3]``,

where ``yf_i`` are vectors of size ``outputVectorSize``:

- ``yf_1`` is the output at the end of sequence {``x_1``}
- ``yf_2`` is the output at the end of sequence {``x_1``, ``x_2``}
- ``yf_3`` is the output at the end of sequence {``x_1``, ``x_2``, ``x_3``}

The backward LSTM output: ``[yb_1, yb_2, yb_3]``,

where ``yb_i`` are vectors of size ``outputVectorSize``:

- ``yb_1`` is the output at the end of sequence {``x_3``}
- ``yb_2`` is the output at the end of sequence {``x_3``, ``x_2``}
- ``yb_3`` is the output at the end of sequence {``x_3``, ``x_2``, ``x_1``}

Output of the bi-dir layer:

- if ``sequenceOutput = True`` : { ``[yf_1, yb_3]``,  ``[yf_2, yb_2]``,  ``[yf_3, yb_1]`` }
- if ``sequenceOutput = False`` : { ``[yf_3, yb_3]`` }


.. code-block:: proto

	message BiDirectionalLSTMLayerParams {

	    uint64 inputVectorSize = 1;
	    uint64 outputVectorSize = 2;

	    repeated ActivationParams activationsForwardLSTM = 10;
	    repeated ActivationParams activationsBackwardLSTM = 11;

	    LSTMParams params = 15;

	    repeated LSTMWeightParams weightParams = 20;

	}






CustomLayerParams
________________________________________________________________________________




.. code-block:: proto

	message CustomLayerParams {

	    message CustomLayerParamValue {
	        oneof value {
	            double doubleValue = 10;
	            string stringValue = 20;
	            int32 intValue = 30;
	            int64 longValue = 40;
	            bool boolValue = 50;
	        }
	    }

	    string className = 10; // The name of the class (conforming to MLCustomLayer) corresponding to this layer
	    repeated WeightParams weights = 20; // Any weights -- these are serialized in binary format and memmapped at runtime
	    map<string, CustomLayerParamValue> parameters = 30; // these may be handled as strings, so this should not be large
	    string description = 40; // An (optional) description of the layer provided by the model creator. This information is displayed when viewing the model, but does not affect the model's execution on device.

	}






CustomLayerParams.CustomLayerParamValue
--------------------------------------------------------------------------------




.. code-block:: proto

	    message CustomLayerParamValue {
	        oneof value {
	            double doubleValue = 10;
	            string stringValue = 20;
	            int32 intValue = 30;
	            int64 longValue = 40;
	            bool boolValue = 50;
	        }
	    }






CustomLayerParams.ParametersEntry
--------------------------------------------------------------------------------




.. code-block:: proto

	    message CustomLayerParamValue {
	        oneof value {
	            double doubleValue = 10;
	            string stringValue = 20;
	            int32 intValue = 30;
	            int64 longValue = 40;
	            bool boolValue = 50;
	        }
	    }






TransposeLayerParams
________________________________________________________________________________




.. code-block:: proto

	message TransposeLayerParams {

	    repeated uint64 axes = 1; //

	}






BatchedMatMulLayerParams
________________________________________________________________________________

A layer that computes the matrix multiplication of two tensors with numpy-like broadcasting
where the matrices reside in the last two indices of the tensor.

.. code::

     y = BatchedMatMul(a,b)

Requires 1 or 2 inputs and produces 1 output.

The first tensor, "a", must be provided as an input. The second tensor can either be an input or provided as a weight matrix parameter.

Input
    - a: First N-Dimensional tensor
    - b: Second N-Dimensional tensor (either a rank-N input or a matrix, i.e. N=2, provided as a layer parameter)

Output
    A tensor containing the matrix product of two tensors.
    When there are two inputs: rank is max(2, rank(a), rank(b))
    When there is one input: rank is same as that of the input.

This operation behaves as following:

 When there are two inputs:
     - If N >= 2 for both tensors, it is treated as a batch of matrices residing in the last two indices.
       All the indices, except for the last two, are broadcasted using conventional rules.
     - If the first tensor is 1-D, it is converted to a 2-D tensor by prepending a 1 to its shape. Eg. (D) -> (1,D)
     - If the second tensor is 1-D, it is converted to a 2-D tensor by appending a 1 to its shape. Eg. (D) -> (D,1)

 When there is one input:
     - The weight matrix corresponds to a matrix, of shape (X1, X2). Values of X1, X2 must be provided as layer parameters.
     - The input, "a", is reshaped into a matrix by combining all the leading dimensions, except the last, into a batch dimension. eg:
            - if "a" is rank 1 (X1,) -->  (1, X1). Output shape will be (X2,)
            - if "a" is rank 2 (B1, X1) --> no need to reshape. Output shape will be (B1, X2)
            - if "a" is rank 3 (B1, B2, X1) --> (B1 * B2, X1). Output shape will be (B1, B2, X2)
            - etc


.. code-block:: proto

	message BatchedMatMulLayerParams {

	    bool transposeA = 1;
	    bool transposeB = 2;


	    uint64 weightMatrixFirstDimension = 5;
	    uint64 weightMatrixSecondDimension = 6;

	    bool hasBias = 7;

	    WeightParams weights = 8;
	    WeightParams bias = 9;

	    bool int8DynamicQuantize = 10;

	}






ConcatNDLayerParams
________________________________________________________________________________

A layer that concatenates a list of tensors along a specified axis.

.. code::

     y = ConcatNDLayer(x1,x2,....)

Requires at least 2 input and produces 1 output.

Input
    A Sequence of N-dimensional tensors. The rank of the input tensors must match and all dimensions except 'axis' must be equal.
Output
    A N-Dimensional tensor with the same rank .


.. code-block:: proto

	message ConcatNDLayerParams {

	    int64 axis = 1;

	}






SoftmaxNDLayerParams
________________________________________________________________________________

A layer that performs softmax normalization along a specified axis.

.. code::

     y = SoftmaxNDLayer(x)

Requires 1 input and produces 1 output.

Output shape is same as the input.


.. code-block:: proto

	message SoftmaxNDLayerParams {

	    int64 axis = 1;

	}






ReverseLayerParams
________________________________________________________________________________

A layer that reverses specific dimensions of the input tensor.
It is similar in functionality to the numpy.flip method.

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message ReverseLayerParams {

	    repeated bool reverseDim = 1;

	}






ReverseSeqLayerParams
________________________________________________________________________________

A layer that reverses variable length slices.

Requires 2 inputs and produces 1 output.

2 inputs, in order are denoted by "data", "seq_lengths".
"seq_lenghts" must be a rank 1 tensor, i.e. seq_lengths.shape = (B,)
which contains the lengths of the amount of sequence to be reversed, for each element of the batch.
Dimension "batchAxis" in "data" must be equal to B, i.e,
data.shape[batchAxis] = B.

According to the batch axis, input "data" is first divided into a batch of B inputs,
each of which is flipped along the dimension "sequenceAxis", by the amount specified in
"seq_lengths", the second input.

e.g.:

data [shape = (2,4)]:
[0 1 2 3]
[4 5 6 7]
seq_lengths [shape = (2,)]:
[3, 0]
batchAxis = 0
sequenceAxis = 1

output [shape = (2,4)]:
[2 1 0 3]
[4 5 6 7]


data [shape = (2,3,2)]:
[0 1]
[2 3]
[4 5] (slice = 0)
[6 7]
[8 9]
[10 11] (slice = 1)
seq_lengths [shape = (2,)]:
[2, 3]
batchAxis = 0
sequenceAxis = 1

output [shape = (2,3,2)]:
[2 3]
[0 1]
[4 5] (slice = 0)
[10 11]
[8 9]
[6 7] (slice = 1)

Output shape is same as the input.


.. code-block:: proto

	message ReverseSeqLayerParams {

	    int64 batchAxis = 1; // batch axis has to be strictly less than seq_axis
	    int64 sequenceAxis = 2;

	}






LoadConstantNDLayerParams
________________________________________________________________________________

A layer that loads data as a parameter and provides it as an output.

.. code::

     y = LoadConstantNDLayer()

Requires no input and produces 1 output.

Output: A tensor with shape as provided in the parameter "shape"


.. code-block:: proto

	message LoadConstantNDLayerParams {

	    repeated uint64 shape = 1;
	    WeightParams data = 2;

	}






FillLikeLayerParams
________________________________________________________________________________

A layer that generates an output tensor with a constant value.
Input is only used to determine the shape of the output.
This layer is used to allocate a tensor with a dynamic shape (that of the input) and constant value.

Requires 1 input and produces 1 output.

.. code::

     y = FillLikeLayer(x)

Input
    A N-Dimensional tensor, whose values are ignored. Only the shape is used to
    infer the shape of the output.

Output
    A N-Dimensional tensor with the same shape as the input tensor.


.. code-block:: proto

	message FillLikeLayerParams {

	    float value = 1;

	}






FillStaticLayerParams
________________________________________________________________________________

A layer that generates an output tensor with a constant value.
This layer is used to allocate a tensor with a static shape and constant value.

Requires no input and produces 1 output.

.. code::

     y = FillStaticLayer(x)

Output
    A N-Dimensional tensor of shape "targetShape".


.. code-block:: proto

	message FillStaticLayerParams {

	    float value = 1;
	    repeated uint64 targetShape = 2;

	}






FillDynamicLayerParams
________________________________________________________________________________

A layer that generates an output tensor with a constant value.
This layer is used to allocate a tensor with a dynamic shape (as specified by the input) and constant value.

Requires 1 input and produces 1 output.

.. code::

     y = FillDynamicLayer(x)

Input
    A rank 1 tensor specifying the shape of the output

Output
    An N-Dimensional tensor with the shape specified by the values in the input tensor.


.. code-block:: proto

	message FillDynamicLayerParams {

	    float value = 1;

	}






WhereBroadcastableLayerParams
________________________________________________________________________________

A layer that returns the elements either from tensor x or tensor y,
depending on the value in the condition tensor.
It is similar in functionality to the numpy.where method with 3 inputs.

Requires 3 inputs and produces 1 output.
Inputs, in order, are the condition tensor, x and y.

for each vector index (i,...,j):
   output[i,...,j] = x[i,...,j] if condition[i,...,j] = True
                     y[i,...,j] if condition[i,...,j] = False

All the 3 inputs are first broadcasted to a common shape.
(the shapes must be broadcastable)

output.rank = max(input[0].rank, input[1].rank, input[2].rank)


.. code-block:: proto

	message WhereBroadcastableLayerParams {

	}






SinLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric sine function.


.. code::

     y = SinLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message SinLayerParams {

	}






CosLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric cosine function.


.. code::

     y = CosLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message CosLayerParams {

	}






TanLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric tangent function.


.. code::

     y = TanLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message TanLayerParams {

	}






AsinLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric arcsine function.


.. code::

     y = AsinLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message AsinLayerParams {

	}






AcosLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric arccosine function.


.. code::

     y = AcosLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message AcosLayerParams {

	}






AtanLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric arctangent function.


.. code::

     y = AtanLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message AtanLayerParams {

	}






SinhLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric hyperbolic sine function.


.. code::

     y = SinhLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message SinhLayerParams {

	}






CoshLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric hyperbolic cosine function.


.. code::

     y = CoshLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message CoshLayerParams {

	}






TanhLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric hyperbolic tangent function.


.. code::

     y = TanhLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message TanhLayerParams {

	}






AsinhLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric hyperbolic arcsine function.


.. code::

     y = AsinhLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message AsinhLayerParams {

	}






AcoshLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric hyperbolic arccosine function.


.. code::

     y = AcoshLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message AcoshLayerParams {

	}






AtanhLayerParams
________________________________________________________________________________

A layer that computes elementwise trigonometric hyperbolic arctangent function.


.. code::

     y = AtanhLayer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message AtanhLayerParams {

	}






PowBroadcastableLayerParams
________________________________________________________________________________

A layer that raises each element in first tensor to the power of
corresponding element in the second tensor.
Supports conventional numpy-like broadcasting.

.. code::

     y = PowBroadcastableLayer(x)

Requires 2 inputs and produces 1 output.

Input
    - First N-Dimensional tensor
    - Second N-Dimensional tensor

Output
    An N-Dimensional tensor with the broadcast shape.


.. code-block:: proto

	message PowBroadcastableLayerParams {

	}






Exp2LayerParams
________________________________________________________________________________

A layer that computes the exponential of all elements in the input tensor, with the base 2.


.. code::

     y = Exp2Layer(x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message Exp2LayerParams {

	}






WhereNonZeroLayerParams
________________________________________________________________________________

A layer that returns a tensor containing the indices of all non-zero
elements of input tensor.
It is similar in functionality to the numpy.where method with 1 input.

Requires 1 input and produces 1 output.
Output is of rank 2, of shape (N,R),
where N is the number of non-zero elements in the input and R is the rank of the input.

Output contains indices represented in the multi-index form

e.g.:
input {shape = (4,)}:
[0 1 0 2]
output {shape = (2,1)}:
[1]
[3]


input {shape = (3, 3)}:
[1 2 1]
[0 2 2]
[2 1 0]
output {shape = (7,1)}:
[0. 0.]
[0. 1.]
[0. 2.]
[1. 1.]
[1. 2.]
[2. 0.]
[2. 1.]


.. code-block:: proto

	message WhereNonZeroLayerParams {

	}






MatrixBandPartLayerParams
________________________________________________________________________________

A layer that copies a tensor setting everything outside a central band in
each inner-most matrix to zero.

Requires 1 input and produces 1 output.

Parameters for matrix_band_part layer
band(m, n) = (num_lower < 0 || (m-n) <= num_lower) && (num_upper < 0 || (n-m) <= num_upper).
output[i, j, k, ..., m, n] = band(m, n) * input[i, j, k, ..., m, n]


Output shape is same as the input shape.
Rank of the input must be at least 2.
For rank higher than 2, the last 2 dimensions are treated as the matrix, while the rest are treated as batch.


.. code-block:: proto

	message MatrixBandPartLayerParams {

	    int64 numLower = 1;
	    int64 numUpper = 2;

	}






UpperTriangularLayerParams
________________________________________________________________________________

A layer that copies a tensor setting everything outside upper triangular to zero.

Requires 1 input and produces 1 output.

Output shape is same as the input shape.
Rank of the input must be at least 2.
For rank higher than 2, the last 2 dimensions are treated as the matrix, while the rest are treated as batch.


.. code-block:: proto

	message UpperTriangularLayerParams {

	    int64 k = 1; // Diagonal below which to zero elements. k = 0 (the default) is the main diagonal, k < 0 is below it and k > 0 is above

	}






LowerTriangularLayerParams
________________________________________________________________________________

A layer that copies a tensor setting everything outside lower triangular to zero.

Requires 1 input and produces 1 output.

Output shape is same as the input shape.
Rank of the input must be at least 2.
For rank higher than 2, the last 2 dimensions are treated as the matrix, while the rest are treated as batch.


.. code-block:: proto

	message LowerTriangularLayerParams {

	    int64 k = 1; // Diagonal above which to zero elements. k = 0 (the default) is the main diagonal, k < 0 is below it and k > 0 is above

	}






BroadcastToLikeLayerParams
________________________________________________________________________________

A layer that broadcasts a tensor to a new shape.

Requires 2 inputs and produces 1 output.

First input is broadcast to produce the output, while the second input is only
used to determine the shape of the output. Values of second input are not used.

Output is a tensor with the same shape as the second input.


.. code-block:: proto

	message BroadcastToLikeLayerParams {

	}






BroadcastToStaticLayerParams
________________________________________________________________________________

A layer that broadcasts a tensor to a new shape.

Requires 1 input and produces 1 output.

Output tensor is the broadcasted version of the input and has shape as specified in the
parameter "targetShape".


.. code-block:: proto

	message BroadcastToStaticLayerParams {

	    repeated uint64 targetShape = 1;

	}






BroadcastToDynamicLayerParams
________________________________________________________________________________

A layer that broadcasts a tensor to a new shape.

Requires 2 inputs and produces 1 output.

First input is the one that is broadcasted to produce the output.
Second input is a rank 1 tensor specifying the shape of the output.
Output tensor has shape as specified by the values in the 2nd input tensor.


.. code-block:: proto

	message BroadcastToDynamicLayerParams {

	}






AddBroadcastableLayerParams
________________________________________________________________________________

A layer that performs element-wise addition operation with broadcast support.

Requires 2 inputs and produces 1 output.


.. code-block:: proto

	message AddBroadcastableLayerParams {

	}






MaxBroadcastableLayerParams
________________________________________________________________________________

A layer that performs element-wise maximum operation with broadcast support.

Requires 2 inputs and produces 1 output.


.. code-block:: proto

	message MaxBroadcastableLayerParams {

	}






MinBroadcastableLayerParams
________________________________________________________________________________

A layer that performs element-wise minimum operation with broadcast support.

Requires 2 inputs and produces 1 output.


.. code-block:: proto

	message MinBroadcastableLayerParams {

	}






ModBroadcastableLayerParams
________________________________________________________________________________

A layer that performs element-wise modular operation with broadcast support.

Requires 2 inputs and produces 1 output.


.. code-block:: proto

	message ModBroadcastableLayerParams {

	}






FloorDivBroadcastableLayerParams
________________________________________________________________________________

A layer that performs element-wise floor division operation with broadcast support.

Requires 2 inputs and produces 1 output.


.. code-block:: proto

	message FloorDivBroadcastableLayerParams {

	}






SubtractBroadcastableLayerParams
________________________________________________________________________________

A layer that performs element-wise subtract operation with broadcast support.

Requires 2 inputs and produces 1 output.


.. code-block:: proto

	message SubtractBroadcastableLayerParams {

	}






MultiplyBroadcastableLayerParams
________________________________________________________________________________

A layer that performs element-wise multiply operation with broadcast support.

Requires 2 inputs and produces 1 output.


.. code-block:: proto

	message MultiplyBroadcastableLayerParams {

	}






DivideBroadcastableLayerParams
________________________________________________________________________________

A layer that performs element-wise division operation with broadcast support.

Requires 2 inputs and produces 1 output.


.. code-block:: proto

	message DivideBroadcastableLayerParams {

	}






GatherLayerParams
________________________________________________________________________________

Gather layer that gathers elements from the first input, along a specified axis,
at indices specified in the second input.
It is similar in functionality to the numpy.take method.

Requires 2 inputs and produces 1 output.

Given two inputs, 'data' and 'indices', gather the slices of 'data'
and store into output.
e.g.
for i in [0, length(indices) - 1]
   output[i] = data[indices[i]]  (1-D case, axis=0)

if axis = 0:
for each vector index (i,...,j)
   output[i,...,j,:,..,:] = data[indices[i,...,j],:,..,:]

output.rank = (data.rank - 1) + indices.rank

Negative indices and negative axis are supported.

e.g:

data shape = (2, 3)
indices shape = (6, 8)
axis = 0
output shape = (6, 8) + (3,) = (6, 8, 3)

data shape = (2, 3, 5)
indices shape = (6, 8)
axis = 1
output shape = (2,) + (6, 8) + (5,) =  (2, 6, 8, 5)


.. code-block:: proto

	message GatherLayerParams {

	    int64 axis = 1;

	}






ScatterLayerParams
________________________________________________________________________________




.. code-block:: proto

	message ScatterLayerParams {

	    int64 axis = 1;
	    ScatterMode mode = 2;

	}






GatherNDLayerParams
________________________________________________________________________________

A layer that gathers elements from the first input, 'params', at the multi-indices specified
by the second input, 'indices'.

Requires 2 inputs and produces 1 output.

'params' = input[0], 'indices' = input[1]

'indices' is a rank K+1 tensor of shape [I_0, I_1, .., I_(K-1), I_K] which is viewed as a collection of
indices of (I_0 * I_1 * ... * I_(K-1)) points in the I_K dimensional space. For instance, the multi-index of the first point
is indices[0,0,...,0,:].

Here is how the output is constructed:

for i = 0,1,...,(I_0-1)
  ...
    for j = 0,1,....,(I_(K-1)-1)
         output[i,....,j,:,:,..,:] = params[indices[i,...,j,:], :,:,..,:]

Hence, output shape is [I_0, I_1,...,I(K-1)] + params.shape[I_K:]

output.rank = indices.rank - 1 + params.rank - indices.shape[-1]

e.g:

input[0] shape = (4, 2, 3, 4)
input[1] shape = (6, 2)
output shape = (6,) + (3, 4) = (6, 3, 4)

input[0] shape = (3, 3, 3, 4, 7)
input[1] shape = (3, 5)
output shape = (3,) + () = (3,)

input[0] shape = (5, 3, 2, 5)
input[1] shape = (2, 7, 3, 2)
output shape = (2, 7, 3) + (2, 5) = (2, 7, 3, 2, 5)


.. code-block:: proto

	message GatherNDLayerParams {

	}






ScatterNDLayerParams
________________________________________________________________________________




.. code-block:: proto

	message ScatterNDLayerParams {

	    ScatterMode mode = 1;

	}






GatherAlongAxisLayerParams
________________________________________________________________________________

Gather layer that gathers elements from the first input, along a specified axis,
at indices specified in the second input.
It is similar in functionality to the numpy.take_along_axis method.

Requires 2 inputs and produces 1 output.

Given two inputs, 'data' and 'indices', gather the slices of 'data'
and store into output.

Both inputs and output have the same rank.
Output shape is same as the shape of 'indices'
Shapes of 'indices' and 'data' match, except at the 'axis' dimension.

This operation performs the following operation for axis=0:
for each vector index (i,j,....,k)
   output[i,j,....,k] = data[index[i,j,....,k],j,....,k]

Negative indices and negative axis are supported.

e.g:

data shape = (4, 4, 7)
indices shape = (4, 5, 7)
axis = 1
output shape = (4, 5, 7)


.. code-block:: proto

	message GatherAlongAxisLayerParams {

	    int64 axis = 1;

	}






ScatterAlongAxisLayerParams
________________________________________________________________________________

A layer that scatters data into a new tensor according to indices from
the input along the given axis into the output tensor.
This is the inverse operation of GatherAlongAxis.
It is similar in functionality to the numpy.put_along_axis method.

Requires 3 inputs and produces 1 output.
3 inputs, in order are denoted as "container", "indices", "updates".

All inputs and output have the same rank.
Output shape is same as the shape of 'container'
Shapes of 'indices' and 'updates' match, which is same as the shape of 'container' except at the 'axis' dimension.

Negative indices and negative axis are supported.

This operation performs the following operation for axis=0:
output = container
for each vector index (i,j,....,k)
   output[index[i,j,....,k],j,....,k] = updates[i,j,....,k]

e.g.:

container shape = (2, 5, 6)
indices shape = (2, 2, 6)
updates shape = (2, 2, 6)
axis = -2
output shape = (2, 5, 6)


.. code-block:: proto

	message ScatterAlongAxisLayerParams {

	    int64 axis = 1;
	    ScatterMode mode = 2;

	}






StackLayerParams
________________________________________________________________________________

A layer that stacks the input tensors along the given axis.
It is similar in functionality to the numpy.stack method.

Requires at least 2 inputs and produces 1 output.
All inputs must have the same shape.
Rank of the output is 1 greater than the rank of the inputs.

Negative indexing is supported for the "axis" parameter.

e.g.:

input shape = (2, 4, 2)
number of inputs = 5
axis = 3
output shape = (2, 4, 2, 5)

input shape = (2, 4, 2)
number of inputs = 5
axis = -2
output shape = (2, 4, 5, 2)


.. code-block:: proto

	message StackLayerParams {

	    int64 axis = 1;

	}






RankPreservingReshapeLayerParams
________________________________________________________________________________

A layer that reshapes a tensor that does not alter the rank of the input.
Order of the data is left unchanged.

Requires 1 input and produces 1 output.

e.g:

input shape = (20,10)
targetShape = (5,-1)
output shape = (5,40)

input shape = (20,10,5)
targetShape = (0,2,25)
output shape = (20,2,25)

input shape = (10,3,5)
targetShape = (25,0,-1)
output shape = (25,3,2)


.. code-block:: proto

	message RankPreservingReshapeLayerParams {

	    repeated int64 targetShape = 1;

	}






ConstantPaddingLayerParams
________________________________________________________________________________

Constant padding layer.
Pad the input array with a constant value, either along a single given axis or along a set of axes.

Requires 1 or 2 inputs and produces 1 output.
The amount of padding can be either set as a parameter ("padAmounts") or provided as a second input.

Output rank is same as the rank of the first input.

when "padToGivenOutputSizeMode" is False:

output_shape[i] = input_shape[i] + padAmounts[2*i] + padAmounts[2*i+1], i=0,...,rank-1

Examples:

input shape = (20,10)
padAmounts = [0,1,4,0]
output shape = (21,14)

input shape = (20,10,5)
padAmounts = [0,0,3,4,0,9]
output shape = (20,17,14)


when "padToGivenOutputSizeMode" is True

output_shape[i] = max(input_shape[i], max(padAmounts[2*i] + padAmounts[2*i+1])), i=0,...,rank-1

input shape = (20,10)
padAmounts = [0,21,14,0]
output shape = (21,14)

input shape = (20,10,5)
padAmounts = [0,0,17,0,0,14]
output shape = (20,17,14)


.. code-block:: proto

	message ConstantPaddingLayerParams {
	    float value = 1;

	    repeated uint64 padAmounts = 2;

	    bool padToGivenOutputSizeMode = 3;
	}






RandomNormalLikeLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the normal distribution.

Requires 1 input and produces 1 output.

Parameters
    seed: seed used for the normal distribution.
    mean: mean of the normal distribution.
    stdDev: standard deviation of the normal distribution.

Input
    An N-Dimensional tensor, whose values are ignored. Only the shape is used to
    infer the shape of the output.

Output
    An N-Dimensional tensor with the same shape as the input tensor.


.. code-block:: proto

	message RandomNormalLikeLayerParams {

	    int64 seed = 1;
	    float mean = 2;
	    float stdDev = 3;

	}






RandomNormalStaticLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the normal distribution.

Requires no input and produces 1 output.

Parameters
    seed: seed used for the normal distribution.
    mean: mean of the normal distribution.
    stdDev: standard deviation of the normal distribution.
    outputShape: shape of the output tensor.

Output
    An N-Dimensional tensor of shape "outputShape".


.. code-block:: proto

	message RandomNormalStaticLayerParams {

	    int64 seed = 1;
	    float mean = 2;
	    float stdDev = 3;
	    repeated uint64 outputShape = 4;

	}






RandomNormalDynamicLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the normal distribution.

Requires 1 input and produces 1 output.

Parameters:
    seed: seed used for the normal distribution.
    mean: mean of the normal distribution.
    stdDev: standard deviation of the normal distribution.

Input
    A rank 1 tensor specifying the shape of the output

Output
    An N-Dimensional tensor with the shape specified by the values in the input tensor.


.. code-block:: proto

	message RandomNormalDynamicLayerParams {

	    int64 seed = 1;
	    float mean = 2;
	    float stdDev = 3;

	}






RandomUniformLikeLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the uniform distribution.

Requires 1 input and produces 1 output.

Parameters
    seed: seed used for the uniform distribution.
    minVal: lower bound on the range of random values for the uniform distribution.
    maxVal: upper bound on the range of random values for the uniform distribution.

Input
    An N-Dimensional tensor, whose values are ignored. Only the shape is used to
    infer the shape of the output.

Output
    An N-Dimensional tensor with the same shape as the input tensor.


.. code-block:: proto

	message RandomUniformLikeLayerParams {

	    int64 seed = 1;
	    float minVal = 2;
	    float maxVal = 3;

	}






RandomUniformStaticLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the uniform distribution.

Requires no input and produces 1 output.

Parameters
    seed: seed used for the uniform distribution.
    minVal: lower bound on the range of random values for the uniform distribution.
    maxVal: upper bound on the range of random values for the uniform distribution.
    outputShape: shape of the output tensor.

Output
    An N-Dimensional tensor of shape "outputShape".


.. code-block:: proto

	message RandomUniformStaticLayerParams {

	    int64 seed = 1;
	    float minVal = 2;
	    float maxVal = 3;
	    repeated uint64 outputShape = 4;

	}






RandomUniformDynamicLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the uniform distribution.

Requires 1 input and produces 1 output.

Parameters:
    seed: seed used for the uniform distribution.
    minVal: lower bound on the range of random values for the uniform distribution.
    maxVal: upper bound on the range of random values for the uniform distribution.

Input
    A rank 1 tensor specifying the shape of the output

Output
    An N-Dimensional tensor with the shape specified by the values in the input tensor.


.. code-block:: proto

	message RandomUniformDynamicLayerParams {

	    int64 seed = 1;
	    float minVal = 2;
	    float maxVal = 3;

	}






RandomBernoulliLikeLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the Bernoulli distribution.

Requires 1 input and produces 1 output.

Parameters
    seed: seed used for the Bernoulli distribution.
    prob: probability of a 1 event.

Input
    An N-Dimensional tensor, whose values are ignored. Only the shape is used to
    infer the shape of the output.

Output
    An N-Dimensional tensor with the same shape as the input tensor.


.. code-block:: proto

	message RandomBernoulliLikeLayerParams {

	    int64 seed = 1;
	    float prob = 2;

	}






RandomBernoulliStaticLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the Bernoulli distribution.

Requires no input and produces 1 output.

Parameters
    seed: seed used for the Bernoulli distribution.
    prob: probability of a 1 event.
    outputShape: shape of the output tensor.

Output
    An N-Dimensional tensor of shape "outputShape".


.. code-block:: proto

	message RandomBernoulliStaticLayerParams {

	    int64 seed = 1;
	    float prob = 2;
	    repeated uint64 outputShape = 3;

	}






RandomBernoulliDynamicLayerParams
________________________________________________________________________________

A layer that returns a tensor filled with values from the Bernoulli distribution.

Requires 1 input and produces 1 output.

Parameters:
    seed: seed used for the Bernoulli distribution.
    prob: probability of a 1 event.

Input
    A rank 1 tensor specifying the shape of the output

Output
    An N-Dimensional tensor with the shape specified by the values in the input tensor.


.. code-block:: proto

	message RandomBernoulliDynamicLayerParams {

	    int64 seed = 1;
	    float prob = 2;

	}






CategoricalDistributionLayerParams
________________________________________________________________________________

A layer that returns a tensor of the specified shape filled with values from the categorical distribution.

Requires 1 input and produces 1 output.

Parameter:
    seed: seed used for the categorical distribution.
    numSamples: number of samples to draw.
    isLogits: true if the inputs are logits, false if the inputs are probabilities.
    eps: default value is 1e-10.
    temperature: default value is 1.0.

Input tensor shape = [D_1, D_2, ... , D_(R-1), D_R] (Rank = R)
Then the shape of the output is [D_1, D_2, ... , D_(R-1), numSamples] (Rank = R)


.. code-block:: proto

	message CategoricalDistributionLayerParams {

	    int64 seed = 1;
	    int64 numSamples = 2;
	    bool isLogits = 3;
	    float eps = 4;
	    float temperature = 5;
	}






ReduceL1LayerParams
________________________________________________________________________________

A layer that performs reduction with L1 normalization operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceL1LayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceL2LayerParams
________________________________________________________________________________

A layer that performs reduction with L2 normalization operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceL2LayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceMaxLayerParams
________________________________________________________________________________

A layer that performs reduction with max operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceMaxLayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceMinLayerParams
________________________________________________________________________________

A layer that performs reduction with min operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceMinLayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceSumLayerParams
________________________________________________________________________________

A layer that performs reduction with sum operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceSumLayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceProdLayerParams
________________________________________________________________________________

A layer that performs reduction with prod operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceProdLayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceMeanLayerParams
________________________________________________________________________________

A layer that performs reduction with mean operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceMeanLayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceLogSumLayerParams
________________________________________________________________________________

A layer that performs reduction with logSum operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceLogSumLayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceSumSquareLayerParams
________________________________________________________________________________

A layer that performs reduction with logSumExp operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceSumSquareLayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ReduceLogSumExpLayerParams
________________________________________________________________________________

A layer that performs reduction with logSumExp operation.

Negative indexing is supported.
Requires 1 input and produces 1 output.

Parameters:
   axes: dimensions along which to perform reduction
   keepDims: if True, keep the reduced dimensions (value will be 1), otherwise, reduced dimensions are squeezed
   reduceAll: ignore the "axes" parameter, perform reduction along all axes


.. code-block:: proto

	message ReduceLogSumExpLayerParams {

	    repeated int64 axes = 1;
	    bool keepDims = 2;
	    bool reduceAll = 3;

	}






ExpandDimsLayerParams
________________________________________________________________________________

A layer that increases the rank of the input tensor by adding unit dimensions.

Requires 1 input and produces 1 output.

e.g.:

input shape = (10,5)
axes = (0,1)
output shape = (1,1,10,5)

input shape = (10,5)
axes = (0,2)
output shape = (1,10,1,5)

input shape = (10,5)
axes = (-2,-1)
output shape = (10,5,1,1)


.. code-block:: proto

	message ExpandDimsLayerParams {

	    repeated int64 axes = 1;

	}






FlattenTo2DLayerParams
________________________________________________________________________________

A layer that flattens the input tensor into a 2-dimensional matrix.

Requires 1 input and produces 1 output.
Output tensor is always rank 2.

First dimension of output is the product of all the dimensions in input[:axis] ("axis" is exclusive)
Second dimension of output is the product of all the dimensions in input[axis:] ("axis" is inclusive)

e.g.:
input shape:  (3,)
axis:  -1
output shape:  (1, 3)

input shape:  (3,)
axis:  1
output shape:  (3, 1)

input shape:  (4, 3)
axis:  -1
output shape:  (4, 3)

input shape:  (5, 2)
axis:  0
output shape:  (1, 10)

input shape:  (5, 5, 3)
axis:  -2
output shape:  (5, 15)

input shape:  (2, 3, 2)
axis:  -1
output shape:  (6, 2)


.. code-block:: proto

	message FlattenTo2DLayerParams {

	    int64 axis = 1;

	}






ReshapeStaticLayerParams
________________________________________________________________________________

A layer that reshapes a tensor.

Requires 1 input and produces 1 output.

Output tensor is the reshaped version of the input and has shape as specified in the
parameter "targetShape".


.. code-block:: proto

	message ReshapeStaticLayerParams {

	    repeated int64 targetShape = 1;

	}






ReshapeLikeLayerParams
________________________________________________________________________________

A layer that reshapes a tensor.

Requires 2 inputs and produces 1 output.

First input is reshaped to produce the output, while the second input is only
used to determine the shape of the output. Values of the second input are not used.

Output is a tensor with the same shape as the second input.


.. code-block:: proto

	message ReshapeLikeLayerParams {

	}






ReshapeDynamicLayerParams
________________________________________________________________________________

A layer that reshapes a tensor.

Requires 2 inputs and produces 1 output.

First input is the one that is reshaped to produce the output.
Second input is a rank 1 tensor specifying the shape of the output.
Output tensor has shape as specified by the values in the 2nd input tensor.


.. code-block:: proto

	message ReshapeDynamicLayerParams {

	}






SqueezeLayerParams
________________________________________________________________________________

A layer that decreases the rank of the input tensor by removing unit dimensions.

Requires 1 input and produces 1 output.

Output rank is one less than input rank, if input rank is more than 1.
If input rank is 1, output rank is also 1.

e.g.:

input shape = (1,1,10,5)
axes = (0,1)
output shape = (10,5)

input shape = (1,10,5,1)
axes = (0,3)
output shape = (10,5)

input shape = (10,5,1,1)
axes = (-2,-1)
output shape = (10,5)

input shape = (1,)
axes = (0)
output shape = (1,)


.. code-block:: proto

	message SqueezeLayerParams {

	    repeated int64 axes = 1;
	    bool squeezeAll = 2; // if true squeeze all dimensions that are 1.

	}






TopKLayerParams
________________________________________________________________________________

A layer that returns top K (or bottom K) values and the corresponding indices
of the input along a given axis.

Requires 1 or 2 inputs and produces 2 outputs.

The second input is the value of the K, and is optional.
If there is only one input, value of K that is specified in the layer parameter is used.

Both outputs have the same rank as the first input.
Second input must correspond to a scalar tensor.

e.g.:

first input's shape = (45, 34, 10, 5)
axis = 1
output shape, for both outputs = (45, K, 10, 5)


.. code-block:: proto

	message TopKLayerParams {

	    int64 axis = 1;
	    uint64 K = 2;
	    bool useBottomK = 3;

	}






ArgMaxLayerParams
________________________________________________________________________________

A layer that returns the indices of the maximum value along a specified axis in a tensor.

Requires 1 input and produces 1 output. Negative indexing is supported.

Output has the same rank as the input if "removeDim" is False (default).
Output has rank one less than the input if "removeDim" is True and input rank is more than 1.

e.g.:

input shape = (45, 34, 10, 5)
axis = -2
output shape = (45, 1, 10, 5), if removeDim = False (default)
output shape = (45, 10, 5), if removeDim = True

input shape = (5,)
axis = 0
output shape = (1,), if removeDim = False or True


.. code-block:: proto

	message ArgMaxLayerParams {

	    int64 axis = 1;
	    bool removeDim = 2;

	}






ArgMinLayerParams
________________________________________________________________________________

A layer that returns the indices of the minimum value along a specified axis in a tensor.

Requires 1 input and produces 1 output. Negative indexing is supported.

Output has the same rank as the input if "removeDim" is False (default).
Output has rank one less than the input if "removeDim" is True and input rank is more than 1.

e.g.:

input shape = (45, 34, 10, 5)
axis = -2
output shape = (45, 1, 10, 5), if removeDim = False (default)
output shape = (45, 10, 5), if removeDim = True

input shape = (5,)
axis = 0
output shape = (1,), if removeDim = False or True


.. code-block:: proto

	message ArgMinLayerParams {

	    int64 axis = 1;
	    bool removeDim = 2;

	}






SplitNDLayerParams
________________________________________________________________________________

A layer layer that splits the input tensor into multiple output tensors,
along the specified axis.

The layer either uniformly splits the input tensor into ``num_splits`` tensors, or
splits according to the given split sizes in ``split_sizes``.
Supports unequal splits and negative indexing.

Requires 1 input and produces at least 2 outputs.
Rank of all the outputs is same as that of the input.

If parameter "splitSizes" is provided, value of the parameter "numSplits" is ignored, since in that case
"numSplits" is automatically inferred to be the length of "splitSizes".


e.g.:
input shape:  (5, 3, 4)
axis = -3, split_sizes = [3, 2]
output shape:  (3, 3, 4)
output shape:  (2, 3, 4)


.. code-block:: proto

	message SplitNDLayerParams {

	    int64 axis = 1;
	    uint64 numSplits = 2;
	    repeated uint64 splitSizes = 3;

	}






CeilLayerParams
________________________________________________________________________________

A layer that performs element-wise ceil operation on the input tensor that
rounds the value to the smallest integer not less than x.

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message CeilLayerParams {

	}






RoundLayerParams
________________________________________________________________________________

A layer that performs element-wise round operation on the input tensor
that rounds the value to the nearest integer.

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message RoundLayerParams {

	}






FloorLayerParams
________________________________________________________________________________

A layer that performs element-wise floor operation on the input tensor
that rounds the value to the largest integer not greater than x.

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message FloorLayerParams {

	}






SignLayerParams
________________________________________________________________________________

A layer that performs element-wise sign operation (+1 for positive values,
-1 for negative values, 0 for zeros).

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message SignLayerParams {

	}






ClipLayerParams
________________________________________________________________________________

A layer that performs element-wise clip operation. Clip the values in the
input tensor to the threshold values [min_value, max_value].

Requires 1 input and produces 1 output.

Parameter minVal: the minimum threshold.
Parameter maxVal: the maximum threshold.

output =  min(max(input, minVal), maxVal)

Output shape is same as the input.


.. code-block:: proto

	message ClipLayerParams {

	    float minVal = 1;
	    float maxVal = 2;

	}






SliceStaticLayerParams
________________________________________________________________________________

A layer that extracts a slice of size ``(end - begin) / stride``
from the given input tensor.
Support negative indexing and negative strides.

Requires 1 input and produces 1 output.
Output rank is same as the input rank.

Value of beginIds, beginMasks, endIds, endMasks, strides are required parameters.
Lengths of all the parameters must equal the rank of the input.

i-th element of "beginIds" is ignored and assumed to be 0 if the i-th element of
"beginMasks" is True

i-th element of "endIds" is ignored and assumed to be -1 if the i-th element of
"endMasks" is True

e.g.:
if i-th element of "squeezeMasks" is set to True, only beginIds[i] would be sliced
out, and all other masks and inputs are ignored.

e.g. (without squeezeMasks):
input shape:  (5, 5, 5)
beginIds:  [1, 2, 3]
beginMasks:  [True, False, True]
endIds:  [3, -3, 2]
endMasks:  [False, True, True]
strides:  [2, 2, 2]
SqueezeMasks:  [False, False, False]
output shape:  (2, 2, 3)
This is equivalent to input[:3:2, 2::2, ::2]

e.g. (with squeezeMasks):
input shape:  (5, 5, 5)
beginIds:  [1, 2, 3]
beginMasks:  [True, False, True]
endIds:  [3, -3, 2]
endMasks:  [False, True, True]
strides:  [2, 2, 2]
SqueezeMasks:  [False, True, False]
output shape:  (2, 3)
This is equivalent to input[:3:2, 2, ::2]


.. code-block:: proto

	message SliceStaticLayerParams {

	    repeated int64 beginIds = 1;
	    repeated bool beginMasks = 2;
	    repeated int64 endIds = 3;
	    repeated bool endMasks = 4;
	    repeated int64 strides = 5;
	    repeated bool squeezeMasks = 6;


	}






SliceDynamicLayerParams
________________________________________________________________________________

A layer that extracts a slice of size ``(end - begin) / stride``
from the given input tensor.
Support negative indexing and negative strides.
See "SliceStaticLayerParams" for the description and an example of the functionality of the layer.

Requires 2 to 7 inputs and produces 1 output.
Rank of the output is same as the rank of the first input unless squeezeMask is set.

Value of beginIds, beginMasks, endIds, endMasks, strides can be passed in either
as dynamic inputs or as static parameters.
Lengths of all the parameters or inputs from 2-6 must equal the rank of the first input.

The 2nd input represents the "beginIds".
The 3rd input, if present, corresponds to "endIds". In this case the value of the "endIds" parameter is ignored.
The 4th input, if present, corresponds to "strides". In this case the value of the "strides" parameter is ignored.
The 5th input, if present, corresponds to "beginMasks". In this case the value of the "beginMasks" parameter is ignored.
The 6th input, if present, corresponds to "endMasks". In this case the value of the "endMasks" parameter is ignored.
The 7th input, if present, corresponds to "squeezeMasks". In this case the value of the "squeezeMasks" parameter is ignored.


.. code-block:: proto

	message SliceDynamicLayerParams {

	    repeated bool beginMasks = 2;
	    repeated int64 endIds = 3;
	    repeated bool endMasks = 4;
	    repeated int64 strides = 5;
	    repeated bool squeezeMasks = 6;

	}






TileLayerParams
________________________________________________________________________________

A layer that constructs a tensor by repeating the input tensor multiple
number of times.

Requires 1 or 2 inputs and produces 1 output.
Output rank is same as the input rank.

If two inputs are provided, second input is used as "reps"
and "reps" parameter is ignored.

If only one input is provided,
length of the "reps" parameter must be at least 1 and
not greater than the rank of the input.
If it is less than the input rank, it is made equal to the input rank by prepending 1's to it.

e.g.:

input shape = (2, 4, 2)
reps = (1, 2, 6)
output shape = (2, 8, 12)

input shape = (2, 4, 2)
reps = (6)
reps after prepending ones = (1, 1, 6)
output shape = (2, 4, 12)

input shape = (2, 4, 2)
second input = [1, 2, 6] -> shape: (3,)
reps = N/A [Ignored]
output shape = (2, 8, 12)


.. code-block:: proto

	message TileLayerParams {

	    repeated uint64 reps = 1;

	}






GetShapeLayerParams
________________________________________________________________________________

A layer that returns the shape of an input tensor.

Requires 1 input and produces 1 output.

Input: a tensor.
Output: a vector of length R, where R is the rank of the input tensor
Output is always a rank 1 tensor.


.. code-block:: proto

	message GetShapeLayerParams {

	}






ErfLayerParams
________________________________________________________________________________

A layer that computes the Gauss error function,
which is defined as:

.. math::
    f(x) = \dfrac{1}{\sqrt{\pi}}\int_{-x}^{x}{e^{-t^2}dt}

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message ErfLayerParams {

	}






GeluLayerParams
________________________________________________________________________________

A layer that evaluates the Gaussian Error Linear Unit (GELU) activation.
Following equations are used to compute the activation based on the value of the "mode" parameter:

mode == 'EXACT':
.. math::
    f(x) = 0.5x\left ( 1+\rm{erf}\left ( \frac{x}{\sqrt{2}} \right ) \right )

mode == 'TANH_APPROXIMATION':
.. math::
    f(x) = 0.5x\left ( 1+\rm{tanh}\left ( \sqrt{2/\pi}\left ( x + 0.044715x^3 \right ) \right ) \right )

mode == 'SIGMOID_APPROXIMATION':
.. math::
    f(x) = x*\rm{sigmoid}(1.702x)

Requires 1 input and produces 1 output.
Output shape is same as the input.


.. code-block:: proto

	message GeluLayerParams {

	    enum GeluMode {

	        EXACT = 0;
	        TANH_APPROXIMATION = 1;
	        SIGMOID_APPROXIMATION = 2;

	    }

	    GeluMode mode = 1;

	}






RangeStaticLayerParams
________________________________________________________________________________

RangeStatic layer that returns a tensor that contains evenly spaced values.
It is similar in functionality to the numpy.arange method.

Requires no input and produces 1 output.
Output is a rank 1 tensor.


.. code-block:: proto

	message RangeStaticLayerParams {

	    float endValue = 1;
	    float startValue = 2;
	    float stepSizeValue = 3;

	}






RangeDynamicLayerParams
________________________________________________________________________________

A layer that returns a tensor that contains evenly spaced values.
Its functionality is similar to the numpy.arange method.

Requires at least 1 input, up to a maximum of 3 inputs.
Produces 1 output, which is a rank 1 tensor.

Each input must be a scalar, or rank 1 and shape (1,).

The first input represents the "endValue".
The second input, if present, corresponds to "startValue". In this case the value of the "startValue" parameter is ignored.
The third input, if present, corresponds to "stepSizeValue". In this case the value of the "stepSizeValue" parameter is ignored.


.. code-block:: proto

	message RangeDynamicLayerParams {

	    float startValue = 2;
	    float stepSizeValue = 3;

	}






SlidingWindowsLayerParams
________________________________________________________________________________

A layer that returns a tensor containing all windows of size ``windowSize``
separated by ``step`` along the dimension ``axis``.

.. code::

     y = SlidingWindows(x)

Requires 1 input and produces 1 output.

Input
    An N-Dimensional tensor.

Output
    An (N+1)-Dimensional tensor.

This operation behaves as following:
     - if axis = 0 & input is rank 1 (L,). Output shape will be (M, W).
     - if axis = 1 & input is rank 3 (B1, L, C1). Output shape will be (B1, M, W, C1)
     - if axis = 2 & input is rank 5 (B1, B2, L, C1, C2) --> (B1 * B2, L, C1 * C2) --> (B1 * B2, M, W, C1 * C2). Output shape will be (B1, B2, M, W, C1, C2)
     - etc.
where
     - L, C, B refer to input length, feature dimension length & batch size respectively
     - W is the window size.
     - M is the number of windows/slices calculated as M = (L - W) / step + 1


.. code-block:: proto

	message SlidingWindowsLayerParams {

	    int64 axis = 1;
	    uint64 windowSize = 2;
	    uint64 step = 3;

	}






LayerNormalizationLayerParams
________________________________________________________________________________

A layer that applies layer normalization over the input tensor.

Requires 1 input and produces 1 output.

output = gamma * (input - computed_mean) / (sqrt(computed_variance + eps)) + beta

Parameters
    normalizedShape: subset of the input shape, along with layer norm is performed, rest of the input shape is treated as the batch dimension. The mean and variance are computed for the input, over the last few dimensions as specified by the normalizedShape parameter.
    gamma: must have shape = "normalizedShape"
    beta: must have shape = "normalizedShape"
    eps: small constant to avoid division by 0

Output shape is same as the input.

e.g.:
input shape = (10,5)
normalized shape = (5,) or (10,5)

input shape = (10,5,6,7)
normalized shape = (7,) or (6,7) or (5,6,7) or (10,5,6,7)


.. code-block:: proto

	message LayerNormalizationLayerParams {

	    repeated int64 normalizedShape = 1;
	    float eps = 2;
	    WeightParams gamma = 3;
	    WeightParams beta = 4;

	}






NonMaximumSuppressionLayerParams
________________________________________________________________________________

Non maximum suppression (NMS) layer.
Applies the non maximum suppression algorithm to input bounding box coordinates.
The effect of this layer is similar to the functionality of the "NonMaximumSuppression"
model type (for details please see NonMaximumSuppression.proto) with a couple of differences.
One, this is a layer in a neural network model, whereas that is a different model type. Second,
this layer supports a batch of bounding boxes.

The NMS layer requires at least 2 inputs, and up to a maximum of 5 inputs. It produces 4 outputs.
Following is the description of inputs and outputs:

input 1, shape (B,N,4): coordinates of N boxes, for a batch size B.
input 2, shape (B,N,C): class scores for each box. C can be 1 when there is only 1 score per box, i.e., no class specific score.

input 3, optional, shape (1,): IoU threshold. When present, it overwrites the value provided in layer parameter "iouThreshold".
input 4, optional, shape (1,): Score threshold. When present, it overwrites the value provided in layer parameter "scoreThreshold".
input 5, optional, shape (1,): Maximum number of boxes. When present, it overwrites the value provided in layer parameter "maxBoxes".

output 1, shape (B,maxBoxes,4): box coordinates, corresponding to the surviving boxes.
output 2, shape (B,maxBoxes,C): box scores, corresponding to the surviving boxes.
output 3, shape (B,maxBoxes): indices of the surviving boxes. Hence it will have values in the range [0,N-1], except for padding.
output 4, shape (B,): number of boxes selected after the NMS algorithm, for each batch.

When surviving boxes are less than "maxBoxes", the first 3 outputs are padded.
For the first two outputs, the padding is done using values 0, whereas for the third output the
padding value used is -1, since the output values represent indices.

If no box survives, that is, all the scores are below the "scoreThreshold",
then for that batch, number of boxes (value of the fourth output) will be 1. The first 3 outputs will
correspond to the box with the highest score. This is to avoid generating an "empty" output.

The four values that describe the box dimensions are (in order):

 - x (center location of the box along the horizontal axis)
 - y (center location of the box along the vertical axis)
 - width (size of box along the horizontal axis)
 - height (size of box on along the vertical axis)

In each batch,
the N scores for N boxes, used for suppression, are generated by taking the max of the matrix (N,C)
along the columns.
If "perClassSuppression" flag is false, suppression happens across all classes.
If "perClassSuppression" flag is true, each box is assigned to the class with the highest
score and then the suppression happens separately for boxes within the same class.

Note that the 4th output can be used to dynamically slice the first 3 outputs, in case
the padded outputs are not required.


.. code-block:: proto

	message NonMaximumSuppressionLayerParams {
	    float iouThreshold = 1;

	    float scoreThreshold = 2;

	    uint64 maxBoxes = 3;

	    bool perClassSuppression = 4;
	}






ClampedReLULayerParams
________________________________________________________________________________

A layer that performs element-wise clamped ReLU operation.

Requires 1 input and produces 1 output.

This function has the following formula:

.. math::
    f(x) = \begin{cases}
              \text{min}(\text{beta},x) \;\; \text{if} \;\; x \geq 0\\
              \text{min}(\text{beta} ,\text{alpha}\cdot x) \;\; \text{if} \;\; x<0
           \end{cases}

Output shape is same as the input.

Available (iOS >= 14, macOS >= 11.0, watchOS >= 7)


.. code-block:: proto

	message ClampedReLULayerParams {

	    float alpha = 1;
	    float beta = 2;

	}






ArgSortLayerParams
________________________________________________________________________________

A layer that returns the indices that would sort the input tensor, along a specified axis.

Requires 1 input and produces 1 output.

Output has the same rank and shape as the input.

Value of "axis" must be positive and less than the rank of the input.

e.g.:

input shape = (5,)
axis = 0
input values = [3.1, 5.4, 32.9, 3.2, 77.0]
output shape = (5,)
output values = [0, 3, 1, 2, 4], descending = False
output values = [4, 2, 1, 3, 0], descending = True

input shape = (2,3)
axis = 1
input values = [[3, 5, 32], [3, 77, 6]]
output shape = (2,3)
output values = [[0, 1, 2], [0, 2, 1]], descending = False
output values = [[2, 1, 0], [1, 2, 0]], descending = True


.. code-block:: proto

	message ArgSortLayerParams {

	    int64 axis = 1;
	    bool descending = 2;

	}






SliceBySizeLayerParams
________________________________________________________________________________

A layer that does slice operation by providing size to be extracted
from the given input tensor.

Requires 2 inputs and produces 1 output.
Rank of the output is same as the rank of the first input.

The 1st input represents the tensor to be sliced.
The 2nd input represents the beginning index to be sliced from.

Example:
Input 1: x (x.shape = (2, 3, 4))
Input 2: begin
size: 2
axis: 1

Output: x[:, begin:begin+2, :]


.. code-block:: proto

	message SliceBySizeLayerParams {

	    int64 size = 2;
	    int64 axis = 3;

	}






NeuralNetworkClassifier
________________________________________________________________________________

A neural network specialized as a classifier.


.. code-block:: proto

	message NeuralNetworkClassifier {

	    repeated NeuralNetworkLayer layers = 1;
	    repeated NeuralNetworkPreprocessing preprocessing = 2;

	    // use this enum value to determine the input tensor shapes to the neural network, for multiarray inputs
	    NeuralNetworkMultiArrayShapeMapping arrayInputShapeMapping = 5;

	    // use this enum value to determine the input tensor shapes to the neural network, for image inputs
	    NeuralNetworkImageShapeMapping imageInputShapeMapping = 6;

	    NetworkUpdateParameters updateParams = 10;

	    // The set of labels for every possible class.
	    oneof ClassLabels {
	        StringVector stringClassLabels = 100;
	        Int64Vector int64ClassLabels = 101;
	    }

	    // The name of the output blob containing the probability of each class.
	    // In other words, the score vector. Must be a 1-D tensor with the same
	    // number and order of elements as ClassLabels.
	    string labelProbabilityLayerName = 200;
	}






OneHotLayerParams
________________________________________________________________________________




.. code-block:: proto

	message OneHotLayerParams {

	    uint64 oneHotVectorSize = 1;
	    int64 axis = 2;
	    float onValue = 3;
	    float offValue = 4;
	}






CumSumLayerParams
________________________________________________________________________________




.. code-block:: proto

	message CumSumLayerParams {

	    int64 axis = 1;

	    bool excludeFinalSum = 2;

	    bool reverse = 3;
	}






NeuralNetworkRegressor
________________________________________________________________________________

A neural network specialized as a regressor.


.. code-block:: proto

	message NeuralNetworkRegressor {

	    repeated NeuralNetworkLayer layers = 1;
	    repeated NeuralNetworkPreprocessing preprocessing = 2;

	    // use this enum value to determine the input tensor shapes to the neural network, for multiarray inputs
	    NeuralNetworkMultiArrayShapeMapping arrayInputShapeMapping = 5;

	    // use this enum value to determine the input tensor shapes to the neural network, for image inputs
	    NeuralNetworkImageShapeMapping imageInputShapeMapping = 6;

	    NetworkUpdateParameters updateParams = 10;

	}






NetworkUpdateParameters
________________________________________________________________________________

Details on how the network will be updated


.. code-block:: proto

	message NetworkUpdateParameters {

	    repeated LossLayer lossLayers = 1;
	    Optimizer optimizer = 2;
	    Int64Parameter epochs = 3;

	    BoolParameter shuffle = 10;

	    Int64Parameter seed = 20;
	}






LossLayer
________________________________________________________________________________

Loss layer - categorical cross entropy and mean squared error are the only supported loss functions currently


.. code-block:: proto

	message LossLayer {

	    string name = 1;
	    oneof LossLayerType {

	        CategoricalCrossEntropyLossLayer categoricalCrossEntropyLossLayer = 10;
	        MeanSquaredErrorLossLayer meanSquaredErrorLossLayer = 11;

	    }

	}






CategoricalCrossEntropyLossLayer
________________________________________________________________________________

Categorical cross entropy loss layer
Categorical cross entropy is used for single label categorization (only one category is applicable for each data point).

The input is a vector of length N representing the distribution over N categories.  It must be the output of a softmax.

The target is a single value representing the true category or class label. If the target is the predictedFeatureName of a neural network classifier it will be inverse mapped to the corresponding categorical index for you.

math:
Loss_{CCE}(input, target) = -\sum_{i=1}^{N} (target == i) log( input[i] ) = - log (input[target])


.. code-block:: proto

	message CategoricalCrossEntropyLossLayer {

	    string input = 1;
	    string target = 2;

	}






MeanSquaredErrorLossLayer
________________________________________________________________________________

Mean squared error loss layer,
specifying input and target


.. code-block:: proto

	message MeanSquaredErrorLossLayer {

	    string input = 1;
	    string target = 2;

	}






Optimizer
________________________________________________________________________________

Optimizer - stochastic gradient descent and adam are the only supported optimizers currently


.. code-block:: proto

	message Optimizer {

	    oneof OptimizerType {

	        SGDOptimizer sgdOptimizer = 10;
	        AdamOptimizer adamOptimizer = 11;

	    }

	}






SGDOptimizer
________________________________________________________________________________

Stochastic gradient descent optimizer,
specifying configurable learning rate, mini batch size, and momentum


.. code-block:: proto

	message SGDOptimizer {

	    DoubleParameter learningRate = 1;
	    Int64Parameter miniBatchSize = 2;
	    DoubleParameter momentum = 3;

	}






AdamOptimizer
________________________________________________________________________________

Adam optimizer,
specifying configurable learning rate, mini batch size, betas, and eps


.. code-block:: proto

	message AdamOptimizer {

	    DoubleParameter learningRate = 1;
	    Int64Parameter miniBatchSize = 2;
	    DoubleParameter beta1 = 3;
	    DoubleParameter beta2 = 4;
	    DoubleParameter eps = 5;

	}










BoxCoordinatesMode.Coordinates
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum Coordinates {

	        CORNERS_HEIGHT_FIRST = 0;

	        CORNERS_WIDTH_FIRST = 1;

	        CENTER_SIZE_HEIGHT_FIRST = 2;

	        CENTER_SIZE_WIDTH_FIRST = 3;

	    }



Convolution3DLayerParams.PaddingType
--------------------------------------------------------------------------------

The type of padding.
All padding types pad the input shape with zeros.
CUSTOM padding will add the custom padding values specified below to their respective
dimensions, e.g., `customPaddingFront` number of zeros will be added to one side of the
input's depth dimension and `customPaddingBack` number of zeros will be added to the other
side of the input's depth dimension.
VALID padding adds no padding to any dimension. In this case, the last convolution along
each dimension will be dropped if the input dimension and the kernel size, stride, and
dilation do not match.
SAME padding adds enough padding to each dimension such that the output of the convolution
has size ``Ceiling(inputShape / stride)``. Padding is added evenly to both sides of each
dimension unless the total padding to add is odd, in which case it is added to the
back/bottom/right side of the respective dimension. For example, if the total padding needed
in the depth dimension is 3, 1 zero will be added to the front side of the depth dimension
and 2 zeros will be added to the back side.

.. code-block:: proto

	    enum PaddingType {
	        CUSTOM = 0;
	        VALID = 1;
	        SAME = 2;
	    }



FlattenLayerParams.FlattenOrder
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum FlattenOrder {

	        CHANNEL_FIRST = 0;
	        CHANNEL_LAST = 1;

	    }



GeluLayerParams.GeluMode
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum GeluMode {

	        EXACT = 0;
	        TANH_APPROXIMATION = 1;
	        SIGMOID_APPROXIMATION = 2;

	    }



GlobalPooling3DLayerParams.GlobalPoolingType3D
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum GlobalPoolingType3D {
	        MAX = 0;
	        AVERAGE = 1;
	    }



NeuralNetworkImageShapeMapping
________________________________________________________________________________



.. code-block:: proto

	enum NeuralNetworkImageShapeMapping {


	    RANK5_IMAGE_MAPPING = 0;

	    RANK4_IMAGE_MAPPING = 1;

	}



NeuralNetworkMultiArrayShapeMapping
________________________________________________________________________________



.. code-block:: proto

	enum NeuralNetworkMultiArrayShapeMapping {


	    RANK5_ARRAY_MAPPING = 0;

	    EXACT_ARRAY_MAPPING = 1;

	}



Pooling3DLayerParams.Pooling3DPaddingType
--------------------------------------------------------------------------------

The type of padding.
All padding types pad the input shape with zeros.
CUSTOM padding will add the custom padding values specified below to their respective
dimensions, e.g., `customPaddingFront` number of zeros will be added to one side of the
input's depth dimension and `customPaddingBack` number of zeros will be added to the other
side of the input's depth dimension.
VALID padding adds no padding to any dimension. In this case, the last pool along
each dimension will be dropped if the input dimension and the kernel size, and stride do not match.
SAME padding adds enough padding to each dimension such that the output
has the same spatial dimensions as the input. Padding is added evenly to both
sides of each dimension unless the total padding to add is odd, in which case the extra padding
is added to the back/bottom/right side of the respective dimension.  For example, if the the
total horizontal padding is 3, then there will be 1 padding on the left, and 2 padding on the right.

.. code-block:: proto

	    enum Pooling3DPaddingType {
	        CUSTOM = 0;
	        VALID = 1;
	        SAME = 2;
	    }



Pooling3DLayerParams.PoolingType3D
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum PoolingType3D {
	        MAX = 0;
	        AVERAGE = 1;
	    }



PoolingLayerParams.PoolingType
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum PoolingType {

	        MAX = 0;
	        AVERAGE = 1;
	        L2 = 2;

	    }



ReduceLayerParams.ReduceAxis
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum ReduceAxis {

	        CHW = 0;
	        HW = 1;
	        C = 2;
	        H = 3;
	        W = 4;

	    }



ReduceLayerParams.ReduceOperation
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum ReduceOperation {

	        SUM = 0;
	        AVG = 1;
	        PROD = 2;
	        LOGSUM = 3;
	        SUMSQUARE = 4;
	        L1 = 5;
	        L2 = 6;
	        MAX = 7;
	        MIN = 8;
	        ARGMAX = 9;

	    }



ReorganizeDataLayerParams.ReorganizationType
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum ReorganizationType {

	        SPACE_TO_DEPTH = 0;
	        DEPTH_TO_SPACE = 1;
	        PIXEL_SHUFFLE = 2;

	    }



ReshapeLayerParams.ReshapeOrder
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum ReshapeOrder {

	        CHANNEL_FIRST = 0;
	        CHANNEL_LAST = 1;

	    }



SamePadding.SamePaddingMode
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum SamePaddingMode {

	        BOTTOM_RIGHT_HEAVY = 0;
	        TOP_LEFT_HEAVY = 1;

	    }



SamplingMode.Method
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum Method {

	        STRICT_ALIGN_ENDPOINTS_MODE = 0;

	        ALIGN_ENDPOINTS_MODE = 1;

	        UPSAMPLE_MODE = 2;

	        ROI_ALIGN_MODE = 3;

	    }



ScatterMode
________________________________________________________________________________



.. code-block:: proto

	enum ScatterMode {

	    SCATTER_UPDATE = 0;
	    SCATTER_ADD = 1;
	    SCATTER_SUB = 2;
	    SCATTER_MUL = 3;
	    SCATTER_DIV = 4;
	    SCATTER_MAX = 5;
	    SCATTER_MIN = 6;

	}



SliceLayerParams.SliceAxis
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum SliceAxis {

	        CHANNEL_AXIS = 0;
	        HEIGHT_AXIS = 1;
	        WIDTH_AXIS = 2;

	    }



UnaryFunctionLayerParams.Operation
--------------------------------------------------------------------------------

A unary operator.

The following functions are supported:

``SQRT``
    .. math:: f(x) = \sqrt{x}

``RSQRT``
    .. math:: f(x) = \dfrac{1}{\sqrt{x + \epsilon}}

``INVERSE``
    .. math:: f(x) = \dfrac{1}{x + \epsilon}

``POWER``
    .. math:: f(x) = x^\alpha

``EXP``
    .. math:: f(x) = e^x

``LOG``
    .. math:: f(x) = \log x

``ABS``
    .. math:: f(x) = |x|

``THRESHOLD``
    .. math:: f(x) = \text{max}(\alpha, x)

.. code-block:: proto

	    enum Operation {
	        SQRT = 0;
	        RSQRT = 1;
	        INVERSE = 2;
	        POWER = 3;
	        EXP = 4;
	        LOG = 5;
	        ABS = 6;
	        THRESHOLD = 7;
	    }



UpsampleLayerParams.InterpolationMode
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum InterpolationMode {

	        NN = 0;
	        BILINEAR = 1;

	    }



UpsampleLayerParams.LinearUpsampleMode
--------------------------------------------------------------------------------

LinearUpsampleMode specifies the behavior for linear upsampling. Only valid when Interpolation Mode is BILINEAR.
If input grid is [0, Xin-1] (corresponding to an input size of Xin), and if the output size is Xout,
then the grid points are sampled in the following manner:
DEFAULT:
  spacing = (Xin-Xin/Xout) / (Xout-1)
  grid_point[i] = min(Xin-1, max(0, i * spacing)), for i = 0,1,2,….,Xout-1
ALIGN_CORNERS_TRUE:
  spacing = (Xin-1) / (Xout-1)
  grid_point[i] = min(Xin-1, max(0, i * spacing)), for i = 0,1,2,….,Xout-1
ALIGN_CORNERS_FALSE:
  spacing = Xin / Xout
  grid_point[i] = min(Xin-1, max(0, i * spacing + 0.5 * spacing - 0.5)), for i = 0,1,2,….,Xout-1

.. code-block:: proto

	    enum LinearUpsampleMode {

	        DEFAULT = 0;
	        ALIGN_CORNERS_TRUE = 1;
	        ALIGN_CORNERS_FALSE = 2;

	    }
