#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import sys
from setuptools import setup
from setuptools.command.install import install

PACKAGE_NAME="turicreate"
VERSION='4.2'

class InstallEngine(install):
    def run(self):
        msg = ("""

        ==================================================================================
        ERROR

        If you see this message, pip install did not find an available binary package
        for your system. Supported platforms are:

        * Linux x86_64 (including WSL on Windows 10).
        * macOS 10.12+ x86_64.
        * Python 2.7, 3.5, or 3.6.

        Other possible causes of this error are:

        * Outdated pip version (try `pip install -U pip`).

        ==================================================================================

        """)
        sys.stderr.write(msg)
        sys.exit(1)

if __name__ == '__main__':
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Environment :: Console",
        "Intended Audience :: Developers",
        "Intended Audience :: Financial and Insurance Industry",
        "Intended Audience :: Information Technology",
        "Intended Audience :: Other Audience",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: BSD License",
        "Natural Language :: English",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: POSIX :: Linux",
        "Operating System :: POSIX :: BSD",
        "Operating System :: Unix",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: Implementation :: CPython",
        "Topic :: Scientific/Engineering",
        "Topic :: Scientific/Engineering :: Information Analysis",
    ]

    setup(
        name="turicreate",
        version=VERSION,
        author='Apple Inc.',
        author_email='turi-create@group.apple.com',
        cmdclass=dict(install=InstallEngine),
        url='https://github.com/apple/turicreate',
        description='Turi Create simplifies the development of custom machine learning models.',
        classifiers=classifiers,
    )
