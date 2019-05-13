import React, { Component } from 'react';
import style from './index.module.scss';

class SimilarImage extends Component {
  constructor(props) {
    super(props)
  }
  
  render() {
    return (
      <div className={style.SigleImageSimilarityImage}>
        <img src={this.props.src} />
      </div>
    );
  }
}

export default SimilarImage;