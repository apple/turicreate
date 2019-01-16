import React, { Component } from 'react';
import './index.scss';

import TCEvaluationTopBar from './Components/TCEvaluationTopBar';
import TCEvaluationNavigationBar from './Components/TCEvaluationNavigationBar';
import TCEvaluationScores from './Components/TCEvaluationScores'
import TCEvaluationScoresConsiderations from './Components/TCEvaluationScoresConsiderations';
import TCEvaluationSummary from './Components/TCEvaluationSummary';
import TCEvaluationLabelMenu from './Components/TCEvaluationLabelMenu';
import TCEvaluationLabelStats from './Components/TCEvaluationLabelStats';
import TCEvaluationIncorrectlyClassified from './Components/TCEvaluationIncorrectlyClassified';
import TCEvaluationConfusionMenu from './Components/TCEvaluationConfusionMenu';

class TCEvaluation extends Component {
  constructor(props){
    super(props);
    this.state = {
      left_selected: true,
      selected_label:null,
      incorrect_classification: this.props.spec.label_metrics.reduce(function(map, data) {
        map[data] = null;
        return map;
      }, {}),
      correct_classification: this.props.spec.label_metrics.reduce(function(map, data) {
        map[data] = [];
        return map;
      }, {}),
      confusions:false,
      selected_confusion:null
    }
  }

  componentDidMount(){
    if(window.navigator.platform === 'MacIntel'){
      window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'ready'});
      window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'getCorrects'});

      this.props.spec.labels.map((value, index) => {
        if(this.state.incorrect_classification[value] == null){
          const previous_incorrect_dict = this.state.incorrect_classification
          previous_incorrect_dict[value] = "loading"
          this.setState({"incorrect_classification": previous_incorrect_dict})
          if (window.navigator.platform == 'MacIntel') {
            window.webkit.messageHandlers["scriptHandler"].postMessage({ 
              "status": 'getIncorrects',
              "label": value + ""
            });
          }
        }
      });
    }
  }

  updateData(data) {
    if("incorrects" in data){
      const inc_class = data["incorrects"];
      const previous_incorrect_dict = this.state.incorrect_classification;
      previous_incorrect_dict[inc_class["target"]] = inc_class["data"];
      this.setState({"incorrect_classification": previous_incorrect_dict})
    }

    if("correct" in data){
      this.setState({"correct_classification": data["correct"]})
    }
  }

  changeSelectedState = (value) => {
    this.setState({
      left_selected: value
    });
  }

  setSelectedLabel = (value) => {
    this.setState({
      selected_label: value
    });
  }

  loadIncorrect = () => {
    if(this.state.selected_label != null) {
      if(this.state.incorrect_classification[this.state.selected_label] == null){
        const previous_incorrect_dict = this.state.incorrect_classification
        previous_incorrect_dict[this.state.selected_label] = "loading"
        this.setState({"incorrect_classification": previous_incorrect_dict})
        if (window.navigator.platform == 'MacIntel') {
          window.webkit.messageHandlers["scriptHandler"].postMessage({ 
            "status": 'getIncorrects',
            "label": this.state.selected_label + ""
          });
        }
      }
    }
  }

  resetSelected = () => {
    this.setState({
      selected_label: null
    });
  }

  renderTopBar = () => {
    return(
      <TCEvaluationTopBar accuracy={this.props.spec.accuracy}
                          iterations={10000}
                          precision={this.props.spec.precision}
                          recall={this.props.spec.recall}
                          model_type={"resnet-50"}
                          correct={this.props.spec.corrects_size}
                          total={this.props.spec.num_examples}
                          classes={this.props.spec.num_classes}
                          selected_label={this.state.selected_label}
                          left_selected={this.state.left_selected}
                          reset={this.resetSelected.bind(this)}/>
    );
  }

  renderSummaryContainer = () => {
    if(!this.state.left_selected && (this.state.selected_label == null)) {
      return (
        <div className={"TCEvaluationSummaryContainer"}>
            {this.summarySeedData().map((value, index) => {
              return (
                <TCEvaluationSummary name={value.name}
                                     images={value.correct_images}
                                     num_examples={value.num_examples}
                                     correct={value.correct}
                                     incorrect={value.incorrect}
                                     f1_score={value.f1_score}
                                     precision={value.precision}
                                     recall={value.recall}
                                     onClick={this.setSelectedLabel.bind(this)}/>
              )
            })}
              
            </div>);
    }
  }

  f1ScoreData = () => {
    return this.props.spec.label_metrics.map((data, index) => ({
      "a": data.label,
      "b": (2*(data.recall * data.precision))/(data.recall + data.precision)
    }));
  }

  PrecisionData = () => {
    return this.props.spec.label_metrics.map((data, index) => ({
      "a": data.label,
      "b": data.precision
    }));
  }

  RecallData = () => {
    return this.props.spec.label_metrics.map((data, index) => ({
      "a": data.label,
      "b": data.recall
    }));
  }

  NumTrainedData = () => {
    return this.props.spec.label_metrics.map((data, index) => ({
      "a": data.label,
      "b": data.count
    }));
  }

  EvaluationScoresData = () => {
    return {
      f1_score: this.f1ScoreData(),
      recall: this.RecallData(),
      precision: this.PrecisionData(),
      num_trained: this.NumTrainedData()
    }
  }

  convertCorrectDict = () => {
    return this.state.correct_classification.reduce(function(map, data) {
      map[data.target] = data.images;
      return map;
    }, {});
  }

  createConsiderations = () => {
    const incorrect_keys = Object.keys(this.state.incorrect_classification);
    var considerations_array = [];

    for(var k = 0; k < incorrect_keys.length; k++){
      const current_val = this.state.incorrect_classification[incorrect_keys[k]]
      if(current_val != null && current_val != "loading"){
        for(var x = 0; x < current_val.length; x++){
          considerations_array.push(
            {
              "error": true,
              "message": "An image of a " + incorrect_keys[k] + " is confused for an image of a " + current_val[x].label + ", "+current_val[x].images.length+" times."
            }
          )
        }
      }
    }

    return considerations_array;
  }

  confusionsLabels = () => {
    const incorrect_keys = Object.keys(this.state.incorrect_classification);
    var considerations_array = [];

    for(var k = 0; k < incorrect_keys.length; k++){
      const current_val = this.state.incorrect_classification[incorrect_keys[k]]
      if(current_val != null && current_val != "loading"){
        for(var x = 0; x < current_val.length; x++){
          considerations_array.push(
            {"base":incorrect_keys[k], "confused": current_val[x].label, "num": current_val[x].images.length}
          )
        }
      }
    }

    return considerations_array;
  }

  summarySeedData = () => {
    const imageDict = this.convertCorrectDict()
    return this.props.spec.label_metrics.map((data, index) => ({
      name: data.label,
      num_examples: data.count,
      correct: data.correct_count,
      incorrect: (data.count - data.correct_count),
      f1_score: (2*(data.recall * data.precision))/(data.recall + data.precision),
      precision: data.precision,
      recall: data.recall,
      correct_images: imageDict[data.label]
    }));
  }

  summarySeedDict = () => {
    const imageDict = this.convertCorrectDict()
    return this.props.spec.label_metrics.reduce(function(map, data) {
      const compact_data = {
        name: data.label,
        num_examples: data.count,
        correct: data.correct_count,
        incorrect: (data.count - data.correct_count),
        f1_score: (2*(data.recall * data.precision))/(data.recall + data.precision),
        precision: data.precision,
        recall: data.recall,
        correct_images: imageDict[data.label]
      }

      map[compact_data.name] = compact_data;
      return map;
    }, {});
  }

  changeConfusions = () => {
    this.setState({confusions: !this.state.confusions})
  }

  changeConfused = (label) => {
    this.setState({selected_confusion: label})
  }

  findImagesConfusion = () => {
    const baseImages = this.state.incorrect_classification[this.state.selected_confusion.base];
  }

  renderNavigation = () => {
    if(this.state.selected_label == null && !this.state.confusions){
      return(
        <div className="TCEvaluationNaviationContainer">
          <TCEvaluationNavigationBar selected_state={this.state.left_selected}
                                     changeSelectedState={this.changeSelectedState.bind(this)} />
        </div>
      );
    }
  }

  renderHomePage = () => {
    if(this.state.left_selected && !this.state.confusions) {
      return (
        <div className="TCEvaluationHomePageContainer">
          <div className="TCEvaluationVegaContainer">
            <TCEvaluationScores data={this.EvaluationScoresData()} />
          </div>
          <div className="TCEvaluationConsiderationsContainer">
            <TCEvaluationScoresConsiderations considerations={this.createConsiderations()}
                                              onClick={this.changeConfusions.bind(this)} />
          </div>
        </div>
      );
    }
  }

  renderConfusions = () => {
    if(this.state.left_selected && this.state.confusions) {
      return(
        <div className="TCEvaluationSummarySelectedModal">
          <div className="TCEvaluationSummarySelectedContainer">
            <div className="TCEvaluationSummarySelectedContainerSpacer">
            </div>
            <div className="TCEvaluationSummarySelectedContainerData">
          <TCEvaluationConfusionMenu labels={this.confusionsLabels()}
                                     selected_element={this.state.selected_confusion}
                                     filterImages={this.changeConfused.bind(this)}/>
          </div>
          </div>
          <div className="TCEvaluationSummarySelectedView">
            <div className="TCEvaluationSummarySelectedContainerSpacer">
            </div>
            <div className="TCEvaluationSummarySelectedContainerData">
              <div className="TCEvaluationSummarySelectedContainerDataCont">
                {this.findImagesConfusion()}
              </div>
            </div>
          </div>
        </div>
      )
    }
  }

  renderSelectSummary = () => {
    if(!this.state.left_selected && (this.state.selected_label != null)) {
      this.loadIncorrect()
      return (
        <div className="TCEvaluationSummarySelectedModal">
          <div className="TCEvaluationSummarySelectedContainer">
            <div className="TCEvaluationSummarySelectedContainerSpacer">
            </div>
            <div className="TCEvaluationSummarySelectedContainerData">
              <TCEvaluationLabelMenu labels={this.props.spec.labels}
                                     selectedLabel={this.state.selected_label}
                                     changeLabel={this.setSelectedLabel.bind(this)} />
            </div>
          </div>
          <div className="TCEvaluationSummarySelectedView">
            <div className="TCEvaluationSummarySelectedContainerSpacer">
            </div>
            <div className="TCEvaluationSummarySelectedContainerData">
              <div className="TCEvaluationSummarySelectedContainerDataCont">
                <TCEvaluationLabelStats data={this.summarySeedDict()}
                                        selectedLabel={this.state.selected_label}
                                        total={this.props.spec.num_examples}/>
                <div className="TCEvaluationSummarySelectedContainerDataIncorrectlyCont">
                  <div className="TCEvaluationSummarySelectedContainerDataIncorrectly">
                    <TCEvaluationIncorrectlyClassified incorrect={this.state.incorrect_classification[this.state.selected_label]} />
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      )
    }
  }
  
  render() {
    return (
      <div className="TCEvaluation">
            {this.renderTopBar()}
            {this.renderNavigation()}
            {this.renderHomePage()}
            {this.renderSummaryContainer()}
            {this.renderConfusions()}
            {this.renderSelectSummary()}
      </div>
    );
  }
}

export default TCEvaluation;
