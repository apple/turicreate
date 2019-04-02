import React, { Component } from 'react';
import style from './index.module.scss';

import { Root } from 'protobufjs';
import messageFormat from '../../format/message';

import InfiniteScroll from './InfiniteScroll';
import StatusBar from './StatusBar';
import SingleImage from './SingleImage';
import LabelContainer from './LabelContainer';
import LabelModal from './LabelModal';
import ErrorBar from './ErrorBar';
import NavigationBar from './NavigationBar';
import { LabelType } from './utils';

const DEFAULT_NUM_EXPECTED = 10;
const CACHE_SIZE = 1000;

/* TODO: Indicator to Show how many Images were Annotated */
/* TODO: Render Labels on Images */

class Annotate extends Component {
  constructor(props){
    super(props);
    this.state = {
      infiniteScroll: false,
      hideAnnotated: false,
      labelModal: false,
      totalError: "",
      labelCreationError: null,
      incrementalCurrentIndex: 0,
      LabelModalValue: "",
      imageData: {},
      annotationData: {},
      infiniteSelected:{},
      labels:[],
      type:null
    }
  }

  componentDidMount() {
    var labeltype;
    var propLabels = this.props.metadata.imageClassification.label;

    if (propLabels.length > 0) {
      if(Object.keys(propLabels[0]).includes("stringLabel")){
        labeltype = LabelType.STRING;
      }else{
        labeltype = LabelType.INTEGER;
      }
    } else {
      labeltype = LabelType.STRING;
    }

    if (labeltype == LabelType.STRING) {
      propLabels = propLabels.filter(label => label.stringLabel != "");
    }
    
    const labels = propLabels.map((x) => {
      return {
        name: (labeltype == LabelType.INTEGER)?x.intLabel:x.stringLabel,
        num_annotated: x.elementCount,
        num_expected: DEFAULT_NUM_EXPECTED
      }
    });

    this.setState({
      labels: labels,
      type: labeltype
    });
  }

  cleanCache = (start, end) => {
    if (Object.keys(this.state.imageData).length > CACHE_SIZE) {
      var sorted_keys = Object.keys(this.state.imageData).sort();
      var lower_bound = sorted_keys[0];
      var upper_bound = sorted_keys[sorted_keys.length - 1];

      var intermediate_check = parseInt(((end - start)/2), 10);
      if((Math.abs(intermediate_check - lower_bound)) > (Math.abs(intermediate_check - upper_bound))){
        var delete_keys = sorted_keys.slice(lower_bound, (lower_bound + (end - start)));
        const imgData = this.state.imageData;
        for(var x = 0; x < delete_keys.length; x++){
          delete imgData[delete_keys[x]];
        }
        this.setState({
          imageData: imgData
        });
      }else{
        var delete_keys = sorted_keys.slice((upper_bound - (end - start)), upper_bound);
        const imgData = this.state.imageData;
        for(var x = 0; x < delete_keys.length; x++){
          delete imgData[delete_keys[x]];
        }
        this.setState({
          imageData: imgData
        });
      }
    }
  }
  

  getData = (start, end) => {
    this.cleanCache(start, end);
    this.getAnnotations(start, end);
    this.getHelper(start, end, 0);
  }

  getAnnotations = (start, end) => {
    this.getHelper(start, end, 1);
  }

