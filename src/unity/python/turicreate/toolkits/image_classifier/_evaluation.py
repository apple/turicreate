# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and utilities for the evaluation of the image classification model
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ...visualization import _get_client_app_path

import subprocess as __subprocess
from six.moves import _thread

import json as _json
import base64 as _base64
import sys as _sys
import math as _math

class Evaluation(dict):
  def __init__(self, obj = {}):
    dict.__init__(self)
    self.data = obj
    self.update(obj)

  def _get_eval_json(self):
    evaluation_dictionary = dict()

    for key, value in self.data.iteritems():
      if (isinstance(value, float) or isinstance(value, int)) and _math.isnan(value):
        continue
      if (key is "test_data"):
        continue
      evaluation_dictionary[key] = value

    evaluation_dictionary["table_spec"] = {
      "column_names": ["path", "image", "target_label", "predicted_label"],
      "size": len(self.data["test_data"]),
      "title": "",
      "column_types": ["string", "image", "string", "string"]
    }

    evaluation_dictionary["corrects_size"] = len(self.data["test_data"].filter_by([True], "correct"))
    evaluation_dictionary["incorrects_size"] = evaluation_dictionary["table_spec"]["size"] - evaluation_dictionary["corrects_size"]
    
    return str(_json.dumps({ "evaluation_spec": evaluation_dictionary }, allow_nan = False))

  def _explore(self):
    _thread.start_new_thread(_start_process, (self._get_eval_json()+"\n", self.data["test_data"], self, ))


def _get_data_spec(filters, start, length, row_type, mat_type, sframe, evaluation):
  data_spec = None
  sf = sframe    
  if row_type == "all":
    
    table_data = _filter_sframe(filters, "table", mat_type, sf, evaluation)
    table_data_sliced = _reform_sframe(table_data[(table_data['__idx'] >= start)].head(length)) 

    corrects = _filter_sframe(filters, "corrects", mat_type, sf, evaluation)
    corrects_sliced = _reform_sframe(corrects[(corrects["__idx"] >= start)].head(length))

    incorrects = _filter_sframe(filters, "incorrects", mat_type, sf, evaluation)
    incorrects_sliced = _reform_sframe(incorrects[(incorrects["__idx"] >= start)].head(length))

    data_spec = { 
      "data_spec": { 
        "table": { "data": { "values": table_data_sliced }, "size": table_data.num_rows() },
        "gallery": { 
          "corrects": { "data": corrects_sliced, "size": corrects.num_rows() }, 
          "incorrects": { "data": incorrects_sliced, "size": incorrects.num_rows() }
        }
      }
    }
  else:
    sf = _filter_sframe(filters, row_type, mat_type, sf, evaluation)
    list_test_data = _reform_sframe(sf[(sf['__idx'] >= start)].head(length))
    if (row_type == "table"):
      data_spec = { "data_spec": { "table": { "data": { "values": list_test_data }, "size": sf.num_rows() }}}
    elif (row_type == "corrects"):
      data_spec = { "data_spec": { "gallery": { "corrects": { "data": list_test_data, "size": sf.num_rows() }}}}
    elif (row_type == "incorrects"):
      data_spec = { "data_spec": { "gallery": { "incorrects": { "data": list_test_data, "size": sf.num_rows() }}}}

  return str(_json.dumps(data_spec))
  
def _reform_sframe(sf):
  sf_sending_data = sf.select_columns(["__idx", "path", "target_label", "predicted_label", "confidence", "relative_confidence", "entropy"])
  sf_sending_data["image"] = sf["image"].apply(_image_conversion)
  sf_sending_data["probs"] = sf["probs"].astype(list)
  return list(sf_sending_data)
  
def _filter_sframe(filters, row_type, mat_type, sf, evaluation):
  conf_metric = evaluation["confidence_metric_for_threshold"]

  if mat_type == "conf_wrong":
    sf = sf[sf[conf_metric] > evaluation["confidence_threshold"]]
  elif mat_type == "hesitant":
    sf = sf[sf[conf_metric] < evaluation["hesitant_threshold"]]

  filtered_array = None
  if row_type == "corrects":
    sf = sf.filter_by([True], "correct")
  elif row_type == "incorrects":
    sf = sf.filter_by([False], "correct")

  if len(filters) is 0:
    return sf

  for f in filters:
    target_label = (f['target_label'])
    predicted_label = (f['predicted_label'])
    filtered_sframe = sf.filter_by([target_label], "target_label").filter_by([predicted_label], "predicted_label")
    if filtered_array is None:
      filtered_array = filtered_sframe
    else:
      filtered_array = filtered_array.append(filtered_sframe)
  
  return filtered_array

def __get_incorrects(label, sf, evaluation):
  conf_metric = evaluation["confidence_metric_for_threshold"]
  
  sf = sf.filter_by([False], "correct")
  
  filtered_sframe = sf.filter_by([label], "target_label")
  unique_predictions = list(filtered_sframe["predicted_label"].unique())
  
  data = []
  for u in unique_predictions:
    u_filt = filtered_sframe.filter_by([u], "predicted_label")
    data.append({"label": str(u), "images": list(u_filt["images"].apply(_image_conversion))})

  return {"data_spec": { "incorrects": {"target": label, "data": data }}}

def __get_corrects(sf, evaluation):
  conf_metric = evaluation["confidence_metric_for_threshold"]
  
  sf = sf.filter_by([True], "correct")

  unique_predictions = list(sf["target_label"].unique())
  
  data = []
  for u in unique_predictions:
    u_filt = sf.filter_by([u], "predicted_label")
    data.append({"target": u, "images": list(u_filt["images"].apply(_image_conversion))})
  return {"data_spec": { "correct": data}}

def _process_value(value, extended_sframe, proc, evaluation):
  json_value = None

  try:
    json_value = _json.loads(value)
  except:
    pass

  if json_value != None:
    if(json_value['method'] == "get_rows_eval"):
      proc.stdin.write(_get_data_spec(json_value['cells'], json_value['start'], json_value['length'], json_value['row_type'], json_value['mat_type'], extended_sframe, evaluation)+"\n")
    
    if(json_value['method'] == "get_corrects"):
      proc.stdin.write(str(_json.dumps(__get_corrects(extended_sframe, evaluation)))+"\n")

    if(json_value['method'] == "get_incorrects"):
      proc.stdin.write(str(_json.dumps(__get_incorrects(json_value['label'], extended_sframe, evaluation)))+"\n")

def _start_process(process_input, extended_sframe, evaluation):
  proc = __subprocess.Popen(_get_client_app_path(), stdout=__subprocess.PIPE, stdin=__subprocess.PIPE)
  proc.stdin.write(process_input)
  #https://docs.python.org/2/library/subprocess.html#subprocess.Popen.communicate
  
  while(proc.poll() == None):
    value = proc.stdout.readline()
    if value == '':
      continue

    _process_value(value, extended_sframe, proc, evaluation)
  
  return proc

def _image_conversion(image):
  result = {
    "width": image.width,
    "height": image.height,
    "column": "image",
    "format": "png"
  }

  if(_sys.version_info >= (3, 0)):
    from io import BytesIO

    image_buffer = BytesIO()
    image._to_pil_image().save(image_buffer, format='PNG')
    result["data"] = str(_base64.b64encode(image_buffer.getvalue()))

  else:
    import cStringIO

    image_buffer = cStringIO.StringIO()
    image._to_pil_image().save(image_buffer, format="PNG")
    result["data"] =  str(_base64.b64encode(image_buffer.getvalue()))

  return result
