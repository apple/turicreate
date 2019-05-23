import React, { Component } from 'react';
import style from './index.module.scss';

class SimilarImage extends Component {
  constructor(props) {
    super(props)
  }

  resizeImage(width, height) {
    if((width/height) > 1) {
      return {"width": "auto", "height": style.singleImgSize + "px", "top": "0px", "left": parseInt((-1*(((width/height)*style.singleImgSize)-style.singleImgSize)/2), 10) + "px"};
    } else {
      return {"width": style.singleImgSize + "px", "height": "auto", "top": parseInt((-1*(((height/width)*style.singleImgSize)-style.singleImgSize)/2), 10) + "px", "left": "0px"};
    }
  }

  render() {
    if (!this.props.selected) {
      return (
        <div className={`${style.SingleImageSimilarityImage}`}
             onClick={this.props.onClick.bind(this, this.props.src.index)}>
          <img src={this.props.src.src}
               className={style.SingleImageIndex}
               style={this.resizeImage(this.props.src.width, this.props.src.height)} />
        </div>
      );
    }else{
      return (
        <div className={`${style.SingleImageSimilarityImage} ${style.Selected}`}
             onClick={this.props.onClick.bind(this, this.props.src.index)}>
          <img src={this.props.src.src}
               className={style.SingleImageIndex}
               style={this.resizeImage(this.props.src.width, this.props.src.height)} />
        </div>
      );
    }
  }
}

export default SimilarImage;