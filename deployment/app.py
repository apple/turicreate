# -*- coding: utf-8 -*-
from flask import Flask, jsonify, send_file, request
import turicreate as tc
from enum import Enum
from PIL import Image
import numpy as np
import argparse
import datetime
import zipfile
import json

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

# class ModelType(Enum):
#     image_classifier = 0 #
#     drawing_classification = 1 #
#     sound_classification = 2
#     object_detection = 3 # -
#     style_transfer = 4 #
#     activity_classification = 5 #
#     image_similarity = 6
#     text_classification = 7 #-
#     random_forest_classifier = 8
#     decision_tree_classifier = 9
#     boosted_trees_classifier = 10
#     logistic_classifer = 11
#     support_vector_machine_classifier = 12
#     nearest_neighbor_classifier = 13
#     item_similiarity_recommender = 14
#     item_content_recommender = 15
#     factorization_recommender = 16
#     ranking_factorization_recommender = 17
#     popularity_recomender = 18
#     kmeans = 19
#     random_forest_regression = 20
#     decision_tree_regression = 21
#     boosted_trees_regression = 23
#     linear_regression = 24


def return_success(messages, success):
    messages['success'] = success
    return jsonify(messages)

def serve_pil_image(pil_img):
    img_io = StringIO()
    pil_img.save(img_io, 'JPEG', quality=70)
    img_io.seek(0)
    return send_file(img_io, mimetype='image/jpeg')

# def ModelType2String(info):
#     info = info.copy()
#     if(info["type"] == ModelType.recommender):
#         info["type"] = "Recommender"
#         return info
#     if(info["type"] == ModelType.image_classifier):
#         info["type"] = "Image Classifier"
#         return info
#     if(info["type"] == ModelType.drawing_classification):
#         info["type"] = "Drawing Classifier"
#         return info
#     if(info["type"] == ModelType.sound_classification):
#         info["type"] = "Sound Classifier"
#         return info
#     if(info["type"] == ModelType.object_detection):
#         info["type"] = "Object Detection"
#         return info
#     if(info["type"] == ModelType.style_transfer):
#         info["type"] = "Style Transfer"
#         return info
#     if(info["type"] == ModelType.activity_classification):
#         info["type"] = "Activity Classifier"
#         return info
#     if(info["type"] == ModelType.image_similarity):
#         info["type"] = "Image Similarity"
#         return info
#     if(info["type"] == ModelType.text_classification):
#         info["type"] = "Text Classifier"
#         return info
#     if(info["type"] == ModelType.random_forest_classifier):
#         info["type"] = "Random Forest Classifier"
#         return info
#     if(info["type"] == ModelType.decision_tree_classifier):
#         info["type"] = "Decision Tree Classifier"
#         return info
#     if(info["type"] == ModelType.boosted_tree_classifier):
#         info["type"] = "Boosted Tree Classifier"
#         return info
#     if(info["type"] == ModelType.svm_classifier):
#         info["type"] = "Logistic Classifier"
#         return info
#     if(info["type"] == ModelType.nearest_neighbor_classifier):
#         info["type"] = "Supprt Vector Machine Classifier"
#         return info
#     if(info["type"] == ModelType.item_similiarity_recommender):
#         info["type"] = "Image Similarity Recommender"
#         return info
#     if(info["type"] == ModelType.item_content_recommender):
#         info["type"] = "Item Content Recommender"
#         return info
#     if(info["type"] == ModelType.factorization_recommender):
#         info["type"] = "Factorization Recommender"
#         return info
#     if(info["type"] == ModelType.ranking_factorization_recommender):
#         info["type"] = "Ranking Factorization Recommender"
#         return info
#     if(info["type"] == ModelType.popularity_recomender):
#         info["type"] = "Popularity Recommender"
#         return info
#     if(info["type"] == ModelType.kmeans):
#         info["type"] = "Kmeans Clustering"
#         return info
#     if(info["type"] == ModelType.random_forest_regression):
#         info["type"] = "Random Forest Regression"
#         return info
#     if(info["type"] == ModelType.decision_tree_regression):
#         info["type"] = "Decision Tree Regression"
#         return info
#     if(info["type"] == ModelType.boosted_tree_regression):
#         info["type"] = "Boosted Tree Regression"
#         return info
#     if(info["type"] == ModelType.linear_regression):
#         info["type"] = "Linear Regression"
#         return info
#     return info


