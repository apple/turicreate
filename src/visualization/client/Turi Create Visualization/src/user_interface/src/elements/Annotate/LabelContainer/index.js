import React, { Component } from 'react';
import styles from './index.module.scss';

import Labels from './Labels';

class LabelContainer extends Component {

  setAnnotation = (name) => {
    if(!this.props.infiniteScroll){
      this.props.setAnnotation(this.props.incrementalCurrentIndex, name);
      this.props.setAnnotationSimilar(name);
    }else{
      this.props.setAnnotationMass(name);
    }
  }

  checkActive = (name) => {
    if (!this.props.infiniteScroll) {
      return this.props.annotationData[this.props.incrementalCurrentIndex] === name;
    } else {
      var selected_objects = Object.keys(this.props.infiniteSelected);
      for (var x = 0; x < selected_objects.length; x++) {
        if (this.props.annotationData[selected_objects[x]] === name) {
          return true;
        }
      }
      return false;
    }
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
              <Labels active={this.checkActive(x.name)}
                      similarSelected={this.props.similarSelected}
                      name={x.name}
                      onClick={this.setAnnotation.bind(this, x.name)}
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
