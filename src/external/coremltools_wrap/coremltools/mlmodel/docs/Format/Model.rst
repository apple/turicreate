A Core ML model consists of a specification version
and a model description,
and can be any one of the following types:

Neural Networks
  - `NeuralNetwork`

Regressors
  - ``GLMRegressor``
  - ``SupportVectorRegressor``
  - ``TreeEnsembleRegressor``
  - ``NeuralNetworkRegressor``
  - ``BayesianProbitRegressor``

Classifiers
  - `NeuralNetworkClassifier`
  - `TreeEnsembleClassifier`
  - `GLMClassifier`
  - `SupportVectorClassifier`
  - `KNearestNeighborsClassifier`

Other models
  - `CustomModel`
  - `TextClassifier`
  - `WordTagger`
  - `Gazetteer`
  - `WordEmbedding`
  - `VisionFeaturePrint`
  - `LinkedModel`
  - `SoundAnalysisPreprocessing`
  - `ItemSimilarityRecommender`

Feature Engineering
  - `Imputer`
  - `Scaler`
  - `Normalizer`
  - `OneHotEncoder`
  - `CategoricalMapping`
  - `FeatureVectorizer`
  - `DictVectorizer`
  - `ArrayFeatureExtractor`
  - `NonMaximumSuppression`

Pipelines
  - `PipelineClassifier`
  - `PipelineRegressor`
  - `Pipeline`

Simple Mathematical Functions
  - `Identity`



Pipeline
________________________________________________________________________________

A pipeline consisting of one or more models.


.. code-block:: proto

	message Pipeline {
	    repeated Model models = 1;
	
	    // Optional names given for each model
	    // If not supplied it defaults to ["model0",..., "model"(models.size()-1)]
	    // These names can be used to disambiguate the scope / domain of a parameter
	    repeated string names = 2;
	}






PipelineClassifier
________________________________________________________________________________

A classifier pipeline.


.. code-block:: proto

	message PipelineClassifier {
	    Pipeline pipeline = 1;
	}






PipelineRegressor
________________________________________________________________________________

A regressor pipeline.


.. code-block:: proto

	message PipelineRegressor {
	    Pipeline pipeline = 1;
	}






FeatureDescription
________________________________________________________________________________

A feature description,
consisting of a name, short description, and type.


.. code-block:: proto

	message FeatureDescription {
	    string name = 1;
	    string shortDescription = 2;
	    FeatureType type = 3;
	}






Metadata
________________________________________________________________________________

Model metadata,
consisting of a short description, a version string,
an author, a license, and any other user defined
key/value meta data.


.. code-block:: proto

	message Metadata {
	    string shortDescription = 1;
	    string versionString = 2;
	    string author = 3;
	    string license = 4;
	    map<string, string> userDefined = 100;
	}








ModelDescription
________________________________________________________________________________

A description of a model,
consisting of descriptions of its input and output features.
Both regressor and classifier models require the name of the
primary predicted output feature (``predictedFeatureName``).
Classifier models can specify the output feature containing
probabilities for the predicted classes
(``predictedProbabilitiesName``).


.. code-block:: proto

	message ModelDescription {
	    repeated FeatureDescription input = 1;
	    repeated FeatureDescription output = 10;
	
	    // [Required for regressor and classifier models]: the name
	    // to give to an output feature containing the prediction.
	    string predictedFeatureName = 11;
	
	    // [Optional for classifier models]: the name to give to an
	    // output feature containing a dictionary mapping class
	    // labels to their predicted probabilities. If not specified,
	    // the dictionary will not be returned by the model.
	    string predictedProbabilitiesName = 12;
	
	    repeated FeatureDescription trainingInput = 50;
	
	    Metadata metadata = 100;
	}






SerializedModel
________________________________________________________________________________




.. code-block:: proto

	message SerializedModel {
	    // Identifier whose content describes the model type of the serialized protocol buffer message.
	    string identifier = 1;
	
	    // Must be a valid serialized protocol buffer of the above specified type.
	    bytes model = 2;
	}






Model
________________________________________________________________________________

A Core ML model,
consisting of a specification version,
a model description, and a model type.

