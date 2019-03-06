import React, { Component } from 'react';
import style from './index.module.scss';
import { isRegExp } from 'util';

class ImageContainer extends Component {
  constructor(props) {
    super(props);
	}
	
	image_click = () =>{
		this.props.toggleInfiniteScroll();
		this.props.updateIncrementalCurrentIndex(this.props.value);
	}

  render() {
		if (this.props.src) {
			return(
				<div className={`${style.ImageContainer} ${style.realImage}`}
						 onClick={this.image_click.bind(this)}>
					<img src={this.props.src}/>
				</div>
			);
		} else {
			return(
				<div className={style.ImageContainer}>
					<div class={style.bounce1}></div>
					<div class={style.bounce2}></div>
					<div class={style.bounce3}></div>
				</div>
			);
		}
	}
}

export default ImageContainer;  