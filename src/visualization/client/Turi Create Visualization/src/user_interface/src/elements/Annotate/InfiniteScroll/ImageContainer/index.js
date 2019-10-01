import React, { Component } from 'react';
import style from './index.module.scss';
import { LabelType } from '../../utils';

class ImageContainer extends Component {

  image_click = (e) =>{
    if (e.shiftKey) {
      this.props.addToSelected(this.props.value);
    }else{
      this.props.removeSelected();
      this.props.toggleInfiniteScroll();
      this.props.updateIncrementalCurrentIndex(this.props.value);
    }
  }

  renderAnnotation = () => {
    if(this.props.type === LabelType.INTEGER && Number.isInteger(this.props.annotation)) {
      return (
        <div className={style.ImageLabel}>
          {this.props.annotation}
        </div>
      )
    }else if(this.props.annotation){
      return (
        <div className={style.ImageLabel}>
          {this.props.annotation}
        </div>
      )
    }
  }

  resizeImage(width, height) {
    if((width/height) > 1) {
      return {"width": "auto", "height": style.imgSize + "px", "top": "0px", "left": parseInt((-1*(((width/height)*style.imgSize)-style.imgSize)/2), 10) + "px"};
    } else {
      return {"width": style.imgSize + "px", "height": "auto", "top": parseInt((-1*(((height/width)*style.imgSize)-style.imgSize)/2), 10) + "px", "left": "0px"};
    }
  }

  render() {
    if (this.props.src && this.props.selected) {
      return(
        <div className={`${style.ImageContainer} ${style.realImage} ${style.selected}`}
             onClick={this.image_click.bind(this)}>
          <img src={this.props.src.src}
             className={`${style.ImageContainerSource} ${style.ImageContainerSourceSelected}`}
             style={this.resizeImage(this.props.src.width, this.props.src.height)}
	     alt=""/>
          {this.renderAnnotation()}
        </div>
      );
    } else if (this.props.src) {
      return(
        <div className={`${style.ImageContainer} ${style.realImage}`}
             onClick={this.image_click.bind(this)}>
          <img src={this.props.src.src}
             className={style.ImageContainerSource}
             style={this.resizeImage(this.props.src.width, this.props.src.height)}
	     alt="Can't Find"/>
          {this.renderAnnotation()}
        </div>
      );
    } else {
      return(
        <div className={`${style.ImageContainer}`}>
          <div className={`${style.LoadingContainer}`}>
            <div className={`${style.bounce1} ${style.bouncer}`}></div>
            <div className={`${style.bounce2} ${style.bouncer}`}></div>
            <div className={`${style.bounce2} ${style.bouncer}`}></div>
          </div>
        </div>
      );
    }
  }
}

export default ImageContainer;
