import React, { Component } from 'react';
import './index.css';

import Labels from '../Labels';

class LabelContainer extends Component {
  render() {
    return (
      <div className="LabelContainer">
      	<div className="LabelHeader">
      		Labels
      	</div>
      	<div className="LabelButtonContainer">
      		{/* START: data store access here */}
      		<Labels active={false}
      				name="Dogskhjhklhlhlhljlhlkhlkhggk,hgkjgkgkfgjgljglg"
      				num_annotated={10}
      				num_expected={10}/>

      		<Labels active={true}
      				name="Cats"
      				num_annotated={10}
      				num_expected={40}/>

      		<Labels active={false}
      				name="Melons"
      				num_annotated={1}
      				num_expected={20}/>

      		{/* END: data store access here */}
      	</div>
      	<div className="LabelAddLabelContainer">
      		<button className="LabelAddLabel">
      			+ Add New Label
      		</button>
      	</div>
      </div>
    );
  }
}

export default LabelContainer;
