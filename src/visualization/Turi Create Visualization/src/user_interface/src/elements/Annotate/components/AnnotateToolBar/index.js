import React, { Component } from 'react';
import './index.css';

import clone from "./clone.png"

import ToggleButton from "../ToggleButton";

class AnnotateToolBar extends Component {
	constructor(props){
		super(props);
	}

  render() {
    return (
      <div className="AnnotateToolBar">
      	<div className="AnnotateToolBarStats">
      		Total: 1823 | Annotated: 12
      	</div>
      	<div className="AnnotateToolBarActions">
      		<img className="AnnotateToolBarSwitchViews"
               onClick={this.props.changeValue.bind(this)} src={clone}/>
      		<ToggleButton />
      		<div className="AnnotateToolBarHideText">
						Hide Annotated image
					</div>
      	</div>
      </div>
    );
  }
}

export default AnnotateToolBar;
