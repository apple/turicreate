import React, { Component } from 'react';
import './index.scss';

import TCEvaluationConfusionRow from './TCEvaluationConfusionRow';
import TCEvaluationConfusionHeader from './TCEvaluationConfusionHeader';

class TCEvaluationConfusionTable extends Component {

  maxCount = () => {
    return Math.max.apply(Math, this.props.considerations.map(function(o) { return o.count; }));
  }

  filterData = (element) => {
    if(this.props.filter_confusion != null){
      return element.actual == this.props.filter_confusion;
    }else{
      return true;
    }
  }

  sortData = (a, b) => {
    if(this.props.sort_by_confusions == "actual" && !this.props.sort_direction_confusions){
      if((a.actual) > (b.actual)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by_confusions == "actual" && this.props.sort_direction_confusions){
      if((a.actual) < (b.actual)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by_confusions == "predicted" && !this.props.sort_direction_confusions){
      if((a.predicted) > (b.predicted)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by_confusions == "predicted" && this.props.sort_direction_confusions){
      if((a.predicted) < (b.predicted)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by_confusions == "count" && !this.props.sort_direction_confusions){
      if((a.count) > (b.count)){
        return -1;
      }else{
        return 1;
      }
    }

    if(this.props.sort_by_confusions == "count" && this.props.sort_direction_confusions){
      if((a.count) < (b.count)){
        return -1;
      }else{
        return 1;
      }
    }

    return 0;
  }

  render() {
    const max_value = this.maxCount();
    return (
      <div className="TCEvaluationConfusionTable">
        <div className="TCEvaluationConfusionTableContainer">
          <TCEvaluationConfusionHeader updateSortByConfusion={this.props.updateSortByConfusion.bind(this)}
                                       sort_by_confusions={this.props.sort_by_confusions}
                                       sort_direction_confusions={this.props.sort_direction_confusions}/>

          {
            this.props.considerations.filter(this.filterData).sort(this.sortData).map((consideration, index) => {
              return(<TCEvaluationConfusionRow consideration={consideration}
                                               selectRowConfusions={this.props.selectRowConfusions.bind(this)}
                                               maxValue={max_value}/>);
            })
          }

        </div>
      </div>
    );
  }
}

export default TCEvaluationConfusionTable;
