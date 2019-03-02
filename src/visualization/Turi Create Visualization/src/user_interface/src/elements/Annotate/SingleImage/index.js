import React, { Component } from 'react';
import style from './index.module.scss';

import leftArrow from './assets/left_arrow.svg';
import rightArrow from './assets/right_arrow.svg';
import ImageLabeler from './ImageLabeler/index'

class SingleImage extends Component {
  render() {
    return (
      <div className={style.SingleImage}>
        <div className={style.SingleImageContainer}>
            <div className={style.LeftArrow}>
                <img src={leftArrow} />
            </div>
            <ImageLabeler />
            <div className={style.RightArrow}>
                <img src={rightArrow} />
            </div>
        </div>
      </div>
    );
  }
}

export default SingleImage;