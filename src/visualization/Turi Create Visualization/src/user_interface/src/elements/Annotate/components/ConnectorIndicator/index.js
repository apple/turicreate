import React, { Component } from 'react';
import './index.css';
import connected from "./connected_icon.png"

class ConnectorIndicator extends Component {
  render() {
    return (
      <div className="ConnectorIndicator">
      	<div className="ConnectorText">
					Disconnect from iPhone X
				</div>
      	<div className="ConnectorImageContainer">
					<img className="ConnectorIcon" src={connected} />
				</div>
      </div>
    );
  }
}

export default ConnectorIndicator;
