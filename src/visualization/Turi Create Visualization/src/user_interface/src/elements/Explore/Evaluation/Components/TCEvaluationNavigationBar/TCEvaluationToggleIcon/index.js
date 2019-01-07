import React, { Component } from 'react';
import './index.scss';
import list_view from './assets/list_view.png';
import split_pane from './assets/split_pane.png';

import selected_list_view from './assets/selected_list_view.png';
import selected_split_pane from './assets/selected_split_pane.png';

import * as d3 from "d3";

class TCEvaluationToggleIcon extends Component {
  constructor(props){
    super(props);
    this.state = {
      "left_selected": true
    }
  }

  changeSelectedState = () => {
    this.setState({"left_selected": !this.state.left_selected});
  }

  renderLeftIcon = () => {
    if (this.state.left_selected) {
      return (
        <div className="TCEvaluationToggleIconLeftSelected">
          <img height={12} src={selected_split_pane}/>
        </div>
      );
    } else {
      return (
        <div className="TCEvaluationToggleIconLeft" 
             onClick={this.changeSelectedState.bind(this)}>
          <img height={12} src={split_pane}/>
        </div>
      );
    }
  }

  renderRightIcon = () => {
    if (!this.state.left_selected) {
      return (
        <div className="TCEvaluationToggleIconRightSelected">
          <img height={12} src={selected_list_view}/>
        </div>
      );
    } else {
      return (
        <div className="TCEvaluationToggleIconRight"
             onClick={this.changeSelectedState.bind(this)}>
          <img height={12} src={list_view}/>
        </div>
      );
    }
  }

  render() {
    return (
      <div className="TCEvaluationToggleIcon">
        {this.renderLeftIcon()}
        {this.renderRightIcon()}
      </div>
    );
  }
}

export default TCEvaluationToggleIcon;