Core ML model compatibility is indicated by
a monotonically increasing specification version number,
which is incremented anytime a backward-incompatible change is made
(this is functionally equivalent to the MAJOR version number
described by `Semantic Versioning 2.0.0 <http://semver.org/>`_).

Specification Versions : OS Availability (Core ML Version)

1 : iOS 11, macOS 10.13, tvOS 11, watchOS 4 (Core ML 1)
- Feedforward & Recurrent Neural Networks
- General Linear Models
- Tree Ensembles
- Support Vector Machines
- Pipelines
- Feature Engineering

2 : iOS 11.2, macOS 10.13.2, tvOS 11.2, watchOS 4.2 (Core ML 1.2)
- Custom Layers for Neural Networks
- Float 16 support for Neural Network layers

3 : iOS 12, macOS 10.14, tvOS 12, watchOS 5 (Core ML 2)
- Flexible shapes and image sizes
- Categorical sequences
- Core ML Vision Feature Print, Text Classifier, Word Tagger
- Non Max Suppression
- Crop and Resize Bilinear NN layers
- Custom Models

4 : iOS 13, macOS 10.15, tvOS 13, watchOS 6 (Core ML 3)
- Updatable models
- Exact shape / general rank mapping for neural networks
- Large expansion of supported neural network layers
  - Generalized operations
  - Control flow
  - Dynamic layers
  - See NeuralNetwork.proto
- Nearest Neighbor Classifier
- Sound Analysis Prepreocessing
- Recommender
- Linked Model
- NLP Gazeteer
- NLP WordEmbedding


.. code-block:: proto

	message Model {
	    int32 specificationVersion = 1;
	    ModelDescription description = 2;
	    
	    bool isUpdatable = 10;
	    
	    // start at 200 here
	    // model specific parameters:
	    oneof Type {
	        // pipeline starts at 200
	        PipelineClassifier pipelineClassifier = 200;
	        PipelineRegressor pipelineRegressor = 201;
	        Pipeline pipeline = 202;
	
	        // regressors start at 300
	        GLMRegressor glmRegressor = 300;
	        SupportVectorRegressor supportVectorRegressor = 301;
	        TreeEnsembleRegressor treeEnsembleRegressor = 302;
	        NeuralNetworkRegressor neuralNetworkRegressor = 303;
	        BayesianProbitRegressor bayesianProbitRegressor = 304;
	
	        // classifiers start at 400
	        GLMClassifier glmClassifier = 400;
	        SupportVectorClassifier supportVectorClassifier = 401;
	        TreeEnsembleClassifier treeEnsembleClassifier = 402;
	        NeuralNetworkClassifier neuralNetworkClassifier = 403;
	        KNearestNeighborsClassifier kNearestNeighborsClassifier = 404;
	
	        // generic models start at 500
	        NeuralNetwork neuralNetwork = 500;
	        ItemSimilarityRecommender itemSimilarityRecommender = 501;
	
	        // Custom and linked models
	        CustomModel customModel = 555;
	        LinkedModel linkedModel = 556;
	
	        // feature engineering starts at 600
	        OneHotEncoder oneHotEncoder = 600;
	        Imputer imputer = 601;
	        FeatureVectorizer featureVectorizer = 602;
	        DictVectorizer dictVectorizer = 603;
	        Scaler scaler = 604;
	        CategoricalMapping categoricalMapping = 606;
	        Normalizer normalizer = 607;
	        ArrayFeatureExtractor arrayFeatureExtractor = 609;
	        NonMaximumSuppression nonMaximumSuppression = 610;
	
	
	        // simple mathematical functions used for testing start at 900
	        Identity identity = 900;
	
	        // reserved until 1000
	
	        // CoreML provided models
	        CoreMLModels.TextClassifier textClassifier = 2000;
	        CoreMLModels.WordTagger wordTagger = 2001;
	        CoreMLModels.VisionFeaturePrint visionFeaturePrint = 2002;
	        CoreMLModels.SoundAnalysisPreprocessing soundAnalysisPreprocessing = 2003;
	        CoreMLModels.Gazetteer gazetteer = 2004;
	        CoreMLModels.WordEmbedding wordEmbedding = 2005;
	        
	        // Reserved private messages start at 3000
	        // These messages are subject to change with no notice or support.
	        SerializedModel serializedModel = 3000;
	    }
	}