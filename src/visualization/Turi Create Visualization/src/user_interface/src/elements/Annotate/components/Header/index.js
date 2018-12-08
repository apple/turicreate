import React, { Component } from 'react';
import './index.css';

import ConnectorIndicator from '../ConnectorIndicator';

import question from "./question.png"

class Header extends Component {
	
	constructor(props){
		super(props);
		// TODO: Pipe to Redux
		this.state = {
			"connected": false
		}
	}

	connectPressed = () => {
		this.setState({"connected": true});
	}

	renderPhoneConnect = () => {
		if(!this.state.connected){
			return (
				<button className="HeaderConnectButton" onClick={this.connectPressed.bind(this)}>
					Connect To App
				</button>
			);
		}else{
			return (
				<ConnectorIndicator />
			);
		}
	}

  render() {
    return (
      <div className="Header">
      	<div className="HeaderImageButtonContainer">
      		<button className="HeaderImageButton">
      			<img className="HeaderImage" src={question}/>
      		</button>
      	</div>
      	<div>
      		{this.renderPhoneConnect()}
      	</div>
      </div>
    );
  }
}

export default Header;
