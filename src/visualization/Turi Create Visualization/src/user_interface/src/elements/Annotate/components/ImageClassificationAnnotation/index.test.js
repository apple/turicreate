import React from 'react';
import ReactDOM from 'react-dom';
import ImageClassificationAnnotation from './index';

it('renders without crashing', () => {
  const div = document.createElement('div');
  ReactDOM.render(<ImageClassificationAnnotation />, div);
  ReactDOM.unmountComponentAtNode(div);
});
