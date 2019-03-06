import React, { Component } from 'react';
import style from './index.module.scss';
import { isRegExp } from 'util';
import { LabelType } from '../../utils';

class ImageContainer extends Component {
  constructor(props) {
    super(props);
	}
	
	image_click = () =>{
		this.props.toggleInfiniteScroll();
		this.props.updateIncrementalCurrentIndex(this.props.value);
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

  render() {
		if (this.props.src) {
			return(
				<div className={`${style.ImageContainer} ${style.realImage}`}
						 onClick={this.image_click.bind(this)}>
					<img src={this.props.src}/>
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