import React, { Component } from 'react';
import ReactDOM from 'react-dom';

import {SortType} from '../TCEvaluationSortType'
import {F1Score,
        Recall,
        Precision,
        NumTrained,
        ChartType} from '../TCEvaluationChartType';

import TCEvaluationHelperSort from '../TCEvaluationHelperSort'
import TCEvaluationHelperTitle from '../TCEvaluationHelperTitle'

/*
import { Handler } from 'vega-tooltip';
*/

import './index.scss';

// TODO: add `vega-lib` `react-veg` to package.json
class TCEvaluationScores extends Component {
  constructor(props){
    super(props);
    
    this.tooltip_options = {
      theme: 'dark'
    }
    this.state = {
      "chart": ChartType.F1Score,
      "info": '',
      "sort": SortType.HighToLow
    }
  }

  renderChart = () => {
    if(this.state.chart == ChartType.F1Score){
      return (
        <F1Score data={this.props.data} />
      );
    }else if(this.state.chart == ChartType.Recall){
      return (
        <Recall data={this.props.data} />
      );
    }else if(this.state.chart == ChartType.Precision){
      return (
        <Precision data={this.props.data} />
      );
    }else if(this.state.chart == ChartType.NumTrained){
      return (
        <NumTrained data={this.props.data} />
      );
    }
  }

  chartSelect = (selectedChart) =>{
    this.setState({'chart': selectedChart});
  }

  sortSelect = (selectedSort) =>{
    this.setState({'sort': selectedSort});
  }

  render() {
    return (
       <div className="TCEvaluationScore">
        <div className="TCEvaluationScoresNavigation">
         <TCEvaluationHelperTitle selected={this.state.chart}
                                  setSelect={this.chartSelect.bind(this)}/>
         <TCEvaluationHelperSort selected={this.state.sort}
                                 setSelect={this.sortSelect.bind(this)}/>
        </div>
        <div className="TCEvaluationScoresChart">
          {this.renderChart()}
        </div>
       </div>
    );
  }
}

export default TCEvaluationScores;
