import React, { Component } from 'react';
import styles from './index.module.scss';
	
import Labels from './Labels';
	
class LabelContainer extends Component {

  renderLabels = () => {
    
  }

  render() {
    return (
      <div className={styles.LabelContainer}>
      	<div className={styles.LabelHeader}>
      		Labels
      	</div>
      	<div className={styles.LabelButtonContainer}>
      		{
            this.props.labels.map((x) => 
              <Labels active={false}
                    name={x.name}
                    num_annotated={x.num_annotated}
                    num_expected={x.num_expected}/>
            )
          }
      	</div>
      	<div className={styles.LabelAddLabelContainer}>
      		<button className={styles.LabelAddLabel}
                  onClick={this.props.openLabelModal.bind(this)}>
      			+ Add New Label
      		</button>
      	</div>
      </div>
    );
  }
}
	
export default LabelContainer;