print( 
'''
    +----------------------------------------+
    |                                        |
    |      Turi Create Inference Engine       |
    |                                        |
    +----------------------------------------+
    

                                          $ 
                                        $$  
                                     $$$    
          $$$$                 $$$$$$$      
      $$$$$$$$$$          $$$$$$$$$$        
       $$$$$$$$$$$     $$$$$$$$$$$$$$       
          $$$$$$$$$$$$$$$$$$$$$$$$$$$       
            $$$$$$$$$$$$$$$$$$$$$$$$$$      
             $$$$$$$$$$$$$$$$$$$$$$$$$      
              $$$$$$$$$$$$$$$$$$  $$$$$     
              $$$$$$$$$$$$   $$$    $$$$    
             $$$$$$$$$$      $$        $$   
            $$$   $$$$       $$         $   
          $$$    $$$        $           $$  
       $$$     $$$        $$            $$  
    $$$      $$                             
           $$                               

    Pass in model (and data if necessary) files.
    This file will take care of the rest of the Magic!

    Routes
    ------

    `/info`

    Returns general metadata about the model shown below:

        + type
        + expected inputs

    `/predict`

    The API surface to the predict function for each model.

'''
)

app = Flask(__name__)

parser = argparse.ArgumentParser()

parser.add_argument('model', type=str,
                    help='The path to the model file')

parser.add_argument('data', type=str, nargs='?',
                    help='(optional) The path to the data file')

args = parser.parse_args()

ALLOWED_IMAGE_EXTENSIONS = set(['png', 'jpg', 'jpeg', 'gif'])
def allowed_image_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_IMAGE_EXTENSIONS


data = None
if args.data != None:
    data = tc.SFrame(args.data)

model = tc.load_model(args.model)


@app.route('/info')
def model_info():
    result = dict()
    result["model"] = str(type(model))
    if type(model) == tc.toolkits.style_transfer.style_transfer.StyleTransfer:
        result["feature"] = model.content_feature
        result["num_styles"] = model.num_styles
    else:
        result["features"] = model.features
    if type(model) == tc.toolkits.activity_classifier.ActivityClassifier:
        result["session_id"] = model.session_id
    result["classes"] = model.classes
    result["max_iterations"] = model.max_iterations
    try:
        result["training_accuracy"] = model.training_accuracy
    except:
        result["training_accuracy"] = model.training_log_accuracy
    try:
        result["training_loss"] = model.training_log_loss
    except:
        result["training_loss"] = model.training_loss
    result["training_time"] = model.training_time
    result["training_accuracy"] = model.training_accuracy

    return return_success(result, True)

