# Introduction to SFrames

SFrames are the primary data structure for extracting data from other
sources for use in Turi Create.

SFrames can extract data from the following static file formats:

* [CSV](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.read_csv.html#turicreate.SFrame.read_csv)
* [JSON](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.read_json.html?highlight=read_json#turicreate.SFrame.read_json)
* [SQL databases](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.from_sql.html?highlight=sql#turicreate.SFrame.from_sql)

A very common data format is the comma separated value (csv) file, which
is what we'll use for these examples.  We will use some preprocessed data from
the
[Million Song Dataset](https://labrosa.ee.columbia.edu/millionsong/) to
aid our SFrame-related examples.  The first table contains metadata
about each song in the database.  Here's how we load it into an SFrame:

```python
import turicreate as tc
songs = tc.SFrame.read_csv("millionsong/song_data.csv")
```

No options are needed for the simplest case, as the SFrame parser infers
column types. Of course, there are many options you may need to specify
when importing a csv file.  Some of the more common options come in to
play when we load the usage data of users listening to these songs
online:

```python
usage_data = tc.SFrame.read_csv("millionsong/10000.txt",
                                header=False,
                                delimiter='\t',
                                column_type_hints={'X3':int})
```

The `header` and `delimiter` options are needed because this particular
csv file does not provide column names in its first line, and the values
are separated by tabs, not commas.  The `column_type_hints` keeps the
SFrame csv parser from attempting to infer the datatype of each column,
which it does by default.  For a full list of options when parsing csv
files, check our [API
Reference](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.read_csv.html#turicreate.SFrame.read_csv).

Once done we can inspect the first few rows of the tables we've
imported:

```python
songs
```

```
Columns:
	song_id	str
	title	str
	release	str
	artist_name	str
	year	int

Rows: 1000000

+--------------------+--------------------------------+
|      song_id       |             title              |
+--------------------+--------------------------------+
| SOQMMHC12AB0180CB8 |          Silent Night          |
| SOVFVAK12A8C1350D9 |          Tanssi vaan           |
| SOGTUKN12AB017F4F1 |       No One Could Ever        |
| SOBNYVR12A8C13558C |      Si Vos Quer\xc3\xa9s      |
| SOHSBXH12A8C13B0DF |        Tantce Of Aspens        |
| SOZVAPQ12A8C13B63C | Symphony No. 1 G minor "Si ... |
| SOQVRHI12A6D4FB2D7 |        We Have Got Love        |
| SOEYRFT12AB018936C |       2 Da Beat Ch'yall        |
| SOPMIYT12A6D4F851E |            Goodbye             |
| SOJCFMH12A8C13B0C2 |   Mama_ mama can't you see ?   |
+--------------------+--------------------------------+
+--------------------------------+--------------------------------+------+
|            release             |          artist_name           | year |
+--------------------------------+--------------------------------+------+
|     Monster Ballads X-Mas      |        Faster Pussy cat        | 2003 |
|       Karkuteill\xc3\xa4       |        Karkkiautomaatti        | 1995 |
|             Butter             |         Hudson Mohawke         | 2006 |
|            De Culo             |          Yerba Brava           | 2003 |
| Rene Ablaze Presents Winte ... |           Der Mystic           |  0   |
| Berwald: Symphonies Nos. 1 ... |        David Montgomery        |  0   |
|   Strictly The Best Vol. 34    |       Sasha / Turbulence       |  0   |
|            Da Bomb             |           Kris Kross           | 1993 |
|           Danny Boy            |          Joseph Locke          |  0   |
| March to cadence with the  ... | The Sun Harbor's Chorus-Do ... |  0   |
|              ...               |              ...               | ...  |
+--------------------------------+--------------------------------+------+
[1000000 rows x 5 columns]
Note: Only the head of the SFrame is printed.
You can use print_rows(num_rows=m, num_columns=n) to print more rows and columns.
```


```python
usage_data
```

```
Columns:
	X1	str
	X2	str
	X3	int

Rows: 2000000

+--------------------------------+--------------------+-----+
|               X1               |         X2         |  X3 |
+--------------------------------+--------------------+-----+
| b80344d063b5ccb3212f76538f ... | SOAKIMP12A8C130995 |  1  |
| b80344d063b5ccb3212f76538f ... | SOBBMDR12A8C13253B |  2  |
| b80344d063b5ccb3212f76538f ... | SOBXHDL12A81C204C0 |  1  |
| b80344d063b5ccb3212f76538f ... | SOBYHAJ12A6701BF1D |  1  |
| b80344d063b5ccb3212f76538f ... | SODACBL12A8C13C273 |  1  |
| b80344d063b5ccb3212f76538f ... | SODDNQT12A6D4F5F7E |  5  |
| b80344d063b5ccb3212f76538f ... | SODXRTY12AB0180F3B |  1  |
| b80344d063b5ccb3212f76538f ... | SOFGUAY12AB017B0A8 |  1  |
| b80344d063b5ccb3212f76538f ... | SOFRQTD12A81C233C0 |  1  |
| b80344d063b5ccb3212f76538f ... | SOHQWYZ12A6D4FA701 |  1  |
|              ...               |        ...         | ... |
+--------------------------------+--------------------+-----+
[2000000 rows x 3 columns]
Note: Only the head of the SFrame is printed.
You can use print_rows(num_rows=m, num_columns=n) to print more rows and columns.
```

Here we might want to rename columns from the default names:

```python
usage_data.rename({'X1':'user_id', 'X2':'song_id', 'X3':'listen_count'})
```
SFrames can be saved as a csv file or in the SFrame binary format.  If
your SFrame is saved in binary format loading it is instantaneous, so we
won't ever have to parse that file again.  Here, the default is to save
in binary format, and we supply the name of a directory to be created
which will hold the binary files:

```python
usage_data.save('./music_usage_data.sframe')
```

Loading is then very fast:

```python
same_usage_data = tc.load_sframe('./music_usage_data.sframe')
```

In addition to these functions, JSON imports and exports and SQL/ODBC
imports are also supported. For further information see the respective pages in the Turi
Create API Documentation:
* [read_json](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.read_json.html)
* [export_json](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.export_json.html)
* [read_csv](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.read_csv.html)
* [export_csv](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.export_csv.html)
* [from_sql](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.from_sql.html)


#### Data Types

An SFrame is made up of columns of a contiguous type. For instance the `songs`
SFrame is made up of 5 columns of the following types

```
	song_id	str
	title	str
	release	str
	artist_name	str
	year	int
```

In this SFrame we see only string (`str`) and integer (`int`) columns, but a
number of datatypes are supported:

* `int` (signed 64-bit integer)
* `float` (double-precision floating point)
* `str` (string)
* `array.array` (1-D array of doubles)
* `list` (arbitrarily list of elements)
* `dict` (arbitrary dictionary of elements)
* `datetime.datetime` (datetime with microsecond precision)
* `image` (image)
