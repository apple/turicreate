# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as tc

# The following objects enable the following API

# >>> nlp = SpacyNlpMock()
# >>> doc = nlp(text)

# Get text and part of speech for each token in a doc.
# >>> [w.text for w in doc if w.pos in chosen_pos]

# Get the text of each sentence in a document.
# >>> [s.text for s in doc.sents]
#
# Rather than supply real answers, this mock does the following:
# - splits sentences by splitting on '. '
# - tokenizes into words by splitting on ' '
# - assigns parts of speech to always be NOUN

class SpacyTokenMock(object):
    def __init__(self, text):
        self.text = text
        self.pos = tc.text_analytics.PartOfSpeech.NOUN[1]

class SpacySentMock(object):

    def __init__(self, text):
        self.text = text

class SpacyDocMock(object):

    def __init__(self, text):
        self.text = text
        if text == '':
            self.sents = []
            self.tokens = []
        else:
            self.sents = [SpacySentMock(s) for s in text.split('. ')]
            for s in self.sents:
                if s.text != '' and not s.text.endswith('.'):
                    s.text += '.'
            self.tokens = [SpacyTokenMock(x) for x in text.split(' ')]

    def __iter__(self):
        for x in self.tokens:
            yield x

    def __len__(self):
        return len(self.tokens)

class SpacyNlpMock(object):

    def __call__(self, text ):
        return SpacyDocMock(text)

    def pipe(self, text, batch_size=1, n_threads=1, parse=True, entity=True, tag=True):
        for x in text:
            if x is None:
                yield x
            else:
                yield SpacyDocMock(x)
