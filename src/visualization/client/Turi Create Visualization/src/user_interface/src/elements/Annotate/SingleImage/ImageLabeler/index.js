import React, { Component } from 'react';
import style from './index.module.scss';

class ImageLabeler extends Component {
  constructor(props){
    super(props);
    this.currentComponent = React.createRef();
    this.state = {
      componentHeight: window.innerHeight,
      componentWidth: window.innerWidth,
    }
  }

  componentDidMount() {
    this.updateDimensions();
    window.addEventListener("resize", this.updateDimensions.bind(this));
  }

  componentWillUnmount() {
    window.removeEventListener("resize", this.updateDimensions.bind(this));
  }

  componentWillReceiveProps() {
    this.updateDimensions();
  }

  handleImageLoaded = () => {
    this.updateDimensions();
  }

  updateDimensions = () => {
    const $this = this;
    window.requestAnimationFrame(function() {
      $this.setState({
        componentHeight: $this.currentComponent.current.getBoundingClientRect().height,
        componentWidth: $this.currentComponent.current.getBoundingClientRect().width
      });
    });
  }

  resizeImage(width, height) {
    if((width/height) > (this.state.componentWidth/this.state.componentHeight)){
      var scaling_factor = (height/width)*this.state.componentWidth;
      var left_padding = (parseInt(((this.state.componentHeight - scaling_factor)/2), 10));

      return {"width": this.state.componentWidth, "height": "auto", "top": left_padding, "left": 0};
    } else {
      var scaling_factor = (width/height)*this.state.componentHeight;
      var left_padding = (parseInt(((this.state.componentWidth - scaling_factor)/2), 10));

      return {"width": "auto", "height": this.state.componentHeight, "top": 0, "left": left_padding};
    }
  }

  render() {
    if (this.props.src) {
      return(
        <div className={style.ImageLabeler}>
          <div className={style.Image}
               ref={this.currentComponent}>
            <img src={this.props.src.src}
                 className={style.ImageSrc}
                 onLoad={this.handleImageLoaded.bind(this)}
                 style={this.resizeImage(this.props.src.width, this.props.src.height)}/>
          </div>
        </div>
      );
    } else {
      return (
        <div className={style.ImageLabeler}>
          <div className={`${style.Image} ${style.ImageShadow}`}>
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