@app.route('/predict', methods=['GET', 'POST'])
def model_predict():
    if request.method == 'POST':
        if type(model) == tc.toolkits.image_classifier.image_classifier.ImageClassifier or \
           type(model) == tc.toolkits.drawing_classifier.drawing_classifier.DrawingClassifier or \
           type(model) == tc.toolkits.object_detector.object_detector.ObjectDetector:

            if 'file' not in request.files:
                return return_success({"error": "No selected file"}, False)
            file = request.files['file']
            if file.filename == '':
                return return_success({"error": "No selected file"}, False)
            if file and allowed_image_file(file.filename):
                img = Image.open(request.files['file'].stream)
                if img.mode == 'L':
                    channels = 1
                elif img.mode == 'RGB':
                    channels = 3
                elif img.mode == 'RGBA':
                    channels = 4
                else:
                    return return_success({"error": "Image Format Not Supported"}, False)
                img_bytes = img.tobytes()
                tc_image = tc.Image(_image_data=img_bytes, 
                                    _width=img.width, 
                                    _height=img.height, 
                                    _channels=channels, 
                                    _format_enum=2, 
                                    _image_data_size=len(img_bytes))
                sf_image = tc.SFrame({info["feature"]: [tc_image]});
                prediction = model.predict(sf_image)[0]

                key = "label"
                if info["type"] == ModelType.object_detection:
                    key = "bounding_box"

                return return_success({key: prediction}, True)
        elif type(model) == tc.toolkits.style_transfer.style_transfer.StyleTransfer:
            style_idx = None
            if request.form['idx'] != None:
                style_idx = [int(request.form['idx'])]

            if 'file' not in request.files:
                return return_success({"error": "No selected file"}, False)
            file = request.files['file']
            if file.filename == '':
                return return_success({"error": "No selected file"}, False)
            if file and allowed_image_file(file.filename):
                img = Image.open(request.files['file'].stream)
                if img.mode == 'L':
                    channels = 1
                elif img.mode == 'RGB':
                    channels = 3
                elif img.mode == 'RGBA':
                    channels = 4
                else:
                    return return_success({"error": "Image Format Not Supported"}, False)
                img_bytes = img.tobytes()
                tc_image = tc.Image(_image_data=img_bytes, 
                                    _width=img.width, 
                                    _height=img.height, 
                                    _channels=channels, 
                                    _format_enum=2, 
                                    _image_data_size=len(img_bytes))
                
                sf_image = tc.SFrame({info["feature"]: [tc_image]})
                if style_idx != None:
                    stylized_image = model.stylize(sf_image, style_idx)["stylized_image"]
                else:
                    stylized_image = model.stylize(sf_image)["stylized_image"]

                images = list()
                for i in range(0,len(stylized_image)):
                    images.append(Image.fromarray(np.uint8(stylized_image[i].pixel_data)))

                if(len(images) == 1):
                    return serve_pil_image(images[0])
                else:
                    mf = StringIO()
                    with zipfile.ZipFile(mf, mode='w', compression=zipfile.ZIP_DEFLATED) as zf:
                        for i in range(0, len(images)):
                            img_io = StringIO()
                            images[i].save(img_io, 'JPEG', quality=70)
                            img_io.seek(0)
                            zf.writestr(str(i) + '.jpg', img_io.getvalue())
                    return send_file(mf, mimetype='application/zip')

        elif type(model) == tc.toolkits.activity_classifier.ActivityClassifier or \
             type(model) == tc.toolkits.text_classifier.TextClassifier or \
             type(model) == tc.toolkits.classifier.random_forest_classifier.RandomForestClassifier or \
             type(model) == tc.toolkits.classifier.decision_tree_classifier.DecisionTreeClassifier or \
             type(model) == tc.toolkits.regression.boosted_trees_regression.BoostedTreesRegression or \
             type(model) == tc.toolkits.classifier.nearest_neighbor_classifier.NearestNeighborClassifier or \
             type(model) == tc.toolkits.classifier.svm_classifier.SVMClassifier or \
             type(model) == tc.toolkits.regression.random_forest_regression.RandomForestRegression or \
             type(model) == tc.toolkits.regression.decision_tree_regression.DecisionTreeRegression or \
             type(model) == tc.toolkits.regression.linear_regression.LinearRegression or \
             type(model) == tc.toolkits.regression.boosted_trees_regression.BoostedTreesRegression or \
             type(model) == tc.toolkits.clustering.kmeans.KmeansModel or \
             type(model) == tc.toolkits.recommender.item_similarity_recommender.ItemSimilarityRecommender or \
             type(model) == tc.toolkits.recommender.item_content_recommender.ItemContentRecommender or \
             type(model) == tc.toolkitstc.toolkits.recommender.popularity_recommender.PopularityRecommender or \
             type(model) == tc.toolkits.recommender.ranking_factorization_recommender.RankingFactorizationRecommender:

            loaded_data = request.get_json()
            data = json.dumps(loaded_data)
            loaded_data = json.loads(data)

            inp = {}
            for key in loaded_data.keys():
                if type(loaded_data[key])==list:
                    inp[key]=loaded_data[key]
                else:
                    inp[key]=[loaded_data[key]]
            sf_data = tc.SFrame(inp)
            prediction =  model.predict(sf_data)
            probabilities = model.predict(sf_data, "probability_vector")
            result = dict()
            result["predicted_class"] = []
            for p in prediction:
                result["predicted_class"].append(p)
            for j in range(len(probabilities)):
                for i in range(len(model.classes)):
                    if model.classes[i] in result:
                        result[str(model.classes[i])].append(probabilities[j][i])
                    else:
                        result[str(model.classes[i])] = [probabilities[j][i]]


            return return_success(result, True)

        else:
            return return_success({"error": "Unimplemented Toolkit"}, False)
    return return_success({}, True)



if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')