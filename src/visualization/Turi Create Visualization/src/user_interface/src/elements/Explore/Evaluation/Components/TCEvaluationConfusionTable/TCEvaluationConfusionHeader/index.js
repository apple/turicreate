import React, { Component } from 'react';
import './index.scss';

import TCEvaluationConfusionHeaderCell from './TCEvaluationConfusionHeaderCells';

class TCEvaluationConfusionHeader extends Component {

  clickActual = () => {
    this.props.updateSortByConfusion("actual")
  }

  clickPredicted = () => {
    this.props.updateSortByConfusion("predicted")
  }

  clickCount = () => { 
    this.props.updateSortByConfusion("count")
  }

  render() {
    return (
      <div className="TCEvaluationConfusionHeader">
        <TCEvaluationConfusionHeaderCell name="ACTUAL"
                                         onclick={this.clickActual.bind(this)}
                                         enabled={(this.props.sort_by_confusions == "actual")}
                                         direction={!this.props.sort_direction_confusions} />
        <TCEvaluationConfusionHeaderCell name="PREDICTED"
                                         onclick={this.clickPredicted.bind(this)}
                                         enabled={(this.props.sort_by_confusions == "predicted")}
                                         direction={!this.props.sort_direction_confusions}/>
        <TCEvaluationConfusionHeaderCell name="COUNT"
                                         type="percent"
                                         onclick={this.clickCount.bind(this)}
                                         enabled={(this.props.sort_by_confusions == "count")}
                                         direction={!this.props.sort_direction_confusions}/>
        <TCEvaluationConfusionHeaderCell name="EXAMPLE IMAGES"
                                         type="images"/>
      </div>
    );
  }
}

export default TCEvaluationConfusionHeader;
