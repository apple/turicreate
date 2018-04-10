import React, { Component } from 'react';
import ReactDOM from 'react-dom';

import './index.css';

export default class Cell extends Component {
  render() {
      return (
        <div {...this.props} className={'sticky-table-cell ' + (this.props.className || '')}>
          {this.props.children}
        </div>
      );
  }
}