  getHelper = (start, end, type) => {
    const root = Root.fromJSON(messageFormat);
    const ParcelMessage = root.lookupType("TuriCreate.Annotation.Specification.ClientRequest");
    const payload = {"getter": {"type": type, "start": start, "end": end}};
    const err = ParcelMessage.verify(payload);
    if (err)
      throw Error(err);
    
    const message = ParcelMessage.create(payload);
    const buffer = ParcelMessage.encode(message).finish();
    const encoded = btoa(String.fromCharCode.apply(null, buffer));

    if (window.navigator.platform == 'MacIntel') {
      window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'writeProtoBuf', message: encoded});
    } else {
      window.postMessageToNativeClient(encoded);
    }
  }

  setAnnotationData = (key, value) => {
    var previousAnnotationData = this.state.annotationData;

    if (this.state.type == LabelType.STRING) {
      previousAnnotationData[key] = value.stringLabel;
    } else if(this.state.type == LabelType.INTEGER) {
      previousAnnotationData[key] = value.intLabel;
    }

    this.setState({
      annotationData: previousAnnotationData
    });
  }

  setImageData = (key, value, width, height) => {
    var previousImageData = this.state.imageData;
    previousImageData[key] = {src:value, width:width, height:height};
    this.setState({
      imageData: previousImageData
    });
  }


  clearError = () => {
    this.setState({
      totalError: ""
    });
  }

  updateIncrementalCurrentIndex = (index) => {
    this.setState({
      incrementalCurrentIndex: index
    })
  }

  handleEventLabelModalValue = (e) => {
    this.setState({
      LabelModalValue: e.target.value
    });
  }
  
  openLabelModal = () => {
    this.setState({
      labelCreationError: null,
      labelModal: true,
      LabelModalValue: ""
    });
  }

  closeLabelModal = () => {
    this.setState({
      labelCreationError: null,
      labelModal: false,
      LabelModalValue: ""
    });
  }

  addToSelected = (index) => {
    var infiniteSelected = this.state.infiniteSelected;

    if (infiniteSelected[parseInt(index, 10)]!= null) {
      delete infiniteSelected[parseInt(index, 10)];
    } else {
      infiniteSelected[parseInt(index, 10)] = true;
    }
    
    this.setState({
      infiniteSelected: infiniteSelected
    });
  }

  removeSelected = () => {
    this.setState({
      infiniteSelected:{}
    })
  }

  createLabel = (label) => {
    const notDuplicateLabel = this.state.labels.map(l => (l.name != label))
                                            .reduce((acc, b) => (acc && b), true);

    if(!notDuplicateLabel){
      this.setState({
        labelCreationError: `${label} already exists as a label name`
      });
      return;
    }

    var mutatorLabels = this.state.labels;
    
    mutatorLabels.push({
      name: label,
      num_annotated: 0,
      num_expected: DEFAULT_NUM_EXPECTED
    });

    this.setState({
      labels: mutatorLabels,
      labelCreationError: null,
      labelModal: false,
      LabelModalValue: ""
    });
  }

  toggleInfiniteScroll = () => {
    this.setState({
      infiniteScroll: !this.state.infiniteScroll
    });
  }

  toggleHideAnnotated = () => {
    this.setState({
      hideAnnotated: !this.state.hideAnnotated
    });
  }

  setAnnotationMass = (name) => {
    const selectedValue = Object.keys(this.state.infiniteSelected);
    for (var i = 0; i < selectedValue.length; i++) {
      this.setAnnotation(parseInt(selectedValue[i], 10), name);
    }
  }

  setAnnotation = (rowIndex, labels) => {
    var previousAnnotationData = this.state.annotationData;
    var previousLabelData = this.state.labels;
    var previousLabel = previousAnnotationData[rowIndex];

    if(previousLabel == labels){
      return;
    }

    if(previousLabel != null){
      for (var x = 0; x < previousLabelData.length; x++) {
        if(this.state.labels[x].name == previousLabel) {
          var tempLabel = previousLabelData[x];
          tempLabel.num_annotated -= 1;
          previousLabelData[x] = tempLabel;
        }

        if(this.state.labels[x].name == labels){
          var tempLabel = previousLabelData[x];
          tempLabel.num_annotated += 1;
          previousLabelData[x] = tempLabel;
        }
      }
    }

    if (this.state.type == LabelType.STRING) {
      previousAnnotationData[rowIndex] = labels;
    } else if(this.state.type == LabelType.INTEGER) {
      previousAnnotationData[rowIndex] = parseInt(labels, 10);
    }


    const root = Root.fromJSON(messageFormat);
    const ParcelMessage = root.lookupType("TuriCreate.Annotation.Specification.ClientRequest");
    
    var payload;

    if (this.state.type == LabelType.STRING) {
      payload = {"annotations": {"annotation":[{"labels": [{"stringLabel": labels}], "rowIndex": [rowIndex]}]}};
    } else {
      payload = {"annotations": {"annotation":[{"labels": [{"intLabel": parseInt(labels, 10)}], "rowIndex": [rowIndex]}]}};
    }

    const err = ParcelMessage.verify(payload);
    
    if (err)
      throw Error(err);

    const message = ParcelMessage.create(payload);
    const buffer = ParcelMessage.encode(message).finish();
    const encoded = btoa(String.fromCharCode.apply(null, buffer));

    if (window.navigator.platform == 'MacIntel') {
      window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'writeProtoBuf', message: encoded});
    } else {
      window.postMessageToNativeClient(encoded);
    }

    this.setState({
      annotationData: previousAnnotationData,
      labels:previousLabelData
    });
  }

  renderMainContent = () => {
    if(this.state.infiniteScroll) {
      return (
        <InfiniteScroll numElements={this.props.metadata.numExamples}
                        hideAnnotated={this.state.hideAnnotated}
                        incrementalCurrentIndex={this.state.incrementalCurrentIndex}
                        updateIncrementalCurrentIndex={this.updateIncrementalCurrentIndex.bind(this)}
                        toggleInfiniteScroll={this.toggleInfiniteScroll.bind(this)}
                        infiniteSelected={this.state.infiniteSelected}
                        imageData={this.state.imageData}
                        annotationData={this.state.annotationData}
                        getData={this.getData.bind(this)}
                        type={this.state.type}
                        addToSelected={this.addToSelected.bind(this)}
                        removeSelected={this.removeSelected.bind(this)}
                        getAnnotations={this.getAnnotations.bind(this)} />
      );
    } else {
      return (
        <SingleImage src={this.state.imageData[this.state.incrementalCurrentIndex]}
                     getData={this.getData.bind(this)}
                     getAnnotations={this.getAnnotations.bind(this)}
                     numElements={this.props.metadata.numExamples}
                     incrementalCurrentIndex={this.state.incrementalCurrentIndex}
                     updateIncrementalCurrentIndex={this.updateIncrementalCurrentIndex.bind(this)}/>
      );
    }
  }

  renderModal = () => {
    if(this.state.labelModal){
      return(
        <LabelModal closeLabelModal={this.closeLabelModal.bind(this)}
                    createLabel={this.createLabel.bind(this)}
                    error={this.state.labelCreationError}
                    LabelModalValue={this.state.LabelModalValue}
                    handleEventLabelModalValue={this.handleEventLabelModalValue.bind(this)}/>
      )
    }
  }

  renderError = () => {
    if(this.state.totalError){
      return (<ErrorBar message={this.state.totalError}
                        clearError={this.clearError.bind(this)}/>);
    }
  }

  render() {
    return (
      <div className={style.Annotate}>
        {this.renderModal()}
        {this.renderError()}
        <div>
            {/* Add Annotated to Protobuf Message */}
            <StatusBar total={this.props.metadata.numExamples}
                       infiniteScroll={this.state.infiniteScroll}
                       toggleInfiniteScroll={this.toggleInfiniteScroll.bind(this)}
                       removeSelected={this.removeSelected.bind(this)}
                       hideAnnotated={this.state.hideAnnotated}
                       toggleHideAnnotated={this.toggleHideAnnotated.bind(this)}/>

            {this.renderMainContent()}
        <NavigationBar infiniteScroll={this.state.infiniteScroll}
                       toggleInfiniteScroll={this.toggleInfiniteScroll.bind(this)}
                       updateIncrementalCurrentIndex={this.updateIncrementalCurrentIndex.bind(this)}
                       getData={this.getData.bind(this)}
                       getAnnotations={this.getAnnotations.bind(this)}/>
        </div>
        <div className={style.leftBar}>
        <LabelContainer labels={this.state.labels}
                        incrementalCurrentIndex={this.state.incrementalCurrentIndex}
                        infiniteScroll={this.state.infiniteScroll}
                        infiniteSelected={this.state.infiniteSelected}
                        setAnnotation={this.setAnnotation.bind(this)}
                        setAnnotationMass={this.setAnnotationMass.bind(this)}
                        openLabelModal={this.openLabelModal.bind(this)}
                        closeLabelModal={this.closeLabelModal.bind(this)}
                        annotationData={this.state.annotationData}/>

        </div>
      </div>
    );
  }
}

export default Annotate;
