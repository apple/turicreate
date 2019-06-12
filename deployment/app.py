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

def json_serializable(x):
    try:
        json.dumps(x)
        return True
    except:
        return False

def return_success(messages, success):
    messages['success'] = success
    return jsonify(messages)

def serve_pil_image(pil_img):
    img_io = StringIO()
    pil_img.save(img_io, 'JPEG', quality=70)
    img_io.seek(0)
    return send_file(img_io, mimetype='image/jpeg')


print( 
'''
    +----------------------------------------+
    |                                        |
    |      Turi Create Inference Engine      |
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
# Set up the command-line options
default_host="0.0.0.0"
default_port="5000"
parser = argparse.ArgumentParser()

parser.add_argument('model', type=str,
                    help='The path to the model file')

parser.add_argument('data', type=str, nargs='?',
                    help='(optional) The path to the data file')

parser.add_argument('--debug', dest='debug', action='store_true', 
                    help='print debug messages to stderr')

args = parser.parse_args()

ALLOWED_IMAGE_EXTENSIONS = set(['png', 'jpg', 'jpeg', 'gif'])
ALLOWED_AUDIO_EXTENSIONS = set(['wav', 'mp3'])

def allowed_image_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_IMAGE_EXTENSIONS

def allowed_audio_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_AUDIO_EXTENSIONS

data = None
if args.data != None:
    data = tc.SFrame(args.data)

model = tc.load_model(args.model)

@app.route('/info')
def model_info():
    result = dict()
    result["model"] = str(type(model))
    for field in model._list_fields():
        result[field] = model._get(field) if json_serializable(model._get(field)) else str(model._get(field))
    return return_success(result, True)

@app.route('/predict', methods=['GET', 'POST'])
def model_predict():
    if request.method == 'POST':
        if type(model) == tc.toolkits.image_classifier.image_classifier.ImageClassifier or \
           type(model) == tc.toolkits.drawing_classifier.drawing_classifier.DrawingClassifier or \
           type(model) == tc.toolkits.object_detector.object_detector.ObjectDetector or \
           type(model) == tc.toolkits.one_shot_object_detector.one_shot_object_detector.OneShotObjectDetector:
            images = []
            result = dict()
            result['image'] = []
            result['confidence'] = []
            result['predicted_class'] = []
            result['x'] = []
            result['y'] = []
            result['width'] = []
            result['height'] = []
            if 'file' not in request.files:
                return return_success({"error": "No selected file"}, False)
            files = request.files.getlist("file")
            for file in files :
                if file.filename == '':
                    return return_success({"error": "No selected file"}, False)
                if file and allowed_image_file(file.filename):
                    img = Image.open(file.stream)
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
                    
                    images.append(tc_image)
                    

            sf_image = tc.SFrame({model.feature : images})
                    
            if type(model) == tc.toolkits.image_classifier.image_classifier.ImageClassifier or \
                type(model) == tc.toolkits.drawing_classifier.drawing_classifier.DrawingClassifier :
                prediction = model.predict(sf_image, output_type="class")
                probabilities = model.predict(sf_image, output_type="probability_vector")
                
                for p in prediction:
                    result["predicted_class"].append(p)
                for j in range(len(probabilities)):
                    for i in range(len(model.classes)):
                        if model.classes[i] in result:
                            result[str(model.classes[i])].append(probabilities[j][i])
                        else:
                            result[str(model.classes[i])] = [probabilities[j][i]]
            else:                
                prediction = model.predict(dataset=sf_image)
                
                for images in prediction:
                    for image in range(len(images)):
                        result['confidence'].append(images[image]['confidence'])
                        result['predicted_class'].append(images[image]['label'])
                        result['x'].append(images[image]['coordinates']['x'])
                        result['y'].append(images[image]['coordinates']['y'])
                        result['width'].append(images[image]['coordinates']['width'])
                        result['height'].append(images[image]['coordinates']['height'])
                return return_success(result, True)

        elif type(model) == tc.toolkits.style_transfer.style_transfer.StyleTransfer:
            
            try:
                style_idx = [int(request.form['idx'])]
            except:
                style_idx = None
            if 'file' not in request.files:
                return return_success({"error": "No selected file"}, False)
            files = request.files.getlist("file")
            style_images = list()
            images = list()
            for file in files:
                if file.filename == '':
                    return return_success({"error": "No selected file"}, False)
                if file and allowed_image_file(file.filename):
                    img = Image.open(file.stream)
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
                    style_images.append(tc_image)
            sf_image = tc.SFrame({model.content_feature: style_images})
            stylized_image = model.stylize(sf_image, style_idx)["stylized_image"]
                    
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
             type(model) == tc.toolkits.regression.boosted_trees_classification.BoostedTreesClassification or \
             type(model) == tc.toolkits.classifier.nearest_neighbor_classifier.NearestNeighborClassifier or \
             type(model) == tc.toolkits.classifier.svm_classifier.SVMClassifier or \
             type(model) == tc.toolkits.regression.random_forest_regression.RandomForestRegression or \
             type(model) == tc.toolkits.regression.decision_tree_regression.DecisionTreeRegression or \
             type(model) == tc.toolkits.regression.linear_regression.LinearRegression or \
             type(model) == tc.toolkits.regression.boosted_trees_regression.BoostedTreesRegression or \
             type(model) == tc.toolkits.clustering.kmeans.KmeansModel:

            if 'file' not in request.files:
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
            else:
                file = request.files['file']
                if file and '.' in file.filename and file.filename.rsplit('.', 1)[1].lower()=='csv':
                    sf_data = tc.SFrame.read_csv(file)
                else:
                    return return_success({"error": "File format not supported"}, False)

            result = dict()

            if type(model) == tc.toolkits.activity_classifier.ActivityClassifier or \
             type(model) == tc.toolkits.classifier.random_forest_classifier.RandomForestClassifier or \
             type(model) == tc.toolkits.classifier.decision_tree_classifier.DecisionTreeClassifier or \
             type(model) == tc.toolkits.regression.boosted_trees_classification.BoostedTreesClassification or \
             type(model) == tc.toolkits.classifier.nearest_neighbor_classifier.NearestNeighborClassifier or \
             type(model) == tc.toolkits.classifier.svm_classifier.SVMClassifier:

                prediction =  model.predict(dataset=sf_data, output_type="class")
                probabilities = model.predict(sf_data, output_type="probability_vector")
                
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

            elif type(model) == tc.toolkits.regression.random_forest_regression.RandomForestRegression or \
                 type(model) == tc.toolkits.regression.decision_tree_regression.DecisionTreeRegression or \
                 type(model) == tc.toolkits.regression.linear_regression.LinearRegression or \
                 type(model) == tc.toolkits.regression.boosted_trees_regression.BoostedTreesRegression:

                prediction =  model.predict(dataset=sf_data)
                result["estimated_value"] = []
                for p in prediction:
                    result["estimated_value"].append(p)
                return return_success(result, True) 

            elif type(model) == tc.toolkits.clustering.kmeans.KmeansModel:
                prediction =  model.predict(dataset=sf_data)
                result["clusters"] = []
                for p in prediction:
                    result["clusters"].append(p)
                return return_success(result, True) 

        elif type(model) == tc.toolkits.sound_classifier.sound_classifier.SoundClassifier:

            loaded_data = request.get_json()
            data = json.dumps(loaded_data)
            loaded_data = json.loads(data)
            print(loaded_data)
            inp = {}
            for key in loaded_data.keys():
                print(key)
                if type(loaded_data[key])==list or key == model.feature:
                    inp[key]=loaded_data[key]
                else:
                    inp[key]=[loaded_data[key]]
            audio_data = tc.SFrame(inp)
            print(audio_data)
                # from os.path import basename
                # if 'file' not in request.files:
                #     return return_success({"error": "No selected file"}, False)
                # result = dict()
                # result["predicted_class"] = list()
                # files = request.files.getlist("file")
                # for file in files:
                #     if file.filename == '':
                #         return return_success({"error": "No selected file"}, False)
                #     if file and allowed_audio_file(file.filename):
            # audio_data = tc.SFrame()
            # audio_data['filename'] = audio_data['path'].apply(lambda p: basename(p))
            # audio_data = audio_data.join(data)
            prediction = model.predict(audio_data)
            print(prediction)
            for p in prediction:
                result["predicted_class"].append(p)
            for j in range(len(probabilities)):
                for i in range(len(model.classes)):
                    if model.classes[i] in result:
                        result[str(model.classes[i])].append(probabilities[j][i])
                    else:
                        result[str(model.classes[i])] = [probabilities[j][i]]
            return return_success(result, True)

        elif type(model) == tc.toolkits.recommender.item_similarity_recommender.ItemSimilarityRecommender or \
             type(model) == tc.toolkits.recommender.item_content_recommender.ItemContentRecommender or \
             type(model) == tc.toolkitstc.toolkits.recommender.popularity_recommender.PopularityRecommender or \
             type(model) == tc.toolkits.recommender.ranking_factorization_recommender.RankingFactorizationRecommender or \
             type(model) == tc.toolkits.recommender.factorization_recommender.FactorizationRecommender:
            return return_success({"error": "Recommenders use recommend method instead of predict"}, False)
        else:
            return return_success({"error": "Unimplemented Toolkit"}, False)
    return return_success({}, True)

@app.route('/recommend', methods=['GET', 'POST'])
def recommend():
    if type(model) == tc.toolkits.recommender.item_similarity_recommender.ItemSimilarityRecommender or \
       type(model) == tc.toolkits.recommender.item_content_recommender.ItemContentRecommender or \
       type(model) == tc.toolkitstc.toolkits.recommender.popularity_recommender.PopularityRecommender or \
       type(model) == tc.toolkits.recommender.ranking_factorization_recommender.RankingFactorizationRecommender or \
       type(model) == tc.toolkits.recommender.factorization_recommender.FactorizationRecommender:

        loaded_data = request.get_json()
        data = json.dumps(loaded_data)
        loaded_data = json.loads(data)

        result = dict()
        recommendations = model.recommend_from_interactions(loaded_data.values())
        column_names = recommendations.column_names()
        for r in range(len(column_names)):
            result[str(column_names[r])] = list(recommendations[str(column_names[r])])
        return return_success(result, True)
    else:
        return return_success({"error": "Other toolkits use predict method instead of recommend"}, False)


if __name__ == '__main__':
    app.run(debug= args.debug, host="0.0.0.0")