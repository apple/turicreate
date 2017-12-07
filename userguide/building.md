## Dependencies

To build the userguide, you require [node.js](https://nodejs.org/en/).
version v8.9.1.

```
npm install -g
npm install -g gitbook-cli
```

Install this globally and you'll have access to the gitbook command
anywhere on your system. First, you need to make sure all the plugins
are installed

```
gitbook install .
```

## Building and Serving

You can build the static html as follows. The generated html will be
located at `_book/index.html`.

```
gitbook build .
```

You can also live serve the book using:

```
gitbook serve .
```



