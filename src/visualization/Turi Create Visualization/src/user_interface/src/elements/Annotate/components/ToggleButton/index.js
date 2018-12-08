import React, { Component } from 'react';
import './index.css';

class ToggleButton extends Component {
  constructor(props){
    super(props);
    this.state ={
      "checked": false
    }
  }

  clickedButton = () => {
    this.setState({"checked":!this.state.checked});
  }

  render() {
    var NibClasses = "ToggleButtonNib"
    var BackgroundClasses = "ToggleButton"

    if(this.state.checked){
      NibClasses = "Toggled"
      BackgroundClasses = "ToggleButtonOn"
    }

    return (
      <div className={BackgroundClasses} onClick={this.clickedButton.bind(this)}>
        <div className={NibClasses} onClick={this.clickedButton.bind(this)}>

        </div>
      </div>
    );
  }
}

export default ToggleButton;
