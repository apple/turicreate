import React, { Component } from 'react';
import './index.scss';
import TCEvaluationToggleIcon from './TCEvaluationToggleIcon';

import * as d3 from "d3";

class TCEvaluationNavigationBar extends Component {
  constructor(props){
    super(props);
    this.state = {
      "hello": true
    }
  }

  render() {
    return (
      <div className="TCEvaluationNavigationBar">
        <div className="TCEvaluationNavigationBarSearch">
          <input placeholder="Search"/>
        </div>
        <TCEvaluationToggleIcon />
      </div>
    );
  }
}

export default TCEvaluationNavigationBar;