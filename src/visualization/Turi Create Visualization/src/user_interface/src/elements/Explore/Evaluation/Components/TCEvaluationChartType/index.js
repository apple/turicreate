import React, { PropTypes } from 'react';
import {createClassFromSpec} from 'react-vega';

import f1_score from './specs/f1_score.json'
import recall from './specs/recall.json'
import precision from './specs/precision.json'
import num_trained from './specs/num_trained.json'

export const F1Score = createClassFromSpec('F1Score', f1_score)
export const Recall = createClassFromSpec('Recall', recall)
export const Precision = createClassFromSpec('Precision', precision)
export const NumTrained = createClassFromSpec('NumTrained', num_trained)

export const ChartType = {
    F1Score:"f1_score",
    Recall:"recall",
    Precision:"precision",
    NumTrained:"num_train"
}
