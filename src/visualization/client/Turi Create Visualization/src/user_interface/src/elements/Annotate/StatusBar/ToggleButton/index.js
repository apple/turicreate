import React, { Component } from 'react';
import style from './index.module.scss';

class ToggleButton extends Component {
  render() {
    var NibClasses = style.ToggleButtonNib
    var BackgroundClasses = style.ToggleButton

    if(this.props.hideAnnotated){
      NibClasses = style.Toggled
      BackgroundClasses = style.ToggleButtonOn
    }

    return (
      <div className={BackgroundClasses}
           onClick={this.props.toggleHideAnnotated.bind(this)}>
        <div className={NibClasses}
             onClick={this.props.toggleHideAnnotated.bind(this)}>
        </div>
      </div>
    );
  }
}

export default ToggleButton;