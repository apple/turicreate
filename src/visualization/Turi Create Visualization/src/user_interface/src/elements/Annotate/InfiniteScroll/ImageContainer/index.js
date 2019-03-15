import React, { Component } from 'react';
import style from './index.module.scss';
import { isRegExp } from 'util';
import { LabelType } from '../../utils';

class ImageContainer extends Component {
  	constructor(props) {
		super(props);
	}
	
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
		if(this.props.type == LabelType.INTEGER && Number.isInteger(this.props.annotation)) {
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
		if((width/height) > 1){
			return {"width": "auto", "height": style.imgSize, "top": 0, "left": parseInt((-1*(((width/height)*style.imgSize)-style.imgSize)/2), 10)};
		} else {
			return {"width": style.imgSize, "height": "auto", "top": parseInt((-1*(((height/width)*style.imgSize)-style.imgSize)/2), 10), "left": 0};
		}
	}

  render() {
		if (this.props.src && this.props.selected) {
			return(
				<div className={`${style.ImageContainer} ${style.realImage} ${style.selected}`}
						 onClick={this.image_click.bind(this)}>
					<img src={this.props.src.src}
							 className={style.ImageContainerSource}
							 style={this.resizeImage(this.props.src.width, this.props.src.height)}/>
					{this.renderAnnotation()}
				</div>
			);
		} else if (this.props.src) {
			return(
				<div className={`${style.ImageContainer} ${style.realImage}`}
						 onClick={this.image_click.bind(this)}>
					<img src={this.props.src.src}
							 className={style.ImageContainerSource}
							 style={this.resizeImage(this.props.src.width, this.props.src.height)}/>
					{this.renderAnnotation()}
				</div>
			);
		} else {
			return(
				<div className={style.ImageContainer}>
					<div class={`${style.bounce1} ${style.bouncer}`}></div>
					<div class={`${style.bounce2} ${style.bouncer}`}></div>
					<div class={`${style.bounce2} ${style.bouncer}`}></div>
				</div>
			);
		}
	}
}

export default ImageContainer;  
