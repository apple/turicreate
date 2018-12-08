import React, { Component } from 'react';
import Content from '../Content';
import Header from '../Header';

import './index.css';

class ImageClassificationAnnotation extends Component {
  render() {
    return (
      <div className="App">
        <Header />
        <Content />
      </div>
    );
  }
}

export default ImageClassificationAnnotation;
