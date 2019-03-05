import React, { Component } from 'react';
import style from './index.module.scss';

class ImageLabeler extends Component {


  render() {

    if (this.props.src) {
      return(
        <div className={style.ImageLabeler}>
          <div className={style.Image}>
            <img src={this.props.src}/>
          </div>
        </div>
      );
    } else {
      return (
      <div className={style.ImageLabeler}>
        <div className={style.Image}>
            <div className={style.ImageLoadingHolder}>
                <div className={style.bounce1}></div>
                <div className={style.bounce2}></div>
                <div className={style.bounce3}></div>
            </div>
        </div>
      </div>
    );
    }


    
  }
}

export default ImageLabeler;