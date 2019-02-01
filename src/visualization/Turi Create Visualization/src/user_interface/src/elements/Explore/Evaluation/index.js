import React, { Component } from 'react';

import TCEvaluationOverview from './Components/TCEvaluationOverview';
import TCEvaluationTable from './Components/TCEvaluationTable';
import TCEvaluationFooter from './Components/TCEvaluationFooter';

class TCEvaluation extends Component {
  constructor(props){
    super(props)
    this.state = {
      "accuracy_visible": true,
      "precision_visible": false,
      "f1_score_visible": false,
      "recall_visible": false,
      "sort_by": "class",
      "sort_direction": false,
      "incorrect_classification": this.props.spec.label_metrics.reduce(function(map, data) {
        map[data.label] = null;
        return map;
      }, {}),
      "correct_classification":[],
      "filter_confusion":null,
      "sort_by_confusions": "predicted",
      "sort_direction_confusions": false,
      "selected_actual_confusion": null,
      "selected_actual_prediction": null,
      "footer_open": false
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

      this.loadIncorrect()
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
    
  updateSortDirection = () => {
    this.setState({"sort_direction": !this.state.sort_direction})
  }

  updateSortBy = (value) => {
    this.setState({"sort_by": value})
  }

  updateSortByConfusion = (value) => {
    this.setState({
      "sort_by_confusions": value,
      "sort_direction_confusions": !this.state.sort_direction_confusions
    });
  }

  updateFooterOpen = () => {
    this.setState({
      footer_open:!this.state.footer_open
    });
  }

  summarySeedData = () => {
    return this.props.spec.label_metrics.map((data, index) => {

        var dataArr = {
          name: data.label,
          num_examples: data.count,
          correct: data.correct_count,
          incorrect: (data.count - data.correct_count),
          f1_score: (2*(data.recall * data.precision))/(data.recall + data.precision),
          precision: data.precision,
          recall: data.recall,
          correct_images: []
        }

        var correct_data = []
        for(var x = 0; x < this.state.correct_classification.length; x++){
          if(this.state.correct_classification[x]["target"] == data.label){
            correct_data = this.state.correct_classification[x]["images"]
          }
        }

        dataArr["correct_images"] = correct_data;
        return dataArr;
    });
  }

  totalCorrect = () => {
    var correct_num = 0;
    for(var x = 0; x< this.props.spec.label_metrics.length; x++){
      correct_num += this.props.spec.label_metrics[x].correct_count
    }
    return correct_num;
  }

  totalNum = () => {
    var total_num = 0;
    for(var x = 0; x< this.props.spec.label_metrics.length; x++){
      total_num += this.props.spec.label_metrics[x].count
    }
    return total_num;
  }

  changeAccuracy = () =>{
    this.setState({accuracy_visible: !this.state.accuracy_visible})
  }

  changeF1Score = () =>{
    this.setState({f1_score_visible: !this.state.f1_score_visible})
  }

  changePrecision = () =>{ 
    this.setState({precision_visible: !this.state.precision_visible})
  }

  changeRecall = () =>{
    this.setState({recall_visible: !this.state.recall_visible})
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
              "actual": incorrect_keys[k],
              "predicted": current_val[x].label,
              "images": current_val[x].images,
              "count": current_val[x].images.length
            }
          )
        }
      }
    }

    return considerations_array;
  }

  updatedSelectedRow = (value) =>{
    this.setState({"filter_confusion": value})
  }

  selectRowConfusions = (actual, predicted) => {
    this.setState({
      "selected_actual_confusion": actual,
      "selected_actual_prediction": predicted
    })
  }

  render() {
    return (
      <div className="TCEvaluation">
        <TCEvaluationOverview accuracy={this.props.spec.accuracy}
                              iterations={this.props.spec.max_iterations}
                              precision={this.props.spec.precision}
                              recall={this.props.spec.recall}
                              model_type={"Resnet-50"}
                              accuracy_visible={this.state.accuracy_visible}
                              precision_visible={this.state.precision_visible}
                              f1_score_visible={this.state.f1_score_visible}
                              recall_visible={this.state.recall_visible}
                              changeAccuracy={this.changeAccuracy.bind(this)}
                              changePrecision={this.changePrecision.bind(this)}
                              changeRecall={this.changeRecall.bind(this)}
                              changeF1Score={this.changeF1Score.bind(this)}
                              total_correct={this.totalCorrect()}
                              total_num={this.totalNum()} />
        <TCEvaluationTable data={this.summarySeedData()}
                           accuracy_visible={this.state.accuracy_visible}
                           precision_visible={this.state.precision_visible}
                           f1_score_visible={this.state.f1_score_visible}
                           recall_visible={this.state.recall_visible}
                           sort_by={this.state.sort_by}
                           sort_direction={this.state.sort_direction}
                           updateSortDirection={this.updateSortDirection.bind(this)}
                           updateSortBy={this.updateSortBy.bind(this)}
                           filter_confusion={this.state.filter_confusion}
                           footer_open={this.state.footer_open}
                           row_click={this.updatedSelectedRow.bind(this)}
                            />
        <TCEvaluationFooter considerations={this.createConsiderations()}
                            filter_confusion={this.state.filter_confusion}
                            sort_by_confusions={this.state.sort_by_confusions}
                            sort_direction_confusions={this.state.sort_direction_confusions}
                            row_click={this.updatedSelectedRow.bind(this)}
                            updateSortByConfusion={this.updateSortByConfusion.bind(this)}
                            selectRowConfusions={this.selectRowConfusions.bind(this)}
                            selected_actual={this.state.selected_actual_confusion}
                            selected_prediction={this.state.selected_actual_prediction}
                            footer_open={this.state.footer_open}
                            updateFooterOpen={this.updateFooterOpen.bind(this)}
                            />
      </div>
    );
  }
}

export default TCEvaluation;
