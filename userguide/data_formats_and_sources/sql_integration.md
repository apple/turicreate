# Introduction

Turi Create connects with a database through [Python
DBAPI2](https://www.python.org/dev/peps/pep-0249/), which is a standard
written to encourage database providers to expose a common interface for
executing SQL queries when making Python modules for their database.
Common usage of a DBAPI2-compliant module from Python looks something
like this:

```python
import sqlite3
conn = sqlite3.connect('example.db')
c = conn.cursor()

# Create table
c.execute('''CREATE TABLE stocks
             (date text, trans text, symbol text, qty real, price real)''')

# Insert a row of data
c.execute("INSERT INTO stocks VALUES ('2006-01-05','BUY','RHAT',100,35.14)")

# Save (commit) the changes
conn.commit()

c.execute("SELECT * FROM stocks")
results = c.fetchall()
```
(example adapted from [here](https://docs.python.org/2/library/sqlite3.html))

SFrame offers a DBAPI2 integration that enables you to read and write
SQL data in a similar, concise fashion. Using the connection object in
the previous example, here is how you would read the data as an SFrame
using the
[`from_sql`](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.from_sql.html)
method:

```python
import turicreate as tc
stocks_sf = tc.SFrame.from_sql(conn, "SELECT * FROM stocks")
```

If you would like to then write this table to the database, that's easy
too, using the
[`to_sql`](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.to_sql.html)
method. `to_sql` simply attempts to append to an already existing table,
so if you intend to write the data to a new table in your database, then
you must use the "CREATE TABLE" syntax, including the type syntax
supported by your database. Here's an example of creating a new table
and then appending more data to the table.

```python
import datetime as dt
c = conn.cursor()

c.execute('''CREATE TABLE more_stocks
             (date text, trans text, symbol text, qty real, price real)''')
c.commit()
stocks_sf.to_sql(conn, "more_stocks")

# Append another row
another_row = tc.SFrame({'date':[dt.datetime(2006, 3, 28)],
                         'trans':['BUY'],
                         'symbol':['IBM'],
                         'qty':[1000],
                         'price':[45.00]})
another_row.to_sql(conn, "more_stocks")
```

That is all there is to know to get started using SFrames with Python DBAPI2
modules! For more details you can consult the API documentation of
[`from_sql`](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.from_sql.html)
and
[`to_sql`](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.to_sql.html).
Currently, we have tested our DBAPI2 support with these modules:
 - [MySQLdb](https://github.com/PyMySQL/mysqlclient-python)
 - [psycopg2](http://initd.org/psycopg/)
 - [sqlite3](https://docs.python.org/2/library/sqlite3.html)

This means that our DBAPI2 support may or may not work on other modules
claiming to be DBAPI2-compliant. If you run into errors, let us know by
filing a github issue.
