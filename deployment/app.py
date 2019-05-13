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

class ModelType(Enum):
    recommender = 0
    image_classifier = 1 #
    drawing_classification = 2 #
    sound_classification = 3
    object_detection = 4 # -
    style_transfer = 5 #
    activity_classification = 6 #
    image_similarity = 7
    classifiers = 8
    regression = 9
    clustering = 10
    text_classification = 11 #-

def return_success(messages, success):
    messages['success'] = success
    return jsonify(messages)

def serve_pil_image(pil_img):
    img_io = StringIO()
    pil_img.save(img_io, 'JPEG', quality=70)
    img_io.seek(0)
    return send_file(img_io, mimetype='image/jpeg')

def ModelType2String(info):
    info = info.copy()
    if(info["type"] == ModelType.recommender):
        info["type"] = "Recommender"
        return info
    if(info["type"] == ModelType.image_classifier):
        info["type"] = "Image Classifier"
        return info
    if(info["type"] == ModelType.drawing_classification):
        info["type"] = "Drawing Classifier"
        return info
    if(info["type"] == ModelType.sound_classification):
        info["type"] = "Sound Classifier"
        return info
    if(info["type"] == ModelType.object_detection):
        info["type"] = "Object Detection"
        return info
    if(info["type"] == ModelType.style_transfer):
        info["type"] = "Style Transfer"
        return info
    if(info["type"] == ModelType.activity_classification):
        info["type"] = "Activity Classifier"
        return info
    if(info["type"] == ModelType.image_similarity):
        info["type"] = "Image Similarity"
        return info
    if(info["type"] == ModelType.classifiers):
        info["type"] = "Classifiers"
        return info
    if(info["type"] == ModelType.regression):
        info["type"] = "Regression"
        return info
    if(info["type"] == ModelType.clustering):
        info["type"] = "Clustering"
        return info
    if(info["type"] == ModelType.text_classification):
        info["type"] = "Text Classifier"
        return info
    return info


print( 
'''
    +----------------------------------------+
    |                                        |
    |      TuriCreate Inference Engine       |
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

    Pass in Model (and Data if necessary) Files.
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

info = dict()

if (type(model) == tc.toolkits.image_classifier.image_classifier.ImageClassifier):
    info["type"] = ModelType.image_classifier
    info["model"] = model.model
    info["classes"] = model.classes
    info["feature"] = model.feature
    info["max_iterations"] = model.max_iterations
    info["training_loss"] = model.training_loss
    info["training_time"] = model.training_time
elif (type(model) == tc.toolkits.drawing_classifier.drawing_classifier.DrawingClassifier):
    info["type"] = ModelType.drawing_classification
    info["classes"] = model.classes
    info["feature"] = model.feature
    info["max_iterations"] = model.max_iterations
    info["training_loss"] = model.training_loss
    info["training_time"] = model.training_time
elif (type(model) == tc.toolkits.style_transfer.style_transfer.StyleTransfer):
    info["type"] = ModelType.style_transfer
    info["feature"] = model.content_feature
    info["num_styles"] = model.num_styles
    info["max_iterations"] = model.max_iterations
    info["training_iterations"] = model.training_iterations
    info["training_time"] = model.training_time
elif (type(model) == tc.toolkits.object_detector.object_detector.ObjectDetector):
    info["type"] = ModelType.object_detection
elif (type(model) == tc.toolkits.activity_classifier.ActivityClassifier):
    info["type"] = ModelType.activity_classification
    info["features"] = model.features
    info["training_time"] = model.training_time
    info["classes"] = model.classes
    info["session_id"] = model.session_id
elif (type(model) ==  tc.toolkits.text_classifier.TextClassifier):
    info["type"] = ModelType.text_classification
    info["features"] = model.features
else:
    exit()

@app.route('/info')
def model_info():
    return return_success(ModelType2String(info), True)

@app.route('/predict', methods=['GET', 'POST'])
def model_predict():
    if request.method == 'POST':
        if info["type"] == ModelType.image_classifier or \
           info["type"] == ModelType.drawing_classification or \
           info["type"] == ModelType.object_detection:

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
        elif info["type"] == ModelType.style_transfer:
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

        elif info["type"] ==  ModelType.activity_classification :
            loaded_data = request.get_json()
            data = json.dumps(loaded_data)
            loaded_data = json.loads(data)
            inp = {}
            for key in loaded_data.keys():
                inp[key]=[loaded_data[key]]
            sf_data = tc.SFrame(inp)
            prediction =  model.predict(sf_data)
            return return_success({"label": str(prediction[0])}, True)

        elif info["type"] == ModelType.text_classification:
            loaded_data = request.get_json()
            data = json.dumps(loaded_data)
            loaded_data = json.loads(data)
            inp = {}
            for key in loaded_data.keys():
                inp[key]=[loaded_data[key]]
            sf_data = tc.SFrame(inp)
            prediction =  model.predict(sf_data)
            return return_success({"label": str(prediction[0])}, True)

    


        else:
            return return_success({"error": "Unimplemented Toolkit"}, False)
    return return_success({}, True)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
