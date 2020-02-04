# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..data_structures.sframe import SFrame
from ..data_structures.sarray import SArray
from ..data_structures.image import Image
from ..util import _assert_sframe_equal, generate_random_sframe
from .. import _launch, load_sframe, aggregate
from . import util

import pandas as pd
from .._cython.cy_flexible_type import GMT
from pandas.util.testing import assert_frame_equal
import unittest
import datetime as dt
import tempfile
import os
import csv
import gzip
import string
import time
import numpy as np
import array
import math
import random
import shutil
import functools
import sys
import mock
import sqlite3
from .dbapi2_mock import dbapi2_mock


class SFrameTest(unittest.TestCase):
    def setUp(self):
        self.int_data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        self.float_data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        self.string_data = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"]
        self.a_to_z = [str(chr(97 + i)) for i in range(0, 26)]
        self.dataframe = pd.DataFrame(
            {
                "int_data": self.int_data,
                "float_data": self.float_data,
                "string_data": self.string_data,
            }
        )

        self.int_data2 = range(50, 60)
        self.float_data2 = [1.0 * i for i in range(50, 60)]
        self.string_data2 = [str(i) for i in range(50, 60)]
        self.dataframe2 = pd.DataFrame(
            {
                "int_data": self.int_data2,
                "float_data": self.float_data2,
                "string_data": self.string_data2,
            }
        )
        self.vec_data = [array.array("d", [i, i + 1]) for i in self.int_data]
        self.list_data = [[i, str(i), i * 1.0] for i in self.int_data]
        self.dict_data = [{str(i): i, i: float(i)} for i in self.int_data]
        self.datetime_data = [
            dt.datetime(2013, 5, 7, 10, 4, 10),
            dt.datetime(1902, 10, 21, 10, 34, 10).replace(tzinfo=GMT(0.0)),
        ]
        self.all_type_cols = [
            self.int_data,
            self.float_data,
            self.string_data,
            self.vec_data,
            self.list_data,
            self.dict_data,
            self.datetime_data * 5,
        ]
        self.sf_all_types = SFrame(
            {"X" + str(i[0]): i[1] for i in zip(range(1, 8), self.all_type_cols)}
        )

        # Taken from http://en.wikipedia.org/wiki/Join_(SQL) for fun.
        self.employees_sf = SFrame()
        self.employees_sf.add_column(
            SArray(["Rafferty", "Jones", "Heisenberg", "Robinson", "Smith", "John"]),
            "last_name",
            inplace=True,
        )
        self.employees_sf.add_column(
            SArray([31, 33, 33, 34, 34, None]), "dep_id", inplace=True
        )

        # XXX: below are only used by one test!
        self.departments_sf = SFrame()
        self.departments_sf.add_column(SArray([31, 33, 34, 35]), "dep_id", inplace=True)
        self.departments_sf.add_column(
            SArray(["Sales", "Engineering", "Clerical", "Marketing"]),
            "dep_name",
            inplace=True,
        )

    def __assert_sarray_equal(self, sa1, sa2):
        l1 = list(sa1)
        l2 = list(sa2)
        self.assertEqual(len(l1), len(l2))
        for i in range(len(l1)):
            v1 = l1[i]
            v2 = l2[i]
            if v1 is None:
                self.assertEqual(v2, None)
            else:
                if type(v1) == dict:
                    self.assertEqual(len(v1), len(v2))
                    for key in v1:
                        self.assertTrue(key in v1)
                        self.assertEqual(v1[key], v2[key])

                elif hasattr(v1, "__iter__"):
                    self.assertEqual(len(v1), len(v2))
                    for j in range(len(v1)):
                        t1 = v1[j]
                        t2 = v2[j]
                        if type(t1) == float:
                            if math.isnan(t1):
                                self.assertTrue(math.isnan(t2))
                            else:
                                self.assertEqual(t1, t2)
                        else:
                            self.assertEqual(t1, t2)
                else:
                    self.assertEqual(v1, v2)

    def test_split_datetime(self):
        from_zone = GMT(0)
        to_zone = GMT(4.5)
        utc = dt.datetime.strptime("2011-01-21 02:37:21", "%Y-%m-%d %H:%M:%S")
        utc = utc.replace(tzinfo=from_zone)
        central = utc.astimezone(to_zone)

        sa = SArray([utc, central])

        expected = SFrame()
        expected["X.year"] = [2011, 2011]
        expected["X.month"] = [1, 1]
        expected["X.day"] = [21, 21]
        expected["X.hour"] = [2, 7]
        expected["X.minute"] = [37, 7]
        expected["X.second"] = [21, 21]
        expected["X.timezone"] = [0.0, 4.5]
        result = sa.split_datetime(timezone=True)
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        # column names
        expected = SFrame()
        expected["ttt.year"] = [2011, 2011]
        expected["ttt.minute"] = [37, 7]
        expected["ttt.second"] = [21, 21]

        result = sa.split_datetime(
            column_name_prefix="ttt", limit=["year", "minute", "second"]
        )
        self.assertEqual(
            result.column_names(), ["ttt.year", "ttt.minute", "ttt.second"]
        )
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        sf = SFrame({"datetime": sa})
        result = sf.split_datetime(
            "datetime", column_name_prefix="ttt", limit=["year", "minute", "second"]
        )
        self.assertEqual(
            result.column_names(), ["ttt.year", "ttt.minute", "ttt.second"]
        )
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

    def __test_equal(self, sf, df):
        # asserts two frames are equal, ignoring column ordering.
        self.assertEqual(sf.num_rows(), df.shape[0])
        self.assertEqual(sf.num_columns(), df.shape[1])
        assert_frame_equal(sf.to_dataframe(), df[sf.column_names()])

    def __create_test_df(self, size):
        int_data = []
        float_data = []
        string_data = []
        for i in range(0, size):
            int_data.append(i)
            float_data.append(float(i))
            string_data.append(str(i))

        return pd.DataFrame(
            {"int_data": int_data, "float_data": float_data, "string_data": string_data}
        )

    # Test if the rows are all the same...row order does not matter.
    # (I do expect column order to be the same)
    def __assert_join_results_equal(self, sf, expected_sf):
        _assert_sframe_equal(sf, expected_sf, check_row_order=False)

    def test_creation_from_dataframe(self):
        # created from empty dataframe
        sf_empty = SFrame(data=pd.DataFrame())
        self.__test_equal(sf_empty, pd.DataFrame())

        sf = SFrame(data=self.dataframe, format="dataframe")
        self.__test_equal(sf, self.dataframe)

        sf = SFrame(data=self.dataframe, format="auto")
        self.__test_equal(sf, self.dataframe)

        original_p = pd.DataFrame({"a": [1.0, float("nan")]})
        effective_p = pd.DataFrame({"a": [1.0, None]})
        sf = SFrame(data=original_p)
        self.__test_equal(sf, effective_p)

        original_p = pd.DataFrame({"a": ["a", None, "b"]})
        sf = SFrame(data=original_p)
        self.__test_equal(sf, original_p)

    def test_auto_parse_csv_with_bom(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as csvfile:
            df = pd.DataFrame(
                {
                    "float_data": self.float_data,
                    "int_data": self.int_data,
                    "string_data": self.a_to_z[: len(self.int_data)],
                }
            )
            df.to_csv(csvfile, index=False)
            csvfile.close()

            import codecs

            with open(csvfile.name, "rb") as f:
                content = f.read()
            with open(csvfile.name, "wb") as f:
                f.write(codecs.BOM_UTF8)
                f.write(content)

            sf = SFrame.read_csv(csvfile.name, header=True)
            self.assertEqual(sf.dtype, [float, int, str])
            self.__test_equal(sf, df)

    def test_auto_parse_csv(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as csvfile:
            df = pd.DataFrame(
                {
                    "float_data": self.float_data,
                    "int_data": self.int_data,
                    "string_data": self.a_to_z[: len(self.int_data)],
                }
            )
            df.to_csv(csvfile, index=False)
            csvfile.close()

            sf = SFrame.read_csv(csvfile.name, header=True)

            self.assertEqual(sf.dtype, [float, int, str])
            self.__test_equal(sf, df)

    def test_drop_duplicate(self):
        sf = SFrame(
            {"A": ["a", "b", "a", "C"], "B": ["b", "a", "b", "D"], "C": [1, 2, 1, 8]}
        )
        df = pd.DataFrame(
            {"A": ["a", "b", "a", "C"], "B": ["b", "a", "b", "D"], "C": [1, 2, 1, 8]}
        )
        sf1 = sf.drop_duplicates(subset=["A", "B"])
        sf1 = sf1.topk("C", reverse=True)
        df1 = df.drop_duplicates(subset=["A", "B"]).reset_index(drop=True)
        self.__test_equal(sf1, df1)

    def test_parse_csv(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as csvfile:
            self.dataframe.to_csv(csvfile, index=False)
            csvfile.close()

            # list type hints
            sf = SFrame.read_csv(csvfile.name, column_type_hints=[int, int, str])
            self.assertEqual(sf.dtype, [int, int, str])
            sf["int_data"] = sf["int_data"].astype(int)
            sf["float_data"] = sf["float_data"].astype(float)
            sf["string_data"] = sf["string_data"].astype(str)
            self.__test_equal(sf, self.dataframe)

            # list type hints, incorrect number of columns
            self.assertRaises(
                RuntimeError,
                lambda: SFrame.read_csv(csvfile.name, column_type_hints=[int, float]),
            )

            # dictionary type hints
            sf = SFrame.read_csv(
                csvfile.name,
                column_type_hints={
                    "int_data": int,
                    "float_data": float,
                    "string_data": str,
                },
            )
            self.__test_equal(sf, self.dataframe)

            # partial dictionary type hints
            sf = SFrame.read_csv(
                csvfile.name,
                column_type_hints={"float_data": float, "string_data": str},
            )
            self.__test_equal(sf, self.dataframe)

            # single value type hints
            sf = SFrame.read_csv(csvfile.name, column_type_hints=str)
            self.assertEqual(sf.dtype, [str, str, str])
            all_string_column_df = self.dataframe.apply(
                lambda x: [str(ele) for ele in x]
            )
            self.__test_equal(sf, all_string_column_df)

            # single value type hints row limit
            sf = SFrame.read_csv(csvfile.name, column_type_hints=str, nrows=5)
            self.assertEqual(sf.dtype, [str, str, str])
            all_string_column_df = self.dataframe.apply(
                lambda x: [str(ele) for ele in x]
            )
            self.assertEqual(len(sf), 5)
            self.__test_equal(sf, all_string_column_df[0 : len(sf)])

            sf = SFrame.read_csv(csvfile.name)
            sf2 = SFrame(csvfile.name, format="csv")
            self.__test_equal(sf2, sf.to_dataframe())

            f = open(csvfile.name, "w")
            f.write("a,b,c\n")
            f.write("NA,PIKA,CHU\n")
            f.write("1.0,2,3\n")
            f.close()
            sf = SFrame.read_csv(
                csvfile.name,
                na_values=["NA", "PIKA", "CHU"],
                column_type_hints={"a": float, "b": int, "c": str},
            )
            t = list(sf["a"])
            self.assertEqual(t[0], None)
            self.assertEqual(t[1], 1.0)
            t = list(sf["b"])
            self.assertEqual(t[0], None)
            self.assertEqual(t[1], 2)
            t = list(sf["c"])
            self.assertEqual(t[0], None)
            self.assertEqual(t[1], "3")

    def test_parse_csv_non_multi_line_unmatched_quotation(self):
        data = [
            {"type": "foo", "text_string": "foo foo."},
            {"type": "bar", "text_string": 'bar " bar.'},
            {"type": "foo", "text_string": 'foo".'},
        ]

        with tempfile.NamedTemporaryFile(mode="w", delete=False) as csvfile:
            with open(csvfile.name, "w") as f:
                f.write("type,text_string\n")  # header
                for l in data:
                    f.write(l["type"] + "," + l["text_string"] + "\n")

            sf = SFrame.read_csv(csvfile.name, quote_char=None)
            self.assertEqual(len(sf), len(data))
            for i in range(len(sf)):
                self.assertEqual(sf[i], data[i])

    def test_save_load_file_cleanup(self):
        # when some file is in use, file should not be deleted
        with util.TempDirectory() as f:
            sf = SFrame()
            sf["a"] = SArray(range(1, 1000000))
            sf.save(f)

            # many for each sarray, 1 sframe_idx, 1 object.bin, 1 ini
            file_count = len(os.listdir(f))
            self.assertTrue(file_count > 3)

            # sf1 now references the on disk file
            sf1 = SFrame(f)

            # create another SFrame and save to the same location
            sf2 = SFrame()
            sf2["b"] = SArray([str(i) for i in range(1, 100000)])
            sf2["c"] = SArray(range(1, 100000))
            sf2.save(f)

            file_count = len(os.listdir(f))
            self.assertTrue(file_count > 3)

            # now sf1 should still be accessible
            self.__test_equal(sf1, sf.to_dataframe())

            # and sf2 is correct too
            sf3 = SFrame(f)
            self.__test_equal(sf3, sf2.to_dataframe())

            # when sf1 goes out of scope, the tmp files should be gone
            sf1 = 1
            time.sleep(1)  # give time for the files being deleted
            file_count = len(os.listdir(f))
            self.assertTrue(file_count > 3)

    def test_save_load(self):

        # Check top level load function, with no suffix
        with util.TempDirectory() as f:
            sf = SFrame(data=self.dataframe, format="dataframe")
            sf.save(f)
            sf2 = load_sframe(f)
            self.__test_equal(sf2, self.dataframe)

        # Check individual formats with the SFrame constructor
        formats = [".csv"]

        for suffix in formats:
            f = tempfile.NamedTemporaryFile(suffix=suffix, delete=False)
            sf = SFrame(data=self.dataframe, format="dataframe")
            sf.save(f.name)
            sf2 = SFrame(f.name)
            sf2["int_data"] = sf2["int_data"].astype(int)
            sf2["float_data"] = sf2["float_data"].astype(float)
            sf2["string_data"] = sf2["string_data"].astype(str)
            self.__test_equal(sf2, self.dataframe)
            g = SArray([["a", "b", 3], [{"a": "b"}], [1, 2, 3]])
            g2 = SFrame()
            g2["x"] = g
            g2.save(f.name)
            g3 = SFrame.read_csv(f.name, column_type_hints=list)
            self.__test_equal(g2, g3.to_dataframe())
            f.close()
            os.unlink(f.name)

        # Make sure this file don't exist before testing
        self.assertRaises(
            IOError, lambda: SFrame(data="__no_such_file__.frame_idx", format="sframe")
        )

        del sf2

    def test_save_load_reference(self):

        # Check top level load function, with no suffix
        with util.TempDirectory() as f:
            sf = SFrame(data=self.dataframe, format="dataframe")
            originallen = len(sf)
            sf.save(f)
            del sf

            sf = SFrame(f)
            # make a new column of "1s and save it back
            int_data2 = sf["int_data"] + 1
            int_data2.materialize()
            sf["int_data2"] = int_data2
            sf._save_reference(f)
            del sf

            sf = SFrame(f)
            self.assertTrue(((sf["int_data2"] - sf["int_data"]) == 1).all())

            # try to append and save reference
            expected = sf.to_dataframe()
            sf = sf.append(sf)
            sf._save_reference(f)

            sf = SFrame(f)
            self.assertTrue(((sf["int_data2"] - sf["int_data"]) == 1).all())
            self.assertEqual(2 * originallen, len(sf))
            assert_frame_equal(sf[originallen:].to_dataframe(), expected)
            assert_frame_equal(sf[:originallen].to_dataframe(), expected)

    def test_save_to_csv(self):
        f = tempfile.NamedTemporaryFile(suffix=".csv", delete=False)
        sf = SFrame(data=self.dataframe, format="dataframe")
        sf.save(f.name, format="csv")
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
        )
        self.__test_equal(sf2, self.dataframe)

        sf.export_csv(f.name, delimiter=":")
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
            delimiter=":",
        )
        self.__test_equal(sf2, self.dataframe)

        sf.export_csv(f.name, delimiter=":", line_terminator="\r\n")
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
            delimiter=":",
            line_terminator="\r\n",
        )
        self.__test_equal(sf2, self.dataframe)

        sf.export_csv(f.name, delimiter=":", line_terminator="\r\n", double_quote=False)
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
        )
        self.__test_equal(sf2, self.dataframe)

        sf.export_csv(
            f.name,
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
            quote_char="'",
        )
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
            quote_char="'",
        )
        self.__test_equal(sf2, self.dataframe)

        import csv

        sf.export_csv(
            f.name,
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
            quote_char="'",
            quote_level=csv.QUOTE_MINIMAL,
        )
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
            quote_char="'",
        )
        self.__test_equal(sf2, self.dataframe)

        sf.export_csv(
            f.name,
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
            quote_char="'",
            quote_level=csv.QUOTE_ALL,
        )
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
            quote_char="'",
        )
        self.__test_equal(sf2, self.dataframe)

        sf.export_csv(
            f.name,
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
            quote_char="'",
            quote_level=csv.QUOTE_NONE,
        )
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
            delimiter=":",
            line_terminator="\r\n",
            double_quote=False,
            quote_char="'",
        )
        self.__test_equal(sf2, self.dataframe)

        # Pandas compatibility options
        sf.export_csv(
            f.name,
            sep=":",
            lineterminator="\r\n",
            doublequote=False,
            quotechar="'",
            quote_level=csv.QUOTE_NONE,
        )
        sf2 = SFrame.read_csv(
            f.name,
            column_type_hints={
                "int_data": int,
                "float_data": float,
                "string_data": str,
            },
            sep=":",
            lineterminator="\r\n",
            doublequote=False,
            quotechar="'",
        )
        self.__test_equal(sf2, self.dataframe)
        f.close()
        os.unlink(f.name)

    def test_save_to_json(self):
        f = tempfile.NamedTemporaryFile(suffix=".json", delete=False)
        sf = SFrame(data=self.dataframe, format="dataframe")
        sf.save(f.name, format="json")
        sf2 = SFrame.read_json(f.name)
        # the float column will be parsed as integer
        sf2["float_data"] = sf2["float_data"].astype(float)
        self.__test_equal(sf2, self.dataframe)

        sf = SFrame(data=self.dataframe, format="dataframe")
        sf.export_json(f.name)
        sf2 = SFrame.read_json(f.name)
        sf2["float_data"] = sf2["float_data"].astype(float)
        self.__test_equal(sf2, self.dataframe)

        with open(f.name, "w") as out:
            out.write("[\n]")
        sf = SFrame.read_json(f.name)
        self.__test_equal(SFrame(), sf.to_dataframe())

        with open(f.name, "w") as out:
            out.write("")
        sf = SFrame.read_json(f.name, orient="lines")
        self.__test_equal(SFrame(), sf.to_dataframe())

        sf = SFrame(data=self.dataframe, format="dataframe")
        sf.export_json(f.name, orient="lines")
        sf2 = SFrame.read_json(f.name, orient="lines")
        sf2["float_data"] = sf2["float_data"].astype(float)
        self.__test_equal(sf2, self.dataframe)
        f.close()
        os.unlink(f.name)

    def _remove_sframe_files(self, prefix):
        filelist = [f for f in os.listdir(".") if f.startswith(prefix)]
        for f in filelist:
            os.remove(f)

    def test_creation_from_txt(self):
        f = tempfile.NamedTemporaryFile(suffix=".txt", delete=False)
        df = self.dataframe[["string_data"]]
        df.to_csv(f.name, index=False)
        sf = SFrame(f.name)
        self.assertEqual(sf["string_data"].dtype, int)
        sf["string_data"] = sf["string_data"].astype(str)
        self.__test_equal(sf, df)

        fgzip = tempfile.NamedTemporaryFile(suffix=".txt.gz", delete=False)
        f_in = open(f.name, "rb")
        f_out = gzip.open(fgzip.name, "wb")
        f_out.writelines(f_in)
        f_out.close()
        f_in.close()
        sf = SFrame(fgzip.name)
        self.assertEqual(sf["string_data"].dtype, int)
        sf["string_data"] = sf["string_data"].astype(str)
        self.__test_equal(sf, df)

        fgzip.close()
        os.unlink(fgzip.name)
        f.close()
        os.unlink(f.name)

    def test_creation_from_csv_on_local(self):
        if os.path.exists("./foo.csv"):
            os.remove("./foo.csv")
        with open("./foo.csv", "w") as f:
            url = f.name
            basesf = SFrame(self.dataframe)
            basesf.save(url, format="csv")
            f.close()
            sf = SFrame("./foo.csv")
            self.assertEqual(sf["float_data"].dtype, int)
            sf["float_data"] = sf["float_data"].astype(float)
            self.assertEqual(sf["string_data"].dtype, int)
            sf["string_data"] = sf["string_data"].astype(str)
            self.__test_equal(sf, self.dataframe)
            sf = SFrame(url)
            self.assertEqual(sf["float_data"].dtype, int)
            sf["float_data"] = sf["float_data"].astype(float)
            self.assertEqual(sf["string_data"].dtype, int)
            sf["string_data"] = sf["string_data"].astype(str)
            self.__test_equal(sf, self.dataframe)
            os.remove(url)

    def test_alternate_line_endings(self):
        # test Windows line endings
        if os.path.exists("./windows_lines.csv"):
            os.remove("./windows_lines.csv")
        windows_file_url = None
        with open("./windows_lines.csv", "w") as f:
            windows_file_url = f.name
            def_writer = csv.writer(f, dialect="excel")
            column_list = ["numbers"]
            def_writer.writerow(column_list)
            for i in self.int_data:
                def_writer.writerow([i])

        sf = SFrame.read_csv("./windows_lines.csv", column_type_hints={"numbers": int})
        self.assertEqual(sf.column_names(), column_list)
        self.assertEqual(sf.column_types(), [int])
        self.assertEqual(list(sf["numbers"].head()), self.int_data)

        sf = SFrame.read_csv(
            "./windows_lines.csv",
            column_type_hints={"numbers": list},
            error_bad_lines=False,
        )
        self.assertEqual(sf.column_names(), column_list)
        self.assertEqual(sf.num_rows(), 0)

        os.remove(windows_file_url)

    def test_skip_rows(self):
        # test line skipping
        if os.path.exists("./skip_lines.csv"):
            os.remove("./skip_lines.csv")
        skip_file_url = None
        with open("./skip_lines.csv", "w") as f:
            f.write("trash\n")
            f.write("junk\n")
            skip_file_url = f.name
            def_writer = csv.writer(f, dialect="excel")
            column_list = ["numbers"]
            def_writer.writerow(column_list)
            for i in self.int_data:
                def_writer.writerow([i])

        sf = SFrame.read_csv(
            "./skip_lines.csv", skiprows=2, column_type_hints={"numbers": int}
        )
        self.assertEqual(sf.column_names(), column_list)
        self.assertEqual(sf.column_types(), [int])
        self.assertEqual(list(sf["numbers"].head()), self.int_data)

        sf = SFrame.read_csv(
            "./skip_lines.csv",
            skiprows=2,
            column_type_hints={"numbers": list},
            error_bad_lines=False,
        )
        self.assertEqual(sf.column_names(), column_list)
        self.assertEqual(sf.num_rows(), 0)

        os.remove(skip_file_url)

    def test_creation_from_csv_dir_local(self):
        csv_dir = "./csv_dir"

        if os.path.exists(csv_dir):
            shutil.rmtree(csv_dir)
        os.mkdir(csv_dir)

        for i in range(0, 100):
            with open(os.path.join(csv_dir, "foo.%d.csv" % i), "w") as f:
                url = f.name
                self.dataframe.to_csv(url, index=False)
                f.close()

        singleton_sf = SFrame.read_csv(os.path.join(csv_dir, "foo.0.csv"))
        self.assertEqual(singleton_sf.num_rows(), 10)

        many_sf = SFrame.read_csv(csv_dir)
        self.assertEqual(many_sf.num_rows(), 1000)

        glob_sf = SFrame.read_csv(os.path.join(csv_dir, "foo.*2.csv"))
        self.assertEqual(glob_sf.num_rows(), 100)

        with self.assertRaises(IOError):
            SFrame.read_csv("missingdirectory")

        with self.assertRaises(ValueError):
            SFrame.read_csv("")

        shutil.rmtree(csv_dir)

    def test_creation_from_iterable(self):
        # Normal dict of lists
        the_dict = {
            "ints": self.int_data,
            "floats": self.float_data,
            "strings": self.string_data,
        }
        sf = SFrame(the_dict)
        df = pd.DataFrame(the_dict)
        self.__test_equal(sf, df)

        # Test that a missing value does not change the data type
        the_dict["ints"][0] = None
        sf = SFrame(the_dict)
        self.assertEqual(sf["ints"].dtype, int)

        # numpy.nan is actually a float, so it should cast the column to float
        the_dict["ints"][0] = np.nan
        sf = SFrame(the_dict)
        self.assertEqual(sf["ints"].dtype, float)

        # Just a single list
        sf = SFrame(self.int_data)
        df = pd.DataFrame(self.int_data)
        df.columns = ["X1"]
        self.__test_equal(sf, df)

        # Normal list of lists
        list_of_lists = [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]]
        sf = SFrame(list_of_lists)
        cntr = 0
        for i in sf:
            self.assertEqual(list_of_lists[cntr], list(i["X1"]))
            cntr += 1

        self.assertEqual(sf.num_columns(), 1)

        the_dict = {
            "ints": self.int_data,
            "floats": self.float_data,
            "strings": self.string_data,
        }
        sf = SFrame(the_dict)
        sf2 = SFrame(
            {"ints": sf["ints"], "floats": sf["floats"], "strings": sf["strings"]}
        )
        df = pd.DataFrame(the_dict)
        self.__test_equal(sf2, df)
        sf2 = SFrame([sf["ints"], sf["floats"], sf["strings"]])
        self.assertEqual(["X1", "X2", "X3"], sf2.column_names())
        sf2.rename({"X1": "ints", "X2": "floats", "X3": "strings"}, inplace=True)
        sf2 = sf2[["floats", "ints", "strings"]]
        self.__test_equal(sf2, df)

        sf = SFrame({"text": ("foo", "bar", "biz")})
        df = pd.DataFrame({"text": ["foo", "bar", "biz"]})
        self.__test_equal(sf, df)

    def test_head_tail(self):
        sf = SFrame(data=self.dataframe)
        assert_frame_equal(sf.head(4).to_dataframe(), self.dataframe.head(4))
        # Cannot test for equality the same way because of dataframe indices
        taildf = sf.tail(4)
        for i in range(0, 4):
            self.assertEqual(taildf["int_data"][i], self.dataframe["int_data"][i + 6])
            self.assertEqual(
                taildf["float_data"][i], self.dataframe["float_data"][i + 6]
            )
            self.assertEqual(
                taildf["string_data"][i], self.dataframe["string_data"][i + 6]
            )

    def test_head_tail_edge_case(self):
        sf = SFrame()
        self.assertEqual(sf.head().num_columns(), 0)
        self.assertEqual(sf.tail().num_columns(), 0)
        self.assertEqual(sf.head().num_rows(), 0)
        self.assertEqual(sf.tail().num_rows(), 0)
        sf = SFrame()
        sf["a"] = []
        self.assertEqual(sf.head().num_columns(), 1)
        self.assertEqual(sf.tail().num_columns(), 1)
        self.assertEqual(sf.head().num_rows(), 0)
        self.assertEqual(sf.tail().num_rows(), 0)

    def test_transform(self):
        sf = SFrame(data=self.dataframe)
        for i in range(sf.num_columns()):
            colname = sf.column_names()[i]
            sa = sf.apply(lambda x: x[colname], sf.column_types()[i])
            self.__assert_sarray_equal(sa, sf[sf.column_names()[i]])

        sa = sf.apply(lambda x: x["int_data"] + x["float_data"], float)
        self.__assert_sarray_equal(sf["int_data"] + sf["float_data"], sa)

    def test_add(self):
        sf1 = SFrame({"a": [1, 2, 3]})
        sf2 = SFrame({"a": [6, 7]})
        sf1 = sf1 + sf2
        expected = SFrame({"a": [1, 2, 3, 6, 7]})
        _assert_sframe_equal(sf1, expected)

    def test_transform_with_recursion(self):
        sf = SFrame(data={"a": [0, 1, 2, 3, 4], "b": ["0", "1", "2", "3", "4"]})
        # this should be the equivalent to sf.apply(lambda x:x since a is
        # equivalent to range(4)
        sa = sf.apply(lambda x: sf[x["a"]])
        sb = sf.apply(lambda x: x)
        self.__assert_sarray_equal(sa, sb)

    def test_transform_with_type_inference(self):
        sf = SFrame(data=self.dataframe)
        for i in range(sf.num_columns()):
            colname = sf.column_names()[i]
            sa = sf.apply(lambda x: x[colname])
            self.__assert_sarray_equal(sa, sf[sf.column_names()[i]])

        sa = sf.apply(lambda x: x["int_data"] + x["float_data"])
        self.__assert_sarray_equal(sf["int_data"] + sf["float_data"], sa)

        # SFrame apply returns list of vector of numeric should be vector, not list
        sa = sf.apply(lambda x: [x["int_data"], x["float_data"]])
        self.assertEqual(sa.dtype, array.array)

    def test_transform_with_exception(self):
        sf = SFrame(data=self.dataframe)
        self.assertRaises(
            KeyError, lambda: sf.apply(lambda x: x["some random key"])
        )  # cannot find the key
        self.assertRaises(
            TypeError, lambda: sf.apply(lambda x: sum(x.values()))
        )  # lambda cannot sum int and str
        self.assertRaises(
            ZeroDivisionError, lambda: sf.apply(lambda x: x["int_data"] / 0)
        )  # divide by 0 error
        self.assertRaises(
            IndexError, lambda: sf.apply(lambda x: list(x.values())[10])
        )  # index out of bound error

    def test_empty_transform(self):
        sf = SFrame()
        b = sf.apply(lambda x: x)
        self.assertEqual(len(b.head()), 0)

    def test_flatmap(self):
        # Correctness of typical usage
        n = 10
        sf = SFrame({"id": range(n)})
        new_sf = sf.flat_map(["id_range"], lambda x: [[str(i)] for i in range(x["id"])])
        self.assertEqual(new_sf.column_names(), ["id_range"])
        self.assertEqual(new_sf.column_types(), [str])
        expected_col = [str(x) for i in range(n) for x in range(i)]
        self.assertListEqual(list(new_sf["id_range"]), expected_col)

        # Empty SFrame, without explicit column types
        sf = SFrame()
        with self.assertRaises(TypeError):
            new_sf = sf.flat_map(["id_range"], lambda x: [[i] for i in range(x["id"])])

        # Empty rows successfully removed
        sf = SFrame({"id": range(15)})
        new_sf = sf.flat_map(["id"], lambda x: [[x["id"]]] if x["id"] > 8 else [])
        self.assertEqual(new_sf.num_rows(), 6)

        # First ten rows are empty raises error
        with self.assertRaises(TypeError):
            new_sf = sf.flat_map(["id"], lambda x: [[x["id"]]] if x["id"] > 9 else [])

    def test_select_column(self):
        sf = SFrame(data=self.dataframe)

        sub_sf = sf.select_columns(["int_data", "string_data"])
        exp_df = pd.DataFrame(
            {"int_data": self.int_data, "string_data": self.string_data}
        )
        self.__test_equal(sub_sf, exp_df)

        with self.assertRaises(ValueError):
            sf.select_columns(["int_data", "string_data", "int_data"])

        # test indexing
        sub_col = sf["float_data"]
        self.assertEqual(list(sub_col.head(10)), self.float_data)

        with self.assertRaises(TypeError):
            sub_sf = sf.select_columns(["duh", 1])

        with self.assertRaises(TypeError):
            sub_sf = sf.select_columns(0)

        with self.assertRaises(RuntimeError):
            sub_sf = sf.select_columns(["not_a_column"])

        self.assertEqual(sf.select_columns([int]).column_names(), ["int_data"])
        self.assertEqual(
            sf.select_columns([int, str]).column_names(), ["int_data", "string_data"]
        )

        self.assertEqual(sf[int].column_names(), ["int_data"])
        self.assertEqual(sf[[int, str]].column_names(), ["int_data", "string_data"])
        self.assertEqual(sf[int, str].column_names(), ["int_data", "string_data"])
        self.assertEqual(
            sf["int_data", "string_data"].column_names(), ["int_data", "string_data"]
        )
        self.assertEqual(
            sf["string_data", "int_data"].column_names(), ["string_data", "int_data"]
        )

        sf = SFrame()
        with self.assertRaises(RuntimeError):
            sf.select_column("x")

        with self.assertRaises(RuntimeError):
            sf.select_columns(["x"])

        sf.add_column(SArray(), "x", inplace=True)
        # does not throw
        sf.select_column("x")
        sf.select_columns(["x"])
        with self.assertRaises(RuntimeError):
            sf.select_column("y")

        with self.assertRaises(RuntimeError):
            sf.select_columns(["y"])

    def test_topk(self):
        sf = SFrame(data=self.dataframe)

        # Test that order is preserved
        df2 = sf.topk("int_data").to_dataframe()
        df2_expected = self.dataframe.sort_values("int_data", ascending=False)
        df2_expected.index = range(df2.shape[0])
        assert_frame_equal(df2, df2_expected)

        df2 = sf.topk("float_data", 3).to_dataframe()
        df2_expected = self.dataframe.sort_values("float_data", ascending=False).head(3)
        df2_expected.index = range(3)
        assert_frame_equal(df2, df2_expected)

        df2 = sf.topk("string_data", 3).to_dataframe()
        for i in range(0, 3):
            self.assertEqual(df2["int_data"][2 - i], i + 7)

        with self.assertRaises(TypeError):
            sf.topk(2, 3)

        sf = SFrame()
        sf.add_column(SArray([1, 2, 3, 4, 5]), "a", inplace=True)
        sf.add_column(SArray([1, 2, 3, 4, 5]), "b", inplace=True)

        sf.topk("a", 1)  # should not fail

    def test_filter(self):
        sf = SFrame(data=self.dataframe)

        filter_sa = SArray([1, 1, 1, 0, 0, 0, 0, 1, 1, 1])

        sf2 = sf[filter_sa]
        exp_df = sf.head(3).append(sf.tail(3))
        self.__test_equal(sf2, exp_df.to_dataframe())

        # filter by 1s
        sf2 = sf[SArray(self.int_data)]
        exp_df = sf.head(10).to_dataframe()
        self.__test_equal(sf2, exp_df)

        # filter by 0s
        sf2 = sf[SArray([0, 0, 0, 0, 0, 0, 0, 0, 0, 0])]
        exp_df = sf.head(0).to_dataframe()
        self.__test_equal(sf2, exp_df)

        # wrong size
        with self.assertRaises(IndexError):
            sf2 = sf[SArray([0, 1, 205])]

        # slightly bigger size
        sf = SFrame()
        n = 1000000
        sf["a"] = range(n)
        result = sf[sf["a"] == -1]
        self.assertEqual(len(result), 0)

        result = sf[sf["a"] > n - 123]
        self.assertEqual(len(result), 122)
        l = list(result["a"])
        for i in range(len(result)):
            self.assertEqual(i + n - 122, l[i])

        result = sf[sf["a"] < 2000]
        self.assertEqual(len(result), 2000)
        l = list(result["a"])
        for i in range(len(result)):
            self.assertEqual(i, l[i])

        # map input type
        toy_data = SFrame({"a": range(100)})
        map_result = map(lambda x: x + 1, [1, 30])
        result = toy_data.filter_by(map_result, "a")
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]["a"], 2)
        self.assertEqual(result[1]["a"], 31)

    def test_sample_split(self):
        sf = SFrame(data=self.__create_test_df(100))
        entry_list = set()
        for i in sf:
            entry_list.add(str(i))

        sample_sf = sf.sample(0.12, 9)
        sample_sf2 = sf.sample(0.12, 9)
        self.assertEqual(len(sample_sf), len(sample_sf2))
        assert_frame_equal(
            sample_sf.head().to_dataframe(), sample_sf2.head().to_dataframe()
        )
        self.assertEqual(len(sf.sample(0.5, 1, exact=True)), 50)
        self.assertEqual(len(sf.sample(0.5, 2, exact=True)), 50)

        for i in sample_sf:
            self.assertTrue(str(i) in entry_list)

        with self.assertRaises(ValueError):
            sf.sample(3)

        sample_sf = SFrame().sample(0.12, 9)
        self.assertEqual(len(sample_sf), 0)

        a_split = sf.random_split(0.12, 9)

        first_split_entries = set()
        for i in a_split[0]:
            first_split_entries.add(str(i))

        for i in a_split[1]:
            self.assertTrue(str(i) in entry_list)
            self.assertTrue(str(i) not in first_split_entries)

        with self.assertRaises(ValueError):
            sf.random_split(3)

        self.assertEqual(len(SFrame().random_split(0.4)[0]), 0)
        self.assertEqual(len(SFrame().random_split(0.4)[1]), 0)

        self.assertEqual(len(sf.random_split(0.5, 1, exact=True)[0]), 50)
        self.assertEqual(len(sf.random_split(0.5, 2, exact=True)[0]), 50)

    # tests add_column, rename
    def test_edit_column_ops(self):
        sf = SFrame()

        # typical add column stuff
        sf.add_column(SArray(self.int_data), inplace=True)
        sf.add_column(SArray(self.float_data), inplace=True)
        sf.add_column(SArray(self.string_data), inplace=True)

        # Make sure auto names work
        names = sf.column_names()
        cntr = 1
        for i in names:
            self.assertEqual("X" + str(cntr), i)
            cntr = cntr + 1

        # Remove a column
        del sf["X2"]

        # names
        names = sf.column_names()
        self.assertEqual(len(names), 2)
        self.assertEqual("X1", names[0])
        self.assertEqual("X3", names[1])

        # check content
        self.assertEqual(list(sf["X1"].head(10)), self.int_data)
        self.assertEqual(list(sf["X3"].head(10)), self.string_data)

        # check that a new automatically named column will not conflict
        sf.add_column(SArray(self.string_data), inplace=True)

        names = sf.column_names()
        self.assertEqual(len(names), 3)
        uniq_set = set()
        for i in names:
            uniq_set.add(i)
            if len(uniq_set) == 1:
                self.assertEqual(list(sf[i].head(10)), self.int_data)
            else:
                self.assertEqual(list(sf[i].head(10)), self.string_data)
        self.assertEqual(len(uniq_set), 3)

        # replacing columns preserves order
        names = sf.column_names()
        for n in names:
            sf[n] = sf[n].apply(lambda x: x)
            self.assertEqual(sf.column_names(), names)

        # do it again!
        del sf["X1"]

        sf.add_column(SArray(self.string_data), inplace=True)
        names = sf.column_names()
        self.assertEqual(len(names), 3)
        uniq_set = set()
        for i in names:
            uniq_set.add(i)
            self.assertEqual(list(sf[i].head(10)), self.string_data)
        self.assertEqual(len(uniq_set), len(names))

        # standard rename
        rename_dict = {"X3": "data", "X3.1": "more_data", "X3.2": "even_more"}
        sf.rename(rename_dict, inplace=True)
        self.assertEqual(sf.column_names(), ["data", "more_data", "even_more"])

        # rename a column to a name that's already taken
        with self.assertRaises(RuntimeError):
            sf.rename({"data": "more_data"}, inplace=True)

        # try to rename a column that doesn't exist
        with self.assertRaises(ValueError):
            sf.rename({"foo": "bar"}, inplace=True)

        # pass something other than a dict
        with self.assertRaises(TypeError):
            sf.rename("foo", inplace=True)

        # Setting a column to const preserves order
        names = sf.column_names()
        for n in names:
            sf[n] = 1
            self.assertEqual(sf.column_names(), names)

    def test_duplicate_add_column_failure(self):
        sf = SFrame()

        # typical add column stuff
        sf.add_column(SArray(self.int_data), "hello", inplace=True)
        with self.assertRaises(RuntimeError):
            sf.add_column(SArray(self.float_data), "hello", inplace=True)

    def test_remove_column(self):
        sf = SFrame()

        # typical add column stuff
        sf.add_column(SArray(self.int_data), inplace=True)
        sf.add_column(SArray(self.int_data), inplace=True)
        sf.add_column(SArray(self.int_data), inplace=True)
        sf.add_column(SArray(self.float_data), inplace=True)
        sf.add_column(SArray(self.string_data), inplace=True)

        self.assertEqual(sf.column_names(), ["X1", "X2", "X3", "X4", "X5"])

        sf2 = sf.remove_column("X3", inplace=True)

        assert sf is sf2

        self.assertEqual(sf.column_names(), ["X1", "X2", "X4", "X5"])

        sf2 = sf.remove_columns(["X2", "X5"], inplace=True)

        assert sf is sf2

        self.assertEqual(sf.column_names(), ["X1", "X4"])

        # with a generator expression
        sf2 = sf.remove_columns(
            (n for n in ["X1", "X5"] if n in sf.column_names()), inplace=True
        )

        assert sf is sf2

        self.assertEqual(sf.column_names(), ["X4"])

    def test_remove_bad_column(self):
        sf = SFrame()

        # typical add column stuff
        sf.add_column(SArray(self.int_data), inplace=True)
        sf.add_column(SArray(self.int_data), inplace=True)
        sf.add_column(SArray(self.int_data), inplace=True)
        sf.add_column(SArray(self.float_data), inplace=True)
        sf.add_column(SArray(self.string_data), inplace=True)

        self.assertEqual(sf.column_names(), ["X1", "X2", "X3", "X4", "X5"])

        self.assertRaises(KeyError, lambda: sf.remove_column("bad", inplace=True))

        self.assertEqual(sf.column_names(), ["X1", "X2", "X3", "X4", "X5"])

        self.assertRaises(
            KeyError,
            lambda: sf.remove_columns(["X1", "X2", "X3", "bad", "X4"], inplace=True),
        )

        self.assertEqual(sf.column_names(), ["X1", "X2", "X3", "X4", "X5"])

    def __generate_synthetic_sframe__(self, num_users):
        """
        synthetic collaborative data.
        generate 1000 users, user i watched movie 0, ... i-1.
        rating(i, j) = i + j
        length(i, j) = i - j
        """
        sf = SFrame()
        sparse_matrix = {}
        for i in range(1, num_users + 1):
            sparse_matrix[i] = [(j, i + j, i - j) for j in range(1, i + 1)]
        user_ids = []
        movie_ids = []
        ratings = []
        length_of_watching = []
        for u in sparse_matrix:
            user_ids += [u] * len(sparse_matrix[u])
            movie_ids += [x[0] for x in sparse_matrix[u]]
            ratings += [x[1] for x in sparse_matrix[u]]
            length_of_watching += [x[2] for x in sparse_matrix[u]]
        # typical add column stuff
        sf["user_id"] = SArray(user_ids, int)
        sf["movie_id"] = SArray(movie_ids, str)
        sf["rating"] = SArray(ratings, float)
        sf["length"] = SArray(length_of_watching, int)
        return sf

    def test_aggregate_ops(self):
        """
        Test builtin groupby aggregators
        """
        for m in [1, 10, 20, 50, 100]:
            values = range(m)
            vector_values = [
                [random.randint(1, 100) for num in range(10)] for y in range(m)
            ]
            nd_values = [
                np.array([float(random.randint(1, 100)) for num in range(10)]).reshape(
                    2, 5
                )
                for y in range(m)
            ]
            sf = SFrame()
            sf["key"] = [1] * m
            sf["value"] = values
            sf["vector_values"] = vector_values
            sf["nd_values"] = nd_values
            sf.materialize()
            built_ins = [
                aggregate.COUNT(),
                aggregate.SUM("value"),
                aggregate.AVG("value"),
                aggregate.MIN("value"),
                aggregate.MAX("value"),
                aggregate.VAR("value"),
                aggregate.STDV("value"),
                aggregate.SUM("vector_values"),
                aggregate.MEAN("vector_values"),
                aggregate.COUNT_DISTINCT("value"),
                aggregate.DISTINCT("value"),
                aggregate.FREQ_COUNT("value"),
                aggregate.SUM("nd_values"),
                aggregate.MEAN("nd_values"),
            ]
            sf2 = sf.groupby("key", built_ins)
            self.assertEqual(len(sf2), 1)
            self.assertEqual(sf2["Count"][0], m)
            self.assertEqual(sf2["Sum of value"][0], sum(values))
            self.assertAlmostEqual(sf2["Avg of value"][0], np.mean(values))
            self.assertEqual(sf2["Min of value"][0], min(values))
            self.assertEqual(sf2["Max of value"][0], max(values))
            self.assertAlmostEqual(sf2["Var of value"][0], np.var(values))
            self.assertAlmostEqual(sf2["Stdv of value"][0], np.std(values))
            np.testing.assert_almost_equal(
                list(sf2["Vector Sum of vector_values"][0]),
                list(np.sum(vector_values, axis=0)),
            )
            np.testing.assert_almost_equal(
                list(sf2["Vector Avg of vector_values"][0]),
                list(np.mean(vector_values, axis=0)),
            )
            np.testing.assert_almost_equal(
                list(sf2["Vector Sum of nd_values"][0]), list(np.sum(nd_values, axis=0))
            )
            np.testing.assert_almost_equal(
                list(sf2["Vector Avg of nd_values"][0]),
                list(np.mean(nd_values, axis=0)),
            )
            self.assertEqual(sf2["Count Distinct of value"][0], len(np.unique(values)))
            self.assertEqual(
                sorted(sf2["Distinct of value"][0]), sorted(list(np.unique(values)))
            )
            self.assertEqual(
                sf2["Frequency Count of value"][0], {k: 1 for k in np.unique(values)}
            )

        # For vectors

    def test_min_max_with_missing_values(self):
        """
        Test builtin groupby aggregators
        """
        sf = SFrame()
        sf["key"] = [1, 1, 1, 1, 1, 1, 2, 2, 2, 2]
        sf["value"] = [1, None, None, None, None, None, None, None, None, None]
        built_ins = [
            aggregate.COUNT(),
            aggregate.SUM("value"),
            aggregate.AVG("value"),
            aggregate.MIN("value"),
            aggregate.MAX("value"),
            aggregate.VAR("value"),
            aggregate.STDV("value"),
            aggregate.COUNT_DISTINCT("value"),
            aggregate.DISTINCT("value"),
            aggregate.FREQ_COUNT("value"),
        ]
        sf2 = sf.groupby("key", built_ins).sort("key")
        self.assertEqual(list(sf2["Count"]), [6, 4])
        self.assertEqual(list(sf2["Sum of value"]), [1, 0])
        self.assertEqual(list(sf2["Avg of value"]), [1, None])
        self.assertEqual(list(sf2["Min of value"]), [1, None])
        self.assertEqual(list(sf2["Max of value"]), [1, None])
        self.assertEqual(list(sf2["Var of value"]), [0, 0])
        self.assertEqual(list(sf2["Stdv of value"]), [0, 0])
        self.assertEqual(list(sf2["Count Distinct of value"]), [2, 1])
        self.assertEqual(set(sf2["Distinct of value"][0]), set([1, None]))
        self.assertEqual(set(sf2["Distinct of value"][1]), set([None]))
        self.assertEqual(sf2["Frequency Count of value"][0], {1: 1, None: 5})
        self.assertEqual(sf2["Frequency Count of value"][1], {None: 4})

    def test_aggregate_ops_on_lazy_frame(self):
        """
        Test builtin groupby aggregators
        """
        for m in [1, 10, 20, 50, 100]:
            values = range(m)
            vector_values = [
                [random.randint(1, 100) for num in range(10)] for y in range(m)
            ]
            sf = SFrame()
            sf["key"] = [1] * m
            sf["value"] = values
            sf["vector_values"] = vector_values
            sf["value"] = sf["value"] + 0
            built_ins = [
                aggregate.COUNT(),
                aggregate.SUM("value"),
                aggregate.AVG("value"),
                aggregate.MIN("value"),
                aggregate.MAX("value"),
                aggregate.VAR("value"),
                aggregate.STDV("value"),
                aggregate.SUM("vector_values"),
                aggregate.MEAN("vector_values"),
                aggregate.COUNT_DISTINCT("value"),
                aggregate.DISTINCT("value"),
            ]
            sf2 = sf.groupby("key", built_ins)
            self.assertEqual(len(sf2), 1)
            self.assertEqual(sf2["Count"][0], m)
            self.assertEqual(sf2["Sum of value"][0], sum(values))
            self.assertAlmostEqual(sf2["Avg of value"][0], np.mean(values))
            self.assertEqual(sf2["Min of value"][0], min(values))
            self.assertEqual(sf2["Max of value"][0], max(values))
            self.assertAlmostEqual(sf2["Var of value"][0], np.var(values))
            self.assertAlmostEqual(sf2["Stdv of value"][0], np.std(values))
            np.testing.assert_almost_equal(
                list(sf2["Vector Sum of vector_values"][0]),
                list(np.sum(vector_values, axis=0)),
            )
            np.testing.assert_almost_equal(
                list(sf2["Vector Avg of vector_values"][0]),
                list(np.mean(vector_values, axis=0)),
            )
            self.assertEqual(sf2["Count Distinct of value"][0], len(np.unique(values)))
            self.assertEqual(
                sorted(sf2["Distinct of value"][0]), sorted(np.unique(values))
            )

    def test_aggregate_ops2(self):
        """
        Test builtin groupby aggregators using explicit named columns
        """
        for m in [1, 10, 20, 50, 100]:
            values = range(m)
            vector_values = [
                [random.randint(1, 100) for num in range(10)] for y in range(m)
            ]
            sf = SFrame()
            sf["key"] = [1] * m
            sf["value"] = values
            sf["vector_values"] = vector_values
            built_ins = {
                "count": aggregate.COUNT,
                "sum": aggregate.SUM("value"),
                "avg": aggregate.AVG("value"),
                "avg2": aggregate.MEAN("value"),
                "min": aggregate.MIN("value"),
                "max": aggregate.MAX("value"),
                "var": aggregate.VAR("value"),
                "var2": aggregate.VARIANCE("value"),
                "stdv": aggregate.STD("value"),
                "stdv2": aggregate.STDV("value"),
                "vector_sum": aggregate.SUM("vector_values"),
                "vector_mean": aggregate.MEAN("vector_values"),
                "count_unique": aggregate.COUNT_DISTINCT("value"),
                "unique": aggregate.DISTINCT("value"),
                "frequency": aggregate.FREQ_COUNT("value"),
            }
            sf2 = sf.groupby("key", built_ins)
            self.assertEqual(len(sf2), 1)
            self.assertEqual(sf2["count"][0], m)
            self.assertEqual(sf2["sum"][0], sum(values))
            self.assertAlmostEqual(sf2["avg"][0], np.mean(values))
            self.assertAlmostEqual(sf2["avg2"][0], np.mean(values))
            self.assertEqual(sf2["min"][0], min(values))
            self.assertEqual(sf2["max"][0], max(values))
            self.assertAlmostEqual(sf2["var"][0], np.var(values))
            self.assertAlmostEqual(sf2["var2"][0], np.var(values))
            self.assertAlmostEqual(sf2["stdv"][0], np.std(values))
            self.assertAlmostEqual(sf2["stdv2"][0], np.std(values))
            np.testing.assert_almost_equal(
                sf2["vector_sum"][0], list(np.sum(vector_values, axis=0))
            )
            np.testing.assert_almost_equal(
                sf2["vector_mean"][0], list(np.mean(vector_values, axis=0))
            )
            self.assertEqual(sf2["count_unique"][0], len(np.unique(values)))
            self.assertEqual(sorted(sf2["unique"][0]), sorted(np.unique(values)))
            self.assertEqual(sf2["frequency"][0], {k: 1 for k in np.unique(values)})

    def test_groupby(self):
        """
        Test builtin groupby and aggregate on different column types
        """
        num_users = 500
        sf = self.__generate_synthetic_sframe__(num_users=num_users)

        built_ins = [
            aggregate.COUNT(),
            aggregate.SUM("rating"),
            aggregate.AVG("rating"),
            aggregate.MIN("rating"),
            aggregate.MAX("rating"),
            aggregate.VAR("rating"),
            aggregate.STDV("rating"),
        ]

        built_in_names = ["Sum", "Avg", "Min", "Max", "Var", "Stdv"]

        """
        Test groupby user_id and aggregate on rating
        """
        sf_user_rating = sf.groupby("user_id", built_ins)
        actual = sf_user_rating.column_names()
        expected = (
            ["%s of rating" % v for v in built_in_names] + ["user_id"] + ["Count"]
        )
        self.assertSetEqual(set(actual), set(expected))
        for row in sf_user_rating:
            uid = row["user_id"]
            mids = range(1, uid + 1)
            ratings = [uid + i for i in mids]
            expected = [
                len(ratings),
                sum(ratings),
                np.mean(ratings),
                min(ratings),
                max(ratings),
                np.var(ratings),
                np.sqrt(np.var(ratings)),
            ]
            actual = [row["Count"]] + [
                row["%s of rating" % op] for op in built_in_names
            ]
            for i in range(len(actual)):
                self.assertAlmostEqual(actual[i], expected[i])

        """
        Test that count can be applied on empty aggregate column.
        """
        sf_user_rating = sf.groupby("user_id", {"counter": aggregate.COUNT()})
        actual = {x["user_id"]: x["counter"] for x in sf_user_rating}
        expected = {i: i for i in range(1, num_users + 1)}
        self.assertDictEqual(actual, expected)

        """
        Test groupby movie_id and aggregate on length_of_watching
        """
        built_ins = [
            aggregate.COUNT(),
            aggregate.SUM("length"),
            aggregate.AVG("length"),
            aggregate.MIN("length"),
            aggregate.MAX("length"),
            aggregate.VAR("length"),
            aggregate.STDV("length"),
        ]
        sf_movie_length = sf.groupby("movie_id", built_ins)
        actual = sf_movie_length.column_names()
        expected = (
            ["%s of length" % v for v in built_in_names] + ["movie_id"] + ["Count"]
        )
        self.assertSetEqual(set(actual), set(expected))
        for row in sf_movie_length:
            mid = row["movie_id"]
            uids = range(int(mid), num_users + 1)
            values = [i - int(mid) for i in uids]
            expected = [
                len(values),
                sum(values),
                np.mean(values),
                min(values),
                max(values),
                np.var(values),
                np.std(values),
            ]
            actual = [row["Count"]] + [
                row["%s of length" % op] for op in built_in_names
            ]
            for i in range(len(actual)):
                self.assertAlmostEqual(actual[i], expected[i])

    def test_quantile_groupby(self):
        sf = self.__generate_synthetic_sframe__(num_users=500)
        # max and min rating for each user
        g = sf.groupby(
            "user_id",
            [
                aggregate.MIN("rating"),
                aggregate.MAX("rating"),
                aggregate.QUANTILE("rating", 0, 1),
            ],
        )
        self.assertEqual(len(g), 500)
        for row in g:
            minrating = row["Min of rating"]
            maxrating = row["Max of rating"]
            arr = list(row["Quantiles of rating"])
            self.assertEqual(len(arr), 2)
            self.assertEqual(arr[0], minrating)
            self.assertEqual(arr[1], maxrating)

    def test_argmax_argmin_groupby(self):
        sf = self.__generate_synthetic_sframe__(num_users=500)
        sf_ret = sf.groupby(
            "user_id",
            {
                "movie with max rating": aggregate.ARGMAX("rating", "movie_id"),
                "movie with min rating": aggregate.ARGMIN("rating", "movie_id"),
            },
        )
        self.assertEqual(len(sf_ret), 500)
        self.assertEqual(sf_ret["movie with max rating"].dtype, str)
        self.assertEqual(sf_ret["movie with min rating"].dtype, str)
        self.assertEqual(sf_ret["user_id"].dtype, int)
        # make sure we have computed correctly.
        max_d = {}
        min_d = {}
        for i in sf:
            key = i["user_id"]
            if key not in max_d:
                max_d[key] = (i["movie_id"], i["rating"])
                min_d[key] = (i["movie_id"], i["rating"])
            else:
                if max_d[key][1] < i["rating"]:
                    max_d[key] = (i["movie_id"], i["rating"])
                if min_d[key][1] > i["rating"]:
                    min_d[key] = (i["movie_id"], i["rating"])
        for i in sf_ret:
            key = i["user_id"]
            self.assertEqual(i["movie with max rating"], max_d[key][0])
            self.assertEqual(i["movie with min rating"], min_d[key][0])

    def test_multicolumn_groupby(self):
        sf = self.__generate_synthetic_sframe__(num_users=500)
        sf_um = sf.groupby(["user_id", "movie_id"], aggregate.COUNT)
        # I can query it
        t = sf_um.to_dataframe()
        self.assertEqual(sf_um["user_id"].dtype, int)
        self.assertEqual(sf_um["movie_id"].dtype, str)
        # make sure we have counted correctly
        d = {}
        for i in sf:
            key = str(i["user_id"]) + "," + i["movie_id"]
            if key not in d:
                d[key] = 0
            d[key] = d[key] + 1

        for i in sf_um:
            key = str(i["user_id"]) + "," + i["movie_id"]
            self.assertTrue(key in d)
            self.assertEqual(i["Count"], d[key])

        sf_um = sf.groupby(["movie_id", "user_id"], aggregate.COUNT())
        # I can query it
        t = sf_um.to_dataframe()
        self.assertEqual(sf_um["user_id"].dtype, int)
        self.assertEqual(sf_um["movie_id"].dtype, str)

        # make sure we have counted correctly
        d = {}
        for i in sf:
            key = str(i["user_id"]) + "," + i["movie_id"]
            if key not in d:
                d[key] = 0
            d[key] = d[key] + 1

        for i in sf_um:
            key = str(i["user_id"]) + "," + i["movie_id"]
            self.assertTrue(key in d)
            self.assertEqual(i["Count"], d[key])

    def __assert_concat_result_equal(self, result, expected, list_columns):
        self.assertEqual(result.num_columns(), expected.num_columns())
        for column in result.column_names():
            c1 = result[column]
            c2 = expected[column]
            self.assertEqual(c1.dtype, c2.dtype)
            self.assertEqual(len(c1), len(c2))
            if column in list_columns:
                for i in range(len(c1)):
                    if c1[i] is None:
                        self.assertTrue(c2[i] is None)
                        continue
                    if c1.dtype == dict:
                        for k in c1[i]:
                            self.assertEqual(c2[i][k], c1[i][k])
                    else:
                        s1 = list(c1[i])
                        if s1 is not None:
                            s1.sort()
                        s2 = list(c2[i])
                        if s2 is not None:
                            s2.sort()
                        self.assertEqual(s1, s2)
            else:
                self.assertEqual(list(c1), list(c2))

    def test_groupby_dict_key(self):
        t = SFrame({"a": [{1: 2}, {3: 4}]})
        with self.assertRaises(TypeError):
            t.groupby("a", {})

    def test_concat(self):
        sf = SFrame()
        sf["a"] = [1, 1, 1, 1, 2, 2, 2, 3, 4, 4, 5]
        sf["b"] = [1, 2, 1, 2, 3, 3, 1, 4, None, 2, None]
        sf["c"] = ["a", "b", "a", "b", "e", "e", None, "h", "i", "j", "k"]
        sf["d"] = [1.0, 2.0, 1.0, 2.0, 3.0, 3.0, 1.0, 4.0, None, 2.0, None]
        sf["e"] = [{"x": 1}] * len(sf["a"])

        print(sf["b"].dtype)

        result = sf.groupby("a", aggregate.CONCAT("b"))
        expected_result = SFrame(
            {
                "a": [1, 2, 3, 4, 5],
                "List of b": [[1.0, 1.0, 2.0, 2.0], [1.0, 3.0, 3.0], [4.0], [2.0], []],
            }
        )
        expected_result["List of b"] = expected_result["List of b"].astype(list)
        self.__assert_concat_result_equal(
            result.sort("a"), expected_result.sort("a"), ["List of b"]
        )

        result = sf.groupby("a", aggregate.CONCAT("d"))

        expected_result = SFrame(
            {"a": [1, 2, 3, 4, 5], "List of d": [[1, 1, 2, 2], [1, 3, 3], [4], [2], []]}
        )
        self.__assert_concat_result_equal(
            result.sort("a"), expected_result.sort("a"), ["List of d"]
        )

        result = sf.groupby("a", {"c_c": aggregate.CONCAT("c")})
        expected_result = SFrame(
            {
                "a": [1, 2, 3, 4, 5],
                "c_c": [["a", "b", "a", "b"], ["e", "e"], ["h"], ["i", "j"], ["k"]],
            }
        )

        self.__assert_concat_result_equal(
            result.sort("a"), expected_result.sort("a"), ["c_c"]
        )

        result = sf.groupby("a", aggregate.CONCAT("b", "c"))
        expected_result = SFrame(
            {
                "a": [1, 2, 3, 4, 5],
                "Dict of b_c": [
                    {1: "a", 2: "b"},
                    {3: "e", 1: None},
                    {4: "h"},
                    {2: "j"},
                    {},
                ],
            }
        )

        self.__assert_concat_result_equal(
            result.sort("a"), expected_result.sort("a"), ["Dict of b_c"]
        )

        result = sf.groupby("a", {"c_b": aggregate.CONCAT("c", "b")})
        expected_result = SFrame(
            {
                "a": [1, 2, 3, 4, 5],
                "c_b": [
                    {"a": 1, "b": 2},
                    {"e": 3},
                    {"h": 4},
                    {"i": None, "j": 2},
                    {"k": None},
                ],
            }
        )

        self.__assert_concat_result_equal(
            result.sort("a"), expected_result.sort("a"), ["c_b"]
        )

        result = sf.groupby(
            "a", {"cs": aggregate.CONCAT("c"), "bs": aggregate.CONCAT("b")}
        )
        expected_result = SFrame(
            {
                "a": [1, 2, 3, 4, 5],
                "bs": [[1, 1, 2, 2], [1, 3, 3], [4], [2], []],
                "cs": [["a", "b", "a", "b"], ["e", "e"], ["h"], ["i", "j"], ["k"]],
            }
        )
        expected_result["bs"] = expected_result["bs"].astype(list)
        self.__assert_concat_result_equal(
            result.sort("a"), expected_result.sort("a"), ["bs", "cs"]
        )

        # exception fail if there is not column
        with self.assertRaises(TypeError):
            sf.groupby("a", aggregate.CONCAT())

        with self.assertRaises(KeyError):
            sf.groupby("a", aggregate.CONCAT("nonexist"))

        with self.assertRaises(TypeError):
            sf.groupby("a", aggregate.CONCAT("e", "a"))

    def test_select_one(self):
        sf = SFrame(
            {"a": [1, 1, 2, 2, 3, 3, 4, 4, 5, 5], "b": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]}
        )
        res = list(sf.groupby("a", {"b": aggregate.SELECT_ONE("b")}))
        self.assertEqual(len(res), 5)
        for i in res:
            self.assertTrue(i["b"] == 2 * i["a"] or i["b"] == 2 * i["a"] - 1)

    def test_unique(self):
        sf = SFrame(
            {"a": [1, 1, 2, 2, 3, 3, 4, 4, 5, 5], "b": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]}
        )
        self.assertEqual(len(sf.unique()), 10)

        vals = [1, 1, 2, 2, 3, 3, 4, 4, None, None]
        sf = SFrame({"a": vals, "b": vals})
        res = sf.unique()
        self.assertEqual(len(res), 5)
        self.assertEqual(set(res["a"]), set([1, 2, 3, 4, None]))
        self.assertEqual(set(res["b"]), set([1, 2, 3, 4, None]))

    def test_append_empty(self):
        sf_with_data = SFrame(data=self.dataframe)
        empty_sf = SFrame()
        self.assertFalse(sf_with_data.append(empty_sf) is sf_with_data)
        self.assertFalse(empty_sf.append(sf_with_data) is sf_with_data)
        self.assertFalse(empty_sf.append(empty_sf) is empty_sf)

    def test_append_all_match(self):
        sf1 = SFrame(data=self.dataframe)
        sf2 = SFrame(data=self.dataframe2)

        new_sf = sf1.append(sf2)
        assert_frame_equal(
            self.dataframe.append(self.dataframe2, ignore_index=True),
            new_sf.to_dataframe(),
        )

    def test_append_lazy(self):
        sf1 = SFrame(data=self.dataframe)
        sf2 = SFrame(data=self.dataframe2)

        new_sf = sf1.append(sf2)
        self.assertTrue(new_sf.__is_materialized__())

        filter_sf1 = SArray(
            [1 for i in range(sf1.num_rows())] + [0 for i in range(sf2.num_rows())]
        )
        filter_sf2 = SArray(
            [0 for i in range(sf1.num_rows())] + [1 for i in range(sf2.num_rows())]
        )
        new_sf1 = new_sf[filter_sf1]
        new_sf2 = new_sf[filter_sf2]
        assert_frame_equal(
            self.dataframe.append(self.dataframe2, ignore_index=True),
            new_sf.to_dataframe(),
        )
        assert_frame_equal(sf1.to_dataframe(), new_sf1.to_dataframe())
        assert_frame_equal(sf2.to_dataframe(), new_sf2.to_dataframe())

        row = sf1.head(1)
        sf = SFrame()
        for i in range(10):
            sf = sf.append(row)
        df = sf.to_dataframe()
        for i in range(10):
            self.assertEqual(
                list(df.iloc[[i]]), list(sf.head(1).to_dataframe().iloc[[0]])
            )

    def test_recursive_append(self):
        sf = SFrame()
        for i in range(200):
            sf = sf.append(SFrame(data=self.dataframe))

        # consume
        sf.materialize()

    def test_print_sframe(self):
        sf = SFrame()

        def _test_print():
            sf.__repr__()
            sf._repr_html_()
            try:
                from StringIO import StringIO
            except ImportError:
                from io import StringIO
            output = StringIO()
            sf.print_rows(output_file=output)

        n = 20
        sf["int"] = [i for i in range(n)]
        sf["float"] = [float(i) for i in range(n)]
        sf["str"] = [str(i) for i in range(n)]
        uc = "\xe5\xa4\xa7\xe5\xa4\xb4"
        sf["unicode"] = [uc for i in range(n)]
        sf["array"] = [array.array("d", [i]) for i in range(n)]
        sf["list"] = [[i, float(i), [i]] for i in range(n)]
        utc = dt.datetime.strptime("2011-01-21 02:37:21", "%Y-%m-%d %H:%M:%S")
        sf["dt"] = [utc for i in range(n)]
        sf["img"] = [Image() for i in range(n)]
        sf["long_str"] = ["".join([str(i)] * 50) for i in range(n)]
        sf["long_unicode"] = ["".join([uc] * 50) for i in range(n)]
        sf["bad_unicode"] = ["\x9d" + uc for i in range(n)]
        _test_print()

    def test_print_lazy_sframe(self):
        sf1 = SFrame(data=self.dataframe)
        self.assertTrue(sf1.__is_materialized__())
        sf2 = sf1[sf1["int_data"] > 3]
        sf2.__repr__()
        sf2.__str__()
        self.assertFalse(sf2.__is_materialized__())
        len(sf2)
        self.assertTrue(sf2.__is_materialized__())

    def test_append_order_diff(self):
        # name match but column order not match
        sf1 = SFrame(data=self.dataframe)
        sf2 = SFrame(data=self.dataframe2)
        sf2.swap_columns("int_data", "string_data", inplace=True)

        new_sf = sf1.append(sf2)
        assert_frame_equal(
            self.dataframe.append(self.dataframe2, ignore_index=True),
            new_sf.to_dataframe(),
        )

    def test_append_empty_sframe(self):
        sf = SFrame(data=self.dataframe)
        other = SFrame()

        # non empty append empty
        assert_frame_equal(sf.append(other).to_dataframe(), self.dataframe)

        # empty append non empty
        assert_frame_equal(other.append(sf).to_dataframe(), self.dataframe)

        # empty append empty
        assert_frame_equal(other.append(other).to_dataframe(), pd.DataFrame())

    def test_append_exception(self):
        sf = SFrame(data=self.dataframe)

        # column number not match
        other = SFrame()
        other.add_column(SArray(), "test", inplace=True)
        self.assertRaises(RuntimeError, lambda: sf.append(other))  # column not the same

        # column name not match
        other = SFrame()
        names = sf.column_names()
        for name in sf.column_names():
            other.add_column(SArray(), name, inplace=True)
        names[0] = "some name not match"
        self.assertRaises(RuntimeError, lambda: sf.append(other))

        # name match but column type order not match
        sf1 = SFrame(data=self.dataframe)
        sf2 = SFrame(data=self.dataframe2)

        # change one column type
        sf1["int_data"] = sf2.select_column("int_data").astype(float)
        self.assertRaises(RuntimeError, lambda: sf.append(other))

    def test_simple_joins(self):
        inner_expected = SFrame()
        inner_expected.add_column(
            SArray(["Robinson", "Jones", "Smith", "Heisenberg", "Rafferty"]),
            "last_name",
            inplace=True,
        )
        inner_expected.add_column(SArray([34, 33, 34, 33, 31]), "dep_id", inplace=True)
        inner_expected.add_column(
            SArray(["Clerical", "Engineering", "Clerical", "Engineering", "Sales"]),
            "dep_name",
            inplace=True,
        )

        # Tests the "natural join" case
        beg = time.time()
        res = self.employees_sf.join(self.departments_sf)
        end = time.time()
        print("Really small join: " + str(end - beg) + " s")

        self.__assert_join_results_equal(res, inner_expected)

        left_join_row = SFrame()
        left_join_row.add_column(SArray(["John"]), "last_name", inplace=True)
        left_join_row.add_column(SArray([None], int), "dep_id", inplace=True)
        left_join_row.add_column(SArray([None], str), "dep_name", inplace=True)

        left_expected = inner_expected.append(left_join_row)

        # Left outer join, passing string to 'on'
        res = self.employees_sf.join(self.departments_sf, how="left", on="dep_id")
        self.__assert_join_results_equal(res, left_expected)

        right_join_row = SFrame()
        right_join_row.add_column(SArray([None], str), "last_name", inplace=True)
        right_join_row.add_column(SArray([35]), "dep_id", inplace=True)
        right_join_row.add_column(SArray(["Marketing"]), "dep_name", inplace=True)

        right_expected = inner_expected.append(right_join_row)

        # Right outer join, passing list to 'on'
        res = self.employees_sf.join(self.departments_sf, how="right", on=["dep_id"])
        self.__assert_join_results_equal(res, right_expected)

        outer_expected = left_expected.append(right_join_row)

        # Full outer join, passing dict to 'on'
        res = self.employees_sf.join(
            self.departments_sf, how="outer", on={"dep_id": "dep_id"}
        )
        self.__assert_join_results_equal(res, outer_expected)

        # Test a join on non-matching key
        res = self.employees_sf.join(self.departments_sf, on={"last_name": "dep_name"})
        self.assertEqual(res.num_rows(), 0)
        self.assertEqual(res.num_columns(), 3)
        self.assertEqual(res.column_names(), ["last_name", "dep_id", "dep_id.1"])

        # Test a join on a non-unique key
        bad_departments = SFrame()
        bad_departments["dep_id"] = SArray([33, 33, 31, 31])
        bad_departments["dep_name"] = self.departments_sf["dep_name"]

        no_pk_expected = SFrame()
        no_pk_expected["last_name"] = SArray(
            ["Rafferty", "Rafferty", "Heisenberg", "Jones", "Heisenberg", "Jones"]
        )
        no_pk_expected["dep_id"] = SArray([31, 31, 33, 33, 33, 33])
        no_pk_expected["dep_name"] = SArray(
            ["Clerical", "Marketing", "Sales", "Sales", "Engineering", "Engineering"]
        )
        res = self.employees_sf.join(bad_departments, on="dep_id")
        self.__assert_join_results_equal(res, no_pk_expected)

        # Left join on non-unique key
        bad_departments = bad_departments.append(right_join_row[["dep_id", "dep_name"]])
        bad_departments = bad_departments.append(right_join_row[["dep_id", "dep_name"]])
        no_pk_expected = no_pk_expected.append(right_join_row)
        no_pk_expected = no_pk_expected.append(right_join_row)
        no_pk_expected = no_pk_expected[["dep_id", "dep_name", "last_name"]]
        res = bad_departments.join(self.employees_sf, on="dep_id", how="left")
        self.__assert_join_results_equal(res, no_pk_expected)

    def test_simple_joins_with_customized_name(self):
        # redundant name conflict resolution
        with self.assertRaises(KeyError):
            self.employees_sf.join(
                self.departments_sf, alter_name={"non_existing_name": "random_name"}
            )

        with self.assertRaises(KeyError):
            self.employees_sf.join(
                self.departments_sf, alter_name={"dep_id": "random_name"}
            )

        with self.assertRaises(ValueError):
            self.employees_sf.join(
                self.departments_sf, alter_name={"dep_name": "last_name"}
            )

        # nothing should happen
        # Tests the "natural join" case
        inner_expected = SFrame()
        inner_expected.add_column(
            SArray(["Robinson", "Jones", "Smith", "Heisenberg", "Rafferty"]),
            "last_name",
            inplace=True,
        )
        inner_expected.add_column(SArray([34, 33, 34, 33, 31]), "dep_id", inplace=True)
        inner_expected.add_column(
            SArray(["Marketing", "Engineering", "Cooking", "Clerical", "Sales"]),
            "dep_name",
            inplace=True,
        )

        # add extra column for employee table
        employees_sf_extra = self.employees_sf.add_column(
            SArray(
                [
                    "Sales",
                    "Engineering",
                    "Clerical",
                    "Marketing",
                    "Cooking",
                    "Basketball",
                ]
            ),
            "dep_name",
        )

        res = employees_sf_extra.join(self.departments_sf, on="dep_id")
        inner_expected_tmp = inner_expected.add_column(
            SArray(["Clerical", "Engineering", "Clerical", "Engineering", "Sales"]),
            "dep_name.1",
        )
        self.__assert_join_results_equal(res, inner_expected_tmp)

        inner_expected_tmp = inner_expected.add_column(
            SArray(["Clerical", "Engineering", "Clerical", "Engineering", "Sales"]), "X"
        )
        res = employees_sf_extra.join(
            self.departments_sf, on="dep_id", alter_name={"dep_name": "X"}
        )
        self.__assert_join_results_equal(res, inner_expected_tmp)

        ###### A simple and navive test start ######
        employees_ = SFrame()
        employees_.add_column(SArray(["A", "B", "C", "D"]), "last_name", inplace=True)
        employees_.add_column(SArray([31, 32, 33, None]), "dep_id", inplace=True)
        employees_.add_column(SArray([1, 2, 3, 4]), "org_id", inplace=True)
        employees_.add_column(SArray([1, 2, 3, 4]), "bed_id", inplace=True)

        departments_ = SFrame()
        departments_.add_column(SArray([31, 33, 34]), "dep_id", inplace=True)
        departments_.add_column(SArray(["A", "C", "F"]), "last_name", inplace=True)
        departments_.add_column(
            SArray(["Sales", "Engineering", None]), "dep_name", inplace=True
        )
        # intentionally dup at the second last
        departments_.add_column(SArray([1, 3, 5]), "bed_id", inplace=True)
        departments_.add_column(SArray([1, 3, None]), "car_id", inplace=True)

        join_keys_ = ["dep_id", "last_name"]

        ## left
        expected_ = SFrame()
        expected_.add_column(SArray(["A", "B", "C", "D"]), "last_name", inplace=True)
        expected_.add_column(SArray([31, 32, 33, None]), "dep_id", inplace=True)
        expected_.add_column(SArray([1, 2, 3, 4]), "org_id", inplace=True)
        expected_.add_column(SArray([1, 2, 3, 4]), "bed_id", inplace=True)
        expected_.add_column(
            SArray(["Sales", None, "Engineering", None]), "dep_name", inplace=True
        )
        expected_.add_column(SArray([1, None, 3, None]), "bed_id.1", inplace=True)
        expected_.add_column(SArray([1, None, 3, None]), "car_id", inplace=True)

        res = employees_.join(departments_, on=join_keys_, how="left")
        self.__assert_join_results_equal(res, expected_)

        expected_ = SFrame()
        expected_.add_column(SArray(["A", "B", "C", "D"]), "last_name", inplace=True)
        expected_.add_column(SArray([31, 32, 33, None]), "dep_id", inplace=True)
        expected_.add_column(SArray([1, 2, 3, 4]), "org_id", inplace=True)
        expected_.add_column(SArray([1, 2, 3, 4]), "bed_id", inplace=True)
        expected_.add_column(
            SArray(["Sales", None, "Engineering", None]), "dep_name", inplace=True
        )
        expected_.add_column(SArray([1, None, 3, None]), "Y", inplace=True)
        expected_.add_column(SArray([1, None, 3, None]), "car_id", inplace=True)

        res = employees_.join(
            departments_,
            on=join_keys_,
            how="left",
            alter_name={"car_id": "X", "bed_id": "Y"},
        )
        self.__assert_join_results_equal(res, expected_)

        ## left size is smaller than right
        expected_ = SFrame()
        expected_.add_column(SArray([31, 33, 34]), "dep_id", inplace=True)
        expected_.add_column(SArray(["A", "C", "F"]), "last_name", inplace=True)
        expected_.add_column(
            SArray(["Sales", "Engineering", None]), "dep_name", inplace=True
        )
        expected_.add_column(SArray([1, 3, 5]), "bed_id", inplace=True)
        expected_.add_column(SArray([1, 3, None]), "car_id", inplace=True)
        expected_.add_column(SArray([1, 3, None]), "org_id", inplace=True)
        expected_.add_column(SArray([1, 3, None]), "Y", inplace=True)

        res = departments_.join(
            employees_, on=join_keys_, how="left", alter_name={"bed_id": "Y"}
        )
        self.__assert_join_results_equal(res, expected_)

        ## right
        expected_ = SFrame()
        expected_.add_column(SArray(["A", "C", "F"]), "last_name", inplace=True)
        expected_.add_column(SArray([31, 33, 34]), "dep_id", inplace=True)
        expected_.add_column(SArray([1, 3, None]), "org_id", inplace=True)
        expected_.add_column(SArray([1, 3, None]), "bed_id", inplace=True)
        expected_.add_column(
            SArray(["Sales", "Engineering", None]), "dep_name", inplace=True
        )
        expected_.add_column(SArray([1, 3, 5]), "Y", inplace=True)
        expected_.add_column(SArray([1, 3, None]), "car_id", inplace=True)

        res = employees_.join(
            departments_,
            on=join_keys_,
            how="right",
            alter_name={"car_id": "X", "bed_id": "Y"},
        )
        self.__assert_join_results_equal(res, expected_)

        ## outer
        expected_ = SFrame()
        expected_.add_column(
            SArray(["A", "B", "C", "D", "F"]), "last_name", inplace=True
        )
        expected_.add_column(SArray([31, 32, 33, None, 34]), "dep_id", inplace=True)
        expected_.add_column(SArray([1, 2, 3, 4, None]), "org_id", inplace=True)
        expected_.add_column(SArray([1, 2, 3, 4, None]), "bed_id", inplace=True)
        expected_.add_column(
            SArray(["Sales", None, "Engineering", None, None]), "dep_name", inplace=True
        )
        expected_.add_column(SArray([1, None, 3, None, 5]), "Y", inplace=True)
        expected_.add_column(SArray([1, None, 3, None, None]), "car_id", inplace=True)

        res = employees_.join(
            departments_,
            on=join_keys_,
            how="outer",
            alter_name={"car_id": "X", "bed_id": "Y"},
        )
        self.__assert_join_results_equal(res, expected_)

        ## error cases
        with self.assertRaises(KeyError):
            res = employees_.join(
                departments_,
                on=join_keys_,
                how="right",
                alter_name={"some_id": "car_id", "bed_id": "Y"},
            )

        with self.assertRaises(ValueError):
            res = employees_.join(
                departments_,
                on=join_keys_,
                how="right",
                alter_name={"car_id": "car_id", "bed_id": "car_id"},
            )

        ## resolution order is not independent
        with self.assertRaises(ValueError):
            res = employees_.join(
                departments_,
                on=join_keys_,
                how="right",
                alter_name={"car_id": "X", "bed_id": "car_id"},
            )

        with self.assertRaises(ValueError):
            res = employees_.join(
                departments_,
                on=join_keys_,
                how="right",
                alter_name={"car_id": "bed_id", "bed_id": "car_id"},
            )

        ## duplicate values
        with self.assertRaises(RuntimeError):
            res = employees_.join(
                departments_,
                on=join_keys_,
                how="right",
                alter_name={"car_id": "X", "bed_id": "X"},
            )

    def test_big_composite_join(self):
        # Create a semi large SFrame with composite primary key (letter, number)
        letter_keys = []
        number_keys = []
        data = []
        for i in string.ascii_lowercase:
            for j in range(0, 100):
                letter_keys.append(i)
                number_keys.append(j)
                which = j % 3
                if which == 0:
                    data.append(string.ascii_uppercase)
                elif which == 1:
                    data.append(string.digits)
                elif which == 2:
                    data.append(string.hexdigits)
        pk_gibberish = SFrame()
        pk_gibberish["letter"] = SArray(letter_keys, str)
        pk_gibberish["number"] = SArray(number_keys, int)
        pk_gibberish["data"] = SArray(data, str)

        # Some rows that won't match
        more_data = []
        more_letter_keys = []
        more_number_keys = []
        for i in range(0, 40000):
            more_data.append("fish")
            more_letter_keys.append("A")
            more_number_keys.append(200)
        for i in range(0, 80):
            for j in range(100, 1000):
                more_data.append("waffles")
                more_letter_keys.append(letter_keys[j])
                more_number_keys.append(number_keys[j])
                # Non-matching row in this stretch
                if j == 147:
                    more_letter_keys[-1] = "A"
        for i in range(0, 5000):
            more_data.append("pizza")
            more_letter_keys.append("Z")
            more_number_keys.append(400)

        join_with_gibberish = SFrame()
        join_with_gibberish["data"] = SArray(more_data, str)
        join_with_gibberish["moredata"] = SArray(more_data, str)
        join_with_gibberish["a_number"] = SArray(more_number_keys, int)
        join_with_gibberish["a_letter"] = SArray(more_letter_keys, str)

        expected_answer = SFrame()
        exp_letter = []
        exp_number = []
        exp_data = []
        for i in range(0, 80):
            exp_letter.extend(letter_keys[100:147])
            exp_number.extend(number_keys[100:147])
            exp_letter.extend(letter_keys[148:1000])
            exp_number.extend(number_keys[148:1000])
            exp_data.extend(data[100:147])
            exp_data.extend(data[148:1000])
        expected_answer["letter"] = SArray(exp_letter, str)
        expected_answer["number"] = SArray(exp_number, int)
        expected_answer["data"] = SArray(exp_data, str)
        expected_answer["data.1"] = "waffles"
        expected_answer["moredata"] = "waffles"

        beg = time.time()
        res = pk_gibberish.join(
            join_with_gibberish, on={"letter": "a_letter", "number": "a_number"}
        )
        end = time.time()
        print("Join took " + str(end - beg) + " seconds")
        self.__assert_join_results_equal(res, expected_answer)

    def test_convert_dataframe_empty(self):
        sf = SFrame()
        sf["a"] = SArray([], int)
        df = sf.to_dataframe()
        self.assertEqual(df["a"].dtype, int)
        sf1 = SFrame(df)
        self.assertEqual(sf1["a"].dtype, int)
        self.assertEqual(sf1.num_rows(), 0)

    def test_replace_one_column(self):
        sf = SFrame()
        sf["a"] = [1, 2, 3]
        self.assertEqual(list(sf["a"]), [1, 2, 3])

        # this should succeed as we are replacing a new column
        sf["a"] = [1, 2]
        self.assertEqual(list(sf["a"]), [1, 2])

        # failed to add new column should revert original sframe
        with self.assertRaises(TypeError):
            sf["a"] = [1, 2, "a"]

        self.assertEqual(list(sf["a"]), [1, 2])

        # add a column with different length should fail if there are more than one column
        sf = SFrame()
        sf["a"] = [1, 2, 3]
        sf["b"] = ["a", "b", "c"]
        with self.assertRaises(RuntimeError):
            sf["a"] = [1, 2]

    def test_filter_by(self):
        # Set up SFrame to filter by
        sf = SFrame()
        sf.add_column(SArray(self.int_data), "ints", inplace=True)
        sf.add_column(SArray(self.float_data), "floats", inplace=True)
        sf.add_column(SArray(self.string_data), "strings", inplace=True)

        # filter by None should take no effect on data set without missing val
        res = sf.filter_by(None, "ints", exclude=True)
        self.__assert_join_results_equal(res, sf)
        res = sf.filter_by(None, "floats", exclude=True)
        self.__assert_join_results_equal(res, sf)
        res = sf.filter_by(None, "strings", exclude=True)
        self.__assert_join_results_equal(res, sf)

        res = sf.filter_by(None, "ints")
        self.assertEqual(len(res), 0)
        res = sf.filter_by(None, "strings")
        self.assertEqual(len(res), 0)

        # private use only
        def __build_data_list_with_none(data_lst):
            data_lst.insert(len(data_lst) // 2, None)
            data_lst.insert(0, None)
            data_lst.append(None)
            return data_lst

        sf_none = SFrame()
        sf_none.add_column(
            SArray(__build_data_list_with_none(self.int_data[:])), "ints", inplace=True
        )
        sf_none.add_column(
            SArray(__build_data_list_with_none(self.float_data[:])),
            "floats",
            inplace=True,
        )
        sf_none.add_column(
            SArray(__build_data_list_with_none(self.string_data[:])),
            "strings",
            inplace=True,
        )

        res = sf_none.filter_by(None, "ints")
        self.assertEqual(len(res), 3)
        res = sf_none.filter_by(None, "ints", exclude=True)
        self.__assert_join_results_equal(res, sf)

        res = sf_none.filter_by(None, "floats")
        self.assertEqual(len(res), 3)
        res = sf_none.filter_by(None, "floats", exclude=True)
        self.__assert_join_results_equal(res, sf)

        res = sf_none.filter_by(None, "strings")
        self.assertEqual(len(res), 3)
        res = sf_none.filter_by(None, "strings", exclude=True)
        self.__assert_join_results_equal(res, sf)

        # by generator, filter, map, range
        res = sf.filter_by(range(10), "ints")
        self.assertEqual(len(res), 9)
        self.assertEqual(res["ints"][0], 1)
        res = sf.filter_by(range(10), "ints", exclude=True)
        self.assertEqual(len(res), 1)
        self.assertEqual(res["ints"][0], 10)

        res = sf.filter_by(map(lambda x: x - 5.0, self.float_data), "floats")
        self.assertEqual(len(res), 5)
        self.assertEqual(res["floats"][0], self.float_data[0])
        res = sf.filter_by(
            map(lambda x: x - 5.0, self.float_data), "floats", exclude=True
        )
        self.assertEqual(len(res), 5)
        self.assertEqual(res["floats"][0], self.float_data[5])

        res = sf.filter_by(filter(lambda x: len(x) > 1, self.string_data), "strings")
        self.assertEqual(len(res), 1)
        self.assertEqual(res["strings"][0], self.string_data[-1])
        res = sf.filter_by(
            filter(lambda x: len(x) > 1, self.string_data), "strings", exclude=True
        )
        self.assertEqual(len(res), 9)
        self.assertEqual(res["strings"][0], self.string_data[0])

        # Normal cases
        res = sf.filter_by(SArray(self.int_data), "ints")
        self.__assert_join_results_equal(res, sf)
        res = sf.filter_by(SArray(self.int_data), "ints", exclude=True)
        self.assertEqual(list(res), [])

        res = sf.filter_by([5, 6], "ints")
        exp = SFrame()
        exp.add_column(SArray(self.int_data[4:6]), "ints", inplace=True)
        exp.add_column(SArray(self.float_data[4:6]), "floats", inplace=True)
        exp.add_column(SArray(self.string_data[4:6]), "strings", inplace=True)
        self.__assert_join_results_equal(res, exp)
        exp_opposite = SFrame()
        exp_opposite.add_column(
            SArray(self.int_data[:4] + self.int_data[6:]), "ints", inplace=True
        )
        exp_opposite.add_column(
            SArray(self.float_data[:4] + self.float_data[6:]), "floats", inplace=True
        )
        exp_opposite.add_column(
            SArray(self.string_data[:4] + self.string_data[6:]), "strings", inplace=True
        )
        res = sf.filter_by([5, 6], "ints", exclude=True)
        self.__assert_join_results_equal(res, exp_opposite)

        exp_one = SFrame()
        exp_one.add_column(SArray(self.int_data[4:5]), "ints", inplace=True)
        exp_one.add_column(SArray(self.float_data[4:5]), "floats", inplace=True)
        exp_one.add_column(SArray(self.string_data[4:5]), "strings", inplace=True)
        exp_all_but_one = SFrame()
        exp_all_but_one.add_column(
            SArray(self.int_data[:4] + self.int_data[5:]), "ints", inplace=True
        )
        exp_all_but_one.add_column(
            SArray(self.float_data[:4] + self.float_data[5:]), "floats", inplace=True
        )
        exp_all_but_one.add_column(
            SArray(self.string_data[:4] + self.string_data[5:]), "strings", inplace=True
        )

        res = sf.filter_by(5, "ints")
        self.__assert_join_results_equal(res, exp_one)
        res = sf.filter_by(5, "ints", exclude=True)
        self.__assert_join_results_equal(res, exp_all_but_one)

        res = sf.filter_by("5", "strings")
        self.__assert_join_results_equal(res, exp_one)
        res = sf.filter_by(5, "ints", exclude=True)
        self.__assert_join_results_equal(res, exp_all_but_one)

        # Only missing values
        res = sf.filter_by([77, 77, 88, 88], "ints")
        # Test against empty SFrame with correct columns/types
        self.__assert_join_results_equal(res, exp_one[exp_one["ints"] == 9000])
        res = sf.filter_by([77, 77, 88, 88], "ints", exclude=True)
        self.__assert_join_results_equal(res, sf)

        # Duplicate values
        res = sf.filter_by([6, 6, 5, 5, 6, 5, 5, 6, 5, 5, 5], "ints")
        self.__assert_join_results_equal(res, exp)
        res = sf.filter_by([6, 6, 5, 5, 6, 5, 5, 6, 5, 5, 5], "ints", exclude=True)
        self.__assert_join_results_equal(res, exp_opposite)

        # Duplicate and missing
        res = sf.filter_by([11, 12, 46, 6, 6, 55, 5, 5], "ints")
        self.__assert_join_results_equal(res, exp)
        res = sf.filter_by([11, 12, 46, 6, 6, 55, 5, 5], "ints", exclude=True)
        self.__assert_join_results_equal(res, exp_opposite)

        # Type mismatch
        with self.assertRaises(TypeError):
            res = sf.filter_by(["hi"], "ints")

        # Column doesn't exist
        with self.assertRaises(KeyError):
            res = sf.filter_by([1, 2], "intssss")

        # Something that can't be turned into an SArray
        with self.assertRaises(Exception):
            res = sf.filter_by({1: 2, 3: 4}, "ints")

        # column_name not given as string
        with self.assertRaises(TypeError):
            res = sf.filter_by(1, 2)

        # Duplicate column names after join. Should be last because of the
        # renames.
        sf.rename({"ints": "id", "floats": "id1", "strings": "id11"}, inplace=True)
        exp.rename({"ints": "id", "floats": "id1", "strings": "id11"}, inplace=True)
        exp_opposite.rename(
            {"ints": "id", "floats": "id1", "strings": "id11"}, inplace=True
        )
        res = sf.filter_by([5, 6], "id")
        self.__assert_join_results_equal(res, exp)
        res = sf.filter_by([5, 6], "id", exclude=True)
        self.__assert_join_results_equal(res, exp_opposite)

    # XXXXXX: should be inner function
    def __test_to_from_dataframe(self, data, type):
        sf = SFrame()
        sf["a"] = data
        df = sf.to_dataframe()
        sf1 = SFrame(df)
        self.assertTrue(sf1.dtype[0] == type)

        df = pd.DataFrame({"val": data})
        sf1 = SFrame(df)
        self.assertTrue(sf1.dtype[0] == type)

    def test_to_from_dataframe(self):
        self.__test_to_from_dataframe([1, 2, 3], int)
        self.__test_to_from_dataframe(["a", "b", "c"], str)
        self.__test_to_from_dataframe([1.0, 2.0, 3.0], float)
        self.__test_to_from_dataframe([[1, "b", {"a": 1}], [1, 2, 3]], list)
        self.__test_to_from_dataframe([{"a": 1, 1: None}, {"b": 2}], dict)
        self.__test_to_from_dataframe([[1, 2], [1, 2], []], array.array)

    def test_pack_columns_exception(self):
        sf = SFrame()
        sf["a"] = [1, 2, 3, None, None]
        sf["b"] = [None, "2", "3", None, "5"]
        sf["c"] = [None, 2.0, 3.0, None, 5.0]

        # cannot pack non array value into array
        with self.assertRaises(TypeError):
            sf.pack_columns(dtype=array.array)

        # cannot given non numeric na vlaue to array
        with self.assertRaises(ValueError):
            sf.pack_columns(dtype=array.array, fill_na="c")

        # cannot pack non exist columns
        with self.assertRaises(ValueError):
            sf.pack_columns(["d", "a"])

        # dtype has to be dict/array/list
        with self.assertRaises(ValueError):
            sf.pack_columns(dtype=str)

        # pack duplicate columns
        with self.assertRaises(ValueError):
            sf.pack_columns(["a", "a"])

        # pack partial columns to array, should fail if for columns that are not numeric
        with self.assertRaises(TypeError):
            sf.pack_columns(["a", "b"], dtype=array.array)

        with self.assertRaises(TypeError):
            sf.pack_columns(column_name_prefix=1)

        with self.assertRaises(ValueError):
            sf.pack_columns(column_name_prefix="1")

        with self.assertRaises(ValueError):
            sf.pack_columns(column_name_prefix="c", column_names=["a", "b"])

    def test_pack_columns2(self):
        sf = SFrame()
        sf["id"] = [1, 2, 3, 4]
        sf["category.a"] = [None, "2", "3", None]
        sf["category.b"] = [None, 2.0, None, 4.0]

        expected = SArray([[None, None], ["2", 2.0], ["3", None], [None, 4.0]])
        result = sf.pack_columns(column_name_prefix="category")
        self.assertEqual(result.column_names(), ["id", "category"])
        self.__assert_sarray_equal(result["id"], sf["id"])
        self.__assert_sarray_equal(result["category"], expected)

        result = sf.pack_columns(
            column_name_prefix="category", new_column_name="new name"
        )
        self.assertEqual(result.column_names(), ["id", "new name"])
        self.__assert_sarray_equal(result["id"], sf["id"])
        self.__assert_sarray_equal(result["new name"], expected)

        # default dtype is list
        result = sf.pack_columns(column_name_prefix="category", dtype=list)
        self.assertEqual(result.column_names(), ["id", "category"])
        self.__assert_sarray_equal(result["category"], expected)

        # remove prefix == True by default
        expected = SArray([{}, {"a": "2", "b": 2.0}, {"a": "3"}, {"b": 4.0}])
        result = sf.pack_columns(column_name_prefix="category", dtype=dict)
        self.__assert_sarray_equal(result["category"], expected)

        # remove prefix == False
        expected = SArray(
            [
                {},
                {"category.a": "2", "category.b": 2.0},
                {"category.a": "3"},
                {"category.b": 4.0},
            ]
        )
        result = sf.pack_columns(
            column_name_prefix="category", dtype=dict, remove_prefix=False
        )
        self.assertEqual(result.column_names(), ["id", "category"])
        self.__assert_sarray_equal(result["category"], expected)

        # fill_na
        expected = SArray(
            [
                {"a": 1, "b": 1},
                {"a": "2", "b": 2.0},
                {"a": "3", "b": 1},
                {"a": 1, "b": 4.0},
            ]
        )
        result = sf.pack_columns(column_name_prefix="category", dtype=dict, fill_na=1)
        self.__assert_sarray_equal(result["category"], expected)

        expected = SArray([[1], [2], [3], [4]], list)
        result = sf.pack_columns(["id"], new_column_name="id")
        self.assertEqual(
            sorted(result.column_names()), sorted(["id", "category.a", "category.b"])
        )
        self.__assert_sarray_equal(result["id"], expected)

    def test_pack_columns(self):
        sf = SFrame()
        sf["id"] = [1, 2, 3, 4, 5]
        sf["b"] = [None, "2", "3", None, "5"]
        sf["c"] = [None, 2.0, 3.0, None, 5.0]

        expected_all_default = SArray(
            [
                [1, None, None],
                [2, "2", 2.0],
                [3, "3", 3.0],
                [4, None, None],
                [5, "5", 5.0],
            ]
        )

        # pack all columns, all default values
        self.__assert_sarray_equal(sf.pack_columns()["X1"], expected_all_default)

        expected_ab_default = SArray(
            [[1, None], [2, "2"], [3, "3"], [4, None], [5, "5"]]
        )

        expected_all_fillna_1 = SArray(
            [[1, -1, -1], [2, "2", 2.0], [3, "3", 3.0], [4, -1, -1], [5, "5", 5.0]]
        )

        # pack all columns do not drop na and also fill with some value
        result = sf.pack_columns(fill_na=-1)
        self.assertEqual(result.column_names(), ["X1"])
        self.__assert_sarray_equal(result["X1"], expected_all_fillna_1)

        # pack partial columns, all default value
        result = sf.pack_columns(["id", "b"])
        self.assertEqual(result.column_names(), ["c", "X2"])
        self.__assert_sarray_equal(result["c"], sf["c"])
        self.__assert_sarray_equal(result["X2"], expected_ab_default)

        expected_sarray_ac_fillna_default = SArray(
            [[1, float("NaN")], [2, 2.0], [3, 3.0], [4, float("NaN")], [5, 5.0]]
        )

        result = sf.pack_columns(["id", "c"], dtype=array.array)
        self.assertEqual(result.column_names(), ["b", "X2"])
        self.__assert_sarray_equal(result["b"], sf["b"])
        self.__assert_sarray_equal(result["X2"], expected_sarray_ac_fillna_default)

        expected_dict_default = SArray(
            [
                {"id": 1},
                {"id": 2, "b": "2", "c": 2.0},
                {"id": 3, "b": "3", "c": 3.0},
                {"id": 4},
                {"id": 5, "b": "5", "c": 5.0},
            ]
        )

        result = sf.pack_columns(dtype=dict)
        self.__assert_sarray_equal(result["X1"], expected_dict_default)

        expected_dict_fillna = SArray(
            [
                {"id": 1, "b": -1, "c": -1},
                {"id": 2, "b": "2", "c": 2.0},
                {"id": 3, "b": "3", "c": 3.0},
                {"id": 4, "b": -1, "c": -1},
                {"id": 5, "b": "5", "c": 5.0},
            ]
        )

        result = sf.pack_columns(dtype=dict, fill_na=-1)
        self.__assert_sarray_equal(result["X1"], expected_dict_fillna)

        # pack large number of rows
        sf = SFrame()
        num_rows = 100000
        sf["a"] = range(0, num_rows)
        sf["b"] = range(0, num_rows)
        result = sf.pack_columns(["a", "b"])
        self.assertEqual(len(result), num_rows)

    def test_pack_columns_dtype(self):
        a = SFrame({"name": [-140500967, -1405039672], "data": [3, 4]})
        b = a.pack_columns(["name", "data"], dtype=array.array)
        expected = SArray([[-140500967, 3], [-1405039672, 4]])
        self.__assert_sarray_equal(b["X1"], expected)

    def test_unpack_dict_mixtype(self):
        sf = SFrame(
            {"a": [{"a": ["haha", "hoho"]}, {"a": array.array("d", [1, 2, 3])}]}
        )
        sf = sf.unpack("a", column_name_prefix="")
        self.assertEqual(sf["a"].dtype, list)

        sf = SFrame(
            {"a": [{"a": ["haha", "hoho"]}, {"a": array.array("d", [1, 2, 3])}]}
        )
        sf = sf.unpack()
        self.assertEqual(sf["a"].dtype, list)

        sf = SFrame({"a": [{"a": ["haha", "hoho"]}, {"a": None}]})
        sf = sf.unpack("a", column_name_prefix="")
        self.assertEqual(sf["a"].dtype, list)

        sf = SFrame({"a": [{"a": ["haha", "hoho"]}, {"a": None}]})
        sf = sf.unpack("a", column_name_prefix="")
        self.assertEqual(sf["a"].dtype, list)

        sa = SArray([{"a": array.array("d", [1, 2, 3])}, {"a": None}])
        sf = sa.unpack(column_name_prefix="")
        self.assertEqual(sf["a"].dtype, array.array)

        sa = SArray([{"a": array.array("d", [1, 2, 3])}, {"a": {"b": 1}}])
        sf = sa.unpack(column_name_prefix="")
        self.assertEqual(sf["a"].dtype, str)

        sa = SArray([{"a": 1, "b": 0.1}, {"a": 0.1, "b": 1}])
        sf = sa.unpack(column_name_prefix="")
        self.assertEqual(sf["a"].dtype, float)
        self.assertEqual(sf["b"].dtype, float)

    def test_unpack_list(self):
        sa = SArray(
            [
                [1, None, None],
                [2, "2", 2.0],
                [3, "3", 3.0],
                [4, None, None],
                [5, "5", 5.0],
            ]
        )

        expected = SFrame()
        expected["a"] = [1, 2, 3, 4, 5]
        expected["b"] = [None, "2", "3", None, "5"]
        expected["c"] = [None, 2.0, 3.0, None, 5.0]

        result = sa.unpack()
        result.rename(dict(zip(result.column_names(), ["a", "b", "c"])), inplace=True)
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        result = sa.unpack(column_name_prefix="ttt")
        self.assertEqual(result.column_names(), ["ttt.0", "ttt.1", "ttt.2"])
        result.rename(dict(zip(result.column_names(), ["a", "b", "c"])), inplace=True)
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        # column types
        result = sa.unpack(column_types=[int, str, float])
        result.rename(dict(zip(result.column_names(), ["a", "b", "c"])), inplace=True)
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        # more column types
        result = sa.unpack(column_types=[int, str, float, int])
        result.rename(
            dict(zip(result.column_names(), ["a", "b", "c", "d"])), inplace=True
        )
        e = expected.select_columns(["a", "b", "c"])
        e.add_column(SArray([None for i in range(5)], int), "d", inplace=True)
        assert_frame_equal(result.to_dataframe(), e.to_dataframe())

        # less column types
        result = sa.unpack(column_types=[int, str])
        result.rename(dict(zip(result.column_names(), ["a", "b"])), inplace=True)
        e = expected.select_columns(["a", "b"])
        assert_frame_equal(result.to_dataframe(), e.to_dataframe())

        # fill na_value
        e = SFrame()
        e["a"] = [1, 2, None, 4, 5]
        e["b"] = [None, "2", "3", None, "5"]
        e["c"] = [None, 2.0, None, None, 5.0]
        result = sa.unpack(na_value=3)
        result.rename(dict(zip(result.column_names(), ["a", "b", "c"])), inplace=True)
        assert_frame_equal(result.to_dataframe(), e.to_dataframe())

        # wrong length
        with self.assertRaises(TypeError):
            sa.unpack(column_name_prefix=["a", "b"])

        # wrong type
        with self.assertRaises(RuntimeError):
            sa.unpack(column_types=[str, int, float])

        # wrong limit types
        with self.assertRaises(TypeError):
            sa.unpack(limit=["1"])

        # int array cannot be unpacked
        with self.assertRaises(TypeError):
            SArray([1, 2, 3, 4]).unpack()

        # column name must be a string
        with self.assertRaises(TypeError):
            sa.unpack(1)

        # invalid column type
        with self.assertRaises(TypeError):
            sa.unpack(column_types=int)

        # invalid column type
        with self.assertRaises(TypeError):
            sa.unpack(column_types=[np.array])

        # cannot infer type if no values
        with self.assertRaises(RuntimeError):
            SArray([], list).unpack()

    def test_unpack_array(self):
        import array

        sa = SArray(
            [
                array.array("d", [1, 1, 0]),
                array.array("d", [2, -1, 1]),
                array.array("d", [3, 3, 2]),
                array.array("d", [-1, 2, 3]),
                array.array("d", [5, 5, 4]),
            ]
        )

        expected = SFrame()
        expected["a"] = [1.0, 2.0, 3.0, -1.0, 5.0]
        expected["b"] = [1.0, -1.0, 3.0, 2.0, 5.0]
        expected["c"] = [0.0, 1.0, 2.0, 3.0, 4.0]

        result = sa.unpack()
        result.rename(dict(zip(result.column_names(), ["a", "b", "c"])), inplace=True)
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        # right amount column names
        result = sa.unpack(column_name_prefix="unpacked")
        result.rename(
            dict(zip(result.column_names(), ["t.0", "t.1", "t.2"])), inplace=True
        )
        result.rename(dict(zip(result.column_names(), ["a", "b", "c"])), inplace=True)
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        # column types
        result = sa.unpack(column_types=[int, str, float])
        result.rename(dict(zip(result.column_names(), ["a", "b", "c"])), inplace=True)
        expected["a"] = expected["a"].astype(int)
        expected["b"] = expected["b"].astype(str)
        expected["c"] = expected["c"].astype(float)
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        # more column types
        result = sa.unpack(column_types=[int, str, float, int])
        result.rename(
            dict(zip(result.column_names(), ["a", "b", "c", "d"])), inplace=True
        )
        e = expected.select_columns(["a", "b", "c"])
        e.add_column(SArray([None for i in range(5)], int), "d", inplace=True)
        assert_frame_equal(result.to_dataframe(), e.to_dataframe())

        # less column types
        result = sa.unpack(column_types=[int, str])
        result.rename(dict(zip(result.column_names(), ["a", "b"])), inplace=True)
        e = expected.select_columns(["a", "b"])
        assert_frame_equal(result.to_dataframe(), e.to_dataframe())

        # fill na_value
        e = SFrame()
        e["a"] = SArray([1, 2, 3, None, 5], float)
        e["b"] = SArray([1, None, 3, 2, 5], float)
        e["c"] = SArray([0, 1, 2, 3, 4], float)
        result = sa.unpack(na_value=-1)
        result.rename(dict(zip(result.column_names(), ["a", "b", "c"])), inplace=True)
        assert_frame_equal(result.to_dataframe(), e.to_dataframe())

    def test_unpack_dict(self):

        sf = SFrame([{"a": 1, "b": 2, "c": 3}, {"a": 4, "b": 5, "c": 6}])
        expected_sf = SFrame()
        expected_sf["a"] = [1, 4]
        expected_sf["b"] = [2, 5]
        expected_sf["c"] = [3, 6]
        unpacked_sf = sf.unpack()
        assert_frame_equal(unpacked_sf.to_dataframe(), expected_sf.to_dataframe())

        expected_sf = SFrame()
        expected_sf["xx.a"] = [1, 4]
        expected_sf["xx.b"] = [2, 5]
        expected_sf["xx.c"] = [3, 6]
        unpacked_sf = sf.unpack(column_name_prefix="xx")
        assert_frame_equal(unpacked_sf.to_dataframe(), expected_sf.to_dataframe())

        packed_sf = SFrame(
            {"X1": {"a": 1, "b": 2, "c": 3}, "X2": {"a": 4, "b": 5, "c": 6}}
        )

        with self.assertRaises(RuntimeError):
            packed_sf.unpack()

        sf = SFrame()

        sf["user_id"] = [1, 2, 3, 4, 5, 6, 7]
        sf["is_restaurant"] = [1, 1, 0, 0, 1, None, None]
        sf["is_retail"] = [None, 1, 1, None, 1, None, None]
        sf["is_electronics"] = ["yes", "no", "yes", None, "no", None, None]

        packed_sf = SFrame()
        packed_sf["user_id"] = sf["user_id"]
        packed_sf["category"] = [
            {"is_restaurant": 1, "is_electronics": "yes"},
            {"is_restaurant": 1, "is_retail": 1, "is_electronics": "no"},
            {"is_restaurant": 0, "is_retail": 1, "is_electronics": "yes"},
            {"is_restaurant": 0},
            {"is_restaurant": 1, "is_retail": 1, "is_electronics": "no"},
            {},
            None,
        ]

        with self.assertRaises(TypeError):
            packed_sf["user_id"].unpack()

        with self.assertRaises(TypeError):
            packed_sf["category"].unpack(1)

        with self.assertRaises(TypeError):
            packed_sf["category"].unpack(value_types=[int])

        # unpack only one column
        expected_sf = SFrame()
        expected_sf["is_retail"] = sf["is_retail"]
        unpacked_sf = packed_sf["category"].unpack(
            limit=["is_retail"], column_types=[int], column_name_prefix=None
        )
        assert_frame_equal(unpacked_sf.to_dataframe(), expected_sf.to_dataframe())

        # unpack all
        unpacked_sf = packed_sf["category"].unpack(
            column_name_prefix=None,
            column_types=[int, int, str],
            limit=["is_restaurant", "is_retail", "is_electronics"],
        )
        assert_frame_equal(
            unpacked_sf.to_dataframe(),
            sf[["is_restaurant", "is_retail", "is_electronics"]].to_dataframe(),
        )

        # auto infer types, the column order may be different, so use order here before comparison
        unpacked_sf = packed_sf["category"].unpack()
        unpacked_sf.rename(
            {
                "X.is_restaurant": "is_restaurant",
                "X.is_retail": "is_retail",
                "X.is_electronics": "is_electronics",
            },
            inplace=True,
        )
        assert_frame_equal(
            unpacked_sf.to_dataframe().sort_index(axis=1),
            sf[["is_restaurant", "is_retail", "is_electronics"]]
            .to_dataframe()
            .sort_index(axis=1),
        )

        unpacked_sf = packed_sf["category"].unpack(na_value=0, column_name_prefix="new")
        expected = SFrame()
        expected["new.is_restaurant"] = [1, 1, None, None, 1, None, None]
        expected["new.is_retail"] = [None, 1, 1, None, 1, None, None]
        expected["new.is_electronics"] = ["yes", "no", "yes", None, "no", None, None]
        assert_frame_equal(
            unpacked_sf.to_dataframe().sort_index(axis=1),
            expected.to_dataframe().sort_index(axis=1),
        )

        # unpack a dictionary key integer as key
        sa = SArray([{1: "a"}, {2: "b"}])
        result = sa.unpack()
        expected = SFrame({"X.1": ["a", None], "X.2": [None, "b"]})
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        result = sa.unpack(limit=[2])
        expected = SFrame({"X.2": [None, "b"]})
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        result = sa.unpack(limit=[2], column_name_prefix="expanded")
        expected = SFrame({"expanded.2": [None, "b"]})
        assert_frame_equal(result.to_dataframe(), expected.to_dataframe())

        sa = SArray([{i: i} for i in range(500)])
        unpacked_sa = sa.unpack()
        self.assertEqual(len(unpacked_sa), len(sa))
        i = 0
        for v in unpacked_sa:
            for j in range(500):
                val = v["X." + str(j)]
                if j == i:
                    self.assertEqual(val, i)
                else:
                    self.assertEqual(val, None)
            i = i + 1

        # if types don't agree, convert to string automatically
        sa = SArray([{"a": 1}, {"a": "a_3"}])
        sf = sa.unpack()
        self.assertEqual(sf.column_types(), [str])

        sa = SArray([{"a": None}, {"a": 1}])
        sf = sa.unpack()
        self.assertEqual(sf.column_types(), [int])

        sa = SArray([{"a": 1}, {"a": None}])
        sf = sa.unpack()
        self.assertEqual(sf.column_types(), [int])

        # type inference is already at server side even if limit is given
        sa = SArray(
            [{"c" + str(i): i if i % 2 == 0 else "v" + str(i)} for i in range(1000)]
        )
        unpacked = sa.unpack(
            limit=["c" + str(i) for i in range(10)], column_name_prefix=""
        )
        for i in range(10):
            v = unpacked[i]
            for j in range(10):
                if j != i:
                    self.assertEqual(v["c" + str(j)], None)
                elif j % 2 == 0:
                    self.assertEqual(v["c" + str(j)], j)
                else:
                    self.assertEqual(v["c" + str(j)], "v" + str(j))

    def test_unpack_sframe(self):
        sf = SFrame()
        sf["user_id"] = range(7)
        sf["category"] = [
            {"is_restaurant": 1, "is_electronics": "yes"},
            {"is_restaurant": 1, "is_retail": 1, "is_electronics": "no"},
            {"is_restaurant": 0, "is_retail": 1, "is_electronics": "yes"},
            {"is_restaurant": 0},
            {"is_restaurant": 1, "is_retail": 1, "is_electronics": "no"},
            {},
            None,
        ]
        sf["list"] = [
            None,
            range(1),
            range(2),
            range(3),
            range(1),
            range(2),
            range(3),
        ]

        with self.assertRaises(TypeError):
            sf.unpack("user_id")

        expected = SFrame()
        expected["user_id"] = sf["user_id"]
        expected["list"] = sf["list"]
        expected["is_restaurant"] = [1, 1, 0, 0, 1, None, None]
        expected["is_retail"] = [None, 1, 1, None, 1, None, None]
        expected["is_electronics"] = ["yes", "no", "yes", None, "no", None, None]

        result = sf.unpack("category")
        result.rename(
            {
                "category.is_restaurant": "is_restaurant",
                "category.is_retail": "is_retail",
                "category.is_electronics": "is_electronics",
            },
            inplace=True,
        )
        assert_frame_equal(
            expected.to_dataframe().sort_index(axis=1),
            result.to_dataframe().sort_index(axis=1),
        )

        result = sf.unpack(column_name="category", column_name_prefix="")
        assert_frame_equal(
            expected.to_dataframe().sort_index(axis=1),
            result.to_dataframe().sort_index(axis=1),
        )

        result = sf.unpack(column_name="category", column_name_prefix="abc")
        result.rename(
            {
                "abc.is_restaurant": "is_restaurant",
                "abc.is_retail": "is_retail",
                "abc.is_electronics": "is_electronics",
            },
            inplace=True,
        )
        assert_frame_equal(
            expected.to_dataframe().sort_index(axis=1),
            result.to_dataframe().sort_index(axis=1),
        )

        result = sf.unpack(
            column_name="category",
            column_name_prefix="",
            column_types=[str],
            limit=["is_restaurant"],
        )
        new_expected = expected[["user_id", "list", "is_restaurant"]]
        new_expected["is_restaurant"] = new_expected["is_restaurant"].astype(str)
        assert_frame_equal(
            new_expected.to_dataframe().sort_index(axis=1),
            result.to_dataframe().sort_index(axis=1),
        )

        result = sf.unpack(column_name="category", column_name_prefix="", na_value=None)
        assert_frame_equal(
            expected.to_dataframe().sort_index(axis=1),
            result.to_dataframe().sort_index(axis=1),
        )

        result = sf.unpack(column_name="list")
        expected = SFrame()
        expected["user_id"] = sf["user_id"]
        expected["list.0"] = [None, 0, 0, 0, 0, 0, 0]
        expected["list.1"] = [None, None, 1, 1, None, 1, 1]
        expected["list.2"] = [None, None, None, 2, None, None, 2]
        expected["category"] = sf["category"]
        assert_frame_equal(
            expected.to_dataframe().sort_index(axis=1),
            result.to_dataframe().sort_index(axis=1),
        )

        result = sf.unpack(column_name="list", na_value=2)
        expected = SFrame()
        expected["user_id"] = sf["user_id"]
        expected["list.0"] = [None, 0, 0, 0, 0, 0, 0]
        expected["list.1"] = [None, None, 1, 1, None, 1, 1]
        expected["list.2"] = [None, None, None, None, None, None, None]
        expected["category"] = sf["category"]
        assert_frame_equal(
            expected.to_dataframe().sort_index(axis=1),
            result.to_dataframe().sort_index(axis=1),
        )

        # auto resolving conflicting names
        sf = SFrame()
        sf["a"] = range(100)
        sf["b"] = [range(5) for i in range(100)]
        sf["b.0"] = range(100)
        sf["b.0.1"] = range(100)
        result = sf.unpack("b")
        self.assertEqual(
            result.column_names(),
            [
                "a",
                "b.0",
                "b.0.1",
                "b.0.1.1",
                "b.1.1.1",
                "b.2.1.1",
                "b.3.1.1",
                "b.4.1.1",
            ],
        )

        sf = SFrame()
        sf["a"] = range(100)
        sf["b"] = [{"str1": i, "str2": i + 1} for i in range(100)]
        sf["b.str1"] = range(100)
        result = sf.unpack("b")
        self.assertEqual(len(result.column_names()), 4)

    def test_stack_dict(self):
        sf = SFrame()
        sf["user_id"] = [1, 2, 3, 4, 5]
        sf["user_name"] = ["user" + str(i) for i in list(sf["user_id"])]
        sf["category"] = [
            {"is_restaurant": 1,},
            {"is_restaurant": 0, "is_retail": 1},
            {"is_retail": 0},
            {},
            None,
        ]

        expected_sf = SFrame()
        expected_sf["user_id"] = [1, 2, 2, 3, 4, 5]
        expected_sf["user_name"] = [
            "user" + str(i) for i in list(expected_sf["user_id"])
        ]
        expected_sf["category"] = [
            "is_restaurant",
            "is_restaurant",
            "is_retail",
            "is_retail",
            None,
            None,
        ]
        expected_sf["value"] = [1, 0, 1, 0, None, None]
        df_expected = (
            expected_sf.to_dataframe()
            .sort_values(["user_id", "category"])
            .reset_index(drop=True)
        )

        with self.assertRaises(TypeError):
            sf.stack()

        with self.assertRaises(ValueError):
            sf.stack("sss")

        with self.assertRaises(ValueError):
            sf.stack("category", ["user_id", "value"])

        # normal case
        stacked_sf = sf.stack("category", ["category", "value"])
        assert_frame_equal(
            stacked_sf.to_dataframe()
            .sort_values(["user_id", "category"])
            .reset_index(drop=True),
            df_expected,
        )

        # set column types
        stacked_sf = sf.stack("category")
        self.assertTrue(stacked_sf.column_types()[2] == str)
        self.assertTrue(stacked_sf.column_types()[3] == int)

        # auto generate column names
        stacked_sf = sf.stack("category")
        new_column_names = stacked_sf.column_names()
        self.assertTrue(len(new_column_names) == 4)
        expected_sf.rename(
            {"category": new_column_names[2], "value": new_column_names[3]},
            inplace=True,
        )
        df_expected = (
            expected_sf.to_dataframe()
            .sort_values(["user_id", new_column_names[2]])
            .reset_index(drop=True)
        )
        assert_frame_equal(
            stacked_sf.to_dataframe()
            .sort_values(["user_id", new_column_names[2]])
            .reset_index(drop=True),
            df_expected,
        )

        # dropna
        expected_sf = SFrame()
        expected_sf["user_id"] = [1, 2, 2, 3, 4, 5]
        expected_sf["user_name"] = [
            "user" + str(i) for i in list(expected_sf["user_id"])
        ]
        expected_sf["category"] = [
            "is_restaurant",
            "is_restaurant",
            "is_retail",
            "is_retail",
            None,
            None,
        ]
        expected_sf["value"] = [1, 0, 1, 0, None, None]
        df_expected = (
            expected_sf.to_dataframe()
            .sort_values(["user_id", "category"])
            .reset_index(drop=True)
        )

        stacked_sf = sf.stack("category", ["category", "value"], drop_na=False)
        assert_frame_equal(
            stacked_sf.to_dataframe()
            .sort_values(["user_id", "category"])
            .reset_index(drop=True),
            df_expected,
        )

        sf = SFrame()
        sf["a"] = SArray(([{}] * 100) + [{"a": 1}])

        # its a dict need 2 types
        with self.assertRaises(ValueError):
            sf.stack("a", ["key", "value"], new_column_type=[str])
        with self.assertRaises(ValueError):
            sf.stack("a", ["key", "value"], new_column_type=str)

        sf.stack("a", ["key", "value"], new_column_type=[str, int])
        expected_sf = SFrame()
        expected_sf["key"] = SArray([None] * 100 + ["a"])
        expected_sf["value"] = SArray([None] * 100 + [1])

    def test_stack_list(self):
        sf = SFrame()
        sf["a"] = [1, 2, 3, 4, 5]
        sf["b"] = [["a", "b"], ["c"], ["d"], ["e", None], None]
        expected_result = SFrame()
        expected_result["a"] = [1, 1, 2, 3, 4, 4, 5]
        expected_result["X1"] = ["a", "b", "c", "d", "e", None, None]

        with self.assertRaises(TypeError):
            sf.stack()

        with self.assertRaises(ValueError):
            sf.stack("sss")

        with self.assertRaises(TypeError):
            sf.stack("a")

        with self.assertRaises(TypeError):
            sf.stack("b", ["something"])

        result = sf.stack("b", drop_na=False)
        stacked_column_name = result.column_names()[1]
        expected_result.rename({"X1": stacked_column_name}, inplace=True)
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        # default drop_na=False
        result = sf.stack("b")
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        result = sf.stack("b", new_column_name="b", drop_na=False)
        expected_result.rename({stacked_column_name: "b"}, inplace=True)
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        result = sf.stack("b", new_column_name="b", drop_na=False)
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        # drop_na=True
        result = sf.stack("b", drop_na=True)
        expected_result = SFrame()
        expected_result["a"] = [1, 1, 2, 3, 4, 4]
        expected_result[result.column_names()[1]] = ["a", "b", "c", "d", "e", None]
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        sf = SFrame()
        n = 1000000
        sf["a"] = range(1, n)
        sf["b"] = [[str(i), str(i + 1)] for i in range(1, n)]
        result = sf.stack("b")
        self.assertTrue(len(result), n * 2)

        sf = SFrame()
        sf["a"] = SArray(([[]] * 100) + [["a", "b"]])

        # its a dict need 2 types
        with self.assertRaises(ValueError):
            sf.stack("a", "a", new_column_type=[str, int])

        sf.stack("a", "a", new_column_type=str)
        expected_sf = SFrame()
        expected_sf["a"] = SArray([None] * 100 + ["a", "b"])

    def test_stack_vector(self):
        sf = SFrame()
        sf["a"] = [1, 2, 3, 4, 5]
        sf["b"] = [[1], [1, 2], [1, 2, 3], [1, 2, 3, 4], None]
        expected_result = SFrame()
        expected_result["a"] = [1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5]
        expected_result["X1"] = [1, 1, 2, 1, 2, 3, 1, 2, 3, 4, None]

        with self.assertRaises(TypeError):
            sf.stack()

        with self.assertRaises(ValueError):
            sf.stack("sss")

        with self.assertRaises(TypeError):
            sf.stack("a")

        with self.assertRaises(TypeError):
            sf.stack("b", ["something"])

        result = sf.stack("b", drop_na=False)
        stacked_column_name = result.column_names()[1]
        expected_result.rename({"X1": stacked_column_name}, inplace=True)
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        # default drop_na=False
        result = sf.stack("b")
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        result = sf.stack("b", new_column_name="b", drop_na=False)
        expected_result.rename({stacked_column_name: "b"}, inplace=True)
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        result = sf.stack("b", new_column_name="b", drop_na=False)
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        # drop_na=True
        result = sf.stack("b", drop_na=True)
        expected_result = SFrame()
        expected_result["a"] = [1, 2, 2, 3, 3, 3, 4, 4, 4, 4]
        expected_result[result.column_names()[1]] = SArray(
            [1, 1, 2, 1, 2, 3, 1, 2, 3, 4], float
        )
        assert_frame_equal(result.to_dataframe(), expected_result.to_dataframe())

        import array

        sf = SFrame()
        sf["a"] = SArray(([array.array("d")] * 100) + [array.array("d", [1.0, 2.0])])

        # its a dict need 2 types
        with self.assertRaises(ValueError):
            sf.stack("a", "a", new_column_type=[str, int])

        sf.stack("a", "a", new_column_type=int)
        expected_sf = SFrame()
        expected_sf["a"] = SArray([None] * 100 + [1, 2])

    def test_unstack_dict(self):
        sf = SFrame()
        sf["user_id"] = [1, 2, 3, 4]
        sf["user_name"] = ["user" + str(i) for i in list(sf["user_id"])]
        sf["categories"] = [
            {"is_restaurant": 1,},
            {"is_restaurant": 0, "is_retail": 1},
            {"is_retail": 0},
            None,
        ]

        stacked_sf = sf.stack("categories", ["category", "value"], drop_na=False)

        # normal unstack
        unstacked_sf = stacked_sf.unstack(
            column_names=["category", "value"], new_column_name="categories"
        )
        # these frames are *almost* equal except user4 will be {} instead of None
        assert_frame_equal(
            sf.fillna("categories", {}).to_dataframe(),
            unstacked_sf.to_dataframe().sort_values("user_id").reset_index(drop=True),
        )

        # missing new column name
        unstacked_sf = stacked_sf.unstack(["category", "value"])
        self.assertEqual(len(unstacked_sf.column_names()), 3)
        unstacked_sf.rename(
            {unstacked_sf.column_names()[2]: "categories"}, inplace=True
        )
        assert_frame_equal(
            sf.fillna("categories", {}).to_dataframe(),
            unstacked_sf.to_dataframe().sort_values("user_id").reset_index(drop=True),
        )

        # missing column names
        with self.assertRaises(KeyError):
            stacked_sf.unstack(["category", "value1"])

        # wrong input
        with self.assertRaises(TypeError):
            stacked_sf.unstack(["category"])

        # duplicate new column name
        with self.assertRaises(RuntimeError):
            unstacked_sf = stacked_sf.unstack(["category", "value"], "user_name")

    def test_unstack_list(self):
        sf = SFrame()
        sf["a"] = [1, 2, 3, 4]
        sf["b"] = [range(10), range(20), range(30), range(50)]
        stacked_sf = sf.stack("b", new_column_name="new_b")
        unstacked_sf = stacked_sf.unstack("new_b", new_column_name="b")
        self.__assert_concat_result_equal(sf.sort("a"), unstacked_sf.sort("a"), ["b"])

        unstacked_sf = stacked_sf.unstack("new_b")
        unstacked_sf.rename({unstacked_sf.column_names()[1]: "b"}, inplace=True)
        self.__assert_concat_result_equal(sf.sort("a"), unstacked_sf.sort("a"), ["b"])

        unstacked_sf = stacked_sf.unstack("new_b", new_column_name="b")
        unstacked_sf.rename({unstacked_sf.column_names()[1]: "b"}, inplace=True)
        self.__assert_concat_result_equal(sf.sort("a"), unstacked_sf.sort("a"), ["b"])

        with self.assertRaises(RuntimeError):
            stacked_sf.unstack("new_b", new_column_name="a")

        with self.assertRaises(TypeError):
            stacked_sf.unstack(["new_b"])

        with self.assertRaises(KeyError):
            stacked_sf.unstack("non exist")

    def test_content_identifier(self):
        sf = SFrame({"a": [1, 2, 3, 4], "b": ["1", "2", "3", "4"]})
        a1 = sf["a"].__get_content_identifier__()
        a2 = sf["a"].__get_content_identifier__()
        self.assertEqual(a1, a2)

    def test_random_access(self):
        t1 = list(range(0, 100000))
        t2 = [str(i) for i in t1]
        t = [{"t1": t1[i], "t2": t2[i]} for i in range(len(t1))]
        s = SFrame({"t1": t1, "t2": t2})
        # simple slices
        self.__test_equal(s[1:10000], pd.DataFrame(t[1:10000]))
        self.__test_equal(s[0:10000:3], pd.DataFrame(t[0:10000:3]))
        self.__test_equal(s[1:10000:3], pd.DataFrame(t[1:10000:3]))
        self.__test_equal(s[2:10000:3], pd.DataFrame(t[2:10000:3]))
        self.__test_equal(s[3:10000:101], pd.DataFrame(t[3:10000:101]))
        # negative slices
        self.__test_equal(s[-5:], pd.DataFrame(t[-5:]))
        self.__test_equal(s[-1:], pd.DataFrame(t[-1:]))
        self.__test_equal(s[-100:-10], pd.DataFrame(t[-100:-10]))
        self.__test_equal(s[-100:-10:2], pd.DataFrame(t[-100:-10:2]))
        # single element reads
        self.assertEqual(s[511], t[511])
        self.assertEqual(s[1912], t[1912])
        self.assertEqual(s[-1], t[-1])
        self.assertEqual(s[-10], t[-10])

        # edge case oddities
        self.__test_equal(s[10:100:100], pd.DataFrame(t[10:100:100]))
        self.__test_equal(s[-100 : len(s) : 10], pd.DataFrame(t[-100 : len(t) : 10]))
        self.assertEqual(len(s[-1:-2]), 0)
        self.assertEqual(len(s[-1:-1000:2]), 0)
        with self.assertRaises(IndexError):
            s[len(s)]

    def sort_n_rows(self, nrows=100):
        nrows += 1
        sf = SFrame()
        sf["a"] = range(1, nrows)
        sf["b"] = [float(i) for i in range(1, nrows)]
        sf["c"] = [str(i) for i in range(1, nrows)]
        sf["d"] = [[i, i + 1] for i in range(1, nrows)]

        reversed_sf = SFrame()
        reversed_sf["a"] = range(nrows - 1, 0, -1)
        reversed_sf["b"] = [float(i) for i in range(nrows - 1, 0, -1)]
        reversed_sf["c"] = [str(i) for i in range(nrows - 1, 0, -1)]
        reversed_sf["d"] = [[i, i + 1] for i in range(nrows - 1, 0, -1)]

        with self.assertRaises(TypeError):
            sf.sort()

        with self.assertRaises(TypeError):
            sf.sort(1)

        with self.assertRaises(TypeError):
            sf.sort("d")

        with self.assertRaises(ValueError):
            sf.sort("nonexist")

        with self.assertRaises(TypeError):
            sf.sort({"a": True})

        result = sf.sort("a")
        assert_frame_equal(sf.to_dataframe(), result.to_dataframe())

        # try a lazy input
        result = sf[sf["a"] > 10].sort("a")
        assert_frame_equal(sf[sf["a"] > 10].to_dataframe(), result.to_dataframe())

        result = sf.sort("a", ascending=False)
        assert_frame_equal(reversed_sf.to_dataframe(), result.to_dataframe())

        # lazy reversed
        result = sf[sf["a"] > 10].sort("a", ascending=False)
        assert_frame_equal(
            reversed_sf[reversed_sf["a"] > 10].to_dataframe(), result.to_dataframe()
        )

        # lazy reversed
        result = sf[sf["a"] > 10].sort("a", ascending=False)
        assert_frame_equal(
            reversed_sf[reversed_sf["a"] > 10].to_dataframe(), result.to_dataframe()
        )

        # sort two columns
        result = sf.sort(["a", "b"])
        assert_frame_equal(sf.to_dataframe(), result.to_dataframe())

        result = sf.sort(["a", "c"], ascending=False)
        assert_frame_equal(reversed_sf.to_dataframe(), result.to_dataframe())

        result = sf.sort([("a", True), ("b", False)])
        assert_frame_equal(sf.to_dataframe(), result.to_dataframe())

        result = sf.sort([("a", False), ("b", True)])
        assert_frame_equal(reversed_sf.to_dataframe(), result.to_dataframe())

        # empty sort should not throw
        sf = SFrame({"x": []})
        sf.sort("x")

    def test_sort(self):
        # self.sort_n_rows(100)
        for i in range(1, 10):
            self.sort_n_rows(i)

    def test_dropna(self):
        # empty case
        sf = SFrame()
        self.assertEqual(len(sf.dropna()), 0)

        # normal case
        self.__test_equal(
            self.employees_sf.dropna(), self.employees_sf[0:5].to_dataframe()
        )
        test_split = self.employees_sf.dropna_split()
        self.__test_equal(test_split[0], self.employees_sf[0:5].to_dataframe())
        self.__test_equal(test_split[1], self.employees_sf[5:6].to_dataframe())

        # test recursively removing nan
        test_sf = SFrame(
            {
                "array": SArray(
                    [
                        [1, 1],
                        [2, np.nan],
                        [3, 3],
                        [4, 4],
                        [5, 5],
                        [6, np.nan],
                        [7, 7],
                        [8, np.nan],
                    ],
                    np.ndarray,
                ),
                "lists": SArray(
                    [[1], None, [], [4], [5, 5], [6, np.nan], [7], None], list
                ),
                "dicts": SArray(
                    [
                        {1: 2},
                        {2: 3},
                        {3: 4},
                        {},
                        {5: None},
                        {6: 7},
                        {7: [7, [7, np.nan]]},
                        None,
                    ],
                    dict,
                ),
            }
        )

        # non-recursive dropna
        self.__test_equal(
            test_sf.dropna(how="any"), test_sf[0:1].append(test_sf[2:7]).to_dataframe()
        )
        test_split = test_sf.dropna_split()
        self.__test_equal(
            test_split[0], test_sf[0:1].append(test_sf[2:7]).to_dataframe()
        )

        self.__test_equal(test_sf.dropna(how="all"), test_sf.to_dataframe())
        test_split = test_sf.dropna_split(how="all")
        self.assertEqual(len(test_split[1]), 0)

        # recursive dropna
        self.__test_equal(
            test_sf.dropna(recursive=True),
            pd.DataFrame(
                {
                    "array": [[1, 1], [3, 3], [4, 4]],
                    "lists": [[1], [], [4]],
                    "dicts": [{1: 2}, {3: 4}, {}],
                }
            ),
        )
        test_split = test_sf.dropna_split(recursive=True)
        self.__test_equal(
            test_split[0], test_sf[0:1].append(test_sf[2:4]).to_dataframe()
        )
        # nan is not comparable, so we don't check the nan part
        # self.__test_equal(test_split[1], test_sf[1:2].append(test_sf[4:8]).to_dataframe())

        # the 'all' case
        self.__test_equal(
            test_sf.dropna(how="all", recursive=True), test_sf[0:7].to_dataframe()
        )
        test_split = test_sf.dropna_split(how="all", recursive=True)
        self.__test_equal(test_split[0], test_sf[0:7].to_dataframe())

        # test 'split' cases
        self.__test_equal(
            test_sf.dropna("array", recursive=True),
            test_sf[0:1].append(test_sf[2:5]).append(test_sf[6:7]).to_dataframe(),
        )
        test_split = test_sf.dropna_split("array", recursive=True)
        self.__test_equal(
            test_split[0],
            test_sf[0:1].append(test_sf[2:5]).append(test_sf[6:7]).to_dataframe(),
        )

        self.__test_equal(
            test_sf.dropna("lists", recursive=True),
            test_sf[0:1].append(test_sf[2:5]).append(test_sf[6:7]).to_dataframe(),
        )
        test_split = test_sf.dropna_split("lists", recursive=True)
        self.__test_equal(
            test_split[0],
            test_sf[0:1].append(test_sf[2:5]).append(test_sf[6:7]).to_dataframe(),
        )

        self.__test_equal(
            test_sf.dropna("dicts", recursive=True),
            test_sf[0:4].append(test_sf[5:6]).to_dataframe(),
        )
        test_split = test_sf.dropna_split("dicts", recursive=True)
        self.__test_equal(
            test_split[0], test_sf[0:4].append(test_sf[5:6]).to_dataframe()
        )

        # create some other test sframe
        test_sf = SFrame(
            {
                "ints": SArray([None, None, 3, 4, None], int),
                "floats": SArray([np.nan, 2.0, 3.0, 4.0, np.nan], float),
                "strs": SArray(["1", np.nan, "", "4", None], str),
                "lists": SArray([[1], None, [], [1, 1, 1, 1], None], list),
                "dicts": SArray([{1: 2}, {2: 3}, {}, {4: 5}, None], dict),
            }
        )

        # another normal, but more interesting case
        self.__test_equal(
            test_sf.dropna(),
            pd.DataFrame(
                {
                    "ints": [3, 4],
                    "floats": [3.0, 4.0],
                    "strs": ["", "4"],
                    "lists": [[], [1, 1, 1, 1]],
                    "dicts": [{}, {4: 5}],
                }
            ),
        )
        test_split = test_sf.dropna_split()
        self.__test_equal(test_split[0], test_sf[2:4].to_dataframe())
        self.__test_equal(
            test_split[1], test_sf[0:2].append(test_sf[4:5]).to_dataframe()
        )

        # the 'all' case
        self.__test_equal(test_sf.dropna(how="all"), test_sf[0:4].to_dataframe())
        test_split = test_sf.dropna_split(how="all")
        self.__test_equal(test_split[0], test_sf[0:4].to_dataframe())
        self.__test_equal(test_split[1], test_sf[4:5].to_dataframe())

        # select some columns
        self.__test_equal(
            test_sf.dropna(["ints", "floats"], how="all"), test_sf[1:4].to_dataframe()
        )
        test_split = test_sf.dropna_split(["ints", "floats"], how="all")
        self.__test_equal(test_split[0], test_sf[1:4].to_dataframe())
        self.__test_equal(
            test_split[1], test_sf[0:1].append(test_sf[4:5]).to_dataframe()
        )

        self.__test_equal(test_sf.dropna("strs"), test_sf[0:4].to_dataframe())
        test_split = test_sf.dropna_split("strs")
        self.__test_equal(test_split[0], test_sf[0:4].to_dataframe())
        self.__test_equal(test_split[1], test_sf[4:5].to_dataframe())

        self.__test_equal(
            test_sf.dropna(["strs", "dicts"]), test_sf[0:4].to_dataframe()
        )
        test_split = test_sf.dropna_split(["strs", "dicts"])
        self.__test_equal(test_split[0], test_sf[0:4].to_dataframe())
        self.__test_equal(test_split[1], test_sf[4:5].to_dataframe())

        # bad stuff
        with self.assertRaises(TypeError):
            test_sf.dropna(1)
            test_sf.dropna([1, 2])
            test_sf.dropna("strs", how=1)
            test_sf.dropna_split(1)
            test_sf.dropna_split([1, 2])
            test_sf.dropna_split("strs", how=1)

        with self.assertRaises(ValueError):
            test_sf.dropna("ints", how="blah")
            test_sf.dropna_split("ints", how="blah")

        with self.assertRaises(RuntimeError):
            test_sf.dropna("dontexist")
            test_sf.dropna_split("dontexist")

    def test_add_row_number(self):
        sf = SFrame(self.__create_test_df(400000))

        sf = sf.add_row_number("id")
        self.assertEqual(list(sf["id"]), list(range(0, 400000)))

        del sf["id"]

        sf = sf.add_row_number("id", -20000)
        self.assertEqual(list(sf["id"]), list(range(-20000, 380000)))
        del sf["id"]

        sf = sf.add_row_number("id", 40000)
        self.assertEqual(list(sf["id"]), list(range(40000, 440000)))

        with self.assertRaises(RuntimeError):
            sf.add_row_number("id")

        with self.assertRaises(TypeError):
            sf = sf.add_row_number(46)
            sf = sf.add_row_number("id2", start="hi")

    def test_inplace_not_inplace(self):
        # add row number
        sf = SFrame(self.__create_test_df(1000))
        sf2 = sf.add_row_number("id", inplace=False)
        self.assertTrue(sf2 is not sf)
        self.assertTrue("id" in sf2.column_names())
        self.assertTrue("id" not in sf.column_names())

        sf2 = sf.add_row_number("id", inplace=True)
        self.assertTrue(sf2 is sf)
        self.assertTrue("id" in sf2.column_names())

        # add column
        sf = SFrame(self.__create_test_df(1000))
        newcol = SArray(range(1000))
        sf2 = sf.add_column(newcol, "newcol", inplace=False)
        self.assertTrue(sf2 is not sf)
        self.assertTrue("newcol" in sf2.column_names())
        self.assertTrue("newcol" not in sf.column_names())
        sf2 = sf.add_column(newcol, "newcol", inplace=True)
        self.assertTrue(sf2 is sf)
        self.assertTrue("newcol" in sf2.column_names())

        # add columns
        sf = SFrame(self.__create_test_df(1000))
        newcols = SFrame({"newcol": range(1000), "newcol2": range(1000)})
        sf2 = sf.add_columns(newcols, inplace=False)
        self.assertTrue(sf2 is not sf)
        self.assertTrue("newcol" in sf2.column_names())
        self.assertTrue("newcol2" in sf2.column_names())
        self.assertTrue("newcol" not in sf.column_names())
        self.assertTrue("newcol2" not in sf.column_names())
        sf2 = sf.add_columns(newcols, inplace=True)
        self.assertTrue(sf2 is sf)
        self.assertTrue("newcol" in sf2.column_names())
        self.assertTrue("newcol2" in sf2.column_names())

        # remove column
        sf = SFrame(self.__create_test_df(1000))
        sf2 = sf.remove_column("int_data", inplace=False)
        self.assertTrue(sf2 is not sf)
        self.assertTrue("int_data" in sf.column_names())
        self.assertTrue("int_data" not in sf2.column_names())
        sf2 = sf.remove_column("int_data", inplace=True)
        self.assertTrue(sf2 is sf)
        self.assertTrue("int_data" not in sf2.column_names())

        # remove columns
        sf = SFrame(self.__create_test_df(1000))
        sf2 = sf.remove_columns(["int_data", "float_data"], inplace=False)
        self.assertTrue(sf2 is not sf)
        self.assertTrue("int_data" in sf.column_names())
        self.assertTrue("float_data" in sf.column_names())
        self.assertTrue("int_data" not in sf2.column_names())
        self.assertTrue("float_data" not in sf2.column_names())
        sf2 = sf.remove_columns(["int_data", "float_data"], inplace=True)
        self.assertTrue(sf2 is sf)
        self.assertTrue("int_data" not in sf2.column_names())
        self.assertTrue("float_data" not in sf2.column_names())

        # rename
        sf = SFrame(self.__create_test_df(1000))
        sf2 = sf.rename({"int_data": "int", "float_data": "float"}, inplace=False)
        self.assertTrue(sf2 is not sf)
        self.assertTrue("int_data" in sf.column_names())
        self.assertTrue("float_data" in sf.column_names())
        self.assertTrue("int" not in sf.column_names())
        self.assertTrue("float" not in sf.column_names())
        self.assertTrue("int_data" not in sf2.column_names())
        self.assertTrue("float_data" not in sf2.column_names())
        self.assertTrue("int" in sf2.column_names())
        self.assertTrue("float" in sf2.column_names())
        sf2 = sf.rename({"int_data": "int", "float_data": "float"}, inplace=True)
        self.assertTrue(sf2 is sf)
        self.assertTrue("int_data" not in sf2.column_names())
        self.assertTrue("float_data" not in sf2.column_names())
        self.assertTrue("int" in sf2.column_names())
        self.assertTrue("float" in sf2.column_names())

        # swap
        sf = SFrame(self.__create_test_df(1000))
        old_cnames = sf.column_names()

        # swap int_data and float_data
        new_cnames = sf.column_names()
        int_data_idx = new_cnames.index("int_data")
        float_data_idx = new_cnames.index("float_data")
        new_cnames[int_data_idx], new_cnames[float_data_idx] = (
            new_cnames[float_data_idx],
            new_cnames[int_data_idx],
        )

        sf2 = sf.swap_columns("int_data", "float_data", inplace=False)
        self.assertTrue(sf2 is not sf)
        self.assertEqual(sf.column_names(), old_cnames)
        self.assertEqual(sf2.column_names(), new_cnames)

        sf2 = sf.swap_columns("int_data", "float_data", inplace=True)
        self.assertTrue(sf2 is sf)
        self.assertEqual(sf2.column_names(), new_cnames)

    def test_check_lazy_sframe_size(self):
        # empty sframe, materialized, has_size
        sf = SFrame()
        self.assertTrue(sf.__is_materialized__())
        self.assertTrue(sf.__has_size__())

        # add one column, not materialized, has_size
        sf["a"] = range(1000)
        self.assertTrue(sf.__is_materialized__())
        self.assertTrue(sf.__has_size__())

        # materialize it, materialized, has_size
        sf["a"] = range(1000)
        sf.materialize()
        self.assertTrue(sf.__is_materialized__())
        self.assertTrue(sf.__has_size__())

        # logical filter, not materialized, not has_size
        sf = sf[sf["a"] > 5000]
        self.assertFalse(sf.__is_materialized__())
        self.assertFalse(sf.__has_size__())

    def test_lazy_logical_filter_sarray(self):
        g = SArray(range(10000))
        g2 = SArray(range(10000))
        a = g[g > 10]
        a2 = g2[g > 10]
        z = a[a2 > 20]
        self.assertEqual(len(z), 9979)

    def test_lazy_logical_filter_sframe(self):
        g = SFrame({"a": range(10000)})
        g2 = SFrame({"a": range(10000)})
        a = g[g["a"] > 10]
        a2 = g2[g["a"] > 10]
        z = a[a2["a"] > 20]
        self.assertEqual(len(z), 9979)

    def test_column_manipulation_of_lazy_sframe(self):
        g = SFrame({"a": [1, 2, 3, 4, 5], "id": [1, 2, 3, 4, 5]})
        g = g[g["id"] > 2]
        del g["id"]
        # if lazy column deletion is quirky, this will cause an exception
        self.assertEqual(list(g[0:2]["a"]), [3, 4])
        g = SFrame({"a": [1, 2, 3, 4, 5], "id": [1, 2, 3, 4, 5]})
        g = g[g["id"] > 2]
        g.swap_columns("a", "id", inplace=True)
        # if lazy column swap is quirky, this will cause an exception
        self.assertEqual(list(g[0:2]["a"]), [3, 4])

    def test_empty_sarray(self):
        with util.TempDirectory() as f:
            sf = SArray()
            sf.save(f)
            sf2 = SArray(f)
            self.assertEqual(len(sf2), 0)

    def test_empty_sframe(self):
        with util.TempDirectory() as f:
            sf = SFrame()
            sf.save(f)
            sf2 = SFrame(f)
            self.assertEqual(len(sf2), 0)
            self.assertEqual(sf2.num_columns(), 0)

    def test_none_column(self):
        sf = SFrame({"a": [1, 2, 3, 4, 5]})
        sf["b"] = None
        self.assertEqual(sf["b"].dtype, float)
        df = pd.DataFrame({"a": [1, 2, 3, 4, 5], "b": [None, None, None, None, None]})
        self.__test_equal(sf, df)

        sa = SArray.from_const(None, 100)
        self.assertEqual(list(sa), [None] * 100)
        self.assertEqual(sa.dtype, float)

    def test_apply_with_partial(self):
        sf = SFrame({"a": [1, 2, 3, 4, 5]})

        def concat_fn(character, row):
            return "%s%d" % (character, row["a"])

        my_partial_fn = functools.partial(concat_fn, "x")
        sa = sf.apply(my_partial_fn)
        self.assertEqual(list(sa), ["x1", "x2", "x3", "x4", "x5"])

    def test_apply_with_functor(self):
        sf = SFrame({"a": [1, 2, 3, 4, 5]})

        class Concatenator(object):
            def __init__(self, character):
                self.character = character

            def __call__(self, row):
                return "%s%d" % (self.character, row["a"])

        concatenator = Concatenator("x")
        sa = sf.apply(concatenator)
        self.assertEqual(list(sa), ["x1", "x2", "x3", "x4", "x5"])

    def test_save_sframe(self):
        """save lazily evaluated SFrame should not materialize to target folder
        """
        data = SFrame()
        data["x"] = range(100)
        data["x"] = data["x"] > 50
        # lazy and good
        tmp_dir = tempfile.mkdtemp()
        data.save(tmp_dir)
        shutil.rmtree(tmp_dir)
        print(data)

    def test_empty_argmax_does_not_fail(self):
        # an empty argmax should not result in a crash
        sf = SFrame(
            {
                "id": [0, 0, 0, 1, 1, 2, 2],
                "value": [3.0, 2.0, 2.3, None, None, 4.3, 1.3],
                "category": ["A", "B", "A", "E", "A", "A", "B"],
            }
        )
        sf.groupby("id", aggregate.ARGMAX("value", "category"))

    def test_cache_invalidation(self):
        # Changes to the SFrame should invalidate the indexing cache.

        X = SFrame({"a": range(4000), "b": range(4000)})

        for i in range(0, 4000, 20):
            self.assertEqual(X[i], {"a": i, "b": i})

        X["a"] = range(1000, 5000)

        for i in range(0, 4000, 20):
            self.assertEqual(X[i], {"a": 1000 + i, "b": i})

        del X["b"]

        for i in range(0, 4000, 20):
            self.assertEqual(X[i], {"a": 1000 + i})

        X["b"] = X["a"]

        for i in range(0, 4000, 20):
            self.assertEqual(X[i], {"a": 1000 + i, "b": 1000 + i})

        X.rename({"b": "c"}, inplace=True)

        for i in range(0, 4000, 20):
            self.assertEqual(X[i], {"a": 1000 + i, "c": 1000 + i})

    def test_to_numpy(self):
        X = SFrame({"a": range(100), "b": range(100)})
        import numpy as np
        import numpy.testing as nptest

        Y = np.transpose(np.array([range(100), range(100)]))
        nptest.assert_array_equal(X.to_numpy(), Y)

        X["b"] = X["b"].astype(str)
        s = [str(i) for i in range(100)]
        Y = np.transpose(np.array([s, s]))
        nptest.assert_array_equal(X.to_numpy(), Y)

    @mock.patch(__name__ + ".sqlite3.Cursor", spec=True)
    @mock.patch(__name__ + ".sqlite3.Connection", spec=True)
    def test_from_sql(self, mock_conn, mock_cursor):
        # Set up mock connection and cursor
        conn = mock_conn("example.db")
        curs = mock_cursor()
        conn.cursor.return_value = curs
        sf_type_codes = [44, 44, 41, 22, 114, 199, 43]

        sf_data = list(zip(*self.all_type_cols))
        sf_iter = sf_data.__iter__()

        def mock_fetchone():
            try:
                return next(sf_iter)
            except StopIteration:
                return None

        def mock_fetchmany(size=1):
            count = 0
            ret_list = []
            for i in sf_iter:
                if count == curs.arraysize:
                    break
                ret_list.append(i)
                count += 1

            return ret_list

        curs.fetchone.side_effect = mock_fetchone
        curs.fetchmany.side_effect = mock_fetchmany

        curs.description = [
            ["X" + str(i + 1), sf_type_codes[i]] + [None for j in range(5)]
            for i in range(len(sf_data[0]))
        ]

        # bigger than cache, no Nones
        sf = SFrame.from_sql(
            conn,
            "SELECT * FROM test_table",
            type_inference_rows=5,
            dbapi_module=dbapi2_mock(),
        )
        _assert_sframe_equal(sf, self.sf_all_types)

        # smaller than cache, no Nones
        sf_iter = sf_data.__iter__()
        sf = SFrame.from_sql(
            conn,
            "SELECT * FROM test_table",
            type_inference_rows=100,
            dbapi_module=dbapi2_mock(),
        )
        _assert_sframe_equal(sf, self.sf_all_types)

        none_col = [None for i in range(5)]
        nones_in_cache = list(zip(*[none_col for i in range(len(sf_data[0]))]))
        none_sf = SFrame(
            {"X" + str(i): none_col for i in range(1, len(sf_data[0]) + 1)}
        )
        test_data = nones_in_cache + sf_data
        sf_iter = test_data.__iter__()

        # more None rows than cache & types in description
        sf = SFrame.from_sql(
            conn,
            "SELECT * FROM test_table",
            type_inference_rows=5,
            dbapi_module=dbapi2_mock(),
        )
        sf_inferred_types = SFrame()
        expected_types = [float, float, str, str, str, str, dt.datetime]
        for i in zip(self.sf_all_types.column_names(), expected_types):
            new_col = SArray(none_col).astype(i[1])
            new_col = new_col.append(
                self.sf_all_types[i[0]].apply(
                    lambda x: i[1](x) if i[1] is not dt.datetime else x
                )
            )
            sf_inferred_types.add_column(new_col, inplace=True)

        # Don't test the string representation of dict and list; there are
        # funky consistency issues with the string representations of these
        sf.remove_columns(["X5", "X6"], inplace=True)
        sf_inferred_types.remove_columns(["X5", "X6"], inplace=True)
        _assert_sframe_equal(sf, sf_inferred_types)

        # more None rows than cache & no type information
        for i in range(len(curs.description)):
            curs.description[i][1] = None
        sf_iter = test_data.__iter__()
        sf = SFrame.from_sql(
            conn,
            "SELECT * FROM test_table",
            type_inference_rows=5,
            dbapi_module=dbapi2_mock(),
        )

        sf_inferred_types = SFrame()
        expected_types = [str for i in range(len(sf_data[0]))]
        for i in zip(self.sf_all_types.column_names(), expected_types):
            new_col = SArray(none_col).astype(i[1])
            new_col = new_col.append(self.sf_all_types[i[0]].apply(lambda x: str(x)))
            sf_inferred_types.add_column(new_col, inplace=True)

        # Don't test the string representation of dict, could be out of order
        sf.remove_columns(["X5", "X6"], inplace=True)
        sf_inferred_types.remove_columns(["X5", "X6"], inplace=True)
        _assert_sframe_equal(sf, sf_inferred_types)

        ### column_type_hints tests
        sf_iter = test_data.__iter__()
        sf = SFrame.from_sql(
            conn,
            "SELECT * FROM test_table",
            type_inference_rows=5,
            dbapi_module=dbapi2_mock(),
            column_type_hints=str,
        )
        sf.remove_columns(["X5", "X6"], inplace=True)
        _assert_sframe_equal(sf, sf_inferred_types)

        # Provide unhintable types
        sf_iter = test_data.__iter__()
        expected_types = [int, float, str, array.array, list, dict, dt.datetime]
        with self.assertRaises(TypeError):
            sf = SFrame.from_sql(
                conn,
                "SELECT * FROM test_table",
                type_inference_rows=5,
                dbapi_module=dbapi2_mock(),
                column_type_hints=expected_types,
            )

        sf_iter = test_data.__iter__()
        expected_types = {"X" + str(i + 1): expected_types[i] for i in range(3)}
        sf = SFrame.from_sql(
            conn,
            "SELECT * FROM test_table",
            type_inference_rows=10,
            dbapi_module=dbapi2_mock(),
            column_type_hints=expected_types,
        )
        _assert_sframe_equal(sf[5:], self.sf_all_types)

        # Test a float forced to a str
        sf_iter = test_data.__iter__()
        expected_types["X2"] = str
        self.sf_all_types["X2"] = self.sf_all_types["X2"].apply(lambda x: str(x))
        sf = SFrame.from_sql(
            conn,
            "SELECT * FROM test_table",
            type_inference_rows=10,
            dbapi_module=dbapi2_mock(),
            column_type_hints=expected_types,
        )
        _assert_sframe_equal(sf[5:], self.sf_all_types)

        # Type unsupported by sframe
        curs.description = [["X1", 44], ["X2", 44]]
        sf_iter = [[complex(4.5, 3), 1], [complex(3.4, 5), 2]].__iter__()
        sf = SFrame.from_sql(conn, "SELECT * FROM test_table")
        expected_sf = SFrame({"X1": ["(4.5+3j)", "(3.4+5j)"], "X2": [1, 2]})
        _assert_sframe_equal(sf, expected_sf)

        # bad DBAPI version!
        bad_version = dbapi2_mock()
        bad_version.apilevel = "1.0 "
        with self.assertRaises(NotImplementedError):
            sf = SFrame.from_sql(
                conn, "SELECT * FROM test_table", dbapi_module=bad_version
            )

        # Bad module
        with self.assertRaises(AttributeError):
            sf = SFrame.from_sql(conn, "SELECT * FROM test_table", dbapi_module=os)

        # Bad connection
        with self.assertRaises(AttributeError):
            sf = SFrame.from_sql(4, "SELECT * FROM test_table")

        # Empty query result
        curs.description = []
        sf = SFrame.from_sql(
            conn, "SELECT * FROM test_table", dbapi_module=dbapi2_mock()
        )
        _assert_sframe_equal(sf, SFrame())

    @mock.patch(__name__ + ".sqlite3.Cursor", spec=True)
    @mock.patch(__name__ + ".sqlite3.Connection", spec=True)
    def test_to_sql(self, mock_conn, mock_cursor):
        conn = mock_conn("example.db")
        curs = mock_cursor()
        insert_stmt = "INSERT INTO ins_test (X1,X2,X3,X4,X5,X6,X7) VALUES ({0},{1},{2},{3},{4},{5},{6})"
        num_cols = len(self.sf_all_types.column_names())
        test_cases = [
            ("qmark", insert_stmt.format(*["?" for i in range(num_cols)])),
            (
                "numeric",
                insert_stmt.format(*[":" + str(i) for i in range(1, num_cols + 1)]),
            ),
            (
                "named",
                insert_stmt.format(*[":X" + str(i) for i in range(1, num_cols + 1)]),
            ),
            ("format", insert_stmt.format(*["%s" for i in range(num_cols)])),
            (
                "pyformat",
                insert_stmt.format(
                    *["%(X" + str(i) + ")s" for i in range(1, num_cols + 1)]
                ),
            ),
        ]
        for i in test_cases:
            conn.cursor.return_value = curs

            mock_mod = dbapi2_mock()
            mock_mod.paramstyle = i[0]
            self.sf_all_types.to_sql(conn, "ins_test", dbapi_module=mock_mod)
            conn.cursor.assert_called_once_with()
            calls = []
            col_names = self.sf_all_types.column_names()
            for j in self.sf_all_types:
                if i[0] == "named" or i[0] == "pyformat":
                    calls.append(mock.call(i[1], j))
                else:
                    calls.append(mock.call(i[1], [j[k] for k in col_names]))
            curs.execute.assert_has_calls(calls, any_order=False)
            self.assertEqual(curs.execute.call_count, len(self.sf_all_types))
            conn.commit.assert_called_once_with()
            curs.close.assert_called_once_with()

            conn.reset_mock()
            curs.reset_mock()

        # bad DBAPI version!
        bad_version = dbapi2_mock()
        bad_version.apilevel = "1.0 "
        with self.assertRaises(NotImplementedError):
            self.sf_all_types.to_sql(conn, "ins_test", dbapi_module=bad_version)

        # bad paramstyle
        bad_paramstyle = dbapi2_mock()
        bad_paramstyle.paramstyle = "foo"
        with self.assertRaises(TypeError):
            self.sf_all_types.to_sql(conn, "ins_test", dbapi_module=bad_paramstyle)

    def test_materialize(self):
        sf = SFrame({"a": range(100)})
        sf = sf[sf["a"] > 10]
        self.assertFalse(sf.is_materialized())
        sf.materialize()
        self.assertTrue(sf.is_materialized())

    def test_materialization_slicing(self):
        # Has been known to fail.
        g = SFrame({"a": range(100)})[:10]
        g["b"] = g["a"] + 1
        g["b"].materialize()
        g.materialize()

    def test_copy(self):
        from copy import copy

        sf = generate_random_sframe(100, "Cns")
        sf_copy = copy(sf)

        assert sf is not sf_copy

        _assert_sframe_equal(sf, sf_copy)

    def test_deepcopy(self):
        from copy import deepcopy

        sf = generate_random_sframe(100, "Cns")
        sf_copy = deepcopy(sf)

        assert sf is not sf_copy

        _assert_sframe_equal(sf, sf_copy)

    def test_builtins(self):
        import builtins
        import six

        sf = SFrame(
            {
                "dict": [builtins.dict({"foo": "bar"})],
                "float": [builtins.float(3.14)],
                "int": [builtins.int(12)],
                "bool": [builtins.bool(False)],
                "list": [builtins.list([1, 2, 3])],
                "str": [builtins.str("foo")],
                "tuple": [builtins.tuple((1, 2))],
            }
        )
        sf2 = SFrame(
            {
                "dict": [{"foo": "bar"}],
                "float": [3.14],
                "int": [12],
                "bool": [False],
                "list": [[1, 2, 3]],
                "str": ["foo"],
                "tuple": [(1, 2)],
            }
        )

        if six.PY2:
            sf = sf.add_columns(
                SFrame(
                    {"long": [builtins.long(12)], "unicode": [builtins.unicode("foo")]}
                )
            )
            sf2 = sf2.add_columns(SFrame({"long": [12], "unicode": [unicode("foo")]}))

        _assert_sframe_equal(sf, sf2)

    def test_add_column_nonSArray(self):
        sf = SFrame()
        sf = sf.add_column([1, 2, 3, 4], "x")

        sf_test = SFrame()
        sf_test["x"] = SArray([1, 2, 3, 4])

        _assert_sframe_equal(sf, sf_test)

    def test_add_column_noniterable1(self):
        sf = SFrame()
        sf = sf.add_column([1, 2, 3, 4], "x")
        sf = sf.add_column(5, "y")

        sf_test = SFrame()
        sf_test["x"] = SArray([1, 2, 3, 4])
        sf_test["y"] = 5

        _assert_sframe_equal(sf, sf_test)

    def test_add_column_noniterable2(self):
        # If SFrame is empty then the passed data should be treated as an SArray of size 1
        sf = SFrame()
        sf = sf.add_column(5, "y")

        sf_test = SFrame()
        sf_test["y"] = SArray([5])

        _assert_sframe_equal(sf, sf_test)

    def test_filter_by_dict(self):
        # Check for dict in filter_by
        sf = SFrame({"check": range(10)})
        d = {1: 1}

        sf = sf.filter_by(d.keys(), "check")
        sf_test = sf.filter_by(list(d.keys()), "check")
        _assert_sframe_equal(sf, sf_test)

        sf = sf.filter_by(d.values(), "check")
        sf_test = sf.filter_by(list(d.values()), "check")
        _assert_sframe_equal(sf, sf_test)

    def test_export_empty_SFrame(self):
        f = tempfile.NamedTemporaryFile(suffix=".json", delete=False)
        sf = SFrame()
        sf.export_json(f.name)
        sf2 = SFrame.read_json(f.name)
        _assert_sframe_equal(sf, sf2)


if __name__ == "__main__":

    import sys

    # Check if we are supposed to connect to another server
    for i, v in enumerate(sys.argv):
        if v.startswith("ipc://"):
            _launch(v)

            # The rest of the arguments need to get passed through to
            # the unittest module
            del sys.argv[i]
            break

    unittest.main()
