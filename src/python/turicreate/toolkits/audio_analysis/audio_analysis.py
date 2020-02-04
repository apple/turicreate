# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from fnmatch import fnmatch as _fnmatch
from glob import glob as _glob
import os as _os
from random import shuffle as _shuffle

import turicreate as _tc
from turicreate.toolkits._main import ToolkitError as _ToolkitError


def load_audio(
    path, with_path=True, recursive=True, ignore_failure=True, random_order=False
):
    """
    Loads WAV file(s) from a path.

    Parameters
    ----------
    path : str
        Path to WAV files to be loaded.

    with_path : bool, optional
        Indicates whether a path column is added to the returned SFrame.

    recursive : bool, optional
        Indicates whether ``load_audio`` should do a recursive directory traversal,
        or only load audio files directly under ``path``.

    ignore_failure : bool, optional
        If True, only print warnings for failed files and keep loading the remaining
        audio files.

    random_order : bool, optional
        Load audio files in random order.

    Returns
    -------
    out : SFrame
        Returns an SFrame with either an 'audio' column or both an 'audio' and
        a 'path' column. The 'audio' column is a column of dictionaries.

        Each dictionary contains two items. One item is the sample rate, in
        samples per second (int type). The other item will be the data in a numpy
        array. If the wav file has a single channel, the array will have a single
        dimension. If there are multiple channels, the array will have shape
        (L,C) where L is the number of samples and C is the number of channels.

    Examples
    --------
    >>> audio_path = "~/Documents/myAudioFiles/"
    >>> audio_sframe = tc.audio_analysis.load_audio(audio_path, recursive=True)
    """
    from scipy.io import wavfile as _wavfile

    all_wav_files = []

    if _fnmatch(path, "*.wav"):  # single file
        all_wav_files.append(path)
    elif recursive:
        for (dir_path, _, file_names) in _os.walk(path):
            for cur_file in file_names:
                if _fnmatch(cur_file, "*.wav"):
                    all_wav_files.append(dir_path + "/" + cur_file)
    else:
        all_wav_files = _glob(path + "/*.wav")

    if random_order:
        _shuffle(all_wav_files)

    result_builder = _tc.SFrameBuilder(
        column_types=[dict, str], column_names=["audio", "path"]
    )
    for cur_file_path in all_wav_files:
        try:
            sample_rate, data = _wavfile.read(cur_file_path)
        except Exception as e:
            error_string = "Could not read {}: {}".format(cur_file_path, e)
            if not ignore_failure:
                raise _ToolkitError(error_string)
            else:
                print(error_string)
                continue

        result_builder.append(
            [{"sample_rate": sample_rate, "data": data}, cur_file_path]
        )

    result = result_builder.close()
    if not with_path:
        del result["path"]
    return result
