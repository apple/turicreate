import React, { Component } from 'react';
import './index.css';
import LabelContainer from "../LabelContainer";
import DisplayContainer from "../DisplayContainer";

class Content extends Component {
  render() {
    return (
      <div className="Content">
      	<LabelContainer />
      	<DisplayContainer />
      </div>
    );
  }
}

export default Content;
