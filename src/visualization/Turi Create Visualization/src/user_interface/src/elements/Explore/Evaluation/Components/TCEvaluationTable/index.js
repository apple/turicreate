import React, { Component } from 'react';
import './index.scss';

import TCEvaluationRows from './TCEvaluationRows';
import TCEvaluationHeader from './TCEvaluationHeader';

class TCEvaluationTable extends Component {

  maxValueNumTest = () => {
    return Math.max.apply(Math, this.props.data.map(function(o) { return o.num_examples; }));
  }

  sortingColumns = (a,b) => {
    // class sort
    if(this.props.sort_by == "class" && this.props.sort_direction){
      if(a.name > b.name){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "class" && !this.props.sort_direction){
      if(a.name < b.name){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "accuracy" && this.props.sort_direction){
      if((a.correct/a.incorrect) < (b.correct/b.incorrect)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "accuracy" && !this.props.sort_direction){
      if((a.correct/a.incorrect) > (b.correct/b.incorrect)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "precision" && this.props.sort_direction){
      if((a.precision) < (b.precision)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "precision" && !this.props.sort_direction){
      if((a.precision) > (b.precision)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "f1_score" && this.props.sort_direction){
      if((a.f1_score) < (b.f1_score)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "f1_score" && !this.props.sort_direction){
      if((a.f1_score) > (b.f1_score)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "recall" && this.props.sort_direction){
      if((a.recall) < (b.recall)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "recall" && !this.props.sort_direction){
      if((a.recall) > (b.recall)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "train" && this.props.sort_direction){
      if((a.num_examples) < (b.num_examples)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "train" && !this.props.sort_direction){
      if((a.num_examples) > (b.num_examples)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "test" && this.props.sort_direction){
      if((a.num_examples) < (b.num_examples)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by == "test" && !this.props.sort_direction){
      if((a.num_examples) > (b.num_examples)){
        return -1;
      }else{
        return 1;
      }
    }

    return 0;
  }

  selectClassName = () =>{
    if(this.props.footer_open) {
      return "TCEvaluationTable TCEvaluationTableOpen";
    }else{
      return "TCEvaluationTable";
    }
  }

  render() {
    const max_test = this.maxValueNumTest();
    return (
      <div className={this.selectClassName()}>
          <TCEvaluationHeader accuracy_visible={this.props.accuracy_visible}
                              precision_visible={this.props.precision_visible}
                              f1_score_visible={this.props.f1_score_visible}
                              recall_visible={this.props.recall_visible}
                              updateSortDirection={this.props.updateSortDirection.bind(this)}
                              updateSortBy={this.props.updateSortBy.bind(this)}
                              sort_direction={this.props.sort_direction}
                              sort_by={this.props.sort_by}
                              />
          {this.props.data.sort(this.sortingColumns).map((data, index) => {
            return(<TCEvaluationRows data={data}
                                     accuracy_visible={this.props.accuracy_visible}
                                     precision_visible={this.props.precision_visible}
                                     f1_score_visible={this.props.f1_score_visible}
                                     recall_visible={this.props.recall_visible}
                                     max_value={max_test}
                                     selected={(this.props.filter_confusion == data.name)}
                                     onclick={this.props.row_click.bind(this)} />)
          })}
      </div>
    );
  }
}

export default TCEvaluationTable;
