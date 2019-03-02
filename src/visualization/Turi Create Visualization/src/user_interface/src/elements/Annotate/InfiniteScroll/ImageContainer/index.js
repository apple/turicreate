import React, { Component } from 'react';
import style from './index.module.scss';
import { isRegExp } from 'util';

class ImageContainer extends Component {
  constructor(props) {
    super(props);
  }

  render() {
		if (this.props.src) {
			return(
				<div className={style.ImageContainer}>
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