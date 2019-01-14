import React, { Component } from 'react';
import './index.scss';
import TCEvaluationToggleIcon from './TCEvaluationToggleIcon';

import * as d3 from "d3";

class TCEvaluationNavigationBar extends Component {
  render() {
    return (
      <div className="TCEvaluationNavigationBar">
        <div className="TCEvaluationNavigationBarSearch">
          <input placeholder="Search"/>
        </div>
        <TCEvaluationToggleIcon selected_state={this.props.selected_state}
                                changeSelectedState={this.props.changeSelectedState.bind(this)} />
      </div>
    );
  }
}

export default TCEvaluationNavigationBar;
