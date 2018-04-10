import React, { Component } from 'react';
import ReactDOM from 'react-dom';

import './index.css';

export default class Row extends Component {
  render() {
    if(this.props.className == "accordian_window"){
      console.log(this.props);
    }
    return (
      <div {...this.props} className={'sticky-table-row ' + (this.props.className || '')}>
        {this.props.children}
      </div>
    );
  }
}
