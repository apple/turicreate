(function (global, factory) {
    typeof exports === 'object' && typeof module !== 'undefined' ? factory(exports) :
    typeof define === 'function' && define.amd ? define(['exports'], factory) :
    (global = global || self, factory(global.vl = {}));
}(this, function (exports) { 'use strict';

    var name = "vega-lite";
    var author = "Dominik Moritz, Kanit \"Ham\" Wongsuphasawat, Arvind Satyanarayan, Jeffrey Heer";
    var version = "3.3.0";
    var collaborators = [
    	"Kanit Wongsuphasawat <kanitw@gmail.com> (http://kanitw.yellowpigz.com)",
    	"Dominik Moritz <domoritz@cs.washington.edu> (https://www.domoritz.de)",
    	"Arvind Satyanarayan (https://arvindsatya.com/)",
    	"Jeffrey Heer (https://jheer.org)"
    ];
    var homepage = "https://vega.github.io/vega-lite/";
    var description = "Vega-Lite is a concise high-level language for interactive visualization.";
    var main = "build/vega-lite.js";
    var unpkg = "build/vega-lite.min.js";
    var jsdelivr = "build/vega-lite.min.js";
    var module = "build/src/index";
    var types = "build/src/index.d.ts";
    var bin = {
    	vl2png: "./bin/vl2png",
    	vl2svg: "./bin/vl2svg",
    	vl2vg: "./bin/vl2vg"
    };
    var directories = {
    	test: "test"
    };
    var scripts = {
    	build: "npm run build:only",
    	"build:only": "npm run tsc:src && rollup -c",
    	postbuild: "terser build/vega-lite.js -cm --source-map content=build/vega-lite.js.map,filename=build/vega-lite.min.js.map -o build/vega-lite.min.js && npm run schema",
    	"build:examples": "npm run build:only",
    	"postbuild:examples": "npm run data && TZ=America/Los_Angeles scripts/build-examples.sh",
    	"build:examples-full": "npm run build:only",
    	"postbuild:examples-full": "TZ=America/Los_Angeles scripts/build-examples.sh 1",
    	"build:example": "TZ=America/Los_Angeles scripts/build-example.sh",
    	"build:toc": "npm run data && bundle exec jekyll build -q && scripts/generate-toc",
    	"build:site": "npm run tsc:site && rollup -c site/rollup.config.js",
    	"build:versions": "scripts/update-version.sh",
    	clean: "rm -rf build && rm -f examples/compiled/*.png && find site/examples ! -name 'index.md' -type f -delete",
    	data: "rsync -r node_modules/vega-datasets/data/* data",
    	deploy: "scripts/deploy.sh",
    	"deploy:gh": "scripts/deploy-gh.sh",
    	"deploy:schema": "scripts/deploy-schema.sh",
    	schema: "mkdir -p build && node --stack-size=5000 ./node_modules/.bin/ts-json-schema-generator --no-type-check --path src/index.ts --type TopLevelSpec > build/vega-lite-schema.json && npm run renameschema && cp build/vega-lite-schema.json _data/",
    	renameschema: "scripts/rename-schema.sh",
    	presite: "npm run data && npm run build:site && npm run build:toc && npm run build:versions && scripts/create-example-pages",
    	site: "bundle exec jekyll serve --incremental",
    	"tsc:src": "tsc -b src/tsconfig.src.json",
    	"tsc:site": "tsc -b site/tsconfig.site.json",
    	prettierbase: "prettier '{src,test,test-runtime,site,typings}/**/*.{md,css}'",
    	eslintbase: "eslint '{src,test,test-runtime,site,typings}/**/*.{ts,js}'",
    	format: "npm run eslintbase -- --fix && npm run prettierbase -- --write",
    	lint: "npm run eslintbase && npm run prettierbase -- --check",
    	test: "jest test/ && npm run lint && npm run schema && jest examples/ && npm run test:runtime",
    	"test:inspect": "node --inspect-brk ./node_modules/.bin/jest --runInBand test",
    	"test:runtime": "TZ=America/Los_Angeles jest test-runtime/",
    	"test:runtime:generate": "rm -Rf test-runtime/resources && VL_GENERATE_TESTS=true npm run test:runtime",
    	"watch:build": "npm run build:only && concurrently --kill-others -n Typescript,Rollup 'npm run tsc:src -- -w' 'rollup -c -w'",
    	"watch:site": "concurrently --kill-others -n Typescript,Rollup 'npm run tsc:site -- -w' 'rollup -c site/rollup.config.js -w'",
    	"watch:test": "jest --watch"
    };
    var repository = {
    	type: "git",
    	url: "https://github.com/vega/vega-lite.git"
    };
    var license = "BSD-3-Clause";
    var bugs = {
    	url: "https://github.com/vega/vega-lite/issues"
    };
    var devDependencies = {
    	"@types/chai": "^4.1.7",
    	"@types/d3": "^5.7.2",
    	"@types/highlight.js": "^9.12.3",
    	"@types/jest": "^24.0.13",
    	"@types/jest-environment-puppeteer": "^4.0.0",
    	"@types/mkdirp": "^0.5.2",
    	"@types/node": "^12.0.2",
    	"@types/puppeteer": "^1.12.4",
    	"@typescript-eslint/eslint-plugin": "^1.9.0",
    	"@typescript-eslint/parser": "^1.9.0",
    	ajv: "^6.10.0",
    	chai: "^4.2.0",
    	cheerio: "^1.0.0-rc.2",
    	codecov: "^3.5.0",
    	concurrently: "^4.1.0",
    	d3: "^5.9.2",
    	eslint: "^5.16.0",
    	"eslint-config-prettier": "^4.3.0",
    	"eslint-plugin-prettier": "^3.1.0",
    	"highlight.js": "^9.15.6",
    	"http-server": "^0.11.1",
    	jest: "^24.8.0",
    	"jest-puppeteer": "^4.1.1",
    	mkdirp: "^0.5.1",
    	prettier: "^1.17.1",
    	puppeteer: "^1.16.0",
    	rollup: "^1.12.1",
    	"rollup-plugin-commonjs": "^10.0.0",
    	"rollup-plugin-json": "^4.0.0",
    	"rollup-plugin-node-resolve": "^5.0.0",
    	"rollup-plugin-sourcemaps": "^0.4.2",
    	"rollup-plugin-terser": "^5.0.0",
    	"svg2png-many": "^0.0.7",
    	terser: "^4.0.0",
    	"ts-jest": "^24.0.2",
    	"ts-json-schema-generator": "^0.42.0",
    	typescript: "^3.4.5",
    	"vega-cli": "^5.4.0",
    	"vega-datasets": "^1.25.0",
    	"vega-embed": "^4.2.0",
    	"vega-tooltip": "^0.17.0",
    	"yaml-front-matter": "^4.0.0"
    };
    var dependencies = {
    	"@types/clone": "~0.1.30",
    	"@types/fast-json-stable-stringify": "^2.0.0",
    	clone: "~2.1.2",
    	"fast-deep-equal": "~2.0.1",
    	"fast-json-stable-stringify": "~2.0.0",
    	"json-stringify-pretty-compact": "~2.0.0",
    	tslib: "~1.9.3",
    	"vega-event-selector": "~2.0.0",
    	"vega-expression": "~2.6.0",
    	"vega-typings": "0.7.1",
    	"vega-util": "~1.10.0",
    	yargs: "~13.2.4"
    };
    var peerDependencies = {
    	vega: "^5.4.0"
    };
    var jest = {
    	preset: "jest-puppeteer",
    	transform: {
    		"^.+\\.tsx?$": "ts-jest"
    	},
    	testRegex: "(/__tests__/.*|(\\.|/)(test|spec))\\.(jsx?|tsx?)$",
    	moduleFileExtensions: [
    		"ts",
    		"tsx",
    		"js",
    		"jsx",
    		"json",
    		"node"
    	],
    	testPathIgnorePatterns: [
    		"<rootDir>/node_modules",
    		"<rootDir>/build",
    		"<rootDir>/_site",
    		"<rootDir>/src"
    	],
    	coverageDirectory: "./coverage/",
    	collectCoverage: false
    };
    var pkg = {
    	name: name,
    	author: author,
    	version: version,
    	collaborators: collaborators,
    	homepage: homepage,
    	description: description,
    	main: main,
    	unpkg: unpkg,
    	jsdelivr: jsdelivr,
    	module: module,
    	types: types,
    	bin: bin,
    	directories: directories,
    	scripts: scripts,
    	repository: repository,
    	license: license,
    	bugs: bugs,
    	devDependencies: devDependencies,
    	dependencies: dependencies,
    	peerDependencies: peerDependencies,
    	jest: jest
    };

    function accessor(fn, fields, name) {
      fn.fields = fields || [];
      fn.fname = name;
      return fn;
    }

    function error(message) {
      throw Error(message);
    }

    function splitAccessPath(p) {
      var path = [],
          q = null,
          b = 0,
          n = p.length,
          s = '',
          i, j, c;

      p = p + '';

      function push() {
        path.push(s + p.substring(i, j));
        s = '';
        i = j + 1;
      }

      for (i=j=0; j<n; ++j) {
        c = p[j];
        if (c === '\\') {
          s += p.substring(i, j);
          i = ++j;
        } else if (c === q) {
          push();
          q = null;
          b = -1;
        } else if (q) {
          continue;
        } else if (i === b && c === '"') {
          i = j + 1;
          q = c;
        } else if (i === b && c === "'") {
          i = j + 1;
          q = c;
        } else if (c === '.' && !b) {
          if (j > i) {
            push();
          } else {
            i = j + 1;
          }
        } else if (c === '[') {
          if (j > i) push();
          b = i = j + 1;
        } else if (c === ']') {
          if (!b) error('Access path missing open bracket: ' + p);
          if (b > 0) push();
          b = 0;
          i = j + 1;
        }
      }

      if (b) error('Access path missing closing bracket: ' + p);
      if (q) error('Access path missing closing quote: ' + p);

      if (j > i) {
        j++;
        push();
      }

      return path;
    }

    var isArray = Array.isArray;

    function isObject(_) {
      return _ === Object(_);
    }

    function isString(_) {
      return typeof _ === 'string';
    }

    function $(x) {
      return isArray(x) ? '[' + x.map($) + ']'
        : isObject(x) || isString(x) ?
          // Output valid JSON and JS source strings.
          // See http://timelessrepo.com/json-isnt-a-javascript-subset
          JSON.stringify(x).replace('\u2028','\\u2028').replace('\u2029', '\\u2029')
        : x;
    }

    function field(field, name) {
      var path = splitAccessPath(field),
          code = 'return _[' + path.map($).join('][') + '];';

      return accessor(
        Function('_', code),
        [(field = path.length===1 ? path[0] : field)],
        name || field
      );
    }

    var empty = [];

    var id = field('id');

    var identity = accessor(function(_) { return _; }, empty, 'identity');

    var zero = accessor(function() { return 0; }, empty, 'zero');

    var one = accessor(function() { return 1; }, empty, 'one');

    var truthy = accessor(function() { return true; }, empty, 'true');

    var falsy = accessor(function() { return false; }, empty, 'false');

    function log(method, level, input) {
      var msg = [level].concat([].slice.call(input));
      console[method](...msg); // eslint-disable-line no-console
    }

    var None  = 0;
    var Error$1 = 1;
    var Warn  = 2;
    var Info  = 3;
    var Debug = 4;

    function logger(_, method) {
      var level = _ || None;
      return {
        level: function(_) {
          if (arguments.length) {
            level = +_;
            return this;
          } else {
            return level;
          }
        },
        error: function() {
          if (level >= Error$1) log(method || 'error', 'ERROR', arguments);
          return this;
        },
        warn: function() {
          if (level >= Warn) log(method || 'warn', 'WARN', arguments);
          return this;
        },
        info: function() {
          if (level >= Info) log(method || 'log', 'INFO', arguments);
          return this;
        },
        debug: function() {
          if (level >= Debug) log(method || 'log', 'DEBUG', arguments);
          return this;
        }
      }
    }

    function array(_) {
      return _ != null ? (isArray(_) ? _ : [_]) : [];
    }

    /**
     * Span-preserving range clamp. If the span of the input range is less
     * than (max - min) and an endpoint exceeds either the min or max value,
     * the range is translated such that the span is preserved and one
     * endpoint touches the boundary of the min/max range.
     * If the span exceeds (max - min), the range [min, max] is returned.
     */

    function isFunction(_) {
      return typeof _ === 'function';
    }

    /**
     * Return an array with minimum and maximum values, in the
     * form [min, max]. Ignores null, undefined, and NaN values.
     */

    /**
     * Predicate that returns true if the value lies within the span
     * of the given range. The left and right flags control the use
     * of inclusive (true) or exclusive (false) comparisons.
     */

    function isBoolean(_) {
      return typeof _ === 'boolean';
    }

    function isNumber(_) {
      return typeof _ === 'number';
    }

    function toSet(_) {
      for (var s={}, i=0, n=_.length; i<n; ++i) s[_[i]] = true;
      return s;
    }

    function createCommonjsModule(fn, module) {
    	return module = { exports: {} }, fn(module, module.exports), module.exports;
    }

    var clone_1 = createCommonjsModule(function (module) {
    var clone = (function() {

    function _instanceof(obj, type) {
      return type != null && obj instanceof type;
    }

    var nativeMap;
    try {
      nativeMap = Map;
    } catch(_) {
      // maybe a reference error because no `Map`. Give it a dummy value that no
      // value will ever be an instanceof.
      nativeMap = function() {};
    }

    var nativeSet;
    try {
      nativeSet = Set;
    } catch(_) {
      nativeSet = function() {};
    }

    var nativePromise;
    try {
      nativePromise = Promise;
    } catch(_) {
      nativePromise = function() {};
    }

    /**
     * Clones (copies) an Object using deep copying.
     *
     * This function supports circular references by default, but if you are certain
     * there are no circular references in your object, you can save some CPU time
     * by calling clone(obj, false).
     *
     * Caution: if `circular` is false and `parent` contains circular references,
     * your program may enter an infinite loop and crash.
     *
     * @param `parent` - the object to be cloned
     * @param `circular` - set to true if the object to be cloned may contain
     *    circular references. (optional - true by default)
     * @param `depth` - set to a number if the object is only to be cloned to
     *    a particular depth. (optional - defaults to Infinity)
     * @param `prototype` - sets the prototype to be used when cloning an object.
     *    (optional - defaults to parent prototype).
     * @param `includeNonEnumerable` - set to true if the non-enumerable properties
     *    should be cloned as well. Non-enumerable properties on the prototype
     *    chain will be ignored. (optional - false by default)
    */
    function clone(parent, circular, depth, prototype, includeNonEnumerable) {
      if (typeof circular === 'object') {
        depth = circular.depth;
        prototype = circular.prototype;
        includeNonEnumerable = circular.includeNonEnumerable;
        circular = circular.circular;
      }
      // maintain two arrays for circular references, where corresponding parents
      // and children have the same index
      var allParents = [];
      var allChildren = [];

      var useBuffer = typeof Buffer != 'undefined';

      if (typeof circular == 'undefined')
        circular = true;

      if (typeof depth == 'undefined')
        depth = Infinity;

      // recurse this function so we don't reset allParents and allChildren
      function _clone(parent, depth) {
        // cloning null always returns null
        if (parent === null)
          return null;

        if (depth === 0)
          return parent;

        var child;
        var proto;
        if (typeof parent != 'object') {
          return parent;
        }

        if (_instanceof(parent, nativeMap)) {
          child = new nativeMap();
        } else if (_instanceof(parent, nativeSet)) {
          child = new nativeSet();
        } else if (_instanceof(parent, nativePromise)) {
          child = new nativePromise(function (resolve, reject) {
            parent.then(function(value) {
              resolve(_clone(value, depth - 1));
            }, function(err) {
              reject(_clone(err, depth - 1));
            });
          });
        } else if (clone.__isArray(parent)) {
          child = [];
        } else if (clone.__isRegExp(parent)) {
          child = new RegExp(parent.source, __getRegExpFlags(parent));
          if (parent.lastIndex) child.lastIndex = parent.lastIndex;
        } else if (clone.__isDate(parent)) {
          child = new Date(parent.getTime());
        } else if (useBuffer && Buffer.isBuffer(parent)) {
          if (Buffer.allocUnsafe) {
            // Node.js >= 4.5.0
            child = Buffer.allocUnsafe(parent.length);
          } else {
            // Older Node.js versions
            child = new Buffer(parent.length);
          }
          parent.copy(child);
          return child;
        } else if (_instanceof(parent, Error)) {
          child = Object.create(parent);
        } else {
          if (typeof prototype == 'undefined') {
            proto = Object.getPrototypeOf(parent);
            child = Object.create(proto);
          }
          else {
            child = Object.create(prototype);
            proto = prototype;
          }
        }

        if (circular) {
          var index = allParents.indexOf(parent);

          if (index != -1) {
            return allChildren[index];
          }
          allParents.push(parent);
          allChildren.push(child);
        }

        if (_instanceof(parent, nativeMap)) {
          parent.forEach(function(value, key) {
            var keyChild = _clone(key, depth - 1);
            var valueChild = _clone(value, depth - 1);
            child.set(keyChild, valueChild);
          });
        }
        if (_instanceof(parent, nativeSet)) {
          parent.forEach(function(value) {
            var entryChild = _clone(value, depth - 1);
            child.add(entryChild);
          });
        }

        for (var i in parent) {
          var attrs;
          if (proto) {
            attrs = Object.getOwnPropertyDescriptor(proto, i);
          }

          if (attrs && attrs.set == null) {
            continue;
          }
          child[i] = _clone(parent[i], depth - 1);
        }

        if (Object.getOwnPropertySymbols) {
          var symbols = Object.getOwnPropertySymbols(parent);
          for (var i = 0; i < symbols.length; i++) {
            // Don't need to worry about cloning a symbol because it is a primitive,
            // like a number or string.
            var symbol = symbols[i];
            var descriptor = Object.getOwnPropertyDescriptor(parent, symbol);
            if (descriptor && !descriptor.enumerable && !includeNonEnumerable) {
              continue;
            }
            child[symbol] = _clone(parent[symbol], depth - 1);
            if (!descriptor.enumerable) {
              Object.defineProperty(child, symbol, {
                enumerable: false
              });
            }
          }
        }

        if (includeNonEnumerable) {
          var allPropertyNames = Object.getOwnPropertyNames(parent);
          for (var i = 0; i < allPropertyNames.length; i++) {
            var propertyName = allPropertyNames[i];
            var descriptor = Object.getOwnPropertyDescriptor(parent, propertyName);
            if (descriptor && descriptor.enumerable) {
              continue;
            }
            child[propertyName] = _clone(parent[propertyName], depth - 1);
            Object.defineProperty(child, propertyName, {
              enumerable: false
            });
          }
        }

        return child;
      }

      return _clone(parent, depth);
    }

    /**
     * Simple flat clone using prototype, accepts only objects, usefull for property
     * override on FLAT configuration object (no nested props).
     *
     * USE WITH CAUTION! This may not behave as you wish if you do not know how this
     * works.
     */
    clone.clonePrototype = function clonePrototype(parent) {
      if (parent === null)
        return null;

      var c = function () {};
      c.prototype = parent;
      return new c();
    };

    // private utility functions

    function __objToStr(o) {
      return Object.prototype.toString.call(o);
    }
    clone.__objToStr = __objToStr;

    function __isDate(o) {
      return typeof o === 'object' && __objToStr(o) === '[object Date]';
    }
    clone.__isDate = __isDate;

    function __isArray(o) {
      return typeof o === 'object' && __objToStr(o) === '[object Array]';
    }
    clone.__isArray = __isArray;

    function __isRegExp(o) {
      return typeof o === 'object' && __objToStr(o) === '[object RegExp]';
    }
    clone.__isRegExp = __isRegExp;

    function __getRegExpFlags(re) {
      var flags = '';
      if (re.global) flags += 'g';
      if (re.ignoreCase) flags += 'i';
      if (re.multiline) flags += 'm';
      return flags;
    }
    clone.__getRegExpFlags = __getRegExpFlags;

    return clone;
    })();

    if (module.exports) {
      module.exports = clone;
    }
    });

    var isArray$1 = Array.isArray;
    var keyList = Object.keys;
    var hasProp = Object.prototype.hasOwnProperty;

    var fastDeepEqual = function equal(a, b) {
      if (a === b) return true;

      if (a && b && typeof a == 'object' && typeof b == 'object') {
        var arrA = isArray$1(a)
          , arrB = isArray$1(b)
          , i
          , length
          , key;

        if (arrA && arrB) {
          length = a.length;
          if (length != b.length) return false;
          for (i = length; i-- !== 0;)
            if (!equal(a[i], b[i])) return false;
          return true;
        }

        if (arrA != arrB) return false;

        var dateA = a instanceof Date
          , dateB = b instanceof Date;
        if (dateA != dateB) return false;
        if (dateA && dateB) return a.getTime() == b.getTime();

        var regexpA = a instanceof RegExp
          , regexpB = b instanceof RegExp;
        if (regexpA != regexpB) return false;
        if (regexpA && regexpB) return a.toString() == b.toString();

        var keys = keyList(a);
        length = keys.length;

        if (length !== keyList(b).length)
          return false;

        for (i = length; i-- !== 0;)
          if (!hasProp.call(b, keys[i])) return false;

        for (i = length; i-- !== 0;) {
          key = keys[i];
          if (!equal(a[key], b[key])) return false;
        }

        return true;
      }

      return a!==a && b!==b;
    };

    var fastJsonStableStringify = function (data, opts) {
        if (!opts) opts = {};
        if (typeof opts === 'function') opts = { cmp: opts };
        var cycles = (typeof opts.cycles === 'boolean') ? opts.cycles : false;

        var cmp = opts.cmp && (function (f) {
            return function (node) {
                return function (a, b) {
                    var aobj = { key: a, value: node[a] };
                    var bobj = { key: b, value: node[b] };
                    return f(aobj, bobj);
                };
            };
        })(opts.cmp);

        var seen = [];
        return (function stringify (node) {
            if (node && node.toJSON && typeof node.toJSON === 'function') {
                node = node.toJSON();
            }

            if (node === undefined) return;
            if (typeof node == 'number') return isFinite(node) ? '' + node : 'null';
            if (typeof node !== 'object') return JSON.stringify(node);

            var i, out;
            if (Array.isArray(node)) {
                out = '[';
                for (i = 0; i < node.length; i++) {
                    if (i) out += ',';
                    out += stringify(node[i]) || 'null';
                }
                return out + ']';
            }

            if (node === null) return 'null';

            if (seen.indexOf(node) !== -1) {
                if (cycles) return JSON.stringify('__cycle__');
                throw new TypeError('Converting circular structure to JSON');
            }

            var seenIndex = seen.push(node) - 1;
            var keys = Object.keys(node).sort(cmp && cmp(node));
            out = '';
            for (i = 0; i < keys.length; i++) {
                var key = keys[i];
                var value = stringify(node[key]);

                if (!value) continue;
                if (out) out += ',';
                out += JSON.stringify(key) + ':' + value;
            }
            seen.splice(seenIndex, 1);
            return '{' + out + '}';
        })(data);
    };

    function isLogicalOr(op) {
        return !!op.or;
    }
    function isLogicalAnd(op) {
        return !!op.and;
    }
    function isLogicalNot(op) {
        return !!op.not;
    }
    function forEachLeaf(op, fn) {
        if (isLogicalNot(op)) {
            forEachLeaf(op.not, fn);
        }
        else if (isLogicalAnd(op)) {
            for (const subop of op.and) {
                forEachLeaf(subop, fn);
            }
        }
        else if (isLogicalOr(op)) {
            for (const subop of op.or) {
                forEachLeaf(subop, fn);
            }
        }
        else {
            fn(op);
        }
    }
    function normalizeLogicalOperand(op, normalizer) {
        if (isLogicalNot(op)) {
            return { not: normalizeLogicalOperand(op.not, normalizer) };
        }
        else if (isLogicalAnd(op)) {
            return { and: op.and.map(o => normalizeLogicalOperand(o, normalizer)) };
        }
        else if (isLogicalOr(op)) {
            return { or: op.or.map(o => normalizeLogicalOperand(o, normalizer)) };
        }
        else {
            return normalizer(op);
        }
    }

    const deepEqual = fastDeepEqual;
    const duplicate = clone_1;
    /**
     * Creates an object composed of the picked object properties.
     *
     * var object = {'a': 1, 'b': '2', 'c': 3};
     * pick(object, ['a', 'c']);
     * // → {'a': 1, 'c': 3}
     *
     */
    function pick(obj, props) {
        const copy = {};
        for (const prop of props) {
            if (obj.hasOwnProperty(prop)) {
                copy[prop] = obj[prop];
            }
        }
        return copy;
    }
    /**
     * The opposite of _.pick; this method creates an object composed of the own
     * and inherited enumerable string keyed properties of object that are not omitted.
     */
    function omit(obj, props) {
        const copy = Object.assign({}, obj);
        for (const prop of props) {
            delete copy[prop];
        }
        return copy;
    }
    /**
     * Monkey patch Set so that `stringify` produces a string representation of sets.
     */
    Set.prototype['toJSON'] = function () {
        return `Set(${[...this].map(x => fastJsonStableStringify(x)).join(',')})`;
    };
    /**
     * Converts any object to a string representation that can be consumed by humans.
     */
    const stringify = fastJsonStableStringify;
    /**
     * Converts any object to a string of limited size, or a number.
     */
    function hash(a) {
        if (isNumber(a)) {
            return a;
        }
        const str = isString(a) ? a : fastJsonStableStringify(a);
        // short strings can be used as hash directly, longer strings are hashed to reduce memory usage
        if (str.length < 250) {
            return str;
        }
        // from http://werxltd.com/wp/2010/05/13/javascript-implementation-of-javas-string-hashcode-method/
        let h = 0;
        for (let i = 0; i < str.length; i++) {
            const char = str.charCodeAt(i);
            h = (h << 5) - h + char;
            h = h & h; // Convert to 32bit integer
        }
        return h;
    }
    function isNullOrFalse(x) {
        return x === false || x === null;
    }
    function contains(array, item) {
        return array.indexOf(item) > -1;
    }
    /** Returns the array without the elements in item */
    function without(array, excludedItems) {
        return array.filter(item => !contains(excludedItems, item));
    }
    function union(array, other) {
        return array.concat(without(other, array));
    }
    /**
     * Returns true if any item returns true.
     */
    function some(arr, f) {
        let i = 0;
        for (const [k, a] of arr.entries()) {
            if (f(a, k, i++)) {
                return true;
            }
        }
        return false;
    }
    /**
     * Returns true if all items return true.
     */
    function every(arr, f) {
        let i = 0;
        for (const [k, a] of arr.entries()) {
            if (!f(a, k, i++)) {
                return false;
            }
        }
        return true;
    }
    function flatten(arrays) {
        return [].concat(...arrays);
    }
    function fill(val, len) {
        const arr = new Array(len);
        for (let i = 0; i < len; ++i) {
            arr[i] = val;
        }
        return arr;
    }
    /**
     * recursively merges src into dest
     */
    function mergeDeep(dest, ...src) {
        for (const s of src) {
            dest = deepMerge_(dest, s);
        }
        return dest;
    }
    // recursively merges src into dest
    function deepMerge_(dest, src) {
        if (typeof src !== 'object' || src === null) {
            return dest;
        }
        for (const p in src) {
            if (!src.hasOwnProperty(p)) {
                continue;
            }
            if (src[p] === undefined) {
                continue;
            }
            if (typeof src[p] !== 'object' || isArray(src[p]) || src[p] === null) {
                dest[p] = src[p];
            }
            else if (typeof dest[p] !== 'object' || dest[p] === null) {
                dest[p] = mergeDeep(isArray(src[p].constructor) ? [] : {}, src[p]);
            }
            else {
                mergeDeep(dest[p], src[p]);
            }
        }
        return dest;
    }
    function unique(values, f) {
        const results = [];
        const u = {};
        let v;
        for (const val of values) {
            v = f(val);
            if (v in u) {
                continue;
            }
            u[v] = 1;
            results.push(val);
        }
        return results;
    }
    /**
     * Returns true if the two dictionaries disagree. Applies only to defined values.
     */
    function isEqual(dict, other) {
        const dictKeys = keys(dict);
        const otherKeys = keys(other);
        if (dictKeys.length !== otherKeys.length) {
            return false;
        }
        for (const key of dictKeys) {
            if (dict[key] !== other[key]) {
                return false;
            }
        }
        return true;
    }
    function setEqual(a, b) {
        if (a.size !== b.size) {
            return false;
        }
        for (const e of a) {
            if (!b.has(e)) {
                return false;
            }
        }
        return true;
    }
    function hasIntersection(a, b) {
        for (const key of a) {
            if (b.has(key)) {
                return true;
            }
        }
        return false;
    }
    function prefixGenerator(a) {
        const prefixes = new Set();
        for (const x of a) {
            const splitField = splitAccessPath(x);
            // Wrap every element other than the first in `[]`
            const wrappedWithAccessors = splitField.map((y, i) => (i === 0 ? y : `[${y}]`));
            const computedPrefixes = wrappedWithAccessors.map((_, i) => wrappedWithAccessors.slice(0, i + 1).join(''));
            computedPrefixes.forEach(y => prefixes.add(y));
        }
        return prefixes;
    }
    function fieldIntersection(a, b) {
        return hasIntersection(prefixGenerator(a), prefixGenerator(b));
    }
    function isNumeric(num) {
        return !isNaN(num);
    }
    function differArray(array, other) {
        if (array.length !== other.length) {
            return true;
        }
        array.sort();
        other.sort();
        for (let i = 0; i < array.length; i++) {
            if (other[i] !== array[i]) {
                return true;
            }
        }
        return false;
    }
    // This is a stricter version of Object.keys but with better types. See https://github.com/Microsoft/TypeScript/pull/12253#issuecomment-263132208
    const keys = Object.keys;
    function vals(x) {
        const _vals = [];
        for (const k in x) {
            if (x.hasOwnProperty(k)) {
                _vals.push(x[k]);
            }
        }
        return _vals;
    }
    function entries(x) {
        const _entries = [];
        for (const k in x) {
            if (x.hasOwnProperty(k)) {
                _entries.push({
                    key: k,
                    value: x[k]
                });
            }
        }
        return _entries;
    }
    function flagKeys(f) {
        return keys(f);
    }
    function isBoolean$1(b) {
        return b === true || b === false;
    }
    /**
     * Convert a string into a valid variable name
     */
    function varName(s) {
        // Replace non-alphanumeric characters (anything besides a-zA-Z0-9_) with _
        const alphanumericS = s.replace(/\W/g, '_');
        // Add _ if the string has leading numbers.
        return (s.match(/^\d+/) ? '_' : '') + alphanumericS;
    }
    function logicalExpr(op, cb) {
        if (isLogicalNot(op)) {
            return '!(' + logicalExpr(op.not, cb) + ')';
        }
        else if (isLogicalAnd(op)) {
            return '(' + op.and.map((and) => logicalExpr(and, cb)).join(') && (') + ')';
        }
        else if (isLogicalOr(op)) {
            return '(' + op.or.map((or) => logicalExpr(or, cb)).join(') || (') + ')';
        }
        else {
            return cb(op);
        }
    }
    /**
     * Delete nested property of an object, and delete the ancestors of the property if they become empty.
     */
    function deleteNestedProperty(obj, orderedProps) {
        if (orderedProps.length === 0) {
            return true;
        }
        const prop = orderedProps.shift();
        if (deleteNestedProperty(obj[prop], orderedProps)) {
            delete obj[prop];
        }
        return keys(obj).length === 0;
    }
    function titlecase(s) {
        return s.charAt(0).toUpperCase() + s.substr(1);
    }
    /**
     * Converts a path to an access path with datum.
     * @param path The field name.
     * @param datum The string to use for `datum`.
     */
    function accessPathWithDatum(path, datum = 'datum') {
        const pieces = splitAccessPath(path);
        const prefixes = [];
        for (let i = 1; i <= pieces.length; i++) {
            const prefix = `[${pieces
            .slice(0, i)
            .map($)
            .join('][')}]`;
            prefixes.push(`${datum}${prefix}`);
        }
        return prefixes.join(' && ');
    }
    /**
     * Return access with datum to the flattened field.
     *
     * @param path The field name.
     * @param datum The string to use for `datum`.
     */
    function flatAccessWithDatum(path, datum = 'datum') {
        return `${datum}[${$(splitAccessPath(path).join('.'))}]`;
    }
    /**
     * Replaces path accesses with access to non-nested field.
     * For example, `foo["bar"].baz` becomes `foo\\.bar\\.baz`.
     */
    function replacePathInField(path) {
        return `${splitAccessPath(path)
        .map(p => p.replace('.', '\\.'))
        .join('\\.')}`;
    }
    /**
     * Remove path accesses with access from field.
     * For example, `foo["bar"].baz` becomes `foo.bar.baz`.
     */
    function removePathFromField(path) {
        return `${splitAccessPath(path).join('.')}`;
    }
    /**
     * Count the depth of the path. Returns 1 for fields that are not nested.
     */
    function accessPathDepth(path) {
        if (!path) {
            return 0;
        }
        return splitAccessPath(path).length;
    }
    /**
     * This is a replacement for chained || for numeric properties or properties that respect null so that 0 will be included.
     */
    function getFirstDefined(...args) {
        for (const arg of args) {
            if (arg !== undefined) {
                return arg;
            }
        }
        return undefined;
    }
    // variable used to generate id
    let idCounter = 42;
    /**
     * Returns a new random id every time it gets called.
     *
     * Has side effect!
     */
    function uniqueId(prefix) {
        const id = ++idCounter;
        return prefix ? String(prefix) + id : id;
    }
    /**
     * Resets the id counter used in uniqueId. This can be useful for testing.
     */
    function resetIdCounter() {
        idCounter = 42;
    }
    function internalField(name) {
        return isInternalField(name) ? name : `__${name}`;
    }
    function isInternalField(name) {
        return name.indexOf('__') === 0;
    }
    /**
     * Normalize angle to be within [0,360).
     */
    function normalizeAngle(angle) {
        return ((angle % 360) + 360) % 360;
    }

    var util = /*#__PURE__*/Object.freeze({
        deepEqual: deepEqual,
        duplicate: duplicate,
        pick: pick,
        omit: omit,
        stringify: stringify,
        hash: hash,
        isNullOrFalse: isNullOrFalse,
        contains: contains,
        without: without,
        union: union,
        some: some,
        every: every,
        flatten: flatten,
        fill: fill,
        mergeDeep: mergeDeep,
        unique: unique,
        isEqual: isEqual,
        setEqual: setEqual,
        hasIntersection: hasIntersection,
        prefixGenerator: prefixGenerator,
        fieldIntersection: fieldIntersection,
        isNumeric: isNumeric,
        differArray: differArray,
        keys: keys,
        vals: vals,
        entries: entries,
        flagKeys: flagKeys,
        isBoolean: isBoolean$1,
        varName: varName,
        logicalExpr: logicalExpr,
        deleteNestedProperty: deleteNestedProperty,
        titlecase: titlecase,
        accessPathWithDatum: accessPathWithDatum,
        flatAccessWithDatum: flatAccessWithDatum,
        replacePathInField: replacePathInField,
        removePathFromField: removePathFromField,
        accessPathDepth: accessPathDepth,
        getFirstDefined: getFirstDefined,
        uniqueId: uniqueId,
        resetIdCounter: resetIdCounter,
        internalField: internalField,
        isInternalField: isInternalField,
        normalizeAngle: normalizeAngle
    });

    const AREA = 'area';
    const BAR = 'bar';
    const LINE = 'line';
    const POINT = 'point';
    const RECT = 'rect';
    const RULE = 'rule';
    const TEXT = 'text';
    const TICK = 'tick';
    const TRAIL = 'trail';
    const CIRCLE = 'circle';
    const SQUARE = 'square';
    const GEOSHAPE = 'geoshape';
    // Using mapped type to declare index, ensuring we always have all marks when we add more.
    const MARK_INDEX = {
        area: 1,
        bar: 1,
        line: 1,
        point: 1,
        text: 1,
        tick: 1,
        trail: 1,
        rect: 1,
        geoshape: 1,
        rule: 1,
        circle: 1,
        square: 1
    };
    function isPathMark(m) {
        return contains(['line', 'area', 'trail'], m);
    }
    const PRIMITIVE_MARKS = flagKeys(MARK_INDEX);
    function isMarkDef(mark) {
        return mark['type'];
    }
    const PRIMITIVE_MARK_INDEX = toSet(PRIMITIVE_MARKS);
    const STROKE_CONFIG = [
        'stroke',
        'strokeWidth',
        'strokeDash',
        'strokeDashOffset',
        'strokeOpacity',
        'strokeJoin',
        'strokeMiterLimit'
    ];
    const FILL_CONFIG = ['fill', 'fillOpacity'];
    const FILL_STROKE_CONFIG = [].concat(STROKE_CONFIG, FILL_CONFIG);
    const VL_ONLY_MARK_CONFIG_PROPERTIES = ['filled', 'color', 'tooltip'];
    const VL_ONLY_MARK_SPECIFIC_CONFIG_PROPERTY_INDEX = {
        area: ['line', 'point'],
        bar: ['binSpacing', 'continuousBandSize', 'discreteBandSize'],
        line: ['point'],
        text: ['shortTimeLabels'],
        tick: ['bandSize', 'thickness']
    };
    const defaultMarkConfig = {
        color: '#4c78a8',
        tooltip: { content: 'encoding' }
    };
    const defaultBarConfig = {
        binSpacing: 1,
        continuousBandSize: 5
    };
    const defaultTickConfig = {
        thickness: 1
    };
    function getMarkType(m) {
        return isMarkDef(m) ? m.type : m;
    }

    function isUnitSpec(spec) {
        return !!spec['mark'];
    }

    class CompositeMarkNormalizer {
        constructor(name, run) {
            this.name = name;
            this.run = run;
        }
        hasMatchingType(spec) {
            if (isUnitSpec(spec)) {
                return getMarkType(spec.mark) === this.name;
            }
            return false;
        }
    }

    /*! *****************************************************************************
    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the Apache License, Version 2.0 (the "License"); you may not use
    this file except in compliance with the License. You may obtain a copy of the
    License at http://www.apache.org/licenses/LICENSE-2.0

    THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
    WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
    MERCHANTABLITY OR NON-INFRINGEMENT.

    See the Apache Version 2.0 License for specific language governing permissions
    and limitations under the License.
    ***************************************************************************** */
    /* global Reflect, Promise */

    var extendStatics = function(d, b) {
        extendStatics = Object.setPrototypeOf ||
            ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
            function (d, b) { for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p]; };
        return extendStatics(d, b);
    };

    function __extends(d, b) {
        extendStatics(d, b);
        function __() { this.constructor = d; }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    }

    var __assign = function() {
        __assign = Object.assign || function __assign(t) {
            for (var s, i = 1, n = arguments.length; i < n; i++) {
                s = arguments[i];
                for (var p in s) if (Object.prototype.hasOwnProperty.call(s, p)) t[p] = s[p];
            }
            return t;
        };
        return __assign.apply(this, arguments);
    };

    function __rest(s, e) {
        var t = {};
        for (var p in s) if (Object.prototype.hasOwnProperty.call(s, p) && e.indexOf(p) < 0)
            t[p] = s[p];
        if (s != null && typeof Object.getOwnPropertySymbols === "function")
            for (var i = 0, p = Object.getOwnPropertySymbols(s); i < p.length; i++) if (e.indexOf(p[i]) < 0)
                t[p[i]] = s[p[i]];
        return t;
    }

    function __decorate(decorators, target, key, desc) {
        var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
        else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    }

    function __param(paramIndex, decorator) {
        return function (target, key) { decorator(target, key, paramIndex); }
    }

    function __metadata(metadataKey, metadataValue) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(metadataKey, metadataValue);
    }

    function __awaiter(thisArg, _arguments, P, generator) {
        return new (P || (P = Promise))(function (resolve, reject) {
            function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
            function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
            function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
            step((generator = generator.apply(thisArg, _arguments || [])).next());
        });
    }

    function __generator(thisArg, body) {
        var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g;
        return g = { next: verb(0), "throw": verb(1), "return": verb(2) }, typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
        function verb(n) { return function (v) { return step([n, v]); }; }
        function step(op) {
            if (f) throw new TypeError("Generator is already executing.");
            while (_) try {
                if (f = 1, y && (t = op[0] & 2 ? y["return"] : op[0] ? y["throw"] || ((t = y["return"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t;
                if (y = 0, t) op = [op[0] & 2, t.value];
                switch (op[0]) {
                    case 0: case 1: t = op; break;
                    case 4: _.label++; return { value: op[1], done: false };
                    case 5: _.label++; y = op[1]; op = [0]; continue;
                    case 7: op = _.ops.pop(); _.trys.pop(); continue;
                    default:
                        if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                        if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                        if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                        if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                        if (t[2]) _.ops.pop();
                        _.trys.pop(); continue;
                }
                op = body.call(thisArg, _);
            } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
            if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
        }
    }

    function __exportStar(m, exports) {
        for (var p in m) if (!exports.hasOwnProperty(p)) exports[p] = m[p];
    }

    function __values(o) {
        var m = typeof Symbol === "function" && o[Symbol.iterator], i = 0;
        if (m) return m.call(o);
        return {
            next: function () {
                if (o && i >= o.length) o = void 0;
                return { value: o && o[i++], done: !o };
            }
        };
    }

    function __read(o, n) {
        var m = typeof Symbol === "function" && o[Symbol.iterator];
        if (!m) return o;
        var i = m.call(o), r, ar = [], e;
        try {
            while ((n === void 0 || n-- > 0) && !(r = i.next()).done) ar.push(r.value);
        }
        catch (error) { e = { error: error }; }
        finally {
            try {
                if (r && !r.done && (m = i["return"])) m.call(i);
            }
            finally { if (e) throw e.error; }
        }
        return ar;
    }

    function __spread() {
        for (var ar = [], i = 0; i < arguments.length; i++)
            ar = ar.concat(__read(arguments[i]));
        return ar;
    }

    function __await(v) {
        return this instanceof __await ? (this.v = v, this) : new __await(v);
    }

    function __asyncGenerator(thisArg, _arguments, generator) {
        if (!Symbol.asyncIterator) throw new TypeError("Symbol.asyncIterator is not defined.");
        var g = generator.apply(thisArg, _arguments || []), i, q = [];
        return i = {}, verb("next"), verb("throw"), verb("return"), i[Symbol.asyncIterator] = function () { return this; }, i;
        function verb(n) { if (g[n]) i[n] = function (v) { return new Promise(function (a, b) { q.push([n, v, a, b]) > 1 || resume(n, v); }); }; }
        function resume(n, v) { try { step(g[n](v)); } catch (e) { settle(q[0][3], e); } }
        function step(r) { r.value instanceof __await ? Promise.resolve(r.value.v).then(fulfill, reject) : settle(q[0][2], r); }
        function fulfill(value) { resume("next", value); }
        function reject(value) { resume("throw", value); }
        function settle(f, v) { if (f(v), q.shift(), q.length) resume(q[0][0], q[0][1]); }
    }

    function __asyncDelegator(o) {
        var i, p;
        return i = {}, verb("next"), verb("throw", function (e) { throw e; }), verb("return"), i[Symbol.iterator] = function () { return this; }, i;
        function verb(n, f) { i[n] = o[n] ? function (v) { return (p = !p) ? { value: __await(o[n](v)), done: n === "return" } : f ? f(v) : v; } : f; }
    }

    function __asyncValues(o) {
        if (!Symbol.asyncIterator) throw new TypeError("Symbol.asyncIterator is not defined.");
        var m = o[Symbol.asyncIterator], i;
        return m ? m.call(o) : (o = typeof __values === "function" ? __values(o) : o[Symbol.iterator](), i = {}, verb("next"), verb("throw"), verb("return"), i[Symbol.asyncIterator] = function () { return this; }, i);
        function verb(n) { i[n] = o[n] && function (v) { return new Promise(function (resolve, reject) { v = o[n](v), settle(resolve, reject, v.done, v.value); }); }; }
        function settle(resolve, reject, d, v) { Promise.resolve(v).then(function(v) { resolve({ value: v, done: d }); }, reject); }
    }

    function __makeTemplateObject(cooked, raw) {
        if (Object.defineProperty) { Object.defineProperty(cooked, "raw", { value: raw }); } else { cooked.raw = raw; }
        return cooked;
    }
    function __importStar(mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
        result.default = mod;
        return result;
    }

    function __importDefault(mod) {
        return (mod && mod.__esModule) ? mod : { default: mod };
    }

    var tslib_1 = /*#__PURE__*/Object.freeze({
        __extends: __extends,
        get __assign () { return __assign; },
        __rest: __rest,
        __decorate: __decorate,
        __param: __param,
        __metadata: __metadata,
        __awaiter: __awaiter,
        __generator: __generator,
        __exportStar: __exportStar,
        __values: __values,
        __read: __read,
        __spread: __spread,
        __await: __await,
        __asyncGenerator: __asyncGenerator,
        __asyncDelegator: __asyncDelegator,
        __asyncValues: __asyncValues,
        __makeTemplateObject: __makeTemplateObject,
        __importStar: __importStar,
        __importDefault: __importDefault
    });

    const AGGREGATE_OP_INDEX = {
        argmax: 1,
        argmin: 1,
        average: 1,
        count: 1,
        distinct: 1,
        max: 1,
        mean: 1,
        median: 1,
        min: 1,
        missing: 1,
        q1: 1,
        q3: 1,
        ci0: 1,
        ci1: 1,
        stderr: 1,
        stdev: 1,
        stdevp: 1,
        sum: 1,
        valid: 1,
        values: 1,
        variance: 1,
        variancep: 1
    };
    function isArgminDef(a) {
        return !!a && !!a['argmin'];
    }
    function isArgmaxDef(a) {
        return !!a && !!a['argmax'];
    }
    function isAggregateOp(a) {
        return isString(a) && !!AGGREGATE_OP_INDEX[a];
    }
    const COUNTING_OPS = ['count', 'valid', 'missing', 'distinct'];
    function isCountingAggregateOp(aggregate) {
        return aggregate && isString(aggregate) && contains(COUNTING_OPS, aggregate);
    }
    function isMinMaxOp(aggregate) {
        return aggregate && isString(aggregate) && contains(['min', 'max'], aggregate);
    }
    /** Additive-based aggregation operations.  These can be applied to stack. */
    const SUM_OPS = ['count', 'sum', 'distinct', 'valid', 'missing'];
    /**
     * Aggregation operators that always produce values within the range [domainMin, domainMax].
     */
    const SHARED_DOMAIN_OPS = ['mean', 'average', 'median', 'q1', 'q3', 'min', 'max'];
    const SHARED_DOMAIN_OP_INDEX = toSet(SHARED_DOMAIN_OPS);

    /*
     * Constants and utilities for encoding channels (Visual variables)
     * such as 'x', 'y', 'color'.
     */
    // Facet
    const ROW = 'row';
    const COLUMN = 'column';
    const FACET = 'facet';
    // Position
    const X = 'x';
    const Y = 'y';
    const X2 = 'x2';
    const Y2 = 'y2';
    // Geo Position
    const LATITUDE = 'latitude';
    const LONGITUDE = 'longitude';
    const LATITUDE2 = 'latitude2';
    const LONGITUDE2 = 'longitude2';
    // Mark property with scale
    const COLOR = 'color';
    const FILL = 'fill';
    const STROKE = 'stroke';
    const SHAPE = 'shape';
    const SIZE = 'size';
    const OPACITY = 'opacity';
    const FILLOPACITY = 'fillOpacity';
    const STROKEOPACITY = 'strokeOpacity';
    const STROKEWIDTH = 'strokeWidth';
    // Non-scale channel
    const TEXT$1 = 'text';
    const ORDER = 'order';
    const DETAIL = 'detail';
    const KEY = 'key';
    const TOOLTIP = 'tooltip';
    const HREF = 'href';
    function isGeoPositionChannel(c) {
        switch (c) {
            case LATITUDE:
            case LATITUDE2:
            case LONGITUDE:
            case LONGITUDE2:
                return true;
        }
        return false;
    }
    function getPositionChannelFromLatLong(channel) {
        switch (channel) {
            case LATITUDE:
                return 'y';
            case LATITUDE2:
                return 'y2';
            case LONGITUDE:
                return 'x';
            case LONGITUDE2:
                return 'x2';
        }
    }
    const GEOPOSITION_CHANNEL_INDEX = {
        longitude: 1,
        longitude2: 1,
        latitude: 1,
        latitude2: 1
    };
    const GEOPOSITION_CHANNELS = flagKeys(GEOPOSITION_CHANNEL_INDEX);
    const UNIT_CHANNEL_INDEX = Object.assign({ 
        // position
        x: 1, y: 1, x2: 1, y2: 1 }, GEOPOSITION_CHANNEL_INDEX, { 
        // color
        color: 1, fill: 1, stroke: 1, 
        // other non-position with scale
        opacity: 1, fillOpacity: 1, strokeOpacity: 1, strokeWidth: 1, size: 1, shape: 1, 
        // channels without scales
        order: 1, text: 1, detail: 1, key: 1, tooltip: 1, href: 1 });
    function isColorChannel(channel) {
        return channel === 'color' || channel === 'fill' || channel === 'stroke';
    }
    const FACET_CHANNEL_INDEX = {
        row: 1,
        column: 1,
        facet: 1
    };
    const FACET_CHANNELS = flagKeys(FACET_CHANNEL_INDEX);
    const CHANNEL_INDEX = Object.assign({}, UNIT_CHANNEL_INDEX, FACET_CHANNEL_INDEX);
    const CHANNELS = flagKeys(CHANNEL_INDEX);
    const SINGLE_DEF_CHANNEL_INDEX = __rest(CHANNEL_INDEX, ["order", "detail"]);
    const SINGLE_DEF_UNIT_CHANNEL_INDEX = __rest(CHANNEL_INDEX, ["order", "detail", "row", "column", "facet"]);
    function isSingleDefUnitChannel(str) {
        return !!SINGLE_DEF_UNIT_CHANNEL_INDEX[str];
    }
    function isChannel(str) {
        return !!CHANNEL_INDEX[str];
    }
    const SECONDARY_RANGE_CHANNEL = ['x2', 'y2', 'latitude2', 'longitude2'];
    function isSecondaryRangeChannel(c) {
        const main = getMainRangeChannel(c);
        return main !== c;
    }
    function getMainRangeChannel(channel) {
        switch (channel) {
            case 'x2':
                return 'x';
            case 'y2':
                return 'y';
            case 'latitude2':
                return 'latitude';
            case 'longitude2':
                return 'longitude';
        }
        return channel;
    }
    // NONPOSITION_CHANNELS = UNIT_CHANNELS without X, Y, X2, Y2;
    const // The rest of unit channels then have scale
    NONPOSITION_CHANNEL_INDEX = __rest(UNIT_CHANNEL_INDEX, ["x", "y", "x2", "y2", "latitude", "longitude", "latitude2", "longitude2"]);
    const NONPOSITION_CHANNELS = flagKeys(NONPOSITION_CHANNEL_INDEX);
    // POSITION_SCALE_CHANNELS = X and Y;
    const POSITION_SCALE_CHANNEL_INDEX = { x: 1, y: 1 };
    const POSITION_SCALE_CHANNELS = flagKeys(POSITION_SCALE_CHANNEL_INDEX);
    // NON_POSITION_SCALE_CHANNEL = SCALE_CHANNELS without X, Y
    const NONPOSITION_SCALE_CHANNEL_INDEX = __rest(NONPOSITION_CHANNEL_INDEX, ["text", "tooltip", "href", "detail", "key", "order"]);
    const NONPOSITION_SCALE_CHANNELS = flagKeys(NONPOSITION_SCALE_CHANNEL_INDEX);
    function isNonPositionScaleChannel(channel) {
        return !!NONPOSITION_CHANNEL_INDEX[channel];
    }
    /**
     * @returns whether Vega supports legends for a particular channel
     */
    function supportLegend(channel) {
        switch (channel) {
            case COLOR:
            case FILL:
            case STROKE:
            case SIZE:
            case SHAPE:
            case OPACITY:
                return true;
            case FILLOPACITY:
            case STROKEOPACITY:
            case STROKEWIDTH:
                return false;
        }
    }
    // Declare SCALE_CHANNEL_INDEX
    const SCALE_CHANNEL_INDEX = Object.assign({}, POSITION_SCALE_CHANNEL_INDEX, NONPOSITION_SCALE_CHANNEL_INDEX);
    /** List of channels with scales */
    const SCALE_CHANNELS = flagKeys(SCALE_CHANNEL_INDEX);
    function isScaleChannel(channel) {
        return !!SCALE_CHANNEL_INDEX[channel];
    }
    /**
     * Return whether a channel supports a particular mark type.
     * @param channel  channel name
     * @param mark the mark type
     * @return whether the mark supports the channel
     */
    function supportMark(channel, mark) {
        return getSupportedMark(channel)[mark];
    }
    /**
     * Return a dictionary showing whether a channel supports mark type.
     * @param channel
     * @return A dictionary mapping mark types to 'always', 'binned', or undefined
     */
    function getSupportedMark(channel) {
        switch (channel) {
            case COLOR:
            case FILL:
            case STROKE:
            // falls through
            case DETAIL:
            case KEY:
            case TOOLTIP:
            case HREF:
            case ORDER: // TODO: revise (order might not support rect, which is not stackable?)
            case OPACITY:
            case FILLOPACITY:
            case STROKEOPACITY:
            case STROKEWIDTH:
            // falls through
            case FACET:
            case ROW: // falls through
            case COLUMN:
                return {
                    // all marks
                    point: 'always',
                    tick: 'always',
                    rule: 'always',
                    circle: 'always',
                    square: 'always',
                    bar: 'always',
                    rect: 'always',
                    line: 'always',
                    trail: 'always',
                    area: 'always',
                    text: 'always',
                    geoshape: 'always'
                };
            case X:
            case Y:
            case LATITUDE:
            case LONGITUDE:
                return {
                    // all marks except geoshape. geoshape does not use X, Y -- it uses a projection
                    point: 'always',
                    tick: 'always',
                    rule: 'always',
                    circle: 'always',
                    square: 'always',
                    bar: 'always',
                    rect: 'always',
                    line: 'always',
                    trail: 'always',
                    area: 'always',
                    text: 'always'
                };
            case X2:
            case Y2:
            case LATITUDE2:
            case LONGITUDE2:
                return {
                    rule: 'always',
                    bar: 'always',
                    rect: 'always',
                    area: 'always',
                    circle: 'binned',
                    point: 'binned',
                    square: 'binned',
                    tick: 'binned'
                };
            case SIZE:
                return {
                    point: 'always',
                    tick: 'always',
                    rule: 'always',
                    circle: 'always',
                    square: 'always',
                    bar: 'always',
                    text: 'always',
                    line: 'always',
                    trail: 'always'
                };
            case SHAPE:
                return { point: 'always', geoshape: 'always' };
            case TEXT$1:
                return { text: 'always' };
        }
    }
    function rangeType(channel) {
        switch (channel) {
            case X:
            case Y:
            case SIZE:
            case STROKEWIDTH:
            case OPACITY:
            case FILLOPACITY:
            case STROKEOPACITY:
            // X2 and Y2 use X and Y scales, so they similarly have continuous range. [falls through]
            case X2:
            case Y2:
                return undefined;
            case FACET:
            case ROW:
            case COLUMN:
            case SHAPE:
            // TEXT, TOOLTIP, and HREF have no scale but have discrete output [falls through]
            case TEXT$1:
            case TOOLTIP:
            case HREF:
                return 'discrete';
            // Color can be either continuous or discrete, depending on scale type.
            case COLOR:
            case FILL:
            case STROKE:
                return 'flexible';
            // No scale, no range type.
            case LATITUDE:
            case LONGITUDE:
            case LATITUDE2:
            case LONGITUDE2:
            case DETAIL:
            case KEY:
            case ORDER:
                return undefined;
        }
        /* istanbul ignore next: should never reach here. */
        throw new Error('rangeType not implemented for ' + channel);
    }

    /**
     * Collection of all Vega-Lite Error Messages
     */
    const INVALID_SPEC = 'Invalid spec';
    // FIT
    const FIT_NON_SINGLE = 'Autosize "fit" only works for single views and layered views.';
    const CANNOT_FIX_RANGE_STEP_WITH_FIT = 'Cannot use a fixed value of "rangeStep" when "autosize" is "fit".';
    // SELECTION
    function cannotProjectOnChannelWithoutField(channel) {
        return `Cannot project a selection on encoding channel "${channel}", which has no field.`;
    }
    function nearestNotSupportForContinuous(mark) {
        return `The "nearest" transform is not supported for ${mark} marks.`;
    }
    function selectionNotSupported(mark) {
        return `Selection not supported for ${mark} yet`;
    }
    function selectionNotFound(name) {
        return `Cannot find a selection named "${name}"`;
    }
    const SCALE_BINDINGS_CONTINUOUS = 'Scale bindings are currently only supported for scales with unbinned, continuous domains.';
    const NO_INIT_SCALE_BINDINGS = 'Selections bound to scales cannot be separately initialized.';
    // REPEAT
    function noSuchRepeatedValue(field) {
        return `Unknown repeated value "${field}".`;
    }
    function columnsNotSupportByRowCol(type) {
        return `The "columns" property cannot be used when "${type}" has nested row/column.`;
    }
    // CONCAT
    const CONCAT_CANNOT_SHARE_AXIS = 'Axes cannot be shared in concatenated views yet (https://github.com/vega/vega-lite/issues/2415).';
    // REPEAT
    const REPEAT_CANNOT_SHARE_AXIS = 'Axes cannot be shared in repeated views yet (https://github.com/vega/vega-lite/issues/2415).';
    // DATA
    function unrecognizedParse(p) {
        return `Unrecognized parse "${p}".`;
    }
    function differentParse(field, local, ancestor) {
        return `An ancestor parsed field "${field}" as ${ancestor} but a child wants to parse the field as ${local}.`;
    }
    // TRANSFORMS
    function invalidTransformIgnored(transform) {
        return `Ignoring an invalid transform: ${stringify(transform)}.`;
    }
    const NO_FIELDS_NEEDS_AS = 'If "from.fields" is not specified, "as" has to be a string that specifies the key to be used for the data from the secondary source.';
    // ENCODING & FACET
    function encodingOverridden(channels) {
        return `Layer's shared ${channels.join(',')} channel ${channels.length === 1 ? 'is' : 'are'} overriden`;
    }
    function projectionOverridden(opt) {
        const { parentProjection, projection } = opt;
        return `Layer's shared projection ${stringify(parentProjection)} is overridden by a child projection ${stringify(projection)}.`;
    }
    function primitiveChannelDef(channel, type, value) {
        return `Channel ${channel} is a ${type}. Converted to {value: ${stringify(value)}}.`;
    }
    function invalidFieldType(type) {
        return `Invalid field type "${type}"`;
    }
    function nonZeroScaleUsedWithLengthMark(mark, channel, opt) {
        const scaleText = opt.scaleType
            ? `${opt.scaleType} scale`
            : opt.zeroFalse
                ? 'scale with zero=false'
                : 'scale with custom domain that excludes zero';
        return `A ${scaleText} is used to encode ${mark}'s ${channel}. This can be misleading as the ${channel === 'x' ? 'width' : 'height'} of the ${mark} can be arbitrary based on the scale domain. You may want to use point mark instead.`;
    }
    function invalidFieldTypeForCountAggregate(type, aggregate) {
        return `Invalid field type "${type}" for aggregate: "${aggregate}", using "quantitative" instead.`;
    }
    function invalidAggregate(aggregate) {
        return `Invalid aggregation operator "${aggregate}"`;
    }
    function missingFieldType(channel, newType) {
        return `Missing type for channel "${channel}", using "${newType}" instead.`;
    }
    function droppingColor(type, opt) {
        const { fill, stroke } = opt;
        return (`Dropping color ${type} as the plot also has ` + (fill && stroke ? 'fill and stroke' : fill ? 'fill' : 'stroke'));
    }
    function emptyFieldDef(fieldDef, channel) {
        return `Dropping ${stringify(fieldDef)} from channel "${channel}" since it does not contain data field or value.`;
    }
    function latLongDeprecated(channel, type, newChannel) {
        return `${channel}-encoding with type ${type} is deprecated. Replacing with ${newChannel}-encoding.`;
    }
    const LINE_WITH_VARYING_SIZE = 'Line marks cannot encode size with a non-groupby field. You may want to use trail marks instead.';
    function incompatibleChannel(channel, markOrFacet, when) {
        return `${channel} dropped as it is incompatible with "${markOrFacet}"${when ? ` when ${when}` : ''}.`;
    }
    function invalidEncodingChannel(channel) {
        return `${channel}-encoding is dropped as ${channel} is not a valid encoding channel.`;
    }
    function facetChannelShouldBeDiscrete(channel) {
        return `${channel} encoding should be discrete (ordinal / nominal / binned).`;
    }
    function facetChannelDropped(channels) {
        return `Facet encoding dropped as ${channels.join(' and ')} ${channels.length > 1 ? 'are' : 'is'} also specified.`;
    }
    function discreteChannelCannotEncode(channel, type) {
        return `Using discrete channel "${channel}" to encode "${type}" field can be misleading as it does not encode ${type === 'ordinal' ? 'order' : 'magnitude'}.`;
    }
    // Mark
    const BAR_WITH_POINT_SCALE_AND_RANGESTEP_NULL = 'Bar mark should not be used with point scale when rangeStep is null. Please use band scale instead.';
    function lineWithRange(hasX2, hasY2) {
        const channels = hasX2 && hasY2 ? 'x2 and y2' : hasX2 ? 'x2' : 'y2';
        return `Line mark is for continuous lines and thus cannot be used with ${channels}. We will use the rule mark (line segments) instead.`;
    }
    function orientOverridden(original, actual) {
        return `Specified orient "${original}" overridden with "${actual}"`;
    }
    // SCALE
    const CANNOT_UNION_CUSTOM_DOMAIN_WITH_FIELD_DOMAIN = 'custom domain scale cannot be unioned with default field-based domain';
    function cannotUseScalePropertyWithNonColor(prop) {
        return `Cannot use the scale property "${prop}" with non-color channel.`;
    }
    function unaggregateDomainHasNoEffectForRawField(fieldDef) {
        return `Using unaggregated domain with raw field has no effect (${stringify(fieldDef)}).`;
    }
    function unaggregateDomainWithNonSharedDomainOp(aggregate) {
        return `Unaggregated domain not applicable for "${aggregate}" since it produces values outside the origin domain of the source data.`;
    }
    function unaggregatedDomainWithLogScale(fieldDef) {
        return `Unaggregated domain is currently unsupported for log scale (${stringify(fieldDef)}).`;
    }
    function cannotApplySizeToNonOrientedMark(mark) {
        return `Cannot apply size to non-oriented mark "${mark}".`;
    }
    function rangeStepDropped(channel) {
        return `rangeStep for "${channel}" is dropped as top-level ${channel === 'x' ? 'width' : 'height'} is provided.`;
    }
    function scaleTypeNotWorkWithChannel(channel, scaleType, defaultScaleType) {
        return `Channel "${channel}" does not work with "${scaleType}" scale. We are using "${defaultScaleType}" scale instead.`;
    }
    function scaleTypeNotWorkWithFieldDef(scaleType, defaultScaleType) {
        return `FieldDef does not work with "${scaleType}" scale. We are using "${defaultScaleType}" scale instead.`;
    }
    function scalePropertyNotWorkWithScaleType(scaleType, propName, channel) {
        return `${channel}-scale's "${propName}" is dropped as it does not work with ${scaleType} scale.`;
    }
    function scaleTypeNotWorkWithMark(mark, scaleType) {
        return `Scale type "${scaleType}" does not work with mark "${mark}".`;
    }
    function mergeConflictingProperty(property, propertyOf, v1, v2) {
        return `Conflicting ${propertyOf.toString()} property "${property.toString()}" (${stringify(v1)} and ${stringify(v2)}).  Using ${stringify(v1)}.`;
    }
    function independentScaleMeansIndependentGuide(channel) {
        return `Setting the scale to be independent for "${channel}" means we also have to set the guide (axis or legend) to be independent.`;
    }
    function domainSortDropped(sort) {
        return `Dropping sort property ${stringify(sort)} as unioned domains only support boolean or op 'count'.`;
    }
    const UNABLE_TO_MERGE_DOMAINS = 'Unable to merge domains';
    const MORE_THAN_ONE_SORT = 'Domains that should be unioned has conflicting sort properties. Sort will be set to true.';
    // AXIS
    const INVALID_CHANNEL_FOR_AXIS = 'Invalid channel for axis.';
    // STACK
    function cannotStackRangedMark(channel) {
        return `Cannot stack "${channel}" if there is already "${channel}2"`;
    }
    function cannotStackNonLinearScale(scaleType) {
        return `Cannot stack non-linear scale (${scaleType})`;
    }
    function stackNonSummativeAggregate(aggregate) {
        return `Stacking is applied even though the aggregate function is non-summative ("${aggregate}")`;
    }
    // TIMEUNIT
    function invalidTimeUnit(unitName, value) {
        return `Invalid ${unitName}: ${stringify(value)}`;
    }
    function dayReplacedWithDate(fullTimeUnit) {
        return `Time unit "${fullTimeUnit}" is not supported. We are replacing it with ${fullTimeUnit.replace('day', 'date')}.`;
    }
    function droppedDay(d) {
        return `Dropping day from datetime ${stringify(d)} as day cannot be combined with other units.`;
    }
    function errorBarCenterAndExtentAreNotNeeded(center, extent) {
        return `${extent ? 'extent ' : ''}${extent && center ? 'and ' : ''}${center ? 'center ' : ''}${extent && center ? 'are ' : 'is '}not needed when data are aggregated.`;
    }
    function errorBarCenterIsUsedWithWrongExtent(center, extent, mark) {
        return `${center} is not usually used with ${extent} for ${mark}.`;
    }
    function errorBarContinuousAxisHasCustomizedAggregate(aggregate, compositeMark) {
        return `Continuous axis should not have customized aggregation function ${aggregate}; ${compositeMark} already agregates the axis.`;
    }
    function errorBarCenterIsNotNeeded(extent, mark) {
        return `Center is not needed to be specified in ${mark} when extent is ${extent}.`;
    }
    function errorBand1DNotSupport(property) {
        return `1D error band does not support ${property}`;
    }
    // CHANNEL
    function channelRequiredForBinned(channel) {
        return `Channel ${channel} is required for "binned" bin`;
    }
    function domainRequiredForThresholdScale(channel) {
        return `Domain for ${channel} is required for threshold scale`;
    }

    var message_ = /*#__PURE__*/Object.freeze({
        INVALID_SPEC: INVALID_SPEC,
        FIT_NON_SINGLE: FIT_NON_SINGLE,
        CANNOT_FIX_RANGE_STEP_WITH_FIT: CANNOT_FIX_RANGE_STEP_WITH_FIT,
        cannotProjectOnChannelWithoutField: cannotProjectOnChannelWithoutField,
        nearestNotSupportForContinuous: nearestNotSupportForContinuous,
        selectionNotSupported: selectionNotSupported,
        selectionNotFound: selectionNotFound,
        SCALE_BINDINGS_CONTINUOUS: SCALE_BINDINGS_CONTINUOUS,
        NO_INIT_SCALE_BINDINGS: NO_INIT_SCALE_BINDINGS,
        noSuchRepeatedValue: noSuchRepeatedValue,
        columnsNotSupportByRowCol: columnsNotSupportByRowCol,
        CONCAT_CANNOT_SHARE_AXIS: CONCAT_CANNOT_SHARE_AXIS,
        REPEAT_CANNOT_SHARE_AXIS: REPEAT_CANNOT_SHARE_AXIS,
        unrecognizedParse: unrecognizedParse,
        differentParse: differentParse,
        invalidTransformIgnored: invalidTransformIgnored,
        NO_FIELDS_NEEDS_AS: NO_FIELDS_NEEDS_AS,
        encodingOverridden: encodingOverridden,
        projectionOverridden: projectionOverridden,
        primitiveChannelDef: primitiveChannelDef,
        invalidFieldType: invalidFieldType,
        nonZeroScaleUsedWithLengthMark: nonZeroScaleUsedWithLengthMark,
        invalidFieldTypeForCountAggregate: invalidFieldTypeForCountAggregate,
        invalidAggregate: invalidAggregate,
        missingFieldType: missingFieldType,
        droppingColor: droppingColor,
        emptyFieldDef: emptyFieldDef,
        latLongDeprecated: latLongDeprecated,
        LINE_WITH_VARYING_SIZE: LINE_WITH_VARYING_SIZE,
        incompatibleChannel: incompatibleChannel,
        invalidEncodingChannel: invalidEncodingChannel,
        facetChannelShouldBeDiscrete: facetChannelShouldBeDiscrete,
        facetChannelDropped: facetChannelDropped,
        discreteChannelCannotEncode: discreteChannelCannotEncode,
        BAR_WITH_POINT_SCALE_AND_RANGESTEP_NULL: BAR_WITH_POINT_SCALE_AND_RANGESTEP_NULL,
        lineWithRange: lineWithRange,
        orientOverridden: orientOverridden,
        CANNOT_UNION_CUSTOM_DOMAIN_WITH_FIELD_DOMAIN: CANNOT_UNION_CUSTOM_DOMAIN_WITH_FIELD_DOMAIN,
        cannotUseScalePropertyWithNonColor: cannotUseScalePropertyWithNonColor,
        unaggregateDomainHasNoEffectForRawField: unaggregateDomainHasNoEffectForRawField,
        unaggregateDomainWithNonSharedDomainOp: unaggregateDomainWithNonSharedDomainOp,
        unaggregatedDomainWithLogScale: unaggregatedDomainWithLogScale,
        cannotApplySizeToNonOrientedMark: cannotApplySizeToNonOrientedMark,
        rangeStepDropped: rangeStepDropped,
        scaleTypeNotWorkWithChannel: scaleTypeNotWorkWithChannel,
        scaleTypeNotWorkWithFieldDef: scaleTypeNotWorkWithFieldDef,
        scalePropertyNotWorkWithScaleType: scalePropertyNotWorkWithScaleType,
        scaleTypeNotWorkWithMark: scaleTypeNotWorkWithMark,
        mergeConflictingProperty: mergeConflictingProperty,
        independentScaleMeansIndependentGuide: independentScaleMeansIndependentGuide,
        domainSortDropped: domainSortDropped,
        UNABLE_TO_MERGE_DOMAINS: UNABLE_TO_MERGE_DOMAINS,
        MORE_THAN_ONE_SORT: MORE_THAN_ONE_SORT,
        INVALID_CHANNEL_FOR_AXIS: INVALID_CHANNEL_FOR_AXIS,
        cannotStackRangedMark: cannotStackRangedMark,
        cannotStackNonLinearScale: cannotStackNonLinearScale,
        stackNonSummativeAggregate: stackNonSummativeAggregate,
        invalidTimeUnit: invalidTimeUnit,
        dayReplacedWithDate: dayReplacedWithDate,
        droppedDay: droppedDay,
        errorBarCenterAndExtentAreNotNeeded: errorBarCenterAndExtentAreNotNeeded,
        errorBarCenterIsUsedWithWrongExtent: errorBarCenterIsUsedWithWrongExtent,
        errorBarContinuousAxisHasCustomizedAggregate: errorBarContinuousAxisHasCustomizedAggregate,
        errorBarCenterIsNotNeeded: errorBarCenterIsNotNeeded,
        errorBand1DNotSupport: errorBand1DNotSupport,
        channelRequiredForBinned: channelRequiredForBinned,
        domainRequiredForThresholdScale: domainRequiredForThresholdScale
    });

    /**
     * Vega-Lite's singleton logger utility.
     */
    const message = message_;
    /**
     * Main (default) Vega Logger instance for Vega-Lite
     */
    const main$1 = logger(Warn);
    let current = main$1;
    /**
     * Logger tool for checking if the code throws correct warning
     */
    class LocalLogger {
        constructor() {
            this.warns = [];
            this.infos = [];
            this.debugs = [];
        }
        level() {
            return this;
        }
        warn(...args) {
            this.warns.push(...args);
            return this;
        }
        info(...args) {
            this.infos.push(...args);
            return this;
        }
        debug(...args) {
            this.debugs.push(...args);
            return this;
        }
        error(...args) {
            throw Error(...args);
            return this; // @ts-ignore
        }
    }
    function wrap(f) {
        return () => {
            current = new LocalLogger();
            f(current);
            reset();
        };
    }
    /**
     * Set the singleton logger to be a custom logger
     */
    function set(newLogger) {
        current = newLogger;
        return current;
    }
    /**
     * Reset the main logger to use the default Vega Logger
     */
    function reset() {
        current = main$1;
        return current;
    }
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    function warn(..._) {
        current.warn.apply(current, arguments);
    }
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    function info(..._) {
        current.info.apply(current, arguments);
    }
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    function debug(..._) {
        current.debug.apply(current, arguments);
    }

    var log$1 = /*#__PURE__*/Object.freeze({
        message: message,
        LocalLogger: LocalLogger,
        wrap: wrap,
        set: set,
        reset: reset,
        warn: warn,
        info: info,
        debug: debug
    });

    // DateTime definition object
    /*
     * A designated year that starts on Sunday.
     */
    const SUNDAY_YEAR = 2006;
    function isDateTime(o) {
        return (!!o &&
            (!!o.year ||
                !!o.quarter ||
                !!o.month ||
                !!o.date ||
                !!o.day ||
                !!o.hours ||
                !!o.minutes ||
                !!o.seconds ||
                !!o.milliseconds));
    }
    const MONTHS = [
        'january',
        'february',
        'march',
        'april',
        'may',
        'june',
        'july',
        'august',
        'september',
        'october',
        'november',
        'december'
    ];
    const SHORT_MONTHS = MONTHS.map(m => m.substr(0, 3));
    const DAYS = ['sunday', 'monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday'];
    const SHORT_DAYS = DAYS.map(d => d.substr(0, 3));
    function normalizeQuarter(q) {
        if (isNumber(q)) {
            if (q > 4) {
                warn(message.invalidTimeUnit('quarter', q));
            }
            // We accept 1-based quarter, so need to readjust to 0-based quarter
            return (q - 1).toString();
        }
        else {
            // Invalid quarter
            throw new Error(message.invalidTimeUnit('quarter', q));
        }
    }
    function normalizeMonth(m) {
        if (isNumber(m)) {
            // We accept 1-based month, so need to readjust to 0-based month
            return (m - 1).toString();
        }
        else {
            const lowerM = m.toLowerCase();
            const monthIndex = MONTHS.indexOf(lowerM);
            if (monthIndex !== -1) {
                return monthIndex + ''; // 0 for january, ...
            }
            const shortM = lowerM.substr(0, 3);
            const shortMonthIndex = SHORT_MONTHS.indexOf(shortM);
            if (shortMonthIndex !== -1) {
                return shortMonthIndex + '';
            }
            // Invalid month
            throw new Error(message.invalidTimeUnit('month', m));
        }
    }
    function normalizeDay(d) {
        if (isNumber(d)) {
            // mod so that this can be both 0-based where 0 = sunday
            // and 1-based where 7=sunday
            return (d % 7) + '';
        }
        else {
            const lowerD = d.toLowerCase();
            const dayIndex = DAYS.indexOf(lowerD);
            if (dayIndex !== -1) {
                return dayIndex + ''; // 0 for january, ...
            }
            const shortD = lowerD.substr(0, 3);
            const shortDayIndex = SHORT_DAYS.indexOf(shortD);
            if (shortDayIndex !== -1) {
                return shortDayIndex + '';
            }
            // Invalid day
            throw new Error(message.invalidTimeUnit('day', d));
        }
    }
    /**
     * Return Vega Expression for a particular date time.
     * @param d
     * @param normalize whether to normalize quarter, month, day.
     */
    function dateTimeExpr(d, normalize = false) {
        const units = [];
        if (normalize && d.day !== undefined) {
            if (keys(d).length > 1) {
                warn(message.droppedDay(d));
                d = duplicate(d);
                delete d.day;
            }
        }
        if (d.year !== undefined) {
            units.push(d.year);
        }
        else if (d.day !== undefined) {
            // Set year to 2006 for working with day since January 1 2006 is a Sunday
            units.push(SUNDAY_YEAR);
        }
        else {
            units.push(0);
        }
        if (d.month !== undefined) {
            const month = normalize ? normalizeMonth(d.month) : d.month;
            units.push(month);
        }
        else if (d.quarter !== undefined) {
            const quarter = normalize ? normalizeQuarter(d.quarter) : d.quarter;
            units.push(quarter + '*3');
        }
        else {
            units.push(0); // months start at zero in JS
        }
        if (d.date !== undefined) {
            units.push(d.date);
        }
        else if (d.day !== undefined) {
            // HACK: Day only works as a standalone unit
            // This is only correct because we always set year to 2006 for day
            const day = normalize ? normalizeDay(d.day) : d.day;
            units.push(day + '+1');
        }
        else {
            units.push(1); // Date starts at 1 in JS
        }
        // Note: can't use TimeUnit enum here as importing it will create
        // circular dependency problem!
        for (const timeUnit of ['hours', 'minutes', 'seconds', 'milliseconds']) {
            if (d[timeUnit] !== undefined) {
                units.push(d[timeUnit]);
            }
            else {
                units.push(0);
            }
        }
        if (d.utc) {
            return `utc(${units.join(', ')})`;
        }
        else {
            return `datetime(${units.join(', ')})`;
        }
    }

    function isFacetMapping(f) {
        return !!f['row'] || !!f['column'];
    }
    function isFacetFieldDef(channelDef) {
        return !!channelDef && !!channelDef['header'];
    }
    function isFacetSpec(spec) {
        return spec['facet'] !== undefined;
    }

    var TimeUnit;
    (function (TimeUnit) {
        TimeUnit.YEAR = 'year';
        TimeUnit.MONTH = 'month';
        TimeUnit.DAY = 'day';
        TimeUnit.DATE = 'date';
        TimeUnit.HOURS = 'hours';
        TimeUnit.MINUTES = 'minutes';
        TimeUnit.SECONDS = 'seconds';
        TimeUnit.MILLISECONDS = 'milliseconds';
        TimeUnit.YEARMONTH = 'yearmonth';
        TimeUnit.YEARMONTHDATE = 'yearmonthdate';
        TimeUnit.YEARMONTHDATEHOURS = 'yearmonthdatehours';
        TimeUnit.YEARMONTHDATEHOURSMINUTES = 'yearmonthdatehoursminutes';
        TimeUnit.YEARMONTHDATEHOURSMINUTESSECONDS = 'yearmonthdatehoursminutesseconds';
        // MONTHDATE and MONTHDATEHOURS always include 29 February since we use year 0th (which is a leap year);
        TimeUnit.MONTHDATE = 'monthdate';
        TimeUnit.MONTHDATEHOURS = 'monthdatehours';
        TimeUnit.HOURSMINUTES = 'hoursminutes';
        TimeUnit.HOURSMINUTESSECONDS = 'hoursminutesseconds';
        TimeUnit.MINUTESSECONDS = 'minutesseconds';
        TimeUnit.SECONDSMILLISECONDS = 'secondsmilliseconds';
        TimeUnit.QUARTER = 'quarter';
        TimeUnit.YEARQUARTER = 'yearquarter';
        TimeUnit.QUARTERMONTH = 'quartermonth';
        TimeUnit.YEARQUARTERMONTH = 'yearquartermonth';
        TimeUnit.UTCYEAR = 'utcyear';
        TimeUnit.UTCMONTH = 'utcmonth';
        TimeUnit.UTCDAY = 'utcday';
        TimeUnit.UTCDATE = 'utcdate';
        TimeUnit.UTCHOURS = 'utchours';
        TimeUnit.UTCMINUTES = 'utcminutes';
        TimeUnit.UTCSECONDS = 'utcseconds';
        TimeUnit.UTCMILLISECONDS = 'utcmilliseconds';
        TimeUnit.UTCYEARMONTH = 'utcyearmonth';
        TimeUnit.UTCYEARMONTHDATE = 'utcyearmonthdate';
        TimeUnit.UTCYEARMONTHDATEHOURS = 'utcyearmonthdatehours';
        TimeUnit.UTCYEARMONTHDATEHOURSMINUTES = 'utcyearmonthdatehoursminutes';
        TimeUnit.UTCYEARMONTHDATEHOURSMINUTESSECONDS = 'utcyearmonthdatehoursminutesseconds';
        // UTCMONTHDATE and UTCMONTHDATEHOURS always include 29 February since we use year 0th (which is a leap year);
        TimeUnit.UTCMONTHDATE = 'utcmonthdate';
        TimeUnit.UTCMONTHDATEHOURS = 'utcmonthdatehours';
        TimeUnit.UTCHOURSMINUTES = 'utchoursminutes';
        TimeUnit.UTCHOURSMINUTESSECONDS = 'utchoursminutesseconds';
        TimeUnit.UTCMINUTESSECONDS = 'utcminutesseconds';
        TimeUnit.UTCSECONDSMILLISECONDS = 'utcsecondsmilliseconds';
        TimeUnit.UTCQUARTER = 'utcquarter';
        TimeUnit.UTCYEARQUARTER = 'utcyearquarter';
        TimeUnit.UTCQUARTERMONTH = 'utcquartermonth';
        TimeUnit.UTCYEARQUARTERMONTH = 'utcyearquartermonth';
    })(TimeUnit || (TimeUnit = {}));
    /** Time Unit that only corresponds to only one part of Date objects. */
    const LOCAL_SINGLE_TIMEUNIT_INDEX = {
        year: 1,
        quarter: 1,
        month: 1,
        day: 1,
        date: 1,
        hours: 1,
        minutes: 1,
        seconds: 1,
        milliseconds: 1
    };
    const TIMEUNIT_PARTS = flagKeys(LOCAL_SINGLE_TIMEUNIT_INDEX);
    function isLocalSingleTimeUnit(timeUnit) {
        return !!LOCAL_SINGLE_TIMEUNIT_INDEX[timeUnit];
    }
    const UTC_SINGLE_TIMEUNIT_INDEX = {
        utcyear: 1,
        utcquarter: 1,
        utcmonth: 1,
        utcday: 1,
        utcdate: 1,
        utchours: 1,
        utcminutes: 1,
        utcseconds: 1,
        utcmilliseconds: 1
    };
    function isUtcSingleTimeUnit(timeUnit) {
        return !!UTC_SINGLE_TIMEUNIT_INDEX[timeUnit];
    }
    const LOCAL_MULTI_TIMEUNIT_INDEX = {
        yearquarter: 1,
        yearquartermonth: 1,
        yearmonth: 1,
        yearmonthdate: 1,
        yearmonthdatehours: 1,
        yearmonthdatehoursminutes: 1,
        yearmonthdatehoursminutesseconds: 1,
        quartermonth: 1,
        monthdate: 1,
        monthdatehours: 1,
        hoursminutes: 1,
        hoursminutesseconds: 1,
        minutesseconds: 1,
        secondsmilliseconds: 1
    };
    const UTC_MULTI_TIMEUNIT_INDEX = {
        utcyearquarter: 1,
        utcyearquartermonth: 1,
        utcyearmonth: 1,
        utcyearmonthdate: 1,
        utcyearmonthdatehours: 1,
        utcyearmonthdatehoursminutes: 1,
        utcyearmonthdatehoursminutesseconds: 1,
        utcquartermonth: 1,
        utcmonthdate: 1,
        utcmonthdatehours: 1,
        utchoursminutes: 1,
        utchoursminutesseconds: 1,
        utcminutesseconds: 1,
        utcsecondsmilliseconds: 1
    };
    const UTC_TIMEUNIT_INDEX = Object.assign({}, UTC_SINGLE_TIMEUNIT_INDEX, UTC_MULTI_TIMEUNIT_INDEX);
    function isUTCTimeUnit(t) {
        return !!UTC_TIMEUNIT_INDEX[t];
    }
    function getLocalTimeUnit(t) {
        return t.substr(3);
    }
    const TIMEUNIT_INDEX = Object.assign({}, LOCAL_SINGLE_TIMEUNIT_INDEX, UTC_SINGLE_TIMEUNIT_INDEX, LOCAL_MULTI_TIMEUNIT_INDEX, UTC_MULTI_TIMEUNIT_INDEX);
    function getTimeUnitParts(timeUnit) {
        return TIMEUNIT_PARTS.reduce((parts, part) => {
            if (containsTimeUnit(timeUnit, part)) {
                return [...parts, part];
            }
            return parts;
        }, []);
    }
    /** Returns true if fullTimeUnit contains the timeUnit, false otherwise. */
    function containsTimeUnit(fullTimeUnit, timeUnit) {
        const index = fullTimeUnit.indexOf(timeUnit);
        return (index > -1 && (timeUnit !== TimeUnit.SECONDS || index === 0 || fullTimeUnit.charAt(index - 1) !== 'i') // exclude milliseconds
        );
    }
    /**
     * Returns Vega expresssion for a given timeUnit and fieldRef
     */
    function fieldExpr(fullTimeUnit, field) {
        const fieldRef = accessPathWithDatum(field);
        const utc = isUTCTimeUnit(fullTimeUnit) ? 'utc' : '';
        function func(timeUnit) {
            if (timeUnit === TimeUnit.QUARTER) {
                // quarter starting at 0 (0,3,6,9).
                return `(${utc}quarter(${fieldRef})-1)`;
            }
            else {
                return `${utc}${timeUnit}(${fieldRef})`;
            }
        }
        const d = TIMEUNIT_PARTS.reduce((dateExpr, tu) => {
            if (containsTimeUnit(fullTimeUnit, tu)) {
                dateExpr[tu] = func(tu);
            }
            return dateExpr;
        }, {});
        return dateTimeExpr(d);
    }
    function getDateTimeComponents(timeUnit, shortTimeLabels) {
        if (!timeUnit) {
            return undefined;
        }
        const dateComponents = [];
        const hasYear = containsTimeUnit(timeUnit, TimeUnit.YEAR);
        if (containsTimeUnit(timeUnit, TimeUnit.MONTH)) {
            // By default use short month name
            dateComponents.push(shortTimeLabels !== false ? '%b' : '%B');
        }
        if (containsTimeUnit(timeUnit, TimeUnit.DAY)) {
            dateComponents.push(shortTimeLabels ? '%a' : '%A');
        }
        else if (containsTimeUnit(timeUnit, TimeUnit.DATE)) {
            dateComponents.push('%d' + (hasYear ? ',' : '')); // add comma if there is year
        }
        if (hasYear) {
            dateComponents.push(shortTimeLabels ? '%y' : '%Y');
        }
        const timeComponents = [];
        if (containsTimeUnit(timeUnit, TimeUnit.HOURS)) {
            timeComponents.push('%H');
        }
        if (containsTimeUnit(timeUnit, TimeUnit.MINUTES)) {
            timeComponents.push('%M');
        }
        if (containsTimeUnit(timeUnit, TimeUnit.SECONDS)) {
            timeComponents.push('%S');
        }
        if (containsTimeUnit(timeUnit, TimeUnit.MILLISECONDS)) {
            timeComponents.push('%L');
        }
        const dateTimeComponents = [];
        if (dateComponents.length > 0) {
            dateTimeComponents.push(dateComponents.join(' '));
        }
        if (timeComponents.length > 0) {
            dateTimeComponents.push(timeComponents.join(':'));
        }
        return dateTimeComponents;
    }
    /**
     * returns the signal expression used for axis labels for a time unit
     */
    function formatExpression(timeUnit, field, shortTimeLabels, isUTCScale) {
        if (!timeUnit) {
            return undefined;
        }
        const dateTimeComponents = getDateTimeComponents(timeUnit, shortTimeLabels);
        let expression = '';
        if (containsTimeUnit(timeUnit, TimeUnit.QUARTER)) {
            // special expression for quarter as prefix
            expression = `'Q' + quarter(${field})`;
        }
        if (dateTimeComponents.length > 0) {
            if (expression) {
                // Add space between quarter and main time format
                expression += ` + ' ' + `;
            }
            // We only use utcFormat for utc scale
            // For utc time units, the data is already converted as a part of timeUnit transform.
            // Thus, utc time units should use timeFormat to avoid shifting the time twice.
            if (isUTCScale) {
                expression += `utcFormat(${field}, '${dateTimeComponents.join(' ')}')`;
            }
            else {
                expression += `timeFormat(${field}, '${dateTimeComponents.join(' ')}')`;
            }
        }
        // If expression is still an empty string, return undefined instead.
        return expression || undefined;
    }
    function normalizeTimeUnit(timeUnit) {
        if (timeUnit !== 'day' && timeUnit.indexOf('day') >= 0) {
            warn(message.dayReplacedWithDate(timeUnit));
            return timeUnit.replace('day', 'date');
        }
        return timeUnit;
    }

    /** Constants and utilities for data type */
    /** Data type based on level of measurement */
    const TYPE_INDEX = {
        quantitative: 1,
        ordinal: 1,
        temporal: 1,
        nominal: 1,
        geojson: 1
    };
    const QUANTITATIVE = 'quantitative';
    const ORDINAL = 'ordinal';
    const TEMPORAL = 'temporal';
    const NOMINAL = 'nominal';
    const GEOJSON = 'geojson';
    /**
     * Get full, lowercase type name for a given type.
     * @param  type
     * @return Full type name.
     */
    function getFullName(type) {
        if (type) {
            type = type.toLowerCase();
            switch (type) {
                case 'q':
                case QUANTITATIVE:
                    return 'quantitative';
                case 't':
                case TEMPORAL:
                    return 'temporal';
                case 'o':
                case ORDINAL:
                    return 'ordinal';
                case 'n':
                case NOMINAL:
                    return 'nominal';
                case GEOJSON:
                    return 'geojson';
            }
        }
        // If we get invalid input, return undefined type.
        return undefined;
    }

    function isConditionalSelection(c) {
        return c['selection'];
    }
    function isRepeatRef(field) {
        return field && !isString(field) && 'repeat' in field;
    }
    function toFieldDefBase(fieldDef) {
        const { field, timeUnit, bin, aggregate } = fieldDef;
        return Object.assign({}, (timeUnit ? { timeUnit } : {}), (bin ? { bin } : {}), (aggregate ? { aggregate } : {}), { field });
    }
    function isSortableFieldDef(fieldDef) {
        return isTypedFieldDef(fieldDef) && !!fieldDef['sort'];
    }
    function isConditionalDef(channelDef) {
        return !!channelDef && !!channelDef.condition;
    }
    /**
     * Return if a channelDef is a ConditionalValueDef with ConditionFieldDef
     */
    function hasConditionalFieldDef(channelDef) {
        return !!channelDef && !!channelDef.condition && !isArray(channelDef.condition) && isFieldDef(channelDef.condition);
    }
    function hasConditionalValueDef(channelDef) {
        return !!channelDef && !!channelDef.condition && (isArray(channelDef.condition) || isValueDef(channelDef.condition));
    }
    function isFieldDef(channelDef) {
        return !!channelDef && (!!channelDef['field'] || channelDef['aggregate'] === 'count');
    }
    function isTypedFieldDef(channelDef) {
        return !!channelDef && ((!!channelDef['field'] && !!channelDef['type']) || channelDef['aggregate'] === 'count');
    }
    function isStringFieldDef(channelDef) {
        return isFieldDef(channelDef) && isString(channelDef.field);
    }
    function isValueDef(channelDef) {
        return channelDef && 'value' in channelDef && channelDef['value'] !== undefined;
    }
    function isScaleFieldDef(channelDef) {
        return !!channelDef && (!!channelDef['scale'] || !!channelDef['sort']);
    }
    function isPositionFieldDef(channelDef) {
        return !!channelDef && (!!channelDef['axis'] || !!channelDef['stack'] || !!channelDef['impute']);
    }
    function isMarkPropFieldDef(channelDef) {
        return !!channelDef && !!channelDef['legend'];
    }
    function isTextFieldDef(channelDef) {
        return !!channelDef && !!channelDef['format'];
    }
    function isOpFieldDef(fieldDef) {
        return !!fieldDef['op'];
    }
    /**
     * Get a Vega field reference from a Vega-Lite field def.
     */
    function vgField(fieldDef, opt = {}) {
        let field = fieldDef.field;
        const prefix = opt.prefix;
        let suffix = opt.suffix;
        let argAccessor = ''; // for accessing argmin/argmax field at the end without getting escaped
        if (isCount(fieldDef)) {
            field = internalField('count');
        }
        else {
            let fn;
            if (!opt.nofn) {
                if (isOpFieldDef(fieldDef)) {
                    fn = fieldDef.op;
                }
                else {
                    const { bin, aggregate, timeUnit } = fieldDef;
                    if (isBinning(bin)) {
                        fn = binToString(bin);
                        suffix = (opt.binSuffix || '') + (opt.suffix || '');
                    }
                    else if (aggregate) {
                        if (isArgmaxDef(aggregate)) {
                            argAccessor = `.${field}`;
                            field = `argmax_${aggregate.argmax}`;
                        }
                        else if (isArgminDef(aggregate)) {
                            argAccessor = `.${field}`;
                            field = `argmin_${aggregate.argmin}`;
                        }
                        else {
                            fn = String(aggregate);
                        }
                    }
                    else if (timeUnit) {
                        fn = String(timeUnit);
                    }
                }
            }
            if (fn) {
                field = field ? `${fn}_${field}` : fn;
            }
        }
        if (suffix) {
            field = `${field}_${suffix}`;
        }
        if (prefix) {
            field = `${prefix}_${field}`;
        }
        if (opt.forAs) {
            return field;
        }
        else if (opt.expr) {
            // Expression to access flattened field. No need to escape dots.
            return flatAccessWithDatum(field, opt.expr) + argAccessor;
        }
        else {
            // We flattened all fields so paths should have become dot.
            return replacePathInField(field) + argAccessor;
        }
    }
    function isDiscrete(fieldDef) {
        switch (fieldDef.type) {
            case 'nominal':
            case 'ordinal':
            case 'geojson':
                return true;
            case 'quantitative':
                return !!fieldDef.bin;
            case 'temporal':
                return false;
        }
        throw new Error(message.invalidFieldType(fieldDef.type));
    }
    function isContinuous(fieldDef) {
        return !isDiscrete(fieldDef);
    }
    function isCount(fieldDef) {
        return fieldDef.aggregate === 'count';
    }
    function verbalTitleFormatter(fieldDef, config) {
        const { field, bin, timeUnit, aggregate } = fieldDef;
        if (aggregate === 'count') {
            return config.countTitle;
        }
        else if (isBinning(bin)) {
            return `${field} (binned)`;
        }
        else if (timeUnit) {
            const units = getTimeUnitParts(timeUnit).join('-');
            return `${field} (${units})`;
        }
        else if (aggregate) {
            if (isArgmaxDef(aggregate)) {
                return `${field} for max ${aggregate.argmax}`;
            }
            else if (isArgminDef(aggregate)) {
                return `${field} for min ${aggregate.argmin}`;
            }
            else {
                return `${titlecase(aggregate)} of ${field}`;
            }
        }
        return field;
    }
    function functionalTitleFormatter(fieldDef) {
        const { aggregate, bin, timeUnit, field } = fieldDef;
        if (isArgmaxDef(aggregate)) {
            return `${field} for argmax(${aggregate.argmax})`;
        }
        else if (isArgminDef(aggregate)) {
            return `${field} for argmin(${aggregate.argmin})`;
        }
        const fn = aggregate || timeUnit || (isBinning(bin) && 'bin');
        if (fn) {
            return fn.toUpperCase() + '(' + field + ')';
        }
        else {
            return field;
        }
    }
    const defaultTitleFormatter = (fieldDef, config) => {
        switch (config.fieldTitle) {
            case 'plain':
                return fieldDef.field;
            case 'functional':
                return functionalTitleFormatter(fieldDef);
            default:
                return verbalTitleFormatter(fieldDef, config);
        }
    };
    let titleFormatter = defaultTitleFormatter;
    function setTitleFormatter(formatter) {
        titleFormatter = formatter;
    }
    function resetTitleFormatter() {
        setTitleFormatter(defaultTitleFormatter);
    }
    function title(fieldDef, config, { allowDisabling, includeDefault = true }) {
        const guide = getGuide(fieldDef) || {};
        const guideTitle = guide.title;
        const def = includeDefault ? defaultTitle(fieldDef, config) : undefined;
        if (allowDisabling) {
            return getFirstDefined(guideTitle, fieldDef.title, def);
        }
        else {
            return guideTitle || fieldDef.title || def;
        }
    }
    function getGuide(fieldDef) {
        if (isPositionFieldDef(fieldDef) && fieldDef.axis) {
            return fieldDef.axis;
        }
        else if (isMarkPropFieldDef(fieldDef) && fieldDef.legend) {
            return fieldDef.legend;
        }
        else if (isFacetFieldDef(fieldDef) && fieldDef.header) {
            return fieldDef.header;
        }
        return undefined;
    }
    function defaultTitle(fieldDef, config) {
        return titleFormatter(fieldDef, config);
    }
    function format(fieldDef) {
        if (isTextFieldDef(fieldDef) && fieldDef.format) {
            return fieldDef.format;
        }
        else {
            const guide = getGuide(fieldDef) || {};
            return guide.format;
        }
    }
    function defaultType(fieldDef, channel) {
        if (fieldDef.timeUnit) {
            return 'temporal';
        }
        if (isBinning(fieldDef.bin)) {
            return 'quantitative';
        }
        switch (rangeType(channel)) {
            case 'continuous':
                return 'quantitative';
            case 'discrete':
                return 'nominal';
            case 'flexible': // color
                return 'nominal';
            default:
                return 'quantitative';
        }
    }
    /**
     * Returns the fieldDef -- either from the outer channelDef or from the condition of channelDef.
     * @param channelDef
     */
    function getFieldDef(channelDef) {
        if (isFieldDef(channelDef)) {
            return channelDef;
        }
        else if (hasConditionalFieldDef(channelDef)) {
            return channelDef.condition;
        }
        return undefined;
    }
    function getTypedFieldDef(channelDef) {
        if (isFieldDef(channelDef)) {
            return channelDef;
        }
        else if (hasConditionalFieldDef(channelDef)) {
            return channelDef.condition;
        }
        return undefined;
    }
    /**
     * Convert type to full, lowercase type, or augment the fieldDef with a default type if missing.
     */
    function normalize(channelDef, channel) {
        if (isString(channelDef) || isNumber(channelDef) || isBoolean(channelDef)) {
            const primitiveType = isString(channelDef) ? 'string' : isNumber(channelDef) ? 'number' : 'boolean';
            warn(message.primitiveChannelDef(channel, primitiveType, channelDef));
            return { value: channelDef };
        }
        // If a fieldDef contains a field, we need type.
        if (isFieldDef(channelDef)) {
            return normalizeFieldDef(channelDef, channel);
        }
        else if (hasConditionalFieldDef(channelDef)) {
            return Object.assign({}, channelDef, { 
                // Need to cast as normalizeFieldDef normally return FieldDef, but here we know that it is definitely Condition<FieldDef>
                condition: normalizeFieldDef(channelDef.condition, channel) });
        }
        return channelDef;
    }
    function normalizeFieldDef(fieldDef, channel) {
        const { aggregate, timeUnit, bin } = fieldDef;
        // Drop invalid aggregate
        if (aggregate && !isAggregateOp(aggregate) && !isArgmaxDef(aggregate) && !isArgminDef(aggregate)) {
            const fieldDefWithoutAggregate = __rest(fieldDef, ["aggregate"]);
            warn(message.invalidAggregate(aggregate));
            fieldDef = fieldDefWithoutAggregate;
        }
        // Normalize Time Unit
        if (timeUnit) {
            fieldDef = Object.assign({}, fieldDef, { timeUnit: normalizeTimeUnit(timeUnit) });
        }
        // Normalize bin
        if (isBinning(bin)) {
            fieldDef = Object.assign({}, fieldDef, { bin: normalizeBin(bin, channel) });
        }
        if (isBinned(bin) && !contains(POSITION_SCALE_CHANNELS, channel)) {
            warn(`Channel ${channel} should not be used with "binned" bin`);
        }
        // Normalize Type
        if (isTypedFieldDef(fieldDef)) {
            const { type } = fieldDef;
            const fullType = getFullName(type);
            if (type !== fullType) {
                // convert short type to full type
                fieldDef = Object.assign({}, fieldDef, { type: fullType });
            }
            if (type !== 'quantitative') {
                if (isCountingAggregateOp(aggregate)) {
                    warn(message.invalidFieldTypeForCountAggregate(type, aggregate));
                    fieldDef = Object.assign({}, fieldDef, { type: 'quantitative' });
                }
            }
        }
        else if (!isSecondaryRangeChannel(channel)) {
            // If type is empty / invalid, then augment with default type
            const newType = defaultType(fieldDef, channel);
            warn(message.missingFieldType(channel, newType));
            fieldDef = Object.assign({}, fieldDef, { type: newType });
        }
        if (isTypedFieldDef(fieldDef)) {
            const { compatible, warning } = channelCompatibility(fieldDef, channel);
            if (!compatible) {
                warn(warning);
            }
        }
        return fieldDef;
    }
    function normalizeBin(bin, channel) {
        if (isBoolean(bin)) {
            return { maxbins: autoMaxBins(channel) };
        }
        else if (bin === 'binned') {
            return {
                binned: true
            };
        }
        else if (!bin.maxbins && !bin.step) {
            return Object.assign({}, bin, { maxbins: autoMaxBins(channel) });
        }
        else {
            return bin;
        }
    }
    const COMPATIBLE = { compatible: true };
    function channelCompatibility(fieldDef, channel) {
        const type = fieldDef.type;
        if (type === 'geojson' && channel !== 'shape') {
            return {
                compatible: false,
                warning: `Channel ${channel} should not be used with a geojson data.`
            };
        }
        switch (channel) {
            case 'row':
            case 'column':
            case 'facet':
                if (isContinuous(fieldDef)) {
                    return {
                        compatible: false,
                        warning: message.facetChannelShouldBeDiscrete(channel)
                    };
                }
                return COMPATIBLE;
            case 'x':
            case 'y':
            case 'color':
            case 'fill':
            case 'stroke':
            case 'text':
            case 'detail':
            case 'key':
            case 'tooltip':
            case 'href':
                return COMPATIBLE;
            case 'longitude':
            case 'longitude2':
            case 'latitude':
            case 'latitude2':
                if (type !== QUANTITATIVE) {
                    return {
                        compatible: false,
                        warning: `Channel ${channel} should be used with a quantitative field only, not ${fieldDef.type} field.`
                    };
                }
                return COMPATIBLE;
            case 'opacity':
            case 'fillOpacity':
            case 'strokeOpacity':
            case 'strokeWidth':
            case 'size':
            case 'x2':
            case 'y2':
                if (type === 'nominal' && !fieldDef['sort']) {
                    return {
                        compatible: false,
                        warning: `Channel ${channel} should not be used with an unsorted discrete field.`
                    };
                }
                return COMPATIBLE;
            case 'shape':
                if (!contains(['ordinal', 'nominal', 'geojson'], fieldDef.type)) {
                    return {
                        compatible: false,
                        warning: 'Shape channel should be used with only either discrete or geojson data.'
                    };
                }
                return COMPATIBLE;
            case 'order':
                if (fieldDef.type === 'nominal' && !('sort' in fieldDef)) {
                    return {
                        compatible: false,
                        warning: `Channel order is inappropriate for nominal field, which has no inherent order.`
                    };
                }
                return COMPATIBLE;
        }
        throw new Error('channelCompatability not implemented for channel ' + channel);
    }
    function isNumberFieldDef(fieldDef) {
        return fieldDef.type === 'quantitative' || isBinning(fieldDef.bin);
    }
    /**
     * Check if the field def uses a time format or does not use any format but is temporal (this does not cover field defs that are temporal but use a number format).
     */
    function isTimeFormatFieldDef(fieldDef) {
        const formatType = (isPositionFieldDef(fieldDef) && fieldDef.axis && fieldDef.axis.formatType) ||
            (isMarkPropFieldDef(fieldDef) && fieldDef.legend && fieldDef.legend.formatType) ||
            (isTextFieldDef(fieldDef) && fieldDef.formatType);
        return formatType === 'time' || (!formatType && isTimeFieldDef(fieldDef));
    }
    /**
     * Check if field def has tye `temporal`. If you want to also cover field defs that use a time format, use `isTimeFormatFieldDef`.
     */
    function isTimeFieldDef(fieldDef) {
        return fieldDef.type === 'temporal' || !!fieldDef.timeUnit;
    }
    /**
     * Getting a value associated with a fielddef.
     * Convert the value to Vega expression if applicable (for datetime object, or string if the field def is temporal or has timeUnit)
     */
    function valueExpr(v, { timeUnit, type, time, undefinedIfExprNotRequired }) {
        let expr;
        if (isDateTime(v)) {
            expr = dateTimeExpr(v, true);
        }
        else if (isString(v) || isNumber(v)) {
            if (timeUnit || type === 'temporal') {
                if (isLocalSingleTimeUnit(timeUnit)) {
                    expr = dateTimeExpr({ [timeUnit]: v }, true);
                }
                else if (isUtcSingleTimeUnit(timeUnit)) {
                    // FIXME is this really correct?
                    expr = valueExpr(v, { timeUnit: getLocalTimeUnit(timeUnit) });
                }
                else {
                    // just pass the string to date function (which will call JS Date.parse())
                    expr = `datetime(${JSON.stringify(v)})`;
                }
            }
        }
        if (expr) {
            return time ? `time(${expr})` : expr;
        }
        // number or boolean or normal string
        return undefinedIfExprNotRequired ? undefined : JSON.stringify(v);
    }
    /**
     * Standardize value array -- convert each value to Vega expression if applicable
     */
    function valueArray(fieldDef, values) {
        const { timeUnit, type } = fieldDef;
        return values.map(v => {
            const expr = valueExpr(v, { timeUnit, type, undefinedIfExprNotRequired: true });
            // return signal for the expression if we need an expression
            if (expr !== undefined) {
                return { signal: expr };
            }
            // otherwise just return the original value
            return v;
        });
    }
    /**
     * Checks whether a fieldDef for a particular channel requires a computed bin range.
     */
    function binRequiresRange(fieldDef, channel) {
        if (!isBinning(fieldDef.bin)) {
            console.warn('Only use this method with binned field defs');
            return false;
        }
        // We need the range only when the user explicitly forces a binned field to be use discrete scale. In this case, bin range is used in axis and legend labels.
        // We could check whether the axis or legend exists (not disabled) but that seems overkill.
        return isScaleChannel(channel) && contains(['ordinal', 'nominal'], fieldDef.type);
    }

    /**
     * Create a key for the bin configuration. Not for prebinned bin.
     */
    function binToString(bin) {
        if (isBoolean(bin)) {
            bin = normalizeBin(bin, undefined);
        }
        return ('bin' +
            keys(bin)
                .map(p => varName(`_${p}_${bin[p]}`))
                .join(''));
    }
    /**
     * Vega-Lite should bin the data.
     */
    function isBinning(bin) {
        return bin === true || (isBinParams(bin) && !bin.binned);
    }
    /**
     * The data is already binned and so Vega-Lite should not bin it again.
     */
    function isBinned(bin) {
        return bin === 'binned' || (isBinParams(bin) && bin.binned);
    }
    function isBinParams(bin) {
        return isObject(bin);
    }
    function autoMaxBins(channel) {
        switch (channel) {
            case ROW:
            case COLUMN:
            case SIZE:
            case COLOR:
            case FILL:
            case STROKE:
            case STROKEWIDTH:
            case OPACITY:
            case FILLOPACITY:
            case STROKEOPACITY:
            // Facets and Size shouldn't have too many bins
            // We choose 6 like shape to simplify the rule [falls through]
            case SHAPE:
                return 6; // Vega's "shape" has 6 distinct values
            default:
                return 10;
        }
    }

    function channelHasField(encoding, channel) {
        const channelDef = encoding && encoding[channel];
        if (channelDef) {
            if (isArray(channelDef)) {
                return some(channelDef, fieldDef => !!fieldDef.field);
            }
            else {
                return isFieldDef(channelDef) || hasConditionalFieldDef(channelDef);
            }
        }
        return false;
    }
    function isAggregate(encoding) {
        return some(CHANNELS, channel => {
            if (channelHasField(encoding, channel)) {
                const channelDef = encoding[channel];
                if (isArray(channelDef)) {
                    return some(channelDef, fieldDef => !!fieldDef.aggregate);
                }
                else {
                    const fieldDef = getFieldDef(channelDef);
                    return fieldDef && !!fieldDef.aggregate;
                }
            }
            return false;
        });
    }
    function extractTransformsFromEncoding(oldEncoding, config) {
        const groupby = [];
        const bins = [];
        const timeUnits = [];
        const aggregate = [];
        const encoding = {};
        forEach(oldEncoding, (channelDef, channel) => {
            // Extract potential embedded transformations along with remaining properties
            if (isFieldDef(channelDef)) {
                const { field, aggregate: aggOp, timeUnit, bin } = channelDef, remaining = __rest(channelDef, ["field", "aggregate", "timeUnit", "bin"]);
                if (aggOp || timeUnit || bin) {
                    const guide = getGuide(channelDef);
                    const isTitleDefined = guide && guide.title;
                    let newField = vgField(channelDef, { forAs: true });
                    const newFieldDef = Object.assign({}, (isTitleDefined ? [] : { title: title(channelDef, config, { allowDisabling: true }) }), remaining, { 
                        // Always overwrite field
                        field: newField });
                    const isPositionChannel = channel === 'x' || channel === 'y';
                    if (aggOp) {
                        let op;
                        if (isArgmaxDef(aggOp)) {
                            op = 'argmax';
                            newField = vgField({ aggregate: 'argmax', field: aggOp.argmax }, { forAs: true });
                            newFieldDef.field = `${newField}.${field}`;
                        }
                        else if (isArgminDef(aggOp)) {
                            op = 'argmin';
                            newField = vgField({ aggregate: 'argmin', field: aggOp.argmin }, { forAs: true });
                            newFieldDef.field = `${newField}.${field}`;
                        }
                        else if (aggOp !== 'boxplot' && aggOp !== 'errorbar' && aggOp !== 'errorband') {
                            op = aggOp;
                        }
                        if (op) {
                            const aggregateEntry = {
                                op,
                                as: newField
                            };
                            if (field) {
                                aggregateEntry.field = field;
                            }
                            aggregate.push(aggregateEntry);
                        }
                    }
                    else if (isTypedFieldDef(channelDef) && isBinning(bin)) {
                        bins.push({ bin, field, as: newField });
                        // Add additional groupbys for range and end of bins
                        groupby.push(vgField(channelDef, { binSuffix: 'end' }));
                        if (binRequiresRange(channelDef, channel)) {
                            groupby.push(vgField(channelDef, { binSuffix: 'range' }));
                        }
                        // Create accompanying 'x2' or 'y2' field if channel is 'x' or 'y' respectively
                        if (isPositionChannel) {
                            const secondaryChannel = {
                                field: newField + '_end'
                            };
                            encoding[channel + '2'] = secondaryChannel;
                        }
                        newFieldDef.bin = 'binned';
                        if (!isSecondaryRangeChannel(channel)) {
                            newFieldDef['type'] = 'quantitative';
                        }
                    }
                    else if (timeUnit) {
                        timeUnits.push({ timeUnit, field, as: newField });
                        // Add formatting to appropriate property based on the type of channel we're processing
                        const format = getDateTimeComponents(timeUnit, config.axis.shortTimeLabels).join(' ');
                        const formatType = isTypedFieldDef(channelDef) && channelDef.type !== TEMPORAL && 'time';
                        if (channel === 'text' || channel === 'tooltip') {
                            newFieldDef['format'] = newFieldDef['format'] || format;
                            if (formatType) {
                                newFieldDef['formatType'] = formatType;
                            }
                        }
                        else if (isNonPositionScaleChannel(channel)) {
                            newFieldDef['legend'] = Object.assign({ format }, (formatType ? { formatType } : {}), newFieldDef['legend']);
                        }
                        else if (isPositionChannel) {
                            newFieldDef['axis'] = Object.assign({ format }, (formatType ? { formatType } : {}), newFieldDef['axis']);
                        }
                    }
                    if (!aggOp) {
                        groupby.push(newField);
                    }
                    // now the field should refer to post-transformed field instead
                    encoding[channel] = newFieldDef;
                }
                else {
                    groupby.push(field);
                    encoding[channel] = oldEncoding[channel];
                }
            }
            else {
                // For value def, just copy
                encoding[channel] = oldEncoding[channel];
            }
        });
        return {
            bins,
            timeUnits,
            aggregate,
            groupby,
            encoding
        };
    }
    function markChannelCompatible(encoding, channel, mark) {
        const markSupported = supportMark(channel, mark);
        if (!markSupported) {
            return false;
        }
        else if (markSupported === 'binned') {
            const primaryFieldDef = encoding[channel === 'x2' ? 'x' : 'y'];
            // circle, point, square and tick only support x2/y2 when their corresponding x/y fieldDef
            // has "binned" data and thus need x2/y2 to specify the bin-end field.
            if (isFieldDef(primaryFieldDef) && isFieldDef(encoding[channel]) && primaryFieldDef.bin === 'binned') {
                return true;
            }
            else {
                return false;
            }
        }
        return true;
    }
    function normalizeEncoding(encoding, mark) {
        return keys(encoding).reduce((normalizedEncoding, channel) => {
            if (!isChannel(channel)) {
                // Drop invalid channel
                warn(message.invalidEncodingChannel(channel));
                return normalizedEncoding;
            }
            if (!markChannelCompatible(encoding, channel, mark)) {
                // Drop unsupported channel
                warn(message.incompatibleChannel(channel, mark));
                return normalizedEncoding;
            }
            // Drop line's size if the field is aggregated.
            if (channel === 'size' && mark === 'line') {
                const fieldDef = getTypedFieldDef(encoding[channel]);
                if (fieldDef && fieldDef.aggregate) {
                    warn(message.LINE_WITH_VARYING_SIZE);
                    return normalizedEncoding;
                }
            }
            // Drop color if either fill or stroke is specified
            if (channel === 'color' && ('fill' in encoding || 'stroke' in encoding)) {
                warn(message.droppingColor('encoding', { fill: 'fill' in encoding, stroke: 'stroke' in encoding }));
                return normalizedEncoding;
            }
            const channelDef = encoding[channel];
            if (channel === 'detail' ||
                (channel === 'order' && !isArray(channelDef) && !isValueDef(channelDef)) ||
                (channel === 'tooltip' && isArray(channelDef))) {
                if (channelDef) {
                    // Array of fieldDefs for detail channel (or production rule)
                    normalizedEncoding[channel] = (isArray(channelDef) ? channelDef : [channelDef]).reduce((defs, fieldDef) => {
                        if (!isFieldDef(fieldDef)) {
                            warn(message.emptyFieldDef(fieldDef, channel));
                        }
                        else {
                            defs.push(normalizeFieldDef(fieldDef, channel));
                        }
                        return defs;
                    }, []);
                }
            }
            else {
                if (channel === 'tooltip' && channelDef === null) {
                    // Preserve null so we can use it to disable tooltip
                    normalizedEncoding[channel] = null;
                }
                else if (!isFieldDef(channelDef) && !isValueDef(channelDef) && !isConditionalDef(channelDef)) {
                    warn(message.emptyFieldDef(channelDef, channel));
                    return normalizedEncoding;
                }
                normalizedEncoding[channel] = normalize(channelDef, channel);
            }
            return normalizedEncoding;
        }, {});
    }
    function fieldDefs(encoding) {
        const arr = [];
        for (const channel of keys(encoding)) {
            if (channelHasField(encoding, channel)) {
                const channelDef = encoding[channel];
                (isArray(channelDef) ? channelDef : [channelDef]).forEach(def => {
                    if (isFieldDef(def)) {
                        arr.push(def);
                    }
                    else if (hasConditionalFieldDef(def)) {
                        arr.push(def.condition);
                    }
                });
            }
        }
        return arr;
    }
    function forEach(mapping, f, thisArg) {
        if (!mapping) {
            return;
        }
        for (const channel of keys(mapping)) {
            const el = mapping[channel];
            if (isArray(el)) {
                el.forEach((channelDef) => {
                    f.call(thisArg, channelDef, channel);
                });
            }
            else {
                f.call(thisArg, el, channel);
            }
        }
    }
    function reduce(mapping, f, init, thisArg) {
        if (!mapping) {
            return init;
        }
        return keys(mapping).reduce((r, channel) => {
            const map = mapping[channel];
            if (isArray(map)) {
                return map.reduce((r1, channelDef) => {
                    return f.call(thisArg, r1, channelDef, channel);
                }, r);
            }
            else {
                return f.call(thisArg, r, map, channel);
            }
        }, init);
    }

    function filterTooltipWithAggregatedField(oldEncoding) {
        const { tooltip } = oldEncoding, filteredEncoding = __rest(oldEncoding, ["tooltip"]);
        if (!tooltip) {
            return { filteredEncoding: oldEncoding };
        }
        let customTooltipWithAggregatedField;
        let customTooltipWithoutAggregatedField;
        if (isArray(tooltip)) {
            tooltip.forEach((t) => {
                if (t.aggregate) {
                    if (!customTooltipWithAggregatedField) {
                        customTooltipWithAggregatedField = [];
                    }
                    customTooltipWithAggregatedField.push(t);
                }
                else {
                    if (!customTooltipWithoutAggregatedField) {
                        customTooltipWithoutAggregatedField = [];
                    }
                    customTooltipWithoutAggregatedField.push(t);
                }
            });
            if (customTooltipWithAggregatedField) {
                filteredEncoding.tooltip = customTooltipWithAggregatedField;
            }
        }
        else {
            if (tooltip['aggregate']) {
                filteredEncoding.tooltip = tooltip;
            }
            else {
                customTooltipWithoutAggregatedField = tooltip;
            }
        }
        if (isArray(customTooltipWithoutAggregatedField) && customTooltipWithoutAggregatedField.length === 1) {
            customTooltipWithoutAggregatedField = customTooltipWithoutAggregatedField[0];
        }
        return { customTooltipWithoutAggregatedField, filteredEncoding };
    }
    function getCompositeMarkTooltip(tooltipSummary, continuousAxisChannelDef, encodingWithoutContinuousAxis, withFieldName = true) {
        const fiveSummaryTooltip = tooltipSummary.map(({ fieldPrefix, titlePrefix }) => ({
            field: fieldPrefix + continuousAxisChannelDef.field,
            type: continuousAxisChannelDef.type,
            title: titlePrefix + (withFieldName ? ' of ' + continuousAxisChannelDef.field : '')
        }));
        return {
            tooltip: [
                ...fiveSummaryTooltip,
                // need to cast because TextFieldDef support fewer types of bin
                ...fieldDefs(encodingWithoutContinuousAxis)
            ]
        };
    }
    function makeCompositeAggregatePartFactory(compositeMarkDef, continuousAxis, continuousAxisChannelDef, sharedEncoding, compositeMarkConfig) {
        const { scale, axis } = continuousAxisChannelDef;
        return ({ partName, mark, positionPrefix, endPositionPrefix = undefined, extraEncoding = {} }) => {
            const title = axis && axis.title !== undefined
                ? undefined
                : continuousAxisChannelDef.title !== undefined
                    ? continuousAxisChannelDef.title
                    : continuousAxisChannelDef.field;
            return partLayerMixins(compositeMarkDef, partName, compositeMarkConfig, {
                mark,
                encoding: Object.assign({ [continuousAxis]: Object.assign({ field: positionPrefix + '_' + continuousAxisChannelDef.field, type: continuousAxisChannelDef.type }, (title ? { title } : {}), (scale ? { scale } : {}), (axis ? { axis } : {})) }, (isString(endPositionPrefix)
                    ? {
                        [continuousAxis + '2']: {
                            field: endPositionPrefix + '_' + continuousAxisChannelDef.field,
                            type: continuousAxisChannelDef.type
                        }
                    }
                    : {}), sharedEncoding, extraEncoding)
            });
        };
    }
    function partLayerMixins(markDef, part, compositeMarkConfig, partBaseSpec) {
        const { clip, color, opacity } = markDef;
        const mark = markDef.type;
        if (markDef[part] || (markDef[part] === undefined && compositeMarkConfig[part])) {
            return [
                Object.assign({}, partBaseSpec, { mark: Object.assign({}, compositeMarkConfig[part], (clip ? { clip } : {}), (color ? { color } : {}), (opacity ? { opacity } : {}), (isMarkDef(partBaseSpec.mark) ? partBaseSpec.mark : { type: partBaseSpec.mark }), { style: `${mark}-${part}` }, (isBoolean(markDef[part]) ? {} : markDef[part])) })
            ];
        }
        return [];
    }
    function compositeMarkContinuousAxis(spec, orient, compositeMark) {
        const { encoding } = spec;
        const continuousAxis = orient === 'vertical' ? 'y' : 'x';
        const continuousAxisChannelDef = encoding[continuousAxis]; // Safe to cast because if x is not continuous fielddef, the orient would not be horizontal.
        const continuousAxisChannelDef2 = encoding[continuousAxis + '2'];
        const continuousAxisChannelDefError = encoding[continuousAxis + 'Error'];
        const continuousAxisChannelDefError2 = encoding[continuousAxis + 'Error2'];
        return {
            continuousAxisChannelDef: filterAggregateFromChannelDef(continuousAxisChannelDef, compositeMark),
            continuousAxisChannelDef2: filterAggregateFromChannelDef(continuousAxisChannelDef2, compositeMark),
            continuousAxisChannelDefError: filterAggregateFromChannelDef(continuousAxisChannelDefError, compositeMark),
            continuousAxisChannelDefError2: filterAggregateFromChannelDef(continuousAxisChannelDefError2, compositeMark),
            continuousAxis
        };
    }
    function filterAggregateFromChannelDef(continuousAxisChannelDef, compositeMark) {
        if (continuousAxisChannelDef && continuousAxisChannelDef.aggregate) {
            const { aggregate } = continuousAxisChannelDef, continuousAxisWithoutAggregate = __rest(continuousAxisChannelDef, ["aggregate"]);
            if (aggregate !== compositeMark) {
                warn(message.errorBarContinuousAxisHasCustomizedAggregate(aggregate, compositeMark));
            }
            return continuousAxisWithoutAggregate;
        }
        else {
            return continuousAxisChannelDef;
        }
    }
    function compositeMarkOrient(spec, compositeMark) {
        const { mark, encoding } = spec;
        if (isFieldDef(encoding.x) && isContinuous(encoding.x)) {
            // x is continuous
            if (isFieldDef(encoding.y) && isContinuous(encoding.y)) {
                // both x and y are continuous
                if (encoding.x.aggregate === undefined && encoding.y.aggregate === compositeMark) {
                    return 'vertical';
                }
                else if (encoding.y.aggregate === undefined && encoding.x.aggregate === compositeMark) {
                    return 'horizontal';
                }
                else if (encoding.x.aggregate === compositeMark && encoding.y.aggregate === compositeMark) {
                    throw new Error('Both x and y cannot have aggregate');
                }
                else {
                    if (isMarkDef(mark) && mark.orient) {
                        return mark.orient;
                    }
                    // default orientation = vertical
                    return 'vertical';
                }
            }
            // x is continuous but y is not
            return 'horizontal';
        }
        else if (isFieldDef(encoding.y) && isContinuous(encoding.y)) {
            // y is continuous but x is not
            return 'vertical';
        }
        else {
            // Neither x nor y is continuous.
            throw new Error('Need a valid continuous axis for ' + compositeMark + 's');
        }
    }

    const BOXPLOT = 'boxplot';
    const BOXPLOT_PART_INDEX = {
        box: 1,
        median: 1,
        outliers: 1,
        rule: 1,
        ticks: 1
    };
    const BOXPLOT_PARTS = keys(BOXPLOT_PART_INDEX);
    const boxPlotNormalizer = new CompositeMarkNormalizer(BOXPLOT, normalizeBoxPlot);
    function getBoxPlotType(extent) {
        if (isNumber(extent)) {
            return 'tukey';
        }
        // Ham: If we ever want to, we could add another extent syntax `{kIQR: number}` for the original [Q1-k*IQR, Q3+k*IQR] whisker and call this boxPlotType = `kIQR`.  However, I'm not exposing this for now.
        return extent;
    }
    function normalizeBoxPlot(spec, { config }) {
        // TODO: use selection
        const { mark, encoding: _encoding, selection, projection: _p } = spec, outerSpec = __rest(spec, ["mark", "encoding", "selection", "projection"]);
        const markDef = isMarkDef(mark) ? mark : { type: mark };
        // TODO(https://github.com/vega/vega-lite/issues/3702): add selection support
        if (selection) {
            warn(message.selectionNotSupported('boxplot'));
        }
        const extent = markDef.extent || config.boxplot.extent;
        const sizeValue = getFirstDefined(markDef.size, config.boxplot.size);
        const boxPlotType = getBoxPlotType(extent);
        const { transform, continuousAxisChannelDef, continuousAxis, groupby, aggregate, encodingWithoutContinuousAxis, ticksOrient, customTooltipWithoutAggregatedField } = boxParams(spec, extent, config);
        const { color, size } = encodingWithoutContinuousAxis, encodingWithoutSizeColorAndContinuousAxis = __rest(encodingWithoutContinuousAxis, ["color", "size"]);
        const makeBoxPlotPart = (sharedEncoding) => {
            return makeCompositeAggregatePartFactory(markDef, continuousAxis, continuousAxisChannelDef, sharedEncoding, config.boxplot);
        };
        const makeBoxPlotExtent = makeBoxPlotPart(encodingWithoutSizeColorAndContinuousAxis);
        const makeBoxPlotBox = makeBoxPlotPart(encodingWithoutContinuousAxis);
        const makeBoxPlotMidTick = makeBoxPlotPart(Object.assign({}, encodingWithoutSizeColorAndContinuousAxis, (size ? { size } : {})));
        const fiveSummaryTooltipEncoding = getCompositeMarkTooltip([
            { fieldPrefix: boxPlotType === 'min-max' ? 'upper_whisker_' : 'max_', titlePrefix: 'Max' },
            { fieldPrefix: 'upper_box_', titlePrefix: 'Q3' },
            { fieldPrefix: 'mid_box_', titlePrefix: 'Median' },
            { fieldPrefix: 'lower_box_', titlePrefix: 'Q1' },
            { fieldPrefix: boxPlotType === 'min-max' ? 'lower_whisker_' : 'min_', titlePrefix: 'Min' }
        ], continuousAxisChannelDef, encodingWithoutContinuousAxis);
        // ## Whisker Layers
        const endTick = { type: 'tick', color: 'black', opacity: 1, orient: ticksOrient };
        const whiskerTooltipEncoding = boxPlotType === 'min-max'
            ? fiveSummaryTooltipEncoding // for min-max, show five-summary tooltip for whisker
            : // for tukey / k-IQR, just show upper/lower-whisker
                getCompositeMarkTooltip([
                    { fieldPrefix: 'upper_whisker_', titlePrefix: 'Upper Whisker' },
                    { fieldPrefix: 'lower_whisker_', titlePrefix: 'Lower Whisker' }
                ], continuousAxisChannelDef, encodingWithoutContinuousAxis);
        const whiskerLayers = [
            ...makeBoxPlotExtent({
                partName: 'rule',
                mark: 'rule',
                positionPrefix: 'lower_whisker',
                endPositionPrefix: 'lower_box',
                extraEncoding: whiskerTooltipEncoding
            }),
            ...makeBoxPlotExtent({
                partName: 'rule',
                mark: 'rule',
                positionPrefix: 'upper_box',
                endPositionPrefix: 'upper_whisker',
                extraEncoding: whiskerTooltipEncoding
            }),
            ...makeBoxPlotExtent({
                partName: 'ticks',
                mark: endTick,
                positionPrefix: 'lower_whisker',
                extraEncoding: whiskerTooltipEncoding
            }),
            ...makeBoxPlotExtent({
                partName: 'ticks',
                mark: endTick,
                positionPrefix: 'upper_whisker',
                extraEncoding: whiskerTooltipEncoding
            })
        ];
        // ## Box Layers
        // TODO: support hiding certain mark parts
        const boxLayers = [
            ...(boxPlotType !== 'tukey' ? whiskerLayers : []),
            ...makeBoxPlotBox({
                partName: 'box',
                mark: Object.assign({ type: 'bar' }, (sizeValue ? { size: sizeValue } : {})),
                positionPrefix: 'lower_box',
                endPositionPrefix: 'upper_box',
                extraEncoding: fiveSummaryTooltipEncoding
            }),
            ...makeBoxPlotMidTick({
                partName: 'median',
                mark: Object.assign({ type: 'tick' }, (isObject(config.boxplot.median) && config.boxplot.median.color ? { color: config.boxplot.median.color } : {}), (sizeValue ? { size: sizeValue } : {}), { orient: ticksOrient }),
                positionPrefix: 'mid_box',
                extraEncoding: fiveSummaryTooltipEncoding
            })
        ];
        // ## Filtered Layers
        let filteredLayersMixins;
        if (boxPlotType !== 'min-max') {
            const lowerBoxExpr = `datum["lower_box_${continuousAxisChannelDef.field}"]`;
            const upperBoxExpr = `datum["upper_box_${continuousAxisChannelDef.field}"]`;
            const iqrExpr = `(${upperBoxExpr} - ${lowerBoxExpr})`;
            const lowerWhiskerExpr = `${lowerBoxExpr} - ${extent} * ${iqrExpr}`;
            const upperWhiskerExpr = `${upperBoxExpr} + ${extent} * ${iqrExpr}`;
            const fieldExpr = `datum["${continuousAxisChannelDef.field}"]`;
            const joinaggregateTransform = {
                joinaggregate: boxParamsQuartiles(continuousAxisChannelDef.field),
                groupby
            };
            let filteredWhiskerSpec = undefined;
            if (boxPlotType === 'tukey') {
                filteredWhiskerSpec = {
                    transform: [
                        {
                            filter: `(${lowerWhiskerExpr} <= ${fieldExpr}) && (${fieldExpr} <= ${upperWhiskerExpr})`
                        },
                        {
                            aggregate: [
                                {
                                    op: 'min',
                                    field: continuousAxisChannelDef.field,
                                    as: 'lower_whisker_' + continuousAxisChannelDef.field
                                },
                                {
                                    op: 'max',
                                    field: continuousAxisChannelDef.field,
                                    as: 'upper_whisker_' + continuousAxisChannelDef.field
                                },
                                // preserve lower_box / upper_box
                                {
                                    op: 'min',
                                    field: 'lower_box_' + continuousAxisChannelDef.field,
                                    as: 'lower_box_' + continuousAxisChannelDef.field
                                },
                                {
                                    op: 'max',
                                    field: 'upper_box_' + continuousAxisChannelDef.field,
                                    as: 'upper_box_' + continuousAxisChannelDef.field
                                },
                                ...aggregate
                            ],
                            groupby
                        }
                    ],
                    layer: whiskerLayers
                };
            }
            const encodingWithoutSizeColorContinuousAxisAndTooltip = __rest(encodingWithoutSizeColorAndContinuousAxis, ["tooltip"]);
            const outlierLayersMixins = partLayerMixins(markDef, 'outliers', config.boxplot, {
                transform: [{ filter: `(${fieldExpr} < ${lowerWhiskerExpr}) || (${fieldExpr} > ${upperWhiskerExpr})` }],
                mark: 'point',
                encoding: Object.assign({ [continuousAxis]: {
                        field: continuousAxisChannelDef.field,
                        type: continuousAxisChannelDef.type
                    } }, encodingWithoutSizeColorContinuousAxisAndTooltip, (customTooltipWithoutAggregatedField ? { tooltip: customTooltipWithoutAggregatedField } : {}))
            })[0];
            if (outlierLayersMixins && filteredWhiskerSpec) {
                filteredLayersMixins = {
                    transform: [joinaggregateTransform],
                    layer: [outlierLayersMixins, filteredWhiskerSpec]
                };
            }
            else if (outlierLayersMixins) {
                filteredLayersMixins = outlierLayersMixins;
                filteredLayersMixins.transform.unshift(joinaggregateTransform);
            }
            else if (filteredWhiskerSpec) {
                filteredLayersMixins = filteredWhiskerSpec;
                filteredLayersMixins.transform.unshift(joinaggregateTransform);
            }
        }
        if (filteredLayersMixins) {
            // tukey box plot with outliers included
            return Object.assign({}, outerSpec, { layer: [
                    ...(filteredLayersMixins ? [filteredLayersMixins] : []),
                    {
                        // boxplot
                        transform,
                        layer: boxLayers
                    }
                ] });
        }
        return Object.assign({}, outerSpec, { transform: (outerSpec.transform || []).concat(transform), layer: boxLayers });
    }
    function boxParamsQuartiles(continousAxisField) {
        return [
            {
                op: 'q1',
                field: continousAxisField,
                as: 'lower_box_' + continousAxisField
            },
            {
                op: 'q3',
                field: continousAxisField,
                as: 'upper_box_' + continousAxisField
            }
        ];
    }
    function boxParams(spec, extent, config) {
        const orient = compositeMarkOrient(spec, BOXPLOT);
        const { continuousAxisChannelDef, continuousAxis } = compositeMarkContinuousAxis(spec, orient, BOXPLOT);
        const continuousFieldName = continuousAxisChannelDef.field;
        const boxPlotType = getBoxPlotType(extent);
        const boxplotSpecificAggregate = [
            ...boxParamsQuartiles(continuousFieldName),
            {
                op: 'median',
                field: continuousFieldName,
                as: 'mid_box_' + continuousFieldName
            },
            {
                op: 'min',
                field: continuousFieldName,
                as: (boxPlotType === 'min-max' ? 'lower_whisker_' : 'min_') + continuousFieldName
            },
            {
                op: 'max',
                field: continuousFieldName,
                as: (boxPlotType === 'min-max' ? 'upper_whisker_' : 'max_') + continuousFieldName
            }
        ];
        const postAggregateCalculates = boxPlotType === 'min-max' || boxPlotType === 'tukey'
            ? []
            : [
                // This is for the  original k-IQR, which we do not expose
                {
                    calculate: `datum["upper_box_${continuousFieldName}"] - datum["lower_box_${continuousFieldName}"]`,
                    as: 'iqr_' + continuousFieldName
                },
                {
                    calculate: `min(datum["upper_box_${continuousFieldName}"] + datum["iqr_${continuousFieldName}"] * ${extent}, datum["max_${continuousFieldName}"])`,
                    as: 'upper_whisker_' + continuousFieldName
                },
                {
                    calculate: `max(datum["lower_box_${continuousFieldName}"] - datum["iqr_${continuousFieldName}"] * ${extent}, datum["min_${continuousFieldName}"])`,
                    as: 'lower_whisker_' + continuousFieldName
                }
            ];
        const _a = spec.encoding, _b = continuousAxis, oldContinuousAxisChannelDef = _a[_b], oldEncodingWithoutContinuousAxis = __rest(_a, [typeof _b === "symbol" ? _b : _b + ""]);
        const { customTooltipWithoutAggregatedField, filteredEncoding } = filterTooltipWithAggregatedField(oldEncodingWithoutContinuousAxis);
        const { bins, timeUnits, aggregate, groupby, encoding: encodingWithoutContinuousAxis } = extractTransformsFromEncoding(filteredEncoding, config);
        const ticksOrient = orient === 'vertical' ? 'horizontal' : 'vertical';
        return {
            transform: [
                ...bins,
                ...timeUnits,
                {
                    aggregate: [...aggregate, ...boxplotSpecificAggregate],
                    groupby
                },
                ...postAggregateCalculates
            ],
            groupby,
            aggregate,
            continuousAxisChannelDef,
            continuousAxis,
            encodingWithoutContinuousAxis,
            ticksOrient,
            customTooltipWithoutAggregatedField
        };
    }

    const ERRORBAR = 'errorbar';
    const ERRORBAR_PART_INDEX = {
        ticks: 1,
        rule: 1
    };
    const ERRORBAR_PARTS = keys(ERRORBAR_PART_INDEX);
    const errorBarNormalizer = new CompositeMarkNormalizer(ERRORBAR, normalizeErrorBar);
    function normalizeErrorBar(spec, { config }) {
        const { transform, continuousAxisChannelDef, continuousAxis, encodingWithoutContinuousAxis, ticksOrient, markDef, outerSpec, tooltipEncoding } = errorBarParams(spec, ERRORBAR, config);
        const makeErrorBarPart = makeCompositeAggregatePartFactory(markDef, continuousAxis, continuousAxisChannelDef, encodingWithoutContinuousAxis, config.errorbar);
        const tick = { type: 'tick', orient: ticksOrient };
        return Object.assign({}, outerSpec, { transform, layer: [
                ...makeErrorBarPart({
                    partName: 'ticks',
                    mark: tick,
                    positionPrefix: 'lower',
                    extraEncoding: tooltipEncoding
                }),
                ...makeErrorBarPart({
                    partName: 'ticks',
                    mark: tick,
                    positionPrefix: 'upper',
                    extraEncoding: tooltipEncoding
                }),
                ...makeErrorBarPart({
                    partName: 'rule',
                    mark: 'rule',
                    positionPrefix: 'lower',
                    endPositionPrefix: 'upper',
                    extraEncoding: tooltipEncoding
                })
            ] });
    }
    function errorBarOrientAndInputType(spec, compositeMark) {
        const { encoding } = spec;
        if (errorBarIsInputTypeRaw(encoding)) {
            return {
                orient: compositeMarkOrient(spec, compositeMark),
                inputType: 'raw'
            };
        }
        const isTypeAggregatedUpperLower = errorBarIsInputTypeAggregatedUpperLower(encoding);
        const isTypeAggregatedError = errorBarIsInputTypeAggregatedError(encoding);
        const x = encoding.x;
        const y = encoding.y;
        if (isTypeAggregatedUpperLower) {
            // type is aggregated-upper-lower
            if (isTypeAggregatedError) {
                throw new Error(compositeMark + ' cannot be both type aggregated-upper-lower and aggregated-error');
            }
            const x2 = encoding.x2;
            const y2 = encoding.y2;
            if (isFieldDef(x2) && isFieldDef(y2)) {
                // having both x, x2 and y, y2
                throw new Error(compositeMark + ' cannot have both x2 and y2');
            }
            else if (isFieldDef(x2)) {
                if (isFieldDef(x) && isContinuous(x)) {
                    // having x, x2 quantitative and field y, y2 are not specified
                    return { orient: 'horizontal', inputType: 'aggregated-upper-lower' };
                }
                else {
                    // having x, x2 that are not both quantitative
                    throw new Error('Both x and x2 have to be quantitative in ' + compositeMark);
                }
            }
            else if (isFieldDef(y2)) {
                // y2 is a FieldDef
                if (isFieldDef(y) && isContinuous(y)) {
                    // having y, y2 quantitative and field x, x2 are not specified
                    return { orient: 'vertical', inputType: 'aggregated-upper-lower' };
                }
                else {
                    // having y, y2 that are not both quantitative
                    throw new Error('Both y and y2 have to be quantitative in ' + compositeMark);
                }
            }
            throw new Error('No ranged axis');
        }
        else {
            // type is aggregated-error
            const xError = encoding.xError;
            const xError2 = encoding.xError2;
            const yError = encoding.yError;
            const yError2 = encoding.yError2;
            if (isFieldDef(xError2) && !isFieldDef(xError)) {
                // having xError2 without xError
                throw new Error(compositeMark + ' cannot have xError2 without xError');
            }
            if (isFieldDef(yError2) && !isFieldDef(yError)) {
                // having yError2 without yError
                throw new Error(compositeMark + ' cannot have yError2 without yError');
            }
            if (isFieldDef(xError) && isFieldDef(yError)) {
                // having both xError and yError
                throw new Error(compositeMark + ' cannot have both xError and yError with both are quantiative');
            }
            else if (isFieldDef(xError)) {
                if (isFieldDef(x) && isContinuous(x)) {
                    // having x and xError that are all quantitative
                    return { orient: 'horizontal', inputType: 'aggregated-error' };
                }
                else {
                    // having x, xError, and xError2 that are not all quantitative
                    throw new Error('All x, xError, and xError2 (if exist) have to be quantitative');
                }
            }
            else if (isFieldDef(yError)) {
                if (isFieldDef(y) && isContinuous(y)) {
                    // having y and yError that are all quantitative
                    return { orient: 'vertical', inputType: 'aggregated-error' };
                }
                else {
                    // having y, yError, and yError2 that are not all quantitative
                    throw new Error('All y, yError, and yError2 (if exist) have to be quantitative');
                }
            }
            throw new Error('No ranged axis');
        }
    }
    function errorBarIsInputTypeRaw(encoding) {
        return ((isFieldDef(encoding.x) || isFieldDef(encoding.y)) &&
            !isFieldDef(encoding.x2) &&
            !isFieldDef(encoding.y2) &&
            !isFieldDef(encoding.xError) &&
            !isFieldDef(encoding.xError2) &&
            !isFieldDef(encoding.yError) &&
            !isFieldDef(encoding.yError2));
    }
    function errorBarIsInputTypeAggregatedUpperLower(encoding) {
        return isFieldDef(encoding.x2) || isFieldDef(encoding.y2);
    }
    function errorBarIsInputTypeAggregatedError(encoding) {
        return (isFieldDef(encoding.xError) ||
            isFieldDef(encoding.xError2) ||
            isFieldDef(encoding.yError) ||
            isFieldDef(encoding.yError2));
    }
    function errorBarParams(spec, compositeMark, config) {
        // TODO: use selection
        const { mark, encoding, selection, projection: _p } = spec, outerSpec = __rest(spec, ["mark", "encoding", "selection", "projection"]);
        const markDef = isMarkDef(mark) ? mark : { type: mark };
        // TODO(https://github.com/vega/vega-lite/issues/3702): add selection support
        if (selection) {
            warn(message.selectionNotSupported(compositeMark));
        }
        const { orient, inputType } = errorBarOrientAndInputType(spec, compositeMark);
        const { continuousAxisChannelDef, continuousAxisChannelDef2, continuousAxisChannelDefError, continuousAxisChannelDefError2, continuousAxis } = compositeMarkContinuousAxis(spec, orient, compositeMark);
        const { errorBarSpecificAggregate, postAggregateCalculates, tooltipSummary, tooltipTitleWithFieldName } = errorBarAggregationAndCalculation(markDef, continuousAxisChannelDef, continuousAxisChannelDef2, continuousAxisChannelDefError, continuousAxisChannelDefError2, inputType, compositeMark, config);
        const _a = continuousAxis, oldContinuousAxisChannelDef = encoding[_a], _b = continuousAxis === 'x' ? 'x2' : 'y2', oldContinuousAxisChannelDef2 = encoding[_b], _c = continuousAxis === 'x' ? 'xError' : 'yError', oldContinuousAxisChannelDefError = encoding[_c], _d = continuousAxis === 'x' ? 'xError2' : 'yError2', oldContinuousAxisChannelDefError2 = encoding[_d], oldEncodingWithoutContinuousAxis = __rest(encoding, [typeof _a === "symbol" ? _a : _a + "", typeof _b === "symbol" ? _b : _b + "", typeof _c === "symbol" ? _c : _c + "", typeof _d === "symbol" ? _d : _d + ""]);
        const { bins, timeUnits, aggregate: oldAggregate, groupby: oldGroupBy, encoding: encodingWithoutContinuousAxis } = extractTransformsFromEncoding(oldEncodingWithoutContinuousAxis, config);
        const aggregate = [...oldAggregate, ...errorBarSpecificAggregate];
        const groupby = inputType !== 'raw' ? [] : oldGroupBy;
        const tooltipEncoding = getCompositeMarkTooltip(tooltipSummary, continuousAxisChannelDef, encodingWithoutContinuousAxis, tooltipTitleWithFieldName);
        return {
            transform: [
                ...(outerSpec.transform || []),
                ...bins,
                ...timeUnits,
                ...(!aggregate.length ? [] : [{ aggregate, groupby }]),
                ...postAggregateCalculates
            ],
            groupby,
            continuousAxisChannelDef,
            continuousAxis,
            encodingWithoutContinuousAxis,
            ticksOrient: orient === 'vertical' ? 'horizontal' : 'vertical',
            markDef,
            outerSpec,
            tooltipEncoding
        };
    }
    function errorBarAggregationAndCalculation(markDef, continuousAxisChannelDef, continuousAxisChannelDef2, continuousAxisChannelDefError, continuousAxisChannelDefError2, inputType, compositeMark, config) {
        let errorBarSpecificAggregate = [];
        let postAggregateCalculates = [];
        const continuousFieldName = continuousAxisChannelDef.field;
        let tooltipSummary;
        let tooltipTitleWithFieldName = false;
        if (inputType === 'raw') {
            const center = markDef.center
                ? markDef.center
                : markDef.extent
                    ? markDef.extent === 'iqr'
                        ? 'median'
                        : 'mean'
                    : config.errorbar.center;
            const extent = markDef.extent ? markDef.extent : center === 'mean' ? 'stderr' : 'iqr';
            if ((center === 'median') !== (extent === 'iqr')) {
                warn(message.errorBarCenterIsUsedWithWrongExtent(center, extent, compositeMark));
            }
            if (extent === 'stderr' || extent === 'stdev') {
                errorBarSpecificAggregate = [
                    { op: extent, field: continuousFieldName, as: 'extent_' + continuousFieldName },
                    { op: center, field: continuousFieldName, as: 'center_' + continuousFieldName }
                ];
                postAggregateCalculates = [
                    {
                        calculate: `datum["center_${continuousFieldName}"] + datum["extent_${continuousFieldName}"]`,
                        as: 'upper_' + continuousFieldName
                    },
                    {
                        calculate: `datum["center_${continuousFieldName}"] - datum["extent_${continuousFieldName}"]`,
                        as: 'lower_' + continuousFieldName
                    }
                ];
                tooltipSummary = [
                    { fieldPrefix: 'center_', titlePrefix: titlecase(center) },
                    { fieldPrefix: 'upper_', titlePrefix: getTitlePrefix(center, extent, '+') },
                    { fieldPrefix: 'lower_', titlePrefix: getTitlePrefix(center, extent, '-') }
                ];
                tooltipTitleWithFieldName = true;
            }
            else {
                if (markDef.center && markDef.extent) {
                    warn(message.errorBarCenterIsNotNeeded(markDef.extent, compositeMark));
                }
                let centerOp;
                let lowerExtentOp;
                let upperExtentOp;
                if (extent === 'ci') {
                    centerOp = 'mean';
                    lowerExtentOp = 'ci0';
                    upperExtentOp = 'ci1';
                }
                else {
                    centerOp = 'median';
                    lowerExtentOp = 'q1';
                    upperExtentOp = 'q3';
                }
                errorBarSpecificAggregate = [
                    { op: lowerExtentOp, field: continuousFieldName, as: 'lower_' + continuousFieldName },
                    { op: upperExtentOp, field: continuousFieldName, as: 'upper_' + continuousFieldName },
                    { op: centerOp, field: continuousFieldName, as: 'center_' + continuousFieldName }
                ];
                tooltipSummary = [
                    {
                        fieldPrefix: 'upper_',
                        titlePrefix: title({ field: continuousFieldName, aggregate: upperExtentOp, type: 'quantitative' }, config, {
                            allowDisabling: false
                        })
                    },
                    {
                        fieldPrefix: 'lower_',
                        titlePrefix: title({ field: continuousFieldName, aggregate: lowerExtentOp, type: 'quantitative' }, config, {
                            allowDisabling: false
                        })
                    },
                    {
                        fieldPrefix: 'center_',
                        titlePrefix: title({ field: continuousFieldName, aggregate: centerOp, type: 'quantitative' }, config, {
                            allowDisabling: false
                        })
                    }
                ];
            }
        }
        else {
            if (markDef.center || markDef.extent) {
                warn(message.errorBarCenterAndExtentAreNotNeeded(markDef.center, markDef.extent));
            }
            if (inputType === 'aggregated-upper-lower') {
                tooltipSummary = [];
                postAggregateCalculates = [
                    { calculate: `datum["${continuousAxisChannelDef2.field}"]`, as: 'upper_' + continuousFieldName },
                    { calculate: `datum["${continuousFieldName}"]`, as: 'lower_' + continuousFieldName }
                ];
            }
            else if (inputType === 'aggregated-error') {
                tooltipSummary = [{ fieldPrefix: '', titlePrefix: continuousFieldName }];
                postAggregateCalculates = [
                    {
                        calculate: `datum["${continuousFieldName}"] + datum["${continuousAxisChannelDefError.field}"]`,
                        as: 'upper_' + continuousFieldName
                    }
                ];
                if (continuousAxisChannelDefError2) {
                    postAggregateCalculates.push({
                        calculate: `datum["${continuousFieldName}"] + datum["${continuousAxisChannelDefError2.field}"]`,
                        as: 'lower_' + continuousFieldName
                    });
                }
                else {
                    postAggregateCalculates.push({
                        calculate: `datum["${continuousFieldName}"] - datum["${continuousAxisChannelDefError.field}"]`,
                        as: 'lower_' + continuousFieldName
                    });
                }
            }
            for (const postAggregateCalculate of postAggregateCalculates) {
                tooltipSummary.push({
                    fieldPrefix: postAggregateCalculate.as.substring(0, 6),
                    titlePrefix: postAggregateCalculate.calculate
                        .replace(new RegExp('datum\\[\\"', 'g'), '')
                        .replace(new RegExp('\\"\\]', 'g'), '')
                });
            }
        }
        return { postAggregateCalculates, errorBarSpecificAggregate, tooltipSummary, tooltipTitleWithFieldName };
    }
    function getTitlePrefix(center, extent, operation) {
        return titlecase(center) + ' ' + operation + ' ' + extent;
    }

    const ERRORBAND = 'errorband';
    const ERRORBAND_PART_INDEX = {
        band: 1,
        borders: 1
    };
    const ERRORBAND_PARTS = keys(ERRORBAND_PART_INDEX);
    const errorBandNormalizer = new CompositeMarkNormalizer(ERRORBAND, normalizeErrorBand);
    function normalizeErrorBand(spec, { config }) {
        const { transform, continuousAxisChannelDef, continuousAxis, encodingWithoutContinuousAxis, markDef, outerSpec, tooltipEncoding } = errorBarParams(spec, ERRORBAND, config);
        const errorBandDef = markDef;
        const makeErrorBandPart = makeCompositeAggregatePartFactory(errorBandDef, continuousAxis, continuousAxisChannelDef, encodingWithoutContinuousAxis, config.errorband);
        const is2D = spec.encoding.x !== undefined && spec.encoding.y !== undefined;
        let bandMark = { type: is2D ? 'area' : 'rect' };
        let bordersMark = { type: is2D ? 'line' : 'rule' };
        const interpolate = Object.assign({}, (errorBandDef.interpolate ? { interpolate: errorBandDef.interpolate } : {}), (errorBandDef.tension && errorBandDef.interpolate ? { interpolate: errorBandDef.tension } : {}));
        if (is2D) {
            bandMark = Object.assign({}, bandMark, interpolate);
            bordersMark = Object.assign({}, bordersMark, interpolate);
        }
        else if (errorBandDef.interpolate) {
            warn(message.errorBand1DNotSupport('interpolate'));
        }
        else if (errorBandDef.tension) {
            warn(message.errorBand1DNotSupport('tension'));
        }
        return Object.assign({}, outerSpec, { transform, layer: [
                ...makeErrorBandPart({
                    partName: 'band',
                    mark: bandMark,
                    positionPrefix: 'lower',
                    endPositionPrefix: 'upper',
                    extraEncoding: tooltipEncoding
                }),
                ...makeErrorBandPart({
                    partName: 'borders',
                    mark: bordersMark,
                    positionPrefix: 'lower',
                    extraEncoding: tooltipEncoding
                }),
                ...makeErrorBandPart({
                    partName: 'borders',
                    mark: bordersMark,
                    positionPrefix: 'upper',
                    extraEncoding: tooltipEncoding
                })
            ] });
    }

    /**
     * Registry index for all composite mark's normalizer
     */
    const compositeMarkRegistry = {};
    function add(mark, run, parts) {
        const normalizer = new CompositeMarkNormalizer(mark, run);
        compositeMarkRegistry[mark] = { normalizer, parts };
    }
    function getAllCompositeMarks() {
        return keys(compositeMarkRegistry);
    }
    add(BOXPLOT, normalizeBoxPlot, BOXPLOT_PARTS);
    add(ERRORBAR, normalizeErrorBar, ERRORBAR_PARTS);
    add(ERRORBAND, normalizeErrorBand, ERRORBAND_PARTS);

    const VL_ONLY_GUIDE_CONFIG = ['shortTimeLabels'];
    const VL_ONLY_LEGEND_CONFIG = [
        'gradientHorizontalMaxLength',
        'gradientHorizontalMinLength',
        'gradientVerticalMaxLength',
        'gradientVerticalMinLength'
    ];

    const defaultLegendConfig = {
        gradientHorizontalMaxLength: 200,
        gradientHorizontalMinLength: 100,
        gradientVerticalMaxLength: 200,
        gradientVerticalMinLength: 64 // This is the Vega's minimum.
    };
    const COMMON_LEGEND_PROPERTY_INDEX = {
        clipHeight: 1,
        columnPadding: 1,
        columns: 1,
        cornerRadius: 1,
        direction: 1,
        fillColor: 1,
        format: 1,
        formatType: 1,
        gradientLength: 1,
        gradientOpacity: 1,
        gradientStrokeColor: 1,
        gradientStrokeWidth: 1,
        gradientThickness: 1,
        gridAlign: 1,
        labelAlign: 1,
        labelBaseline: 1,
        labelColor: 1,
        labelFont: 1,
        labelFontSize: 1,
        labelFontStyle: 1,
        labelFontWeight: 1,
        labelLimit: 1,
        labelOffset: 1,
        labelOpacity: 1,
        labelOverlap: 1,
        labelPadding: 1,
        labelSeparation: 1,
        legendX: 1,
        legendY: 1,
        offset: 1,
        orient: 1,
        padding: 1,
        rowPadding: 1,
        strokeColor: 1,
        symbolDash: 1,
        symbolDashOffset: 1,
        symbolFillColor: 1,
        symbolOffset: 1,
        symbolOpacity: 1,
        symbolSize: 1,
        symbolStrokeColor: 1,
        symbolStrokeWidth: 1,
        symbolType: 1,
        tickCount: 1,
        tickMinStep: 1,
        title: 1,
        titleAlign: 1,
        titleAnchor: 1,
        titleBaseline: 1,
        titleColor: 1,
        titleFont: 1,
        titleFontSize: 1,
        titleFontStyle: 1,
        titleFontWeight: 1,
        titleLimit: 1,
        titleOpacity: 1,
        titleOrient: 1,
        titlePadding: 1,
        type: 1,
        values: 1,
        zindex: 1
    };
    const VG_LEGEND_PROPERTY_INDEX = Object.assign({}, COMMON_LEGEND_PROPERTY_INDEX, { 
        // channel scales
        opacity: 1, shape: 1, stroke: 1, fill: 1, size: 1, strokeWidth: 1, 
        // encode
        encode: 1 });
    const LEGEND_PROPERTIES = flagKeys(COMMON_LEGEND_PROPERTY_INDEX);
    const VG_LEGEND_PROPERTIES = flagKeys(VG_LEGEND_PROPERTY_INDEX);

    var ScaleType;
    (function (ScaleType) {
        // Continuous - Quantitative
        ScaleType.LINEAR = 'linear';
        ScaleType.LOG = 'log';
        ScaleType.POW = 'pow';
        ScaleType.SQRT = 'sqrt';
        ScaleType.SYMLOG = 'symlog';
        // Continuous - Time
        ScaleType.TIME = 'time';
        ScaleType.UTC = 'utc';
        // Discretizing scales
        ScaleType.QUANTILE = 'quantile';
        ScaleType.QUANTIZE = 'quantize';
        ScaleType.THRESHOLD = 'threshold';
        ScaleType.BIN_ORDINAL = 'bin-ordinal';
        // Discrete scales
        ScaleType.ORDINAL = 'ordinal';
        ScaleType.POINT = 'point';
        ScaleType.BAND = 'band';
    })(ScaleType || (ScaleType = {}));
    /**
     * Index for scale categories -- only scale of the same categories can be merged together.
     * Current implementation is trying to be conservative and avoid merging scale type that might not work together
     */
    const SCALE_CATEGORY_INDEX = {
        linear: 'numeric',
        log: 'numeric',
        pow: 'numeric',
        sqrt: 'numeric',
        symlog: 'numeric',
        time: 'time',
        utc: 'time',
        ordinal: 'ordinal',
        'bin-ordinal': 'bin-ordinal',
        point: 'ordinal-position',
        band: 'ordinal-position',
        quantile: 'discretizing',
        quantize: 'discretizing',
        threshold: 'discretizing'
    };
    const SCALE_TYPES = keys(SCALE_CATEGORY_INDEX);
    /**
     * Whether the two given scale types can be merged together.
     */
    function scaleCompatible(scaleType1, scaleType2) {
        const scaleCategory1 = SCALE_CATEGORY_INDEX[scaleType1];
        const scaleCategory2 = SCALE_CATEGORY_INDEX[scaleType2];
        return (scaleCategory1 === scaleCategory2 ||
            (scaleCategory1 === 'ordinal-position' && scaleCategory2 === 'time') ||
            (scaleCategory2 === 'ordinal-position' && scaleCategory1 === 'time'));
    }
    /**
     * Index for scale precedence -- high score = higher priority for merging.
     */
    const SCALE_PRECEDENCE_INDEX = {
        // numeric
        linear: 0,
        log: 1,
        pow: 1,
        sqrt: 1,
        symlog: 1,
        // time
        time: 0,
        utc: 0,
        // ordinal-position -- these have higher precedence than continuous scales as they support more types of data
        point: 10,
        band: 11,
        // non grouped types
        ordinal: 0,
        'bin-ordinal': 0,
        quantile: 0,
        quantize: 0,
        threshold: 0
    };
    /**
     * Return scale categories -- only scale of the same categories can be merged together.
     */
    function scaleTypePrecedence(scaleType) {
        return SCALE_PRECEDENCE_INDEX[scaleType];
    }
    const CONTINUOUS_TO_CONTINUOUS_SCALES = ['linear', 'log', 'pow', 'sqrt', 'symlog', 'time', 'utc'];
    const CONTINUOUS_TO_CONTINUOUS_INDEX = toSet(CONTINUOUS_TO_CONTINUOUS_SCALES);
    const CONTINUOUS_TO_DISCRETE_SCALES = ['quantile', 'quantize', 'threshold'];
    const CONTINUOUS_TO_DISCRETE_INDEX = toSet(CONTINUOUS_TO_DISCRETE_SCALES);
    const CONTINUOUS_DOMAIN_SCALES = CONTINUOUS_TO_CONTINUOUS_SCALES.concat([
        'quantile',
        'quantize',
        'threshold'
    ]);
    const CONTINUOUS_DOMAIN_INDEX = toSet(CONTINUOUS_DOMAIN_SCALES);
    const DISCRETE_DOMAIN_SCALES = ['ordinal', 'bin-ordinal', 'point', 'band'];
    const DISCRETE_DOMAIN_INDEX = toSet(DISCRETE_DOMAIN_SCALES);
    function hasDiscreteDomain(type) {
        return type in DISCRETE_DOMAIN_INDEX;
    }
    function hasContinuousDomain(type) {
        return type in CONTINUOUS_DOMAIN_INDEX;
    }
    function isContinuousToContinuous(type) {
        return type in CONTINUOUS_TO_CONTINUOUS_INDEX;
    }
    function isContinuousToDiscrete(type) {
        return type in CONTINUOUS_TO_DISCRETE_INDEX;
    }
    const defaultScaleConfig = {
        textXRangeStep: 90,
        rangeStep: 20,
        pointPadding: 0.5,
        barBandPaddingInner: 0.1,
        rectBandPaddingInner: 0,
        minBandSize: 2,
        minFontSize: 8,
        maxFontSize: 40,
        minOpacity: 0.3,
        maxOpacity: 0.8,
        // FIXME: revise if these *can* become ratios of rangeStep
        minSize: 9,
        minStrokeWidth: 1,
        maxStrokeWidth: 4,
        quantileCount: 4,
        quantizeCount: 4
    };
    function isExtendedScheme(scheme) {
        return scheme && !!scheme['name'];
    }
    function isSelectionDomain(domain) {
        return domain && domain['selection'];
    }
    const SCALE_PROPERTY_INDEX = {
        type: 1,
        domain: 1,
        range: 1,
        rangeStep: 1,
        scheme: 1,
        bins: 1,
        // Other properties
        reverse: 1,
        round: 1,
        // quantitative / time
        clamp: 1,
        nice: 1,
        // quantitative
        base: 1,
        exponent: 1,
        constant: 1,
        interpolate: 1,
        zero: 1,
        // band/point
        padding: 1,
        paddingInner: 1,
        paddingOuter: 1
    };
    const NON_TYPE_DOMAIN_RANGE_VEGA_SCALE_PROPERTY_INDEX = __rest(SCALE_PROPERTY_INDEX, ["type", "domain", "range", "rangeStep", "scheme"]);
    const NON_TYPE_DOMAIN_RANGE_VEGA_SCALE_PROPERTIES = flagKeys(NON_TYPE_DOMAIN_RANGE_VEGA_SCALE_PROPERTY_INDEX);
    const SCALE_TYPE_INDEX = generateScaleTypeIndex();
    function scaleTypeSupportProperty(scaleType, propName) {
        switch (propName) {
            case 'type':
            case 'domain':
            case 'reverse':
            case 'range':
                return true;
            case 'scheme':
            case 'interpolate':
                return !contains(['point', 'band', 'identity'], scaleType);
            case 'bins':
                return !contains(['point', 'band', 'identity', 'ordinal'], scaleType);
            case 'round':
                return isContinuousToContinuous(scaleType) || scaleType === 'band' || scaleType === 'point';
            case 'padding':
                return isContinuousToContinuous(scaleType) || contains(['point', 'band'], scaleType);
            case 'paddingOuter':
            case 'rangeStep':
                return contains(['point', 'band'], scaleType);
            case 'paddingInner':
                return scaleType === 'band';
            case 'clamp':
                return isContinuousToContinuous(scaleType);
            case 'nice':
                return isContinuousToContinuous(scaleType) || scaleType === 'quantize' || scaleType === 'threshold';
            case 'exponent':
                return scaleType === 'pow';
            case 'base':
                return scaleType === 'log';
            case 'constant':
                return scaleType === 'symlog';
            case 'zero':
                return (hasContinuousDomain(scaleType) &&
                    !contains([
                        'log',
                        'time',
                        'utc',
                        'threshold',
                        'quantile' // quantile depends on distribution so zero does not matter
                    ], scaleType));
        }
        /* istanbul ignore next: should never reach here*/
        throw new Error(`Invalid scale property ${propName}.`);
    }
    /**
     * Returns undefined if the input channel supports the input scale property name
     */
    function channelScalePropertyIncompatability(channel, propName) {
        switch (propName) {
            case 'interpolate':
            case 'scheme':
                if (!isColorChannel(channel)) {
                    return message.cannotUseScalePropertyWithNonColor(channel);
                }
                return undefined;
            case 'type':
            case 'bins':
            case 'domain':
            case 'range':
            case 'base':
            case 'exponent':
            case 'constant':
            case 'nice':
            case 'padding':
            case 'paddingInner':
            case 'paddingOuter':
            case 'rangeStep':
            case 'reverse':
            case 'round':
            case 'clamp':
            case 'zero':
                return undefined; // GOOD!
        }
        /* istanbul ignore next: it should never reach here */
        throw new Error(`Invalid scale property "${propName}".`);
    }
    function scaleTypeSupportDataType(specifiedType, fieldDefType) {
        if (contains([ORDINAL, NOMINAL], fieldDefType)) {
            return specifiedType === undefined || hasDiscreteDomain(specifiedType);
        }
        else if (fieldDefType === TEMPORAL) {
            return contains([ScaleType.TIME, ScaleType.UTC, undefined], specifiedType);
        }
        else if (fieldDefType === QUANTITATIVE) {
            return contains([
                ScaleType.LOG,
                ScaleType.POW,
                ScaleType.SQRT,
                ScaleType.SYMLOG,
                ScaleType.QUANTILE,
                ScaleType.QUANTIZE,
                ScaleType.THRESHOLD,
                ScaleType.LINEAR,
                undefined
            ], specifiedType);
        }
        return true;
    }
    function channelSupportScaleType(channel, scaleType) {
        switch (channel) {
            case X:
            case Y:
                return isContinuousToContinuous(scaleType) || contains(['band', 'point'], scaleType);
            case SIZE: // TODO: size and opacity can support ordinal with more modification
            case STROKEWIDTH:
            case OPACITY:
            case FILLOPACITY:
            case STROKEOPACITY:
                // Although it generally doesn't make sense to use band with size and opacity,
                // it can also work since we use band: 0.5 to get midpoint.
                return (isContinuousToContinuous(scaleType) ||
                    isContinuousToDiscrete(scaleType) ||
                    contains(['band', 'point'], scaleType));
            case COLOR:
            case FILL:
            case STROKE:
                return scaleType !== 'band'; // band does not make sense with color
            case SHAPE:
                return scaleType === 'ordinal'; // shape = lookup only
        }
        /* istanbul ignore next: it should never reach here */
        return false;
    }
    // generates ScaleTypeIndex where keys are encoding channels and values are list of valid ScaleTypes
    function generateScaleTypeIndex() {
        const index = {};
        for (const channel of CHANNELS) {
            for (const fieldDefType of keys(TYPE_INDEX)) {
                for (const scaleType of SCALE_TYPES) {
                    const key = generateScaleTypeIndexKey(channel, fieldDefType);
                    if (channelSupportScaleType(channel, scaleType) && scaleTypeSupportDataType(scaleType, fieldDefType)) {
                        index[key] = index[key] || [];
                        index[key].push(scaleType);
                    }
                }
            }
        }
        return index;
    }
    function generateScaleTypeIndexKey(channel, fieldDefType) {
        return channel + '_' + fieldDefType;
    }

    const SELECTION_ID = '_vgsid_';
    function isIntervalSelection(s) {
        return s.type === 'interval';
    }
    const defaultConfig = {
        single: {
            on: 'click',
            fields: [SELECTION_ID],
            resolve: 'global',
            empty: 'all',
            clear: 'dblclick'
        },
        multi: {
            on: 'click',
            fields: [SELECTION_ID],
            toggle: 'event.shiftKey',
            resolve: 'global',
            empty: 'all',
            clear: 'dblclick'
        },
        interval: {
            on: '[mousedown, window:mouseup] > window:mousemove!',
            encodings: ['x', 'y'],
            translate: '[mousedown, window:mouseup] > window:mousemove!',
            zoom: 'wheel!',
            mark: { fill: '#333', fillOpacity: 0.125, stroke: 'white' },
            resolve: 'global',
            clear: 'dblclick'
        }
    };

    function isAnyConcatSpec(spec) {
        return isVConcatSpec(spec) || isHConcatSpec(spec) || isConcatSpec(spec);
    }
    function isConcatSpec(spec) {
        return spec['concat'] !== undefined;
    }
    function isVConcatSpec(spec) {
        return spec['vconcat'] !== undefined;
    }
    function isHConcatSpec(spec) {
        return spec['hconcat'] !== undefined;
    }

    function isRepeatSpec(spec) {
        return spec['repeat'] !== undefined;
    }

    const DEFAULT_SPACING = 20;
    const COMPOSITION_LAYOUT_INDEX = {
        align: 1,
        bounds: 1,
        center: 1,
        columns: 1,
        spacing: 1
    };
    const COMPOSITION_LAYOUT_PROPERTIES = flagKeys(COMPOSITION_LAYOUT_INDEX);
    function extractCompositionLayout(spec, specType, config) {
        const compositionConfig = config[specType];
        const layout = {};
        // Apply config first
        const { spacing: spacingConfig, columns } = compositionConfig;
        if (spacingConfig !== undefined) {
            layout.spacing = spacingConfig;
        }
        if (columns !== undefined) {
            if ((isFacetSpec(spec) && !isFacetMapping(spec.facet)) ||
                (isRepeatSpec(spec) && isArray(spec.repeat)) ||
                isConcatSpec(spec)) {
                layout.columns = columns;
            }
        }
        // Then copy properties from the spec
        for (const prop of COMPOSITION_LAYOUT_PROPERTIES) {
            if (spec[prop] !== undefined) {
                if (prop === 'spacing') {
                    const spacing = spec[prop];
                    layout[prop] = isNumber(spacing)
                        ? spacing
                        : {
                            row: spacing.row || spacingConfig,
                            column: spacing.column || spacingConfig
                        };
                }
                else {
                    layout[prop] = spec[prop];
                }
            }
        }
        return layout;
    }

    function extractTitleConfig(titleConfig) {
        const { 
        // These are non-mark title config that need to be hardcoded
        anchor, frame, offset, orient, 
        // color needs to be redirect to fill
        color } = titleConfig, 
        // The rest are mark config.
        titleMarkConfig = __rest(titleConfig, ["anchor", "frame", "offset", "orient", "color"]);
        const mark = Object.assign({}, titleMarkConfig, (color ? { fill: color } : {}));
        const nonMark = Object.assign({}, (anchor ? { anchor } : {}), (frame ? { frame } : {}), (offset ? { offset } : {}), (orient ? { orient } : {}));
        return { mark, nonMark };
    }

    const defaultViewConfig = {
        width: 200,
        height: 200
    };
    const defaultConfig$1 = {
        padding: 5,
        timeFormat: '%b %d, %Y',
        countTitle: 'Count of Records',
        invalidValues: 'filter',
        view: defaultViewConfig,
        mark: defaultMarkConfig,
        area: {},
        bar: defaultBarConfig,
        circle: {},
        geoshape: {},
        line: {},
        point: {},
        rect: {},
        rule: { color: 'black' },
        square: {},
        text: { color: 'black' },
        tick: defaultTickConfig,
        trail: {},
        boxplot: {
            size: 14,
            extent: 1.5,
            box: {},
            median: { color: 'white' },
            outliers: {},
            rule: {},
            ticks: null
        },
        errorbar: {
            center: 'mean',
            rule: true,
            ticks: false
        },
        errorband: {
            band: {
                opacity: 0.3
            },
            borders: false
        },
        scale: defaultScaleConfig,
        projection: {},
        axis: {},
        axisX: {},
        axisY: {},
        axisLeft: {},
        axisRight: {},
        axisTop: {},
        axisBottom: {},
        axisBand: {},
        legend: defaultLegendConfig,
        header: { titlePadding: 10, labelPadding: 10 },
        headerColumn: {},
        headerRow: {},
        headerFacet: {},
        selection: defaultConfig,
        style: {},
        title: {},
        facet: { spacing: DEFAULT_SPACING },
        repeat: { spacing: DEFAULT_SPACING },
        concat: { spacing: DEFAULT_SPACING }
    };
    function initConfig(config) {
        return mergeDeep(duplicate(defaultConfig$1), config);
    }
    const MARK_STYLES = ['view', ...PRIMITIVE_MARKS];
    const VL_ONLY_CONFIG_PROPERTIES = [
        'padding',
        'facet',
        'concat',
        'repeat',
        'numberFormat',
        'timeFormat',
        'countTitle',
        'header',
        'stack',
        'scale',
        'selection',
        'invalidValues',
        'overlay' // FIXME: Redesign and unhide this
    ];
    const VL_ONLY_ALL_MARK_SPECIFIC_CONFIG_PROPERTY_INDEX = Object.assign({ view: ['width', 'height'] }, VL_ONLY_MARK_SPECIFIC_CONFIG_PROPERTY_INDEX);
    function stripAndRedirectConfig(config) {
        config = duplicate(config);
        for (const prop of VL_ONLY_CONFIG_PROPERTIES) {
            delete config[prop];
        }
        // Remove Vega-Lite only axis/legend config
        if (config.axis) {
            for (const prop of VL_ONLY_GUIDE_CONFIG) {
                delete config.axis[prop];
            }
        }
        if (config.legend) {
            for (const prop of VL_ONLY_GUIDE_CONFIG) {
                delete config.legend[prop];
            }
            for (const prop of VL_ONLY_LEGEND_CONFIG) {
                delete config.legend[prop];
            }
        }
        // Remove Vega-Lite only generic mark config
        if (config.mark) {
            for (const prop of VL_ONLY_MARK_CONFIG_PROPERTIES) {
                delete config.mark[prop];
            }
        }
        for (const markType of MARK_STYLES) {
            // Remove Vega-Lite-only mark config
            for (const prop of VL_ONLY_MARK_CONFIG_PROPERTIES) {
                delete config[markType][prop];
            }
            // Remove Vega-Lite only mark-specific config
            const vlOnlyMarkSpecificConfigs = VL_ONLY_ALL_MARK_SPECIFIC_CONFIG_PROPERTY_INDEX[markType];
            if (vlOnlyMarkSpecificConfigs) {
                for (const prop of vlOnlyMarkSpecificConfigs) {
                    delete config[markType][prop];
                }
            }
            // Redirect mark config to config.style so that mark config only affect its own mark type
            // without affecting other marks that share the same underlying Vega marks.
            // For example, config.rect should not affect bar marks.
            redirectConfig(config, markType);
        }
        for (const m of getAllCompositeMarks()) {
            // Clean up the composite mark config as we don't need them in the output specs anymore
            delete config[m];
        }
        // Redirect config.title -- so that title config do not
        // affect header labels, which also uses `title` directive to implement.
        redirectConfig(config, 'title', 'group-title');
        // Remove empty config objects
        for (const prop in config) {
            if (isObject(config[prop]) && keys(config[prop]).length === 0) {
                delete config[prop];
            }
        }
        return keys(config).length > 0 ? config : undefined;
    }
    function redirectConfig(config, prop, // string = composite mark
    toProp, compositeMarkPart) {
        const propConfig = prop === 'title'
            ? extractTitleConfig(config.title).mark
            : compositeMarkPart
                ? config[prop][compositeMarkPart]
                : config[prop];
        if (prop === 'view') {
            toProp = 'cell'; // View's default style is "cell"
        }
        const style = Object.assign({}, propConfig, config.style[prop]);
        // set config.style if it is not an empty object
        if (keys(style).length > 0) {
            config.style[toProp || prop] = style;
        }
        if (!compositeMarkPart) {
            // For composite mark, so don't delete the whole config yet as we have to do multiple redirections.
            delete config[prop];
        }
    }

    function isLayerSpec(spec) {
        return spec['layer'] !== undefined;
    }

    class SpecMapper {
        map(spec, params) {
            if (isFacetSpec(spec)) {
                return this.mapFacet(spec, params);
            }
            else if (isRepeatSpec(spec)) {
                return this.mapRepeat(spec, params);
            }
            else if (isHConcatSpec(spec)) {
                return this.mapHConcat(spec, params);
            }
            else if (isVConcatSpec(spec)) {
                return this.mapVConcat(spec, params);
            }
            else if (isConcatSpec(spec)) {
                return this.mapConcat(spec, params);
            }
            else {
                return this.mapLayerOrUnit(spec, params);
            }
        }
        mapLayerOrUnit(spec, params) {
            if (isLayerSpec(spec)) {
                return this.mapLayer(spec, params);
            }
            else if (isUnitSpec(spec)) {
                return this.mapUnit(spec, params);
            }
            throw new Error(message.INVALID_SPEC);
        }
        mapLayer(spec, params) {
            return Object.assign({}, spec, { layer: spec.layer.map(subspec => this.mapLayerOrUnit(subspec, params)) });
        }
        mapHConcat(spec, params) {
            return Object.assign({}, spec, { hconcat: spec.hconcat.map(subspec => this.map(subspec, params)) });
        }
        mapVConcat(spec, params) {
            return Object.assign({}, spec, { vconcat: spec.vconcat.map(subspec => this.map(subspec, params)) });
        }
        mapConcat(spec, params) {
            const { concat } = spec, rest = __rest(spec, ["concat"]);
            return Object.assign({}, rest, { concat: concat.map(subspec => this.map(subspec, params)) });
        }
        mapFacet(spec, params) {
            return Object.assign({}, spec, { 
                // TODO: remove "any" once we support all facet listed in https://github.com/vega/vega-lite/issues/2760
                spec: this.map(spec.spec, params) });
        }
        mapRepeat(spec, params) {
            return Object.assign({}, spec, { spec: this.map(spec.spec, params) });
        }
    }

    const STACK_OFFSET_INDEX = {
        zero: 1,
        center: 1,
        normalize: 1
    };
    function isStackOffset(s) {
        return !!STACK_OFFSET_INDEX[s];
    }
    const STACKABLE_MARKS = [BAR, AREA, RULE, POINT, CIRCLE, SQUARE, LINE, TEXT, TICK];
    const STACK_BY_DEFAULT_MARKS = [BAR, AREA];
    function potentialStackedChannel(encoding) {
        const xDef = encoding.x;
        const yDef = encoding.y;
        if (isFieldDef(xDef) && isFieldDef(yDef)) {
            if (xDef.type === 'quantitative' && yDef.type === 'quantitative') {
                if (xDef.stack) {
                    return 'x';
                }
                else if (yDef.stack) {
                    return 'y';
                }
                // if there is no explicit stacking, only apply stack if there is only one aggregate for x or y
                if (!!xDef.aggregate !== !!yDef.aggregate) {
                    return xDef.aggregate ? 'x' : 'y';
                }
            }
            else if (xDef.type === 'quantitative') {
                return 'x';
            }
            else if (yDef.type === 'quantitative') {
                return 'y';
            }
        }
        else if (isFieldDef(xDef) && xDef.type === 'quantitative') {
            return 'x';
        }
        else if (isFieldDef(yDef) && yDef.type === 'quantitative') {
            return 'y';
        }
        return undefined;
    }
    // Note: CompassQL uses this method and only pass in required properties of each argument object.
    // If required properties change, make sure to update CompassQL.
    function stack(m, encoding, stackConfig, opt = {}) {
        const mark = isMarkDef(m) ? m.type : m;
        // Should have stackable mark
        if (!contains(STACKABLE_MARKS, mark)) {
            return null;
        }
        const fieldChannel = potentialStackedChannel(encoding);
        if (!fieldChannel) {
            return null;
        }
        const stackedFieldDef = encoding[fieldChannel];
        const stackedField = isStringFieldDef(stackedFieldDef) ? vgField(stackedFieldDef, {}) : undefined;
        const dimensionChannel = fieldChannel === 'x' ? 'y' : 'x';
        const dimensionDef = encoding[dimensionChannel];
        const dimensionField = isStringFieldDef(dimensionDef) ? vgField(dimensionDef, {}) : undefined;
        // Should have grouping level of detail that is different from the dimension field
        const stackBy = NONPOSITION_CHANNELS.reduce((sc, channel) => {
            // Ignore tooltip in stackBy (https://github.com/vega/vega-lite/issues/4001)
            if (channel !== 'tooltip' && channelHasField(encoding, channel)) {
                const channelDef = encoding[channel];
                (isArray(channelDef) ? channelDef : [channelDef]).forEach(cDef => {
                    const fieldDef = getTypedFieldDef(cDef);
                    if (fieldDef.aggregate) {
                        return;
                    }
                    // Check whether the channel's field is identical to x/y's field or if the channel is a repeat
                    const f = isStringFieldDef(fieldDef) ? vgField(fieldDef, {}) : undefined;
                    if (
                    // if fielddef is a repeat, just include it in the stack by
                    !f ||
                        // otherwise, the field must be different from x and y fields.
                        (f !== dimensionField && f !== stackedField)) {
                        sc.push({ channel, fieldDef });
                    }
                });
            }
            return sc;
        }, []);
        if (stackBy.length === 0) {
            return null;
        }
        // Automatically determine offset
        let offset;
        if (stackedFieldDef.stack !== undefined) {
            if (isBoolean(stackedFieldDef.stack)) {
                offset = stackedFieldDef.stack ? 'zero' : null;
            }
            else {
                offset = stackedFieldDef.stack;
            }
        }
        else if (contains(STACK_BY_DEFAULT_MARKS, mark)) {
            // Bar and Area with sum ops are automatically stacked by default
            offset = getFirstDefined(stackConfig, 'zero');
        }
        else {
            offset = stackConfig;
        }
        if (!offset || !isStackOffset(offset)) {
            return null;
        }
        // warn when stacking non-linear
        if (stackedFieldDef.scale && stackedFieldDef.scale.type && stackedFieldDef.scale.type !== ScaleType.LINEAR) {
            if (opt.disallowNonLinearStack) {
                return null;
            }
            else {
                warn(message.cannotStackNonLinearScale(stackedFieldDef.scale.type));
            }
        }
        // Check if it is a ranged mark
        if (channelHasField(encoding, fieldChannel === X ? X2 : Y2)) {
            if (stackedFieldDef.stack !== undefined) {
                warn(message.cannotStackRangedMark(fieldChannel));
            }
            return null;
        }
        // Warn if stacking summative aggregate
        if (stackedFieldDef.aggregate && !contains(SUM_OPS, stackedFieldDef.aggregate)) {
            warn(message.stackNonSummativeAggregate(stackedFieldDef.aggregate));
        }
        return {
            groupbyChannel: dimensionDef ? dimensionChannel : undefined,
            fieldChannel,
            impute: isPathMark(mark),
            stackBy,
            offset
        };
    }

    function dropLineAndPoint(markDef) {
        const mark = __rest(markDef, ["point", "line"]);
        return keys(mark).length > 1 ? mark : mark.type;
    }
    function dropLineAndPointFromConfig(config) {
        for (const mark of ['line', 'area', 'rule', 'trail']) {
            if (config[mark]) {
                config = Object.assign({}, config, { [mark]: omit(config[mark], ['point', 'line']) });
            }
        }
        return config;
    }
    function getPointOverlay(markDef, markConfig = {}, encoding) {
        if (markDef.point === 'transparent') {
            return { opacity: 0 };
        }
        else if (markDef.point) {
            // truthy : true or object
            return isObject(markDef.point) ? markDef.point : {};
        }
        else if (markDef.point !== undefined) {
            // false or null
            return null;
        }
        else {
            // undefined (not disabled)
            if (markConfig.point || encoding.shape) {
                // enable point overlay if config[mark].point is truthy or if encoding.shape is provided
                return isObject(markConfig.point) ? markConfig.point : {};
            }
            // markDef.point is defined as falsy
            return undefined;
        }
    }
    function getLineOverlay(markDef, markConfig = {}) {
        if (markDef.line) {
            // true or object
            return markDef.line === true ? {} : markDef.line;
        }
        else if (markDef.line !== undefined) {
            // false or null
            return null;
        }
        else {
            // undefined (not disabled)
            if (markConfig.line) {
                // enable line overlay if config[mark].line is truthy
                return markConfig.line === true ? {} : markConfig.line;
            }
            // markDef.point is defined as falsy
            return undefined;
        }
    }
    class PathOverlayNormalizer {
        constructor() {
            this.name = 'path-overlay';
        }
        hasMatchingType(spec, config) {
            if (isUnitSpec(spec)) {
                const { mark, encoding } = spec;
                const markDef = isMarkDef(mark) ? mark : { type: mark };
                switch (markDef.type) {
                    case 'line':
                    case 'rule':
                    case 'trail':
                        return !!getPointOverlay(markDef, config[markDef.type], encoding);
                    case 'area':
                        return (
                        // false / null are also included as we want to remove the properties
                        !!getPointOverlay(markDef, config[markDef.type], encoding) ||
                            !!getLineOverlay(markDef, config[markDef.type]));
                }
            }
            return false;
        }
        run(spec, params, normalize) {
            const { config } = params;
            const { selection, projection, encoding, mark } = spec, outerSpec = __rest(spec, ["selection", "projection", "encoding", "mark"]);
            const markDef = isMarkDef(mark) ? mark : { type: mark };
            const pointOverlay = getPointOverlay(markDef, config[markDef.type], encoding);
            const lineOverlay = markDef.type === 'area' && getLineOverlay(markDef, config[markDef.type]);
            const layer = [
                Object.assign({}, (selection ? { selection } : {}), { 
                    // Do not include point / line overlay in the normalize spec
                    mark: dropLineAndPoint(Object.assign({}, markDef, (markDef.type === 'area' ? { opacity: 0.7 } : {}))), 
                    // drop shape from encoding as this might be used to trigger point overlay
                    encoding: omit(encoding, ['shape']) })
            ];
            // FIXME: determine rules for applying selections.
            // Need to copy stack config to overlayed layer
            const stackProps = stack(markDef, encoding, config ? config.stack : undefined);
            let overlayEncoding = encoding;
            if (stackProps) {
                const { fieldChannel: stackFieldChannel, offset } = stackProps;
                overlayEncoding = Object.assign({}, encoding, { [stackFieldChannel]: Object.assign({}, encoding[stackFieldChannel], (offset ? { stack: offset } : {})) });
            }
            if (lineOverlay) {
                layer.push(Object.assign({}, (projection ? { projection } : {}), { mark: Object.assign({ type: 'line' }, pick(markDef, ['clip', 'interpolate', 'tension']), lineOverlay), encoding: overlayEncoding }));
            }
            if (pointOverlay) {
                layer.push(Object.assign({}, (projection ? { projection } : {}), { mark: Object.assign({ type: 'point', opacity: 1, filled: true }, pick(markDef, ['clip']), pointOverlay), encoding: overlayEncoding }));
            }
            return normalize(Object.assign({}, outerSpec, { layer }), Object.assign({}, params, { config: dropLineAndPointFromConfig(config) }));
        }
    }

    class RuleForRangedLineNormalizer {
        constructor() {
            this.name = 'RuleForRangedLine';
        }
        hasMatchingType(spec) {
            if (isUnitSpec(spec)) {
                const { encoding, mark } = spec;
                if (mark === 'line') {
                    for (const channel of SECONDARY_RANGE_CHANNEL) {
                        const mainChannel = getMainRangeChannel(channel);
                        const mainChannelDef = encoding[mainChannel];
                        if (!!encoding[channel] && isFieldDef(mainChannelDef) && mainChannelDef.bin !== 'binned') {
                            return true;
                        }
                    }
                }
            }
            return false;
        }
        run(spec, params, normalize) {
            const { encoding } = spec;
            warn(message.lineWithRange(!!encoding.x2, !!encoding.y2));
            return normalize(Object.assign({}, spec, { mark: 'rule' }), params);
        }
    }

    class CoreNormalizer extends SpecMapper {
        constructor() {
            super(...arguments);
            this.nonFacetUnitNormalizers = [
                boxPlotNormalizer,
                errorBarNormalizer,
                errorBandNormalizer,
                new PathOverlayNormalizer(),
                new RuleForRangedLineNormalizer()
            ];
        }
        map(spec, params) {
            // Special handling for a faceted unit spec as it can return a facet spec, not just a layer or unit spec like a normal unit spec.
            if (isUnitSpec(spec)) {
                const hasRow = channelHasField(spec.encoding, ROW);
                const hasColumn = channelHasField(spec.encoding, COLUMN);
                const hasFacet = channelHasField(spec.encoding, FACET);
                if (hasRow || hasColumn || hasFacet) {
                    return this.mapFacetedUnit(spec, params);
                }
            }
            return super.map(spec, params);
        }
        // This is for normalizing non-facet unit
        mapUnit(spec, params) {
            const { parentEncoding, parentProjection } = params;
            if (parentEncoding || parentProjection) {
                return this.mapUnitWithParentEncodingOrProjection(spec, params);
            }
            const normalizeLayerOrUnit = this.mapLayerOrUnit.bind(this);
            for (const unitNormalizer of this.nonFacetUnitNormalizers) {
                if (unitNormalizer.hasMatchingType(spec, params.config)) {
                    return unitNormalizer.run(spec, params, normalizeLayerOrUnit);
                }
            }
            return spec;
        }
        mapRepeat(spec, params) {
            const { repeat } = spec;
            if (!isArray(repeat) && spec.columns) {
                // is repeat with row/column
                spec = omit(spec, ['columns']);
                warn(message.columnsNotSupportByRowCol('repeat'));
            }
            return Object.assign({}, spec, { spec: this.map(spec.spec, params) });
        }
        mapFacet(spec, params) {
            const { facet } = spec;
            if (isFacetMapping(facet) && spec.columns) {
                // is facet with row/column
                spec = omit(spec, ['columns']);
                warn(message.columnsNotSupportByRowCol('facet'));
            }
            return super.mapFacet(spec, params);
        }
        mapUnitWithParentEncodingOrProjection(spec, params) {
            const { encoding, projection } = spec;
            const { parentEncoding, parentProjection, config } = params;
            const mergedProjection = mergeProjection({ parentProjection, projection });
            const mergedEncoding = mergeEncoding({ parentEncoding, encoding });
            return this.mapUnit(Object.assign({}, spec, (mergedProjection ? { projection: mergedProjection } : {}), (mergedEncoding ? { encoding: mergedEncoding } : {})), { config });
        }
        mapFacetedUnit(spec, params) {
            // New encoding in the inside spec should not contain row / column
            // as row/column should be moved to facet
            const _a = spec.encoding, { row, column, facet } = _a, encoding = __rest(_a, ["row", "column", "facet"]);
            // Mark and encoding should be moved into the inner spec
            const { mark, width, projection, height, selection, encoding: _ } = spec, outerSpec = __rest(spec, ["mark", "width", "projection", "height", "selection", "encoding"]);
            if (facet && (row || column)) {
                warn(message.facetChannelDropped([...(row ? [ROW] : []), ...(column ? [COLUMN] : [])]));
            }
            return this.mapFacet(Object.assign({}, outerSpec, { 
                // row / column has higher precedence than facet
                facet: row || column
                    ? Object.assign({}, (row ? { row } : {}), (column ? { column } : {})) : facet, spec: Object.assign({}, (projection ? { projection } : {}), { mark }, (width ? { width } : {}), (height ? { height } : {}), { encoding }, (selection ? { selection } : {})) }), params);
        }
        mapLayer(spec, _a) {
            // Special handling for extended layer spec
            var { parentEncoding, parentProjection } = _a, otherParams = __rest(_a, ["parentEncoding", "parentProjection"]);
            const { encoding, projection } = spec, rest = __rest(spec, ["encoding", "projection"]);
            const params = Object.assign({}, otherParams, { parentEncoding: mergeEncoding({ parentEncoding, encoding }), parentProjection: mergeProjection({ parentProjection, projection }) });
            return super.mapLayer(rest, params);
        }
    }
    function mergeEncoding(opt) {
        const { parentEncoding, encoding } = opt;
        if (parentEncoding && encoding) {
            const overriden = keys(parentEncoding).reduce((o, key) => {
                if (encoding[key]) {
                    o.push(key);
                }
                return o;
            }, []);
            if (overriden.length > 0) {
                warn(message.encodingOverridden(overriden));
            }
        }
        const merged = Object.assign({}, (parentEncoding || {}), (encoding || {}));
        return keys(merged).length > 0 ? merged : undefined;
    }
    function mergeProjection(opt) {
        const { parentProjection, projection } = opt;
        if (parentProjection && projection) {
            warn(message.projectionOverridden({ parentProjection, projection }));
        }
        return projection || parentProjection;
    }

    function normalize$1(spec, config) {
        if (config === undefined) {
            config = initConfig(spec.config);
        }
        return normalizeGenericSpec(spec, config);
    }
    const normalizer = new CoreNormalizer();
    /**
     * Decompose extended unit specs into composition of pure unit specs.
     */
    function normalizeGenericSpec(spec, config = {}) {
        return normalizer.map(spec, { config });
    }

    /**
     * Definition for specifications in Vega-Lite.  In general, there are 3 variants of specs for each type of specs:
     * - Generic specs are generic versions of specs and they are parameterized differently for internal and external specs.
     * - The external specs (no prefix) would allow composite marks, row/column encodings, and mark macros like point/line overlay.
     * - The internal specs (with `Normalized` prefix) would only support primitive marks and support no macros/shortcuts.
     */

    function _normalizeAutoSize(autosize) {
        return isString(autosize) ? { type: autosize } : autosize || {};
    }
    function normalizeAutoSize(topLevelAutosize, configAutosize, isUnitOrLayer = true) {
        const autosize = Object.assign({ type: 'pad' }, _normalizeAutoSize(configAutosize), _normalizeAutoSize(topLevelAutosize));
        if (autosize.type === 'fit') {
            if (!isUnitOrLayer) {
                warn(message.FIT_NON_SINGLE);
                autosize.type = 'pad';
            }
        }
        return autosize;
    }
    const TOP_LEVEL_PROPERTIES = [
        'background',
        'padding'
        // We do not include "autosize" here as it is supported by only unit and layer specs and thus need to be normalized
    ];
    function extractTopLevelProperties(t) {
        return TOP_LEVEL_PROPERTIES.reduce((o, p) => {
            if (t && t[p] !== undefined) {
                o[p] = t[p];
            }
            return o;
        }, {});
    }

    function isUrlData(data) {
        return !!data['url'];
    }
    function isInlineData(data) {
        return !!data['values'];
    }
    function isNamedData(data) {
        return !!data['name'] && !isUrlData(data) && !isInlineData(data) && !isGenerator(data);
    }
    function isGenerator(data) {
        return data && (isSequenceGenerator(data) || isSphereGenerator(data) || isGraticuleGenerator(data));
    }
    function isSequenceGenerator(data) {
        return !!data['sequence'];
    }
    function isSphereGenerator(data) {
        return !!data['sphere'];
    }
    function isGraticuleGenerator(data) {
        return !!data['graticule'];
    }
    const MAIN = 'main';
    const RAW = 'raw';

    function isSelectionPredicate(predicate) {
        return predicate && predicate['selection'];
    }
    function isFieldEqualPredicate(predicate) {
        return predicate && !!predicate.field && predicate.equal !== undefined;
    }
    function isFieldLTPredicate(predicate) {
        return predicate && !!predicate.field && predicate.lt !== undefined;
    }
    function isFieldLTEPredicate(predicate) {
        return predicate && !!predicate.field && predicate.lte !== undefined;
    }
    function isFieldGTPredicate(predicate) {
        return predicate && !!predicate.field && predicate.gt !== undefined;
    }
    function isFieldGTEPredicate(predicate) {
        return predicate && !!predicate.field && predicate.gte !== undefined;
    }
    function isFieldRangePredicate(predicate) {
        if (predicate && predicate.field) {
            if (isArray(predicate.range) && predicate.range.length === 2) {
                return true;
            }
        }
        return false;
    }
    function isFieldOneOfPredicate(predicate) {
        return (predicate && !!predicate.field && (isArray(predicate.oneOf) || isArray(predicate.in)) // backward compatibility
        );
    }
    function isFieldValidPredicate(predicate) {
        return predicate && !!predicate.field && predicate.valid !== undefined;
    }
    function isFieldPredicate(predicate) {
        return (isFieldOneOfPredicate(predicate) ||
            isFieldEqualPredicate(predicate) ||
            isFieldRangePredicate(predicate) ||
            isFieldLTPredicate(predicate) ||
            isFieldGTPredicate(predicate) ||
            isFieldLTEPredicate(predicate) ||
            isFieldGTEPredicate(predicate));
    }
    function predicateValueExpr(v, timeUnit) {
        return valueExpr(v, { timeUnit, time: true });
    }
    function predicateValuesExpr(vals, timeUnit) {
        return vals.map(v => predicateValueExpr(v, timeUnit));
    }
    // This method is used by Voyager.  Do not change its behavior without changing Voyager.
    function fieldFilterExpression(predicate, useInRange = true) {
        const { field, timeUnit } = predicate;
        const fieldExpr$1 = timeUnit
            ? // For timeUnit, cast into integer with time() so we can use ===, inrange, indexOf to compare values directly.
                // TODO: We calculate timeUnit on the fly here. Consider if we would like to consolidate this with timeUnit pipeline
                // TODO: support utc
                'time(' + fieldExpr(timeUnit, field) + ')'
            : vgField(predicate, { expr: 'datum' });
        if (isFieldEqualPredicate(predicate)) {
            return fieldExpr$1 + '===' + predicateValueExpr(predicate.equal, timeUnit);
        }
        else if (isFieldLTPredicate(predicate)) {
            const upper = predicate.lt;
            return `${fieldExpr$1}<${predicateValueExpr(upper, timeUnit)}`;
        }
        else if (isFieldGTPredicate(predicate)) {
            const lower = predicate.gt;
            return `${fieldExpr$1}>${predicateValueExpr(lower, timeUnit)}`;
        }
        else if (isFieldLTEPredicate(predicate)) {
            const upper = predicate.lte;
            return `${fieldExpr$1}<=${predicateValueExpr(upper, timeUnit)}`;
        }
        else if (isFieldGTEPredicate(predicate)) {
            const lower = predicate.gte;
            return `${fieldExpr$1}>=${predicateValueExpr(lower, timeUnit)}`;
        }
        else if (isFieldOneOfPredicate(predicate)) {
            return `indexof([${predicateValuesExpr(predicate.oneOf, timeUnit).join(',')}], ${fieldExpr$1}) !== -1`;
        }
        else if (isFieldValidPredicate(predicate)) {
            return predicate.valid ? `${fieldExpr$1}!==null&&!isNaN(${fieldExpr$1})` : `${fieldExpr$1}===null||isNaN(${fieldExpr$1})`;
        }
        else if (isFieldRangePredicate(predicate)) {
            const lower = predicate.range[0];
            const upper = predicate.range[1];
            if (lower !== null && upper !== null && useInRange) {
                return ('inrange(' +
                    fieldExpr$1 +
                    ', [' +
                    predicateValueExpr(lower, timeUnit) +
                    ', ' +
                    predicateValueExpr(upper, timeUnit) +
                    '])');
            }
            const exprs = [];
            if (lower !== null) {
                exprs.push(`${fieldExpr$1} >= ${predicateValueExpr(lower, timeUnit)}`);
            }
            if (upper !== null) {
                exprs.push(`${fieldExpr$1} <= ${predicateValueExpr(upper, timeUnit)}`);
            }
            return exprs.length > 0 ? exprs.join(' && ') : 'true';
        }
        /* istanbul ignore next: it should never reach here */
        throw new Error(`Invalid field predicate: ${JSON.stringify(predicate)}`);
    }
    function normalizePredicate(f) {
        if (isFieldPredicate(f) && f.timeUnit) {
            return Object.assign({}, f, { timeUnit: normalizeTimeUnit(f.timeUnit) });
        }
        return f;
    }

    function isFilter(t) {
        return t['filter'] !== undefined;
    }
    function isImputeSequence(t) {
        return t && t['start'] !== undefined && t['stop'] !== undefined;
    }
    function isLookup(t) {
        return t['lookup'] !== undefined;
    }
    function isSample(t) {
        return t['sample'] !== undefined;
    }
    function isWindow(t) {
        return t['window'] !== undefined;
    }
    function isJoinAggregate(t) {
        return t['joinaggregate'] !== undefined;
    }
    function isFlatten(t) {
        return t['flatten'] !== undefined;
    }
    function isCalculate(t) {
        return t['calculate'] !== undefined;
    }
    function isBin(t) {
        return !!t['bin'];
    }
    function isImpute(t) {
        return t['impute'] !== undefined;
    }
    function isTimeUnit(t) {
        return t['timeUnit'] !== undefined;
    }
    function isAggregate$1(t) {
        return t['aggregate'] !== undefined;
    }
    function isStack(t) {
        return t['stack'] !== undefined;
    }
    function isFold(t) {
        return t['fold'] !== undefined;
    }
    function normalizeTransform(transform) {
        return transform.map(t => {
            if (isFilter(t)) {
                return {
                    filter: normalizeLogicalOperand(t.filter, normalizePredicate)
                };
            }
            return t;
        });
    }

    function isSignalRef(o) {
        return !!o['signal'];
    }
    function isVgRangeStep(range) {
        return !!range['step'];
    }
    function isDataRefUnionedDomain(domain) {
        if (!isArray(domain)) {
            return 'fields' in domain && !('data' in domain);
        }
        return false;
    }
    function isFieldRefUnionDomain(domain) {
        if (!isArray(domain)) {
            return 'fields' in domain && 'data' in domain;
        }
        return false;
    }
    function isDataRefDomain(domain) {
        if (!isArray(domain)) {
            return 'field' in domain && 'data' in domain;
        }
        return false;
    }
    const VG_MARK_CONFIG_INDEX = {
        opacity: 1,
        fill: 1,
        fillOpacity: 1,
        stroke: 1,
        strokeCap: 1,
        strokeWidth: 1,
        strokeOpacity: 1,
        strokeDash: 1,
        strokeDashOffset: 1,
        strokeJoin: 1,
        strokeMiterLimit: 1,
        size: 1,
        shape: 1,
        interpolate: 1,
        tension: 1,
        orient: 1,
        align: 1,
        baseline: 1,
        text: 1,
        dir: 1,
        dx: 1,
        dy: 1,
        ellipsis: 1,
        limit: 1,
        radius: 1,
        theta: 1,
        angle: 1,
        font: 1,
        fontSize: 1,
        fontWeight: 1,
        fontStyle: 1,
        cursor: 1,
        href: 1,
        tooltip: 1,
        cornerRadius: 1,
        x: 1,
        y: 1,
        x2: 1,
        y2: 1
        // commented below are vg channel that do not have mark config.
        // xc'|'width'|'yc'|'height'
        // clip: 1,
        // endAngle: 1,
        // innerRadius: 1,
        // outerRadius: 1,
        // path: 1,
        // startAngle: 1,
        // url: 1,
    };
    const VG_MARK_CONFIGS = flagKeys(VG_MARK_CONFIG_INDEX);

    const AXIS_PARTS = ['domain', 'grid', 'labels', 'ticks', 'title'];
    /**
     * A dictionary listing whether a certain axis property is applicable for only main axes or only grid axes.
     * (Properties not listed are applicable for both)
     */
    const AXIS_PROPERTY_TYPE = {
        grid: 'grid',
        gridColor: 'grid',
        gridDash: 'grid',
        gridOpacity: 'grid',
        gridScale: 'grid',
        gridWidth: 'grid',
        orient: 'main',
        bandPosition: 'both',
        domain: 'main',
        domainColor: 'main',
        domainOpacity: 'main',
        domainWidth: 'main',
        format: 'main',
        formatType: 'main',
        labelAlign: 'main',
        labelAngle: 'main',
        labelBaseline: 'main',
        labelBound: 'main',
        labelColor: 'main',
        labelFlush: 'main',
        labelFlushOffset: 'main',
        labelFont: 'main',
        labelFontSize: 'main',
        labelFontWeight: 'main',
        labelLimit: 'main',
        labelOpacity: 'main',
        labelOverlap: 'main',
        labelPadding: 'main',
        labels: 'main',
        maxExtent: 'main',
        minExtent: 'main',
        offset: 'main',
        position: 'main',
        tickColor: 'main',
        tickExtra: 'main',
        tickOffset: 'both',
        tickOpacity: 'main',
        tickRound: 'main',
        ticks: 'main',
        tickSize: 'main',
        title: 'main',
        titleAlign: 'main',
        titleAngle: 'main',
        titleBaseline: 'main',
        titleColor: 'main',
        titleFont: 'main',
        titleFontSize: 'main',
        titleFontWeight: 'main',
        titleLimit: 'main',
        titleOpacity: 'main',
        titlePadding: 'main',
        titleX: 'main',
        titleY: 'main',
        tickWidth: 'both',
        tickCount: 'both',
        values: 'both',
        scale: 'both',
        zindex: 'both' // this is actually set afterward, so it doesn't matter
    };
    const COMMON_AXIS_PROPERTIES_INDEX = {
        orient: 1,
        bandPosition: 1,
        domain: 1,
        domainColor: 1,
        domainDash: 1,
        domainDashOffset: 1,
        domainOpacity: 1,
        domainWidth: 1,
        format: 1,
        formatType: 1,
        grid: 1,
        gridColor: 1,
        gridDash: 1,
        gridDashOffset: 1,
        gridOpacity: 1,
        gridWidth: 1,
        labelAlign: 1,
        labelAngle: 1,
        labelBaseline: 1,
        labelBound: 1,
        labelColor: 1,
        labelFlush: 1,
        labelFlushOffset: 1,
        labelFont: 1,
        labelFontSize: 1,
        labelFontStyle: 1,
        labelFontWeight: 1,
        labelLimit: 1,
        labelOpacity: 1,
        labelOverlap: 1,
        labelPadding: 1,
        labels: 1,
        labelSeparation: 1,
        maxExtent: 1,
        minExtent: 1,
        offset: 1,
        position: 1,
        tickColor: 1,
        tickCount: 1,
        tickDash: 1,
        tickDashOffset: 1,
        tickExtra: 1,
        tickMinStep: 1,
        tickOffset: 1,
        tickOpacity: 1,
        tickRound: 1,
        ticks: 1,
        tickSize: 1,
        tickWidth: 1,
        title: 1,
        titleAlign: 1,
        titleAnchor: 1,
        titleAngle: 1,
        titleBaseline: 1,
        titleColor: 1,
        titleFont: 1,
        titleFontSize: 1,
        titleFontStyle: 1,
        titleFontWeight: 1,
        titleLimit: 1,
        titleOpacity: 1,
        titlePadding: 1,
        titleX: 1,
        titleY: 1,
        values: 1,
        zindex: 1
    };
    const AXIS_PROPERTIES_INDEX = Object.assign({}, COMMON_AXIS_PROPERTIES_INDEX, { encoding: 1 });
    const VG_AXIS_PROPERTIES_INDEX = Object.assign({ gridScale: 1, scale: 1 }, COMMON_AXIS_PROPERTIES_INDEX, { encode: 1 });
    function isAxisProperty(prop) {
        return !!AXIS_PROPERTIES_INDEX[prop];
    }
    const VG_AXIS_PROPERTIES = flagKeys(VG_AXIS_PROPERTIES_INDEX);

    function assembleTitle(title, config) {
        if (isArray(title)) {
            return title.map(fieldDef => defaultTitle(fieldDef, config)).join(', ');
        }
        return title;
    }
    function assembleAxis(axisCmpt, kind, config, opt = { header: false }) {
        const _a = axisCmpt.combine(), { orient, scale, title, zindex } = _a, axis = __rest(_a, ["orient", "scale", "title", "zindex"]);
        // Remove properties that are not valid for this kind of axis
        keys(axis).forEach(key => {
            const propType = AXIS_PROPERTY_TYPE[key];
            if (propType && propType !== kind && propType !== 'both') {
                delete axis[key];
            }
        });
        if (kind === 'grid') {
            if (!axis.grid) {
                return undefined;
            }
            // Remove unnecessary encode block
            if (axis.encode) {
                // Only need to keep encode block for grid
                const { grid } = axis.encode;
                axis.encode = Object.assign({}, (grid ? { grid } : {}));
                if (keys(axis.encode).length === 0) {
                    delete axis.encode;
                }
            }
            return Object.assign({ scale,
                orient }, axis, { domain: false, labels: false, 
                // Always set min/maxExtent to 0 to ensure that `config.axis*.minExtent` and `config.axis*.maxExtent`
                // would not affect gridAxis
                maxExtent: 0, minExtent: 0, ticks: false, zindex: getFirstDefined(zindex, 0) // put grid behind marks by default
             });
        }
        else {
            // kind === 'main'
            if (!opt.header && axisCmpt.mainExtracted) {
                // if mainExtracted has been extracted to a separate facet
                return undefined;
            }
            // Remove unnecessary encode block
            if (axis.encode) {
                for (const part of AXIS_PARTS) {
                    if (!axisCmpt.hasAxisPart(part)) {
                        delete axis.encode[part];
                    }
                }
                if (keys(axis.encode).length === 0) {
                    delete axis.encode;
                }
            }
            const titleString = assembleTitle(title, config);
            return Object.assign({ scale,
                orient, grid: false }, (titleString ? { title: titleString } : {}), axis, { zindex: getFirstDefined(zindex, 1) // put axis line above marks by default
             });
        }
    }
    /**
     * Add axis signals so grid line works correctly
     * (Fix https://github.com/vega/vega-lite/issues/4226)
     */
    function assembleAxisSignals(model) {
        const { axes } = model.component;
        for (const channel of POSITION_SCALE_CHANNELS) {
            if (axes[channel]) {
                for (const axis of axes[channel]) {
                    if (!axis.get('gridScale')) {
                        // If there is x-axis but no y-scale for gridScale, need to set height/weight so x-axis can draw the grid with the right height.  Same for y-axis and width.
                        const sizeType = channel === 'x' ? 'height' : 'width';
                        return [
                            {
                                name: sizeType,
                                update: model.getSizeSignalRef(sizeType).signal
                            }
                        ];
                    }
                }
            }
        }
        return [];
    }
    function assembleAxes(axisComponents, config) {
        const { x = [], y = [] } = axisComponents;
        return [
            ...x.map(a => assembleAxis(a, 'main', config)),
            ...x.map(a => assembleAxis(a, 'grid', config)),
            ...y.map(a => assembleAxis(a, 'main', config)),
            ...y.map(a => assembleAxis(a, 'grid', config))
        ].filter(a => a); // filter undefined
    }

    const HEADER_TITLE_PROPERTIES_MAP = {
        titleAlign: 'align',
        titleAnchor: 'anchor',
        titleAngle: 'angle',
        titleBaseline: 'baseline',
        titleColor: 'color',
        titleFont: 'font',
        titleFontSize: 'fontSize',
        titleFontWeight: 'fontWeight',
        titleLimit: 'limit',
        titleOrient: 'orient',
        titlePadding: 'offset'
    };
    const HEADER_LABEL_PROPERTIES_MAP = {
        labelAlign: 'align',
        labelAnchor: 'anchor',
        labelAngle: 'angle',
        labelColor: 'color',
        labelFont: 'font',
        labelFontSize: 'fontSize',
        labelLimit: 'limit',
        labelOrient: 'orient',
        labelPadding: 'offset'
    };
    const HEADER_TITLE_PROPERTIES = keys(HEADER_TITLE_PROPERTIES_MAP);
    const HEADER_LABEL_PROPERTIES = keys(HEADER_LABEL_PROPERTIES_MAP);

    const DEFAULT_SORT_OP = 'mean';
    function isSortByEncoding(sort) {
        return !!sort && !!sort['encoding'];
    }
    function isSortField(sort) {
        return !!sort && (sort['op'] === 'count' || !!sort['field']);
    }
    function isSortArray(sort) {
        return !!sort && isArray(sort);
    }

    function getAxisConfig(property, config, channel, orient, scaleType) {
        // configTypes to loop, starting from higher precedence
        const configTypes = [
            ...(scaleType === 'band' ? ['axisBand'] : []),
            channel === 'x' ? 'axisX' : 'axisY',
            // axisTop, axisBottom, ...
            ...(orient ? ['axis' + orient.substr(0, 1).toUpperCase() + orient.substr(1)] : []),
            'axis'
        ];
        for (const configType of configTypes) {
            if (config[configType] && config[configType][property] !== undefined) {
                return config[configType][property];
            }
        }
        return undefined;
    }

    // TODO: we need to refactor this method after we take care of config refactoring
    /**
     * Default rules for whether to show a grid should be shown for a channel.
     * If `grid` is unspecified, the default value is `true` for ordinal scales that are not binned
     */
    function defaultGrid(scaleType, fieldDef) {
        return !hasDiscreteDomain(scaleType) && !isBinning(fieldDef.bin);
    }
    function gridScale(model, channel) {
        const gridChannel = channel === 'x' ? 'y' : 'x';
        if (model.getScaleComponent(gridChannel)) {
            return model.scaleName(gridChannel);
        }
        return undefined;
    }
    function labelAngle(model, specifiedAxis, channel, fieldDef) {
        // try axis value
        if (specifiedAxis.labelAngle !== undefined) {
            return normalizeAngle(specifiedAxis.labelAngle);
        }
        else {
            // try axis config value
            const angle = getAxisConfig('labelAngle', model.config, channel, orient(channel), model.getScaleComponent(channel).get('type'));
            if (angle !== undefined) {
                return normalizeAngle(angle);
            }
            else {
                // get default value
                if (channel === X && contains([NOMINAL, ORDINAL], fieldDef.type)) {
                    return 270;
                }
                // no default
                return undefined;
            }
        }
    }
    function defaultLabelBaseline(angle, axisOrient) {
        if (angle !== undefined) {
            angle = normalizeAngle(angle);
            if (axisOrient === 'top' || axisOrient === 'bottom') {
                if (angle <= 45 || 315 <= angle) {
                    return axisOrient === 'top' ? 'bottom' : 'top';
                }
                else if (135 <= angle && angle <= 225) {
                    return axisOrient === 'top' ? 'top' : 'bottom';
                }
                else {
                    return 'middle';
                }
            }
            else {
                if (angle <= 45 || 315 <= angle || (135 <= angle && angle <= 225)) {
                    return 'middle';
                }
                else if (45 <= angle && angle <= 135) {
                    return axisOrient === 'left' ? 'top' : 'bottom';
                }
                else {
                    return axisOrient === 'left' ? 'bottom' : 'top';
                }
            }
        }
        return undefined;
    }
    function defaultLabelAlign(angle, axisOrient) {
        if (angle !== undefined) {
            angle = normalizeAngle(angle);
            if (axisOrient === 'top' || axisOrient === 'bottom') {
                if (angle % 180 === 0) {
                    return 'center';
                }
                else if (0 < angle && angle < 180) {
                    return axisOrient === 'top' ? 'right' : 'left';
                }
                else {
                    return axisOrient === 'top' ? 'left' : 'right';
                }
            }
            else {
                if ((angle + 90) % 180 === 0) {
                    return 'center';
                }
                else if (90 <= angle && angle < 270) {
                    return axisOrient === 'left' ? 'left' : 'right';
                }
                else {
                    return axisOrient === 'left' ? 'right' : 'left';
                }
            }
        }
        return undefined;
    }
    function defaultLabelFlush(fieldDef, channel) {
        if (channel === 'x' && contains(['quantitative', 'temporal'], fieldDef.type)) {
            return true;
        }
        return undefined;
    }
    function defaultLabelOverlap(fieldDef, scaleType) {
        // do not prevent overlap for nominal data because there is no way to infer what the missing labels are
        if (fieldDef.type !== 'nominal') {
            if (scaleType === 'log') {
                return 'greedy';
            }
            return true;
        }
        return undefined;
    }
    function orient(channel) {
        switch (channel) {
            case X:
                return 'bottom';
            case Y:
                return 'left';
        }
        /* istanbul ignore next: This should never happen. */
        throw new Error(message.INVALID_CHANNEL_FOR_AXIS);
    }
    function defaultTickCount({ fieldDef, scaleType, size }) {
        if (!hasDiscreteDomain(scaleType) &&
            scaleType !== 'log' &&
            !contains(['month', 'hours', 'day', 'quarter'], fieldDef.timeUnit)) {
            if (isBinning(fieldDef.bin)) {
                // for binned data, we don't want more ticks than maxbins
                return { signal: `ceil(${size.signal}/10)` };
            }
            return { signal: `ceil(${size.signal}/40)` };
        }
        return undefined;
    }
    function values(specifiedAxis, model, fieldDef) {
        const vals = specifiedAxis.values;
        if (vals) {
            return valueArray(fieldDef, vals);
        }
        return undefined;
    }

    var properties = /*#__PURE__*/Object.freeze({
        defaultGrid: defaultGrid,
        gridScale: gridScale,
        labelAngle: labelAngle,
        defaultLabelBaseline: defaultLabelBaseline,
        defaultLabelAlign: defaultLabelAlign,
        defaultLabelFlush: defaultLabelFlush,
        defaultLabelOverlap: defaultLabelOverlap,
        orient: orient,
        defaultTickCount: defaultTickCount,
        values: values
    });

    function applyMarkConfig(e, model, propsList) {
        for (const property of propsList) {
            const value = getMarkConfig(property, model.markDef, model.config);
            if (value !== undefined) {
                e[property] = { value: value };
            }
        }
        return e;
    }
    function getStyles(mark) {
        return [].concat(mark.type, mark.style || []);
    }
    /**
     * Return property value from style or mark specific config property if exists.
     * Otherwise, return general mark specific config.
     */
    function getMarkConfig(channel, mark, config, { vgChannel } = {} // Note: Ham: I use `any` here as it's too hard to make TS knows that MarkConfig[vgChannel] would have the same type as MarkConfig[P]
    ) {
        return getFirstDefined(
        // style config has highest precedence
        vgChannel ? getStyleConfig(channel, mark, config.style) : undefined, getStyleConfig(channel, mark, config.style), 
        // then mark-specific config
        vgChannel ? config[mark.type][vgChannel] : undefined, config[mark.type][channel], 
        // If there is vgChannel, skip vl channel.
        // For example, vl size for text is vg fontSize, but config.mark.size is only for point size.
        vgChannel ? config.mark[vgChannel] : config.mark[channel]);
    }
    function getStyleConfig(prop, mark, styleConfigIndex) {
        const styles = getStyles(mark);
        let value;
        for (const style of styles) {
            const styleConfig = styleConfigIndex[style];
            // MarkConfig extends VgMarkConfig so a prop may not be a valid property for style
            // However here we also check if it is defined, so it is okay to cast here
            const p = prop;
            if (styleConfig && styleConfig[p] !== undefined) {
                value = styleConfig[p];
            }
        }
        return value;
    }
    function formatSignalRef(fieldDef, specifiedFormat, expr, config) {
        if (isTimeFormatFieldDef(fieldDef)) {
            const isUTCScale = isScaleFieldDef(fieldDef) && fieldDef['scale'] && fieldDef['scale'].type === ScaleType.UTC;
            return {
                signal: timeFormatExpression(vgField(fieldDef, {
                    expr
                }), fieldDef.timeUnit, specifiedFormat, config.text.shortTimeLabels, config.timeFormat, isUTCScale, true)
            };
        }
        else {
            const format = numberFormat(fieldDef, specifiedFormat, config);
            if (isBinning(fieldDef.bin)) {
                const startField = vgField(fieldDef, { expr });
                const endField = vgField(fieldDef, { expr, binSuffix: 'end' });
                return {
                    signal: binFormatExpression(startField, endField, format, config)
                };
            }
            else if (fieldDef.type === 'quantitative') {
                return {
                    signal: `${formatExpr(vgField(fieldDef, { expr, binSuffix: 'range' }), format)}`
                };
            }
            else {
                return { signal: `''+${vgField(fieldDef, { expr })}` };
            }
        }
    }
    /**
     * Returns number format for a fieldDef
     */
    function numberFormat(fieldDef, specifiedFormat, config) {
        // Specified format in axis/legend has higher precedence than fieldDef.format
        if (specifiedFormat) {
            return specifiedFormat;
        }
        if (fieldDef.type === QUANTITATIVE) {
            // we only apply the default if the field is quantitative
            return config.numberFormat;
        }
        return undefined;
    }
    function formatExpr(field, format) {
        return `format(${field}, "${format || ''}")`;
    }
    function numberFormatExpr(field, specifiedFormat, config) {
        return formatExpr(field, specifiedFormat || config.numberFormat);
    }
    function binFormatExpression(startField, endField, format, config) {
        return `${startField} === null || isNaN(${startField}) ? "null" : ${numberFormatExpr(startField, format, config)} + " - " + ${numberFormatExpr(endField, format, config)}`;
    }
    /**
     * Returns the time expression used for axis/legend labels or text mark for a temporal field
     */
    function timeFormatExpression(field, timeUnit, format, shortTimeLabels, rawTimeFormat, // should be provided only for actual text and headers, not axis/legend labels
    isUTCScale, alwaysReturn = false) {
        if (!timeUnit || format) {
            // If there is not time unit, or if user explicitly specify format for axis/legend/text.
            format = format || rawTimeFormat; // only use provided timeFormat if there is no timeUnit.
            if (format || alwaysReturn) {
                return `${isUTCScale ? 'utc' : 'time'}Format(${field}, '${format}')`;
            }
            else {
                return undefined;
            }
        }
        else {
            return formatExpression(timeUnit, field, shortTimeLabels, isUTCScale);
        }
    }
    /**
     * Return Vega sort parameters (tuple of field and order).
     */
    function sortParams(orderDef, fieldRefOption) {
        return (isArray(orderDef) ? orderDef : [orderDef]).reduce((s, orderChannelDef) => {
            s.field.push(vgField(orderChannelDef, fieldRefOption));
            s.order.push(orderChannelDef.sort || 'ascending');
            return s;
        }, { field: [], order: [] });
    }
    function mergeTitleFieldDefs(f1, f2) {
        const merged = [...f1];
        f2.forEach(fdToMerge => {
            for (const fieldDef1 of merged) {
                // If already exists, no need to append to merged array
                if (stringify(fieldDef1) === stringify(fdToMerge)) {
                    return;
                }
            }
            merged.push(fdToMerge);
        });
        return merged;
    }
    function mergeTitle(title1, title2) {
        if (title1 === title2 || !title2) {
            // if titles are the same or title2 is falsy
            return title1;
        }
        else if (!title1) {
            // if title1 is falsy
            return title2;
        }
        else {
            // join title with comma if they are different
            return title1 + ', ' + title2;
        }
    }
    function mergeTitleComponent(v1, v2) {
        if (isArray(v1.value) && isArray(v2.value)) {
            return {
                explicit: v1.explicit,
                value: mergeTitleFieldDefs(v1.value, v2.value)
            };
        }
        else if (!isArray(v1.value) && !isArray(v2.value)) {
            return {
                explicit: v1.explicit,
                value: mergeTitle(v1.value, v2.value)
            };
        }
        /* istanbul ignore next: Condition should not happen -- only for warning in development. */
        throw new Error('It should never reach here');
    }

    /**
     * A node in the dataflow tree.
     */
    class DataFlowNode {
        constructor(parent, debugName) {
            this.debugName = debugName;
            this._children = [];
            this._parent = null;
            if (parent) {
                this.parent = parent;
            }
        }
        /**
         * Clone this node with a deep copy but don't clone links to children or parents.
         */
        clone() {
            throw new Error('Cannot clone node');
        }
        /**
         * Return a hash of the node.
         */
        hash() {
            if (this._hash === undefined) {
                this._hash = uniqueId();
            }
            return this._hash;
        }
        /**
         * Set of fields that are being created by this node.
         */
        producedFields() {
            return new Set();
        }
        dependentFields() {
            return new Set();
        }
        get parent() {
            return this._parent;
        }
        /**
         * Set the parent of the node and also add this node to the parent's children.
         */
        set parent(parent) {
            this._parent = parent;
            if (parent) {
                parent.addChild(this);
            }
        }
        get children() {
            return this._children;
        }
        numChildren() {
            return this._children.length;
        }
        addChild(child, loc) {
            // do not add the same child twice
            if (this._children.indexOf(child) > -1) {
                console.warn('Attempt to add the same child twice.');
                return;
            }
            if (loc !== undefined) {
                this._children.splice(loc, 0, child);
            }
            else {
                this._children.push(child);
            }
        }
        removeChild(oldChild) {
            const loc = this._children.indexOf(oldChild);
            this._children.splice(loc, 1);
            return loc;
        }
        /**
         * Remove node from the dataflow.
         */
        remove() {
            let loc = this._parent.removeChild(this);
            for (const child of this._children) {
                // do not use the set method because we want to insert at a particular location
                child._parent = this._parent;
                this._parent.addChild(child, loc++);
            }
        }
        /**
         * Insert another node as a parent of this node.
         */
        insertAsParentOf(other) {
            const parent = other.parent;
            parent.removeChild(this);
            this.parent = parent;
            other.parent = this;
        }
        swapWithParent() {
            const parent = this._parent;
            const newParent = parent.parent;
            // reconnect the children
            for (const child of this._children) {
                child.parent = parent;
            }
            // remove old links
            this._children = []; // equivalent to removing every child link one by one
            parent.removeChild(this);
            parent.parent.removeChild(parent);
            // swap two nodes
            this.parent = newParent;
            parent.parent = this;
        }
    }
    class OutputNode extends DataFlowNode {
        /**
         * @param source The name of the source. Will change in assemble.
         * @param type The type of the output node.
         * @param refCounts A global ref counter map.
         */
        constructor(parent, source, type, refCounts) {
            super(parent, source);
            this.type = type;
            this.refCounts = refCounts;
            this._source = this._name = source;
            if (this.refCounts && !(this._name in this.refCounts)) {
                this.refCounts[this._name] = 0;
            }
        }
        clone() {
            const cloneObj = new this.constructor();
            cloneObj.debugName = 'clone_' + this.debugName;
            cloneObj._source = this._source;
            cloneObj._name = 'clone_' + this._name;
            cloneObj.type = this.type;
            cloneObj.refCounts = this.refCounts;
            cloneObj.refCounts[cloneObj._name] = 0;
            return cloneObj;
        }
        /**
         * Request the datasource name and increase the ref counter.
         *
         * During the parsing phase, this will return the simple name such as 'main' or 'raw'.
         * It is crucial to request the name from an output node to mark it as a required node.
         * If nobody ever requests the name, this datasource will not be instantiated in the assemble phase.
         *
         * In the assemble phase, this will return the correct name.
         */
        getSource() {
            this.refCounts[this._name]++;
            return this._source;
        }
        isRequired() {
            return !!this.refCounts[this._name];
        }
        setSource(source) {
            this._source = source;
        }
    }

    var RawCode = 'RawCode';
    var Literal = 'Literal';
    var Property = 'Property';
    var Identifier = 'Identifier';

    var ArrayExpression = 'ArrayExpression';
    var BinaryExpression = 'BinaryExpression';
    var CallExpression = 'CallExpression';
    var ConditionalExpression = 'ConditionalExpression';
    var LogicalExpression = 'LogicalExpression';
    var MemberExpression = 'MemberExpression';
    var ObjectExpression = 'ObjectExpression';
    var UnaryExpression = 'UnaryExpression';

    function ASTNode(type) {
      this.type = type;
    }

    ASTNode.prototype.visit = function(visitor) {
      var node = this, c, i, n;

      if (visitor(node)) return 1;

      for (c=children(node), i=0, n=c.length; i<n; ++i) {
        if (c[i].visit(visitor)) return 1;
      }
    };

    function children(node) {
      switch (node.type) {
        case ArrayExpression:
          return node.elements;
        case BinaryExpression:
        case LogicalExpression:
          return [node.left, node.right];
        case CallExpression:
          var args = node.arguments.slice();
          args.unshift(node.callee);
          return args;
        case ConditionalExpression:
          return [node.test, node.consequent, node.alternate];
        case MemberExpression:
          return [node.object, node.property];
        case ObjectExpression:
          return node.properties;
        case Property:
          return [node.key, node.value];
        case UnaryExpression:
          return [node.argument];
        case Identifier:
        case Literal:
        case RawCode:
        default:
          return [];
      }
    }

    /*
      The following expression parser is based on Esprima (http://esprima.org/).
      Original header comment and license for Esprima is included here:

      Copyright (C) 2013 Ariya Hidayat <ariya.hidayat@gmail.com>
      Copyright (C) 2013 Thaddee Tyl <thaddee.tyl@gmail.com>
      Copyright (C) 2013 Mathias Bynens <mathias@qiwi.be>
      Copyright (C) 2012 Ariya Hidayat <ariya.hidayat@gmail.com>
      Copyright (C) 2012 Mathias Bynens <mathias@qiwi.be>
      Copyright (C) 2012 Joost-Wim Boekesteijn <joost-wim@boekesteijn.nl>
      Copyright (C) 2012 Kris Kowal <kris.kowal@cixar.com>
      Copyright (C) 2012 Yusuke Suzuki <utatane.tea@gmail.com>
      Copyright (C) 2012 Arpad Borsos <arpad.borsos@googlemail.com>
      Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

      Redistribution and use in source and binary forms, with or without
      modification, are permitted provided that the following conditions are met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.

      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
      AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
      IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
      ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
      DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
      (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
      LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
      ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
      THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    */

    var TokenName,
        source,
        index,
        length,
        lookahead;

    var TokenBooleanLiteral = 1,
        TokenEOF = 2,
        TokenIdentifier = 3,
        TokenKeyword = 4,
        TokenNullLiteral = 5,
        TokenNumericLiteral = 6,
        TokenPunctuator = 7,
        TokenStringLiteral = 8,
        TokenRegularExpression = 9;

    TokenName = {};
    TokenName[TokenBooleanLiteral] = 'Boolean';
    TokenName[TokenEOF] = '<end>';
    TokenName[TokenIdentifier] = 'Identifier';
    TokenName[TokenKeyword] = 'Keyword';
    TokenName[TokenNullLiteral] = 'Null';
    TokenName[TokenNumericLiteral] = 'Numeric';
    TokenName[TokenPunctuator] = 'Punctuator';
    TokenName[TokenStringLiteral] = 'String';
    TokenName[TokenRegularExpression] = 'RegularExpression';

    var SyntaxArrayExpression = 'ArrayExpression',
        SyntaxBinaryExpression = 'BinaryExpression',
        SyntaxCallExpression = 'CallExpression',
        SyntaxConditionalExpression = 'ConditionalExpression',
        SyntaxIdentifier = 'Identifier',
        SyntaxLiteral = 'Literal',
        SyntaxLogicalExpression = 'LogicalExpression',
        SyntaxMemberExpression = 'MemberExpression',
        SyntaxObjectExpression = 'ObjectExpression',
        SyntaxProperty = 'Property',
        SyntaxUnaryExpression = 'UnaryExpression';

    // Error messages should be identical to V8.
    var MessageUnexpectedToken = 'Unexpected token %0',
        MessageUnexpectedNumber = 'Unexpected number',
        MessageUnexpectedString = 'Unexpected string',
        MessageUnexpectedIdentifier = 'Unexpected identifier',
        MessageUnexpectedReserved = 'Unexpected reserved word',
        MessageUnexpectedEOS = 'Unexpected end of input',
        MessageInvalidRegExp = 'Invalid regular expression',
        MessageUnterminatedRegExp = 'Invalid regular expression: missing /',
        MessageStrictOctalLiteral = 'Octal literals are not allowed in strict mode.',
        MessageStrictDuplicateProperty = 'Duplicate data property in object literal not allowed in strict mode';

    var ILLEGAL = 'ILLEGAL',
        DISABLED = 'Disabled.';

    // See also tools/generate-unicode-regex.py.
      var RegexNonAsciiIdentifierStart = new RegExp("[\\xAA\\xB5\\xBA\\xC0-\\xD6\\xD8-\\xF6\\xF8-\\u02C1\\u02C6-\\u02D1\\u02E0-\\u02E4\\u02EC\\u02EE\\u0370-\\u0374\\u0376\\u0377\\u037A-\\u037D\\u037F\\u0386\\u0388-\\u038A\\u038C\\u038E-\\u03A1\\u03A3-\\u03F5\\u03F7-\\u0481\\u048A-\\u052F\\u0531-\\u0556\\u0559\\u0561-\\u0587\\u05D0-\\u05EA\\u05F0-\\u05F2\\u0620-\\u064A\\u066E\\u066F\\u0671-\\u06D3\\u06D5\\u06E5\\u06E6\\u06EE\\u06EF\\u06FA-\\u06FC\\u06FF\\u0710\\u0712-\\u072F\\u074D-\\u07A5\\u07B1\\u07CA-\\u07EA\\u07F4\\u07F5\\u07FA\\u0800-\\u0815\\u081A\\u0824\\u0828\\u0840-\\u0858\\u08A0-\\u08B2\\u0904-\\u0939\\u093D\\u0950\\u0958-\\u0961\\u0971-\\u0980\\u0985-\\u098C\\u098F\\u0990\\u0993-\\u09A8\\u09AA-\\u09B0\\u09B2\\u09B6-\\u09B9\\u09BD\\u09CE\\u09DC\\u09DD\\u09DF-\\u09E1\\u09F0\\u09F1\\u0A05-\\u0A0A\\u0A0F\\u0A10\\u0A13-\\u0A28\\u0A2A-\\u0A30\\u0A32\\u0A33\\u0A35\\u0A36\\u0A38\\u0A39\\u0A59-\\u0A5C\\u0A5E\\u0A72-\\u0A74\\u0A85-\\u0A8D\\u0A8F-\\u0A91\\u0A93-\\u0AA8\\u0AAA-\\u0AB0\\u0AB2\\u0AB3\\u0AB5-\\u0AB9\\u0ABD\\u0AD0\\u0AE0\\u0AE1\\u0B05-\\u0B0C\\u0B0F\\u0B10\\u0B13-\\u0B28\\u0B2A-\\u0B30\\u0B32\\u0B33\\u0B35-\\u0B39\\u0B3D\\u0B5C\\u0B5D\\u0B5F-\\u0B61\\u0B71\\u0B83\\u0B85-\\u0B8A\\u0B8E-\\u0B90\\u0B92-\\u0B95\\u0B99\\u0B9A\\u0B9C\\u0B9E\\u0B9F\\u0BA3\\u0BA4\\u0BA8-\\u0BAA\\u0BAE-\\u0BB9\\u0BD0\\u0C05-\\u0C0C\\u0C0E-\\u0C10\\u0C12-\\u0C28\\u0C2A-\\u0C39\\u0C3D\\u0C58\\u0C59\\u0C60\\u0C61\\u0C85-\\u0C8C\\u0C8E-\\u0C90\\u0C92-\\u0CA8\\u0CAA-\\u0CB3\\u0CB5-\\u0CB9\\u0CBD\\u0CDE\\u0CE0\\u0CE1\\u0CF1\\u0CF2\\u0D05-\\u0D0C\\u0D0E-\\u0D10\\u0D12-\\u0D3A\\u0D3D\\u0D4E\\u0D60\\u0D61\\u0D7A-\\u0D7F\\u0D85-\\u0D96\\u0D9A-\\u0DB1\\u0DB3-\\u0DBB\\u0DBD\\u0DC0-\\u0DC6\\u0E01-\\u0E30\\u0E32\\u0E33\\u0E40-\\u0E46\\u0E81\\u0E82\\u0E84\\u0E87\\u0E88\\u0E8A\\u0E8D\\u0E94-\\u0E97\\u0E99-\\u0E9F\\u0EA1-\\u0EA3\\u0EA5\\u0EA7\\u0EAA\\u0EAB\\u0EAD-\\u0EB0\\u0EB2\\u0EB3\\u0EBD\\u0EC0-\\u0EC4\\u0EC6\\u0EDC-\\u0EDF\\u0F00\\u0F40-\\u0F47\\u0F49-\\u0F6C\\u0F88-\\u0F8C\\u1000-\\u102A\\u103F\\u1050-\\u1055\\u105A-\\u105D\\u1061\\u1065\\u1066\\u106E-\\u1070\\u1075-\\u1081\\u108E\\u10A0-\\u10C5\\u10C7\\u10CD\\u10D0-\\u10FA\\u10FC-\\u1248\\u124A-\\u124D\\u1250-\\u1256\\u1258\\u125A-\\u125D\\u1260-\\u1288\\u128A-\\u128D\\u1290-\\u12B0\\u12B2-\\u12B5\\u12B8-\\u12BE\\u12C0\\u12C2-\\u12C5\\u12C8-\\u12D6\\u12D8-\\u1310\\u1312-\\u1315\\u1318-\\u135A\\u1380-\\u138F\\u13A0-\\u13F4\\u1401-\\u166C\\u166F-\\u167F\\u1681-\\u169A\\u16A0-\\u16EA\\u16EE-\\u16F8\\u1700-\\u170C\\u170E-\\u1711\\u1720-\\u1731\\u1740-\\u1751\\u1760-\\u176C\\u176E-\\u1770\\u1780-\\u17B3\\u17D7\\u17DC\\u1820-\\u1877\\u1880-\\u18A8\\u18AA\\u18B0-\\u18F5\\u1900-\\u191E\\u1950-\\u196D\\u1970-\\u1974\\u1980-\\u19AB\\u19C1-\\u19C7\\u1A00-\\u1A16\\u1A20-\\u1A54\\u1AA7\\u1B05-\\u1B33\\u1B45-\\u1B4B\\u1B83-\\u1BA0\\u1BAE\\u1BAF\\u1BBA-\\u1BE5\\u1C00-\\u1C23\\u1C4D-\\u1C4F\\u1C5A-\\u1C7D\\u1CE9-\\u1CEC\\u1CEE-\\u1CF1\\u1CF5\\u1CF6\\u1D00-\\u1DBF\\u1E00-\\u1F15\\u1F18-\\u1F1D\\u1F20-\\u1F45\\u1F48-\\u1F4D\\u1F50-\\u1F57\\u1F59\\u1F5B\\u1F5D\\u1F5F-\\u1F7D\\u1F80-\\u1FB4\\u1FB6-\\u1FBC\\u1FBE\\u1FC2-\\u1FC4\\u1FC6-\\u1FCC\\u1FD0-\\u1FD3\\u1FD6-\\u1FDB\\u1FE0-\\u1FEC\\u1FF2-\\u1FF4\\u1FF6-\\u1FFC\\u2071\\u207F\\u2090-\\u209C\\u2102\\u2107\\u210A-\\u2113\\u2115\\u2119-\\u211D\\u2124\\u2126\\u2128\\u212A-\\u212D\\u212F-\\u2139\\u213C-\\u213F\\u2145-\\u2149\\u214E\\u2160-\\u2188\\u2C00-\\u2C2E\\u2C30-\\u2C5E\\u2C60-\\u2CE4\\u2CEB-\\u2CEE\\u2CF2\\u2CF3\\u2D00-\\u2D25\\u2D27\\u2D2D\\u2D30-\\u2D67\\u2D6F\\u2D80-\\u2D96\\u2DA0-\\u2DA6\\u2DA8-\\u2DAE\\u2DB0-\\u2DB6\\u2DB8-\\u2DBE\\u2DC0-\\u2DC6\\u2DC8-\\u2DCE\\u2DD0-\\u2DD6\\u2DD8-\\u2DDE\\u2E2F\\u3005-\\u3007\\u3021-\\u3029\\u3031-\\u3035\\u3038-\\u303C\\u3041-\\u3096\\u309D-\\u309F\\u30A1-\\u30FA\\u30FC-\\u30FF\\u3105-\\u312D\\u3131-\\u318E\\u31A0-\\u31BA\\u31F0-\\u31FF\\u3400-\\u4DB5\\u4E00-\\u9FCC\\uA000-\\uA48C\\uA4D0-\\uA4FD\\uA500-\\uA60C\\uA610-\\uA61F\\uA62A\\uA62B\\uA640-\\uA66E\\uA67F-\\uA69D\\uA6A0-\\uA6EF\\uA717-\\uA71F\\uA722-\\uA788\\uA78B-\\uA78E\\uA790-\\uA7AD\\uA7B0\\uA7B1\\uA7F7-\\uA801\\uA803-\\uA805\\uA807-\\uA80A\\uA80C-\\uA822\\uA840-\\uA873\\uA882-\\uA8B3\\uA8F2-\\uA8F7\\uA8FB\\uA90A-\\uA925\\uA930-\\uA946\\uA960-\\uA97C\\uA984-\\uA9B2\\uA9CF\\uA9E0-\\uA9E4\\uA9E6-\\uA9EF\\uA9FA-\\uA9FE\\uAA00-\\uAA28\\uAA40-\\uAA42\\uAA44-\\uAA4B\\uAA60-\\uAA76\\uAA7A\\uAA7E-\\uAAAF\\uAAB1\\uAAB5\\uAAB6\\uAAB9-\\uAABD\\uAAC0\\uAAC2\\uAADB-\\uAADD\\uAAE0-\\uAAEA\\uAAF2-\\uAAF4\\uAB01-\\uAB06\\uAB09-\\uAB0E\\uAB11-\\uAB16\\uAB20-\\uAB26\\uAB28-\\uAB2E\\uAB30-\\uAB5A\\uAB5C-\\uAB5F\\uAB64\\uAB65\\uABC0-\\uABE2\\uAC00-\\uD7A3\\uD7B0-\\uD7C6\\uD7CB-\\uD7FB\\uF900-\\uFA6D\\uFA70-\\uFAD9\\uFB00-\\uFB06\\uFB13-\\uFB17\\uFB1D\\uFB1F-\\uFB28\\uFB2A-\\uFB36\\uFB38-\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46-\\uFBB1\\uFBD3-\\uFD3D\\uFD50-\\uFD8F\\uFD92-\\uFDC7\\uFDF0-\\uFDFB\\uFE70-\\uFE74\\uFE76-\\uFEFC\\uFF21-\\uFF3A\\uFF41-\\uFF5A\\uFF66-\\uFFBE\\uFFC2-\\uFFC7\\uFFCA-\\uFFCF\\uFFD2-\\uFFD7\\uFFDA-\\uFFDC]"),
          RegexNonAsciiIdentifierPart = new RegExp("[\\xAA\\xB5\\xBA\\xC0-\\xD6\\xD8-\\xF6\\xF8-\\u02C1\\u02C6-\\u02D1\\u02E0-\\u02E4\\u02EC\\u02EE\\u0300-\\u0374\\u0376\\u0377\\u037A-\\u037D\\u037F\\u0386\\u0388-\\u038A\\u038C\\u038E-\\u03A1\\u03A3-\\u03F5\\u03F7-\\u0481\\u0483-\\u0487\\u048A-\\u052F\\u0531-\\u0556\\u0559\\u0561-\\u0587\\u0591-\\u05BD\\u05BF\\u05C1\\u05C2\\u05C4\\u05C5\\u05C7\\u05D0-\\u05EA\\u05F0-\\u05F2\\u0610-\\u061A\\u0620-\\u0669\\u066E-\\u06D3\\u06D5-\\u06DC\\u06DF-\\u06E8\\u06EA-\\u06FC\\u06FF\\u0710-\\u074A\\u074D-\\u07B1\\u07C0-\\u07F5\\u07FA\\u0800-\\u082D\\u0840-\\u085B\\u08A0-\\u08B2\\u08E4-\\u0963\\u0966-\\u096F\\u0971-\\u0983\\u0985-\\u098C\\u098F\\u0990\\u0993-\\u09A8\\u09AA-\\u09B0\\u09B2\\u09B6-\\u09B9\\u09BC-\\u09C4\\u09C7\\u09C8\\u09CB-\\u09CE\\u09D7\\u09DC\\u09DD\\u09DF-\\u09E3\\u09E6-\\u09F1\\u0A01-\\u0A03\\u0A05-\\u0A0A\\u0A0F\\u0A10\\u0A13-\\u0A28\\u0A2A-\\u0A30\\u0A32\\u0A33\\u0A35\\u0A36\\u0A38\\u0A39\\u0A3C\\u0A3E-\\u0A42\\u0A47\\u0A48\\u0A4B-\\u0A4D\\u0A51\\u0A59-\\u0A5C\\u0A5E\\u0A66-\\u0A75\\u0A81-\\u0A83\\u0A85-\\u0A8D\\u0A8F-\\u0A91\\u0A93-\\u0AA8\\u0AAA-\\u0AB0\\u0AB2\\u0AB3\\u0AB5-\\u0AB9\\u0ABC-\\u0AC5\\u0AC7-\\u0AC9\\u0ACB-\\u0ACD\\u0AD0\\u0AE0-\\u0AE3\\u0AE6-\\u0AEF\\u0B01-\\u0B03\\u0B05-\\u0B0C\\u0B0F\\u0B10\\u0B13-\\u0B28\\u0B2A-\\u0B30\\u0B32\\u0B33\\u0B35-\\u0B39\\u0B3C-\\u0B44\\u0B47\\u0B48\\u0B4B-\\u0B4D\\u0B56\\u0B57\\u0B5C\\u0B5D\\u0B5F-\\u0B63\\u0B66-\\u0B6F\\u0B71\\u0B82\\u0B83\\u0B85-\\u0B8A\\u0B8E-\\u0B90\\u0B92-\\u0B95\\u0B99\\u0B9A\\u0B9C\\u0B9E\\u0B9F\\u0BA3\\u0BA4\\u0BA8-\\u0BAA\\u0BAE-\\u0BB9\\u0BBE-\\u0BC2\\u0BC6-\\u0BC8\\u0BCA-\\u0BCD\\u0BD0\\u0BD7\\u0BE6-\\u0BEF\\u0C00-\\u0C03\\u0C05-\\u0C0C\\u0C0E-\\u0C10\\u0C12-\\u0C28\\u0C2A-\\u0C39\\u0C3D-\\u0C44\\u0C46-\\u0C48\\u0C4A-\\u0C4D\\u0C55\\u0C56\\u0C58\\u0C59\\u0C60-\\u0C63\\u0C66-\\u0C6F\\u0C81-\\u0C83\\u0C85-\\u0C8C\\u0C8E-\\u0C90\\u0C92-\\u0CA8\\u0CAA-\\u0CB3\\u0CB5-\\u0CB9\\u0CBC-\\u0CC4\\u0CC6-\\u0CC8\\u0CCA-\\u0CCD\\u0CD5\\u0CD6\\u0CDE\\u0CE0-\\u0CE3\\u0CE6-\\u0CEF\\u0CF1\\u0CF2\\u0D01-\\u0D03\\u0D05-\\u0D0C\\u0D0E-\\u0D10\\u0D12-\\u0D3A\\u0D3D-\\u0D44\\u0D46-\\u0D48\\u0D4A-\\u0D4E\\u0D57\\u0D60-\\u0D63\\u0D66-\\u0D6F\\u0D7A-\\u0D7F\\u0D82\\u0D83\\u0D85-\\u0D96\\u0D9A-\\u0DB1\\u0DB3-\\u0DBB\\u0DBD\\u0DC0-\\u0DC6\\u0DCA\\u0DCF-\\u0DD4\\u0DD6\\u0DD8-\\u0DDF\\u0DE6-\\u0DEF\\u0DF2\\u0DF3\\u0E01-\\u0E3A\\u0E40-\\u0E4E\\u0E50-\\u0E59\\u0E81\\u0E82\\u0E84\\u0E87\\u0E88\\u0E8A\\u0E8D\\u0E94-\\u0E97\\u0E99-\\u0E9F\\u0EA1-\\u0EA3\\u0EA5\\u0EA7\\u0EAA\\u0EAB\\u0EAD-\\u0EB9\\u0EBB-\\u0EBD\\u0EC0-\\u0EC4\\u0EC6\\u0EC8-\\u0ECD\\u0ED0-\\u0ED9\\u0EDC-\\u0EDF\\u0F00\\u0F18\\u0F19\\u0F20-\\u0F29\\u0F35\\u0F37\\u0F39\\u0F3E-\\u0F47\\u0F49-\\u0F6C\\u0F71-\\u0F84\\u0F86-\\u0F97\\u0F99-\\u0FBC\\u0FC6\\u1000-\\u1049\\u1050-\\u109D\\u10A0-\\u10C5\\u10C7\\u10CD\\u10D0-\\u10FA\\u10FC-\\u1248\\u124A-\\u124D\\u1250-\\u1256\\u1258\\u125A-\\u125D\\u1260-\\u1288\\u128A-\\u128D\\u1290-\\u12B0\\u12B2-\\u12B5\\u12B8-\\u12BE\\u12C0\\u12C2-\\u12C5\\u12C8-\\u12D6\\u12D8-\\u1310\\u1312-\\u1315\\u1318-\\u135A\\u135D-\\u135F\\u1380-\\u138F\\u13A0-\\u13F4\\u1401-\\u166C\\u166F-\\u167F\\u1681-\\u169A\\u16A0-\\u16EA\\u16EE-\\u16F8\\u1700-\\u170C\\u170E-\\u1714\\u1720-\\u1734\\u1740-\\u1753\\u1760-\\u176C\\u176E-\\u1770\\u1772\\u1773\\u1780-\\u17D3\\u17D7\\u17DC\\u17DD\\u17E0-\\u17E9\\u180B-\\u180D\\u1810-\\u1819\\u1820-\\u1877\\u1880-\\u18AA\\u18B0-\\u18F5\\u1900-\\u191E\\u1920-\\u192B\\u1930-\\u193B\\u1946-\\u196D\\u1970-\\u1974\\u1980-\\u19AB\\u19B0-\\u19C9\\u19D0-\\u19D9\\u1A00-\\u1A1B\\u1A20-\\u1A5E\\u1A60-\\u1A7C\\u1A7F-\\u1A89\\u1A90-\\u1A99\\u1AA7\\u1AB0-\\u1ABD\\u1B00-\\u1B4B\\u1B50-\\u1B59\\u1B6B-\\u1B73\\u1B80-\\u1BF3\\u1C00-\\u1C37\\u1C40-\\u1C49\\u1C4D-\\u1C7D\\u1CD0-\\u1CD2\\u1CD4-\\u1CF6\\u1CF8\\u1CF9\\u1D00-\\u1DF5\\u1DFC-\\u1F15\\u1F18-\\u1F1D\\u1F20-\\u1F45\\u1F48-\\u1F4D\\u1F50-\\u1F57\\u1F59\\u1F5B\\u1F5D\\u1F5F-\\u1F7D\\u1F80-\\u1FB4\\u1FB6-\\u1FBC\\u1FBE\\u1FC2-\\u1FC4\\u1FC6-\\u1FCC\\u1FD0-\\u1FD3\\u1FD6-\\u1FDB\\u1FE0-\\u1FEC\\u1FF2-\\u1FF4\\u1FF6-\\u1FFC\\u200C\\u200D\\u203F\\u2040\\u2054\\u2071\\u207F\\u2090-\\u209C\\u20D0-\\u20DC\\u20E1\\u20E5-\\u20F0\\u2102\\u2107\\u210A-\\u2113\\u2115\\u2119-\\u211D\\u2124\\u2126\\u2128\\u212A-\\u212D\\u212F-\\u2139\\u213C-\\u213F\\u2145-\\u2149\\u214E\\u2160-\\u2188\\u2C00-\\u2C2E\\u2C30-\\u2C5E\\u2C60-\\u2CE4\\u2CEB-\\u2CF3\\u2D00-\\u2D25\\u2D27\\u2D2D\\u2D30-\\u2D67\\u2D6F\\u2D7F-\\u2D96\\u2DA0-\\u2DA6\\u2DA8-\\u2DAE\\u2DB0-\\u2DB6\\u2DB8-\\u2DBE\\u2DC0-\\u2DC6\\u2DC8-\\u2DCE\\u2DD0-\\u2DD6\\u2DD8-\\u2DDE\\u2DE0-\\u2DFF\\u2E2F\\u3005-\\u3007\\u3021-\\u302F\\u3031-\\u3035\\u3038-\\u303C\\u3041-\\u3096\\u3099\\u309A\\u309D-\\u309F\\u30A1-\\u30FA\\u30FC-\\u30FF\\u3105-\\u312D\\u3131-\\u318E\\u31A0-\\u31BA\\u31F0-\\u31FF\\u3400-\\u4DB5\\u4E00-\\u9FCC\\uA000-\\uA48C\\uA4D0-\\uA4FD\\uA500-\\uA60C\\uA610-\\uA62B\\uA640-\\uA66F\\uA674-\\uA67D\\uA67F-\\uA69D\\uA69F-\\uA6F1\\uA717-\\uA71F\\uA722-\\uA788\\uA78B-\\uA78E\\uA790-\\uA7AD\\uA7B0\\uA7B1\\uA7F7-\\uA827\\uA840-\\uA873\\uA880-\\uA8C4\\uA8D0-\\uA8D9\\uA8E0-\\uA8F7\\uA8FB\\uA900-\\uA92D\\uA930-\\uA953\\uA960-\\uA97C\\uA980-\\uA9C0\\uA9CF-\\uA9D9\\uA9E0-\\uA9FE\\uAA00-\\uAA36\\uAA40-\\uAA4D\\uAA50-\\uAA59\\uAA60-\\uAA76\\uAA7A-\\uAAC2\\uAADB-\\uAADD\\uAAE0-\\uAAEF\\uAAF2-\\uAAF6\\uAB01-\\uAB06\\uAB09-\\uAB0E\\uAB11-\\uAB16\\uAB20-\\uAB26\\uAB28-\\uAB2E\\uAB30-\\uAB5A\\uAB5C-\\uAB5F\\uAB64\\uAB65\\uABC0-\\uABEA\\uABEC\\uABED\\uABF0-\\uABF9\\uAC00-\\uD7A3\\uD7B0-\\uD7C6\\uD7CB-\\uD7FB\\uF900-\\uFA6D\\uFA70-\\uFAD9\\uFB00-\\uFB06\\uFB13-\\uFB17\\uFB1D-\\uFB28\\uFB2A-\\uFB36\\uFB38-\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46-\\uFBB1\\uFBD3-\\uFD3D\\uFD50-\\uFD8F\\uFD92-\\uFDC7\\uFDF0-\\uFDFB\\uFE00-\\uFE0F\\uFE20-\\uFE2D\\uFE33\\uFE34\\uFE4D-\\uFE4F\\uFE70-\\uFE74\\uFE76-\\uFEFC\\uFF10-\\uFF19\\uFF21-\\uFF3A\\uFF3F\\uFF41-\\uFF5A\\uFF66-\\uFFBE\\uFFC2-\\uFFC7\\uFFCA-\\uFFCF\\uFFD2-\\uFFD7\\uFFDA-\\uFFDC]");

    // Ensure the condition is true, otherwise throw an error.
    // This is only to have a better contract semantic, i.e. another safety net
    // to catch a logic error. The condition shall be fulfilled in normal case.
    // Do NOT use this to enforce a certain condition on any user input.

    function assert(condition, message) {
      /* istanbul ignore next */
      if (!condition) {
        throw new Error('ASSERT: ' + message);
      }
    }

    function isDecimalDigit(ch) {
      return (ch >= 0x30 && ch <= 0x39); // 0..9
    }

    function isHexDigit(ch) {
      return '0123456789abcdefABCDEF'.indexOf(ch) >= 0;
    }

    function isOctalDigit(ch) {
      return '01234567'.indexOf(ch) >= 0;
    }

    // 7.2 White Space

    function isWhiteSpace(ch) {
      return (ch === 0x20) || (ch === 0x09) || (ch === 0x0B) || (ch === 0x0C) || (ch === 0xA0) ||
        (ch >= 0x1680 && [0x1680, 0x180E, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x202F, 0x205F, 0x3000, 0xFEFF].indexOf(ch) >= 0);
    }

    // 7.3 Line Terminators

    function isLineTerminator(ch) {
      return (ch === 0x0A) || (ch === 0x0D) || (ch === 0x2028) || (ch === 0x2029);
    }

    // 7.6 Identifier Names and Identifiers

    function isIdentifierStart(ch) {
      return (ch === 0x24) || (ch === 0x5F) || // $ (dollar) and _ (underscore)
        (ch >= 0x41 && ch <= 0x5A) || // A..Z
        (ch >= 0x61 && ch <= 0x7A) || // a..z
        (ch === 0x5C) || // \ (backslash)
        ((ch >= 0x80) && RegexNonAsciiIdentifierStart.test(String.fromCharCode(ch)));
    }

    function isIdentifierPart(ch) {
      return (ch === 0x24) || (ch === 0x5F) || // $ (dollar) and _ (underscore)
        (ch >= 0x41 && ch <= 0x5A) || // A..Z
        (ch >= 0x61 && ch <= 0x7A) || // a..z
        (ch >= 0x30 && ch <= 0x39) || // 0..9
        (ch === 0x5C) || // \ (backslash)
        ((ch >= 0x80) && RegexNonAsciiIdentifierPart.test(String.fromCharCode(ch)));
    }

    // 7.6.1.1 Keywords

    var keywords = {
      'if':1, 'in':1, 'do':1,
      'var':1, 'for':1, 'new':1, 'try':1, 'let':1,
      'this':1, 'else':1, 'case':1, 'void':1, 'with':1, 'enum':1,
      'while':1, 'break':1, 'catch':1, 'throw':1, 'const':1, 'yield':1, 'class':1, 'super':1,
      'return':1, 'typeof':1, 'delete':1, 'switch':1, 'export':1, 'import':1, 'public':1, 'static':1,
      'default':1, 'finally':1, 'extends':1, 'package':1, 'private':1,
      'function':1, 'continue':1, 'debugger':1,
      'interface':1, 'protected':1,
      'instanceof':1, 'implements':1
    };

    function skipComment() {
      var ch;

      while (index < length) {
        ch = source.charCodeAt(index);

        if (isWhiteSpace(ch) || isLineTerminator(ch)) {
          ++index;
        } else {
          break;
        }
      }
    }

    function scanHexEscape(prefix) {
      var i, len, ch, code = 0;

      len = (prefix === 'u') ? 4 : 2;
      for (i = 0; i < len; ++i) {
        if (index < length && isHexDigit(source[index])) {
          ch = source[index++];
          code = code * 16 + '0123456789abcdef'.indexOf(ch.toLowerCase());
        } else {
          throwError({}, MessageUnexpectedToken, ILLEGAL);
        }
      }
      return String.fromCharCode(code);
    }

    function scanUnicodeCodePointEscape() {
      var ch, code, cu1, cu2;

      ch = source[index];
      code = 0;

      // At least, one hex digit is required.
      if (ch === '}') {
        throwError({}, MessageUnexpectedToken, ILLEGAL);
      }

      while (index < length) {
        ch = source[index++];
        if (!isHexDigit(ch)) {
          break;
        }
        code = code * 16 + '0123456789abcdef'.indexOf(ch.toLowerCase());
      }

      if (code > 0x10FFFF || ch !== '}') {
        throwError({}, MessageUnexpectedToken, ILLEGAL);
      }

      // UTF-16 Encoding
      if (code <= 0xFFFF) {
        return String.fromCharCode(code);
      }
      cu1 = ((code - 0x10000) >> 10) + 0xD800;
      cu2 = ((code - 0x10000) & 1023) + 0xDC00;
      return String.fromCharCode(cu1, cu2);
    }

    function getEscapedIdentifier() {
      var ch, id;

      ch = source.charCodeAt(index++);
      id = String.fromCharCode(ch);

      // '\u' (U+005C, U+0075) denotes an escaped character.
      if (ch === 0x5C) {
        if (source.charCodeAt(index) !== 0x75) {
          throwError({}, MessageUnexpectedToken, ILLEGAL);
        }
        ++index;
        ch = scanHexEscape('u');
        if (!ch || ch === '\\' || !isIdentifierStart(ch.charCodeAt(0))) {
          throwError({}, MessageUnexpectedToken, ILLEGAL);
        }
        id = ch;
      }

      while (index < length) {
        ch = source.charCodeAt(index);
        if (!isIdentifierPart(ch)) {
          break;
        }
        ++index;
        id += String.fromCharCode(ch);

        // '\u' (U+005C, U+0075) denotes an escaped character.
        if (ch === 0x5C) {
          id = id.substr(0, id.length - 1);
          if (source.charCodeAt(index) !== 0x75) {
            throwError({}, MessageUnexpectedToken, ILLEGAL);
          }
          ++index;
          ch = scanHexEscape('u');
          if (!ch || ch === '\\' || !isIdentifierPart(ch.charCodeAt(0))) {
            throwError({}, MessageUnexpectedToken, ILLEGAL);
          }
          id += ch;
        }
      }

      return id;
    }

    function getIdentifier() {
      var start, ch;

      start = index++;
      while (index < length) {
        ch = source.charCodeAt(index);
        if (ch === 0x5C) {
          // Blackslash (U+005C) marks Unicode escape sequence.
          index = start;
          return getEscapedIdentifier();
        }
        if (isIdentifierPart(ch)) {
          ++index;
        } else {
          break;
        }
      }

      return source.slice(start, index);
    }

    function scanIdentifier() {
      var start, id, type;

      start = index;

      // Backslash (U+005C) starts an escaped character.
      id = (source.charCodeAt(index) === 0x5C) ? getEscapedIdentifier() : getIdentifier();

      // There is no keyword or literal with only one character.
      // Thus, it must be an identifier.
      if (id.length === 1) {
        type = TokenIdentifier;
      } else if (keywords.hasOwnProperty(id)) {
        type = TokenKeyword;
      } else if (id === 'null') {
        type = TokenNullLiteral;
      } else if (id === 'true' || id === 'false') {
        type = TokenBooleanLiteral;
      } else {
        type = TokenIdentifier;
      }

      return {
        type: type,
        value: id,
        start: start,
        end: index
      };
    }

    // 7.7 Punctuators

    function scanPunctuator() {
      var start = index,
        code = source.charCodeAt(index),
        code2,
        ch1 = source[index],
        ch2,
        ch3,
        ch4;

      switch (code) {

        // Check for most common single-character punctuators.
        case 0x2E: // . dot
        case 0x28: // ( open bracket
        case 0x29: // ) close bracket
        case 0x3B: // ; semicolon
        case 0x2C: // , comma
        case 0x7B: // { open curly brace
        case 0x7D: // } close curly brace
        case 0x5B: // [
        case 0x5D: // ]
        case 0x3A: // :
        case 0x3F: // ?
        case 0x7E: // ~
          ++index;
          return {
            type: TokenPunctuator,
            value: String.fromCharCode(code),
            start: start,
            end: index
          };

        default:
          code2 = source.charCodeAt(index + 1);

          // '=' (U+003D) marks an assignment or comparison operator.
          if (code2 === 0x3D) {
            switch (code) {
              case 0x2B: // +
              case 0x2D: // -
              case 0x2F: // /
              case 0x3C: // <
              case 0x3E: // >
              case 0x5E: // ^
              case 0x7C: // |
              case 0x25: // %
              case 0x26: // &
              case 0x2A: // *
                index += 2;
                return {
                  type: TokenPunctuator,
                  value: String.fromCharCode(code) + String.fromCharCode(code2),
                  start: start,
                  end: index
                };

              case 0x21: // !
              case 0x3D: // =
                index += 2;

                // !== and ===
                if (source.charCodeAt(index) === 0x3D) {
                  ++index;
                }
                return {
                  type: TokenPunctuator,
                  value: source.slice(start, index),
                  start: start,
                  end: index
                };
            }
          }
      }

      // 4-character punctuator: >>>=

      ch4 = source.substr(index, 4);

      if (ch4 === '>>>=') {
        index += 4;
        return {
          type: TokenPunctuator,
          value: ch4,
          start: start,
          end: index
        };
      }

      // 3-character punctuators: === !== >>> <<= >>=

      ch3 = ch4.substr(0, 3);

      if (ch3 === '>>>' || ch3 === '<<=' || ch3 === '>>=') {
        index += 3;
        return {
          type: TokenPunctuator,
          value: ch3,
          start: start,
          end: index
        };
      }

      // Other 2-character punctuators: ++ -- << >> && ||
      ch2 = ch3.substr(0, 2);

      if ((ch1 === ch2[1] && ('+-<>&|'.indexOf(ch1) >= 0)) || ch2 === '=>') {
        index += 2;
        return {
          type: TokenPunctuator,
          value: ch2,
          start: start,
          end: index
        };
      }

      // 1-character punctuators: < > = ! + - * % & | ^ /

      if ('<>=!+-*%&|^/'.indexOf(ch1) >= 0) {
        ++index;
        return {
          type: TokenPunctuator,
          value: ch1,
          start: start,
          end: index
        };
      }

      throwError({}, MessageUnexpectedToken, ILLEGAL);
    }

    // 7.8.3 Numeric Literals

    function scanHexLiteral(start) {
      var number = '';

      while (index < length) {
        if (!isHexDigit(source[index])) {
          break;
        }
        number += source[index++];
      }

      if (number.length === 0) {
        throwError({}, MessageUnexpectedToken, ILLEGAL);
      }

      if (isIdentifierStart(source.charCodeAt(index))) {
        throwError({}, MessageUnexpectedToken, ILLEGAL);
      }

      return {
        type: TokenNumericLiteral,
        value: parseInt('0x' + number, 16),
        start: start,
        end: index
      };
    }

    function scanOctalLiteral(start) {
      var number = '0' + source[index++];
      while (index < length) {
        if (!isOctalDigit(source[index])) {
          break;
        }
        number += source[index++];
      }

      if (isIdentifierStart(source.charCodeAt(index)) || isDecimalDigit(source.charCodeAt(index))) {
        throwError({}, MessageUnexpectedToken, ILLEGAL);
      }

      return {
        type: TokenNumericLiteral,
        value: parseInt(number, 8),
        octal: true,
        start: start,
        end: index
      };
    }

    function scanNumericLiteral() {
      var number, start, ch;

      ch = source[index];
      assert(isDecimalDigit(ch.charCodeAt(0)) || (ch === '.'),
        'Numeric literal must start with a decimal digit or a decimal point');

      start = index;
      number = '';
      if (ch !== '.') {
        number = source[index++];
        ch = source[index];

        // Hex number starts with '0x'.
        // Octal number starts with '0'.
        if (number === '0') {
          if (ch === 'x' || ch === 'X') {
            ++index;
            return scanHexLiteral(start);
          }
          if (isOctalDigit(ch)) {
            return scanOctalLiteral(start);
          }

          // decimal number starts with '0' such as '09' is illegal.
          if (ch && isDecimalDigit(ch.charCodeAt(0))) {
            throwError({}, MessageUnexpectedToken, ILLEGAL);
          }
        }

        while (isDecimalDigit(source.charCodeAt(index))) {
          number += source[index++];
        }
        ch = source[index];
      }

      if (ch === '.') {
        number += source[index++];
        while (isDecimalDigit(source.charCodeAt(index))) {
          number += source[index++];
        }
        ch = source[index];
      }

      if (ch === 'e' || ch === 'E') {
        number += source[index++];

        ch = source[index];
        if (ch === '+' || ch === '-') {
          number += source[index++];
        }
        if (isDecimalDigit(source.charCodeAt(index))) {
          while (isDecimalDigit(source.charCodeAt(index))) {
            number += source[index++];
          }
        } else {
          throwError({}, MessageUnexpectedToken, ILLEGAL);
        }
      }

      if (isIdentifierStart(source.charCodeAt(index))) {
        throwError({}, MessageUnexpectedToken, ILLEGAL);
      }

      return {
        type: TokenNumericLiteral,
        value: parseFloat(number),
        start: start,
        end: index
      };
    }

    // 7.8.4 String Literals

    function scanStringLiteral() {
      var str = '',
        quote, start, ch, code, octal = false;

      quote = source[index];
      assert((quote === '\'' || quote === '"'),
        'String literal must starts with a quote');

      start = index;
      ++index;

      while (index < length) {
        ch = source[index++];

        if (ch === quote) {
          quote = '';
          break;
        } else if (ch === '\\') {
          ch = source[index++];
          if (!ch || !isLineTerminator(ch.charCodeAt(0))) {
            switch (ch) {
              case 'u':
              case 'x':
                if (source[index] === '{') {
                  ++index;
                  str += scanUnicodeCodePointEscape();
                } else {
                  str += scanHexEscape(ch);
                }
                break;
              case 'n':
                str += '\n';
                break;
              case 'r':
                str += '\r';
                break;
              case 't':
                str += '\t';
                break;
              case 'b':
                str += '\b';
                break;
              case 'f':
                str += '\f';
                break;
              case 'v':
                str += '\x0B';
                break;

              default:
                if (isOctalDigit(ch)) {
                  code = '01234567'.indexOf(ch);

                  // \0 is not octal escape sequence
                  if (code !== 0) {
                    octal = true;
                  }

                  if (index < length && isOctalDigit(source[index])) {
                    octal = true;
                    code = code * 8 + '01234567'.indexOf(source[index++]);

                    // 3 digits are only allowed when string starts
                    // with 0, 1, 2, 3
                    if ('0123'.indexOf(ch) >= 0 &&
                      index < length &&
                      isOctalDigit(source[index])) {
                      code = code * 8 + '01234567'.indexOf(source[index++]);
                    }
                  }
                  str += String.fromCharCode(code);
                } else {
                  str += ch;
                }
                break;
            }
          } else {
            if (ch === '\r' && source[index] === '\n') {
              ++index;
            }
          }
        } else if (isLineTerminator(ch.charCodeAt(0))) {
          break;
        } else {
          str += ch;
        }
      }

      if (quote !== '') {
        throwError({}, MessageUnexpectedToken, ILLEGAL);
      }

      return {
        type: TokenStringLiteral,
        value: str,
        octal: octal,
        start: start,
        end: index
      };
    }

    function testRegExp(pattern, flags) {
      var tmp = pattern;

      if (flags.indexOf('u') >= 0) {
        // Replace each astral symbol and every Unicode code point
        // escape sequence with a single ASCII symbol to avoid throwing on
        // regular expressions that are only valid in combination with the
        // `/u` flag.
        // Note: replacing with the ASCII symbol `x` might cause false
        // negatives in unlikely scenarios. For example, `[\u{61}-b]` is a
        // perfectly valid pattern that is equivalent to `[a-b]`, but it
        // would be replaced by `[x-b]` which throws an error.
        tmp = tmp
          .replace(/\\u\{([0-9a-fA-F]+)\}/g, function($0, $1) {
            if (parseInt($1, 16) <= 0x10FFFF) {
              return 'x';
            }
            throwError({}, MessageInvalidRegExp);
          })
          .replace(/[\uD800-\uDBFF][\uDC00-\uDFFF]/g, 'x');
      }

      // First, detect invalid regular expressions.
      try {
      } catch (e) {
        throwError({}, MessageInvalidRegExp);
      }

      // Return a regular expression object for this pattern-flag pair, or
      // `null` in case the current environment doesn't support the flags it
      // uses.
      try {
        return new RegExp(pattern, flags);
      } catch (exception) {
        return null;
      }
    }

    function scanRegExpBody() {
      var ch, str, classMarker, terminated, body;

      ch = source[index];
      assert(ch === '/', 'Regular expression literal must start with a slash');
      str = source[index++];

      classMarker = false;
      terminated = false;
      while (index < length) {
        ch = source[index++];
        str += ch;
        if (ch === '\\') {
          ch = source[index++];
          // ECMA-262 7.8.5
          if (isLineTerminator(ch.charCodeAt(0))) {
            throwError({}, MessageUnterminatedRegExp);
          }
          str += ch;
        } else if (isLineTerminator(ch.charCodeAt(0))) {
          throwError({}, MessageUnterminatedRegExp);
        } else if (classMarker) {
          if (ch === ']') {
            classMarker = false;
          }
        } else {
          if (ch === '/') {
            terminated = true;
            break;
          } else if (ch === '[') {
            classMarker = true;
          }
        }
      }

      if (!terminated) {
        throwError({}, MessageUnterminatedRegExp);
      }

      // Exclude leading and trailing slash.
      body = str.substr(1, str.length - 2);
      return {
        value: body,
        literal: str
      };
    }

    function scanRegExpFlags() {
      var ch, str, flags;

      str = '';
      flags = '';
      while (index < length) {
        ch = source[index];
        if (!isIdentifierPart(ch.charCodeAt(0))) {
          break;
        }

        ++index;
        if (ch === '\\' && index < length) {
          throwError({}, MessageUnexpectedToken, ILLEGAL);
        } else {
          flags += ch;
          str += ch;
        }
      }

      if (flags.search(/[^gimuy]/g) >= 0) {
        throwError({}, MessageInvalidRegExp, flags);
      }

      return {
        value: flags,
        literal: str
      };
    }

    function scanRegExp() {
      var start, body, flags, value;

      lookahead = null;
      skipComment();
      start = index;

      body = scanRegExpBody();
      flags = scanRegExpFlags();
      value = testRegExp(body.value, flags.value);

      return {
        literal: body.literal + flags.literal,
        value: value,
        regex: {
          pattern: body.value,
          flags: flags.value
        },
        start: start,
        end: index
      };
    }

    function isIdentifierName(token) {
      return token.type === TokenIdentifier ||
        token.type === TokenKeyword ||
        token.type === TokenBooleanLiteral ||
        token.type === TokenNullLiteral;
    }

    function advance() {
      var ch;

      skipComment();

      if (index >= length) {
        return {
          type: TokenEOF,
          start: index,
          end: index
        };
      }

      ch = source.charCodeAt(index);

      if (isIdentifierStart(ch)) {
        return scanIdentifier();
      }

      // Very common: ( and ) and ;
      if (ch === 0x28 || ch === 0x29 || ch === 0x3B) {
        return scanPunctuator();
      }

      // String literal starts with single quote (U+0027) or double quote (U+0022).
      if (ch === 0x27 || ch === 0x22) {
        return scanStringLiteral();
      }


      // Dot (.) U+002E can also start a floating-point number, hence the need
      // to check the next character.
      if (ch === 0x2E) {
        if (isDecimalDigit(source.charCodeAt(index + 1))) {
          return scanNumericLiteral();
        }
        return scanPunctuator();
      }

      if (isDecimalDigit(ch)) {
        return scanNumericLiteral();
      }

      return scanPunctuator();
    }

    function lex() {
      var token;

      token = lookahead;
      index = token.end;

      lookahead = advance();

      index = token.end;

      return token;
    }

    function peek() {
      var pos;

      pos = index;

      lookahead = advance();
      index = pos;
    }

    function finishArrayExpression(elements) {
      var node = new ASTNode(SyntaxArrayExpression);
      node.elements = elements;
      return node;
    }

    function finishBinaryExpression(operator, left, right) {
      var node = new ASTNode((operator === '||' || operator === '&&') ? SyntaxLogicalExpression : SyntaxBinaryExpression);
      node.operator = operator;
      node.left = left;
      node.right = right;
      return node;
    }

    function finishCallExpression(callee, args) {
      var node = new ASTNode(SyntaxCallExpression);
      node.callee = callee;
      node.arguments = args;
      return node;
    }

    function finishConditionalExpression(test, consequent, alternate) {
      var node = new ASTNode(SyntaxConditionalExpression);
      node.test = test;
      node.consequent = consequent;
      node.alternate = alternate;
      return node;
    }

    function finishIdentifier(name) {
      var node = new ASTNode(SyntaxIdentifier);
      node.name = name;
      return node;
    }

    function finishLiteral(token) {
      var node = new ASTNode(SyntaxLiteral);
      node.value = token.value;
      node.raw = source.slice(token.start, token.end);
      if (token.regex) {
        if (node.raw === '//') {
          node.raw = '/(?:)/';
        }
        node.regex = token.regex;
      }
      return node;
    }

    function finishMemberExpression(accessor, object, property) {
      var node = new ASTNode(SyntaxMemberExpression);
      node.computed = accessor === '[';
      node.object = object;
      node.property = property;
      if (!node.computed) property.member = true;
      return node;
    }

    function finishObjectExpression(properties) {
      var node = new ASTNode(SyntaxObjectExpression);
      node.properties = properties;
      return node;
    }

    function finishProperty(kind, key, value) {
      var node = new ASTNode(SyntaxProperty);
      node.key = key;
      node.value = value;
      node.kind = kind;
      return node;
    }

    function finishUnaryExpression(operator, argument) {
      var node = new ASTNode(SyntaxUnaryExpression);
      node.operator = operator;
      node.argument = argument;
      node.prefix = true;
      return node;
    }

    // Throw an exception

    function throwError(token, messageFormat) {
      var error,
        args = Array.prototype.slice.call(arguments, 2),
        msg = messageFormat.replace(
          /%(\d)/g,
          function(whole, index) {
            assert(index < args.length, 'Message reference must be in range');
            return args[index];
          }
        );


      error = new Error(msg);
      error.index = index;
      error.description = msg;
      throw error;
    }

    // Throw an exception because of the token.

    function throwUnexpected(token) {
      if (token.type === TokenEOF) {
        throwError(token, MessageUnexpectedEOS);
      }

      if (token.type === TokenNumericLiteral) {
        throwError(token, MessageUnexpectedNumber);
      }

      if (token.type === TokenStringLiteral) {
        throwError(token, MessageUnexpectedString);
      }

      if (token.type === TokenIdentifier) {
        throwError(token, MessageUnexpectedIdentifier);
      }

      if (token.type === TokenKeyword) {
        throwError(token, MessageUnexpectedReserved);
      }

      // BooleanLiteral, NullLiteral, or Punctuator.
      throwError(token, MessageUnexpectedToken, token.value);
    }

    // Expect the next token to match the specified punctuator.
    // If not, an exception will be thrown.

    function expect(value) {
      var token = lex();
      if (token.type !== TokenPunctuator || token.value !== value) {
        throwUnexpected(token);
      }
    }

    // Return true if the next token matches the specified punctuator.

    function match(value) {
      return lookahead.type === TokenPunctuator && lookahead.value === value;
    }

    // Return true if the next token matches the specified keyword

    function matchKeyword(keyword) {
      return lookahead.type === TokenKeyword && lookahead.value === keyword;
    }

    // 11.1.4 Array Initialiser

    function parseArrayInitialiser() {
      var elements = [];

      index = lookahead.start;
      expect('[');

      while (!match(']')) {
        if (match(',')) {
          lex();
          elements.push(null);
        } else {
          elements.push(parseConditionalExpression());

          if (!match(']')) {
            expect(',');
          }
        }
      }

      lex();

      return finishArrayExpression(elements);
    }

    // 11.1.5 Object Initialiser

    function parseObjectPropertyKey() {
      var token;

      index = lookahead.start;
      token = lex();

      // Note: This function is called only from parseObjectProperty(), where
      // EOF and Punctuator tokens are already filtered out.

      if (token.type === TokenStringLiteral || token.type === TokenNumericLiteral) {
        if (token.octal) {
          throwError(token, MessageStrictOctalLiteral);
        }
        return finishLiteral(token);
      }

      return finishIdentifier(token.value);
    }

    function parseObjectProperty() {
      var token, key, id, value;

      index = lookahead.start;
      token = lookahead;

      if (token.type === TokenIdentifier) {
        id = parseObjectPropertyKey();
        expect(':');
        value = parseConditionalExpression();
        return finishProperty('init', id, value);
      }
      if (token.type === TokenEOF || token.type === TokenPunctuator) {
        throwUnexpected(token);
      } else {
        key = parseObjectPropertyKey();
        expect(':');
        value = parseConditionalExpression();
        return finishProperty('init', key, value);
      }
    }

    function parseObjectInitialiser() {
      var properties = [],
        property, name, key, map = {},
        toString = String;

      index = lookahead.start;
      expect('{');

      while (!match('}')) {
        property = parseObjectProperty();

        if (property.key.type === SyntaxIdentifier) {
          name = property.key.name;
        } else {
          name = toString(property.key.value);
        }

        key = '$' + name;
        if (Object.prototype.hasOwnProperty.call(map, key)) {
          throwError({}, MessageStrictDuplicateProperty);
        } else {
          map[key] = true;
        }

        properties.push(property);

        if (!match('}')) {
          expect(',');
        }
      }

      expect('}');

      return finishObjectExpression(properties);
    }

    // 11.1.6 The Grouping Operator

    function parseGroupExpression() {
      var expr;

      expect('(');

      expr = parseExpression();

      expect(')');

      return expr;
    }


    // 11.1 Primary Expressions

    var legalKeywords = {
      "if": 1,
      "this": 1
    };

    function parsePrimaryExpression() {
      var type, token, expr;

      if (match('(')) {
        return parseGroupExpression();
      }

      if (match('[')) {
        return parseArrayInitialiser();
      }

      if (match('{')) {
        return parseObjectInitialiser();
      }

      type = lookahead.type;
      index = lookahead.start;


      if (type === TokenIdentifier || legalKeywords[lookahead.value]) {
        expr = finishIdentifier(lex().value);
      } else if (type === TokenStringLiteral || type === TokenNumericLiteral) {
        if (lookahead.octal) {
          throwError(lookahead, MessageStrictOctalLiteral);
        }
        expr = finishLiteral(lex());
      } else if (type === TokenKeyword) {
        throw new Error(DISABLED);
      } else if (type === TokenBooleanLiteral) {
        token = lex();
        token.value = (token.value === 'true');
        expr = finishLiteral(token);
      } else if (type === TokenNullLiteral) {
        token = lex();
        token.value = null;
        expr = finishLiteral(token);
      } else if (match('/') || match('/=')) {
        expr = finishLiteral(scanRegExp());
        peek();
      } else {
        throwUnexpected(lex());
      }

      return expr;
    }

    // 11.2 Left-Hand-Side Expressions

    function parseArguments() {
      var args = [];

      expect('(');

      if (!match(')')) {
        while (index < length) {
          args.push(parseConditionalExpression());
          if (match(')')) {
            break;
          }
          expect(',');
        }
      }

      expect(')');

      return args;
    }

    function parseNonComputedProperty() {
      var token;
      index = lookahead.start;
      token = lex();

      if (!isIdentifierName(token)) {
        throwUnexpected(token);
      }

      return finishIdentifier(token.value);
    }

    function parseNonComputedMember() {
      expect('.');

      return parseNonComputedProperty();
    }

    function parseComputedMember() {
      var expr;

      expect('[');

      expr = parseExpression();

      expect(']');

      return expr;
    }

    function parseLeftHandSideExpressionAllowCall() {
      var expr, args, property;

      expr = parsePrimaryExpression();

      for (;;) {
        if (match('.')) {
          property = parseNonComputedMember();
          expr = finishMemberExpression('.', expr, property);
        } else if (match('(')) {
          args = parseArguments();
          expr = finishCallExpression(expr, args);
        } else if (match('[')) {
          property = parseComputedMember();
          expr = finishMemberExpression('[', expr, property);
        } else {
          break;
        }
      }

      return expr;
    }

    // 11.3 Postfix Expressions

    function parsePostfixExpression() {
      var expr = parseLeftHandSideExpressionAllowCall();

      if (lookahead.type === TokenPunctuator) {
        if ((match('++') || match('--'))) {
          throw new Error(DISABLED);
        }
      }

      return expr;
    }

    // 11.4 Unary Operators

    function parseUnaryExpression() {
      var token, expr;

      if (lookahead.type !== TokenPunctuator && lookahead.type !== TokenKeyword) {
        expr = parsePostfixExpression();
      } else if (match('++') || match('--')) {
        throw new Error(DISABLED);
      } else if (match('+') || match('-') || match('~') || match('!')) {
        token = lex();
        expr = parseUnaryExpression();
        expr = finishUnaryExpression(token.value, expr);
      } else if (matchKeyword('delete') || matchKeyword('void') || matchKeyword('typeof')) {
        throw new Error(DISABLED);
      } else {
        expr = parsePostfixExpression();
      }

      return expr;
    }

    function binaryPrecedence(token) {
      var prec = 0;

      if (token.type !== TokenPunctuator && token.type !== TokenKeyword) {
        return 0;
      }

      switch (token.value) {
        case '||':
          prec = 1;
          break;

        case '&&':
          prec = 2;
          break;

        case '|':
          prec = 3;
          break;

        case '^':
          prec = 4;
          break;

        case '&':
          prec = 5;
          break;

        case '==':
        case '!=':
        case '===':
        case '!==':
          prec = 6;
          break;

        case '<':
        case '>':
        case '<=':
        case '>=':
        case 'instanceof':
        case 'in':
          prec = 7;
          break;

        case '<<':
        case '>>':
        case '>>>':
          prec = 8;
          break;

        case '+':
        case '-':
          prec = 9;
          break;

        case '*':
        case '/':
        case '%':
          prec = 11;
          break;

        default:
          break;
      }

      return prec;
    }

    // 11.5 Multiplicative Operators
    // 11.6 Additive Operators
    // 11.7 Bitwise Shift Operators
    // 11.8 Relational Operators
    // 11.9 Equality Operators
    // 11.10 Binary Bitwise Operators
    // 11.11 Binary Logical Operators

    function parseBinaryExpression() {
      var marker, markers, expr, token, prec, stack, right, operator, left, i;

      marker = lookahead;
      left = parseUnaryExpression();

      token = lookahead;
      prec = binaryPrecedence(token);
      if (prec === 0) {
        return left;
      }
      token.prec = prec;
      lex();

      markers = [marker, lookahead];
      right = parseUnaryExpression();

      stack = [left, token, right];

      while ((prec = binaryPrecedence(lookahead)) > 0) {

        // Reduce: make a binary expression from the three topmost entries.
        while ((stack.length > 2) && (prec <= stack[stack.length - 2].prec)) {
          right = stack.pop();
          operator = stack.pop().value;
          left = stack.pop();
          markers.pop();
          expr = finishBinaryExpression(operator, left, right);
          stack.push(expr);
        }

        // Shift.
        token = lex();
        token.prec = prec;
        stack.push(token);
        markers.push(lookahead);
        expr = parseUnaryExpression();
        stack.push(expr);
      }

      // Final reduce to clean-up the stack.
      i = stack.length - 1;
      expr = stack[i];
      markers.pop();
      while (i > 1) {
        markers.pop();
        expr = finishBinaryExpression(stack[i - 1].value, stack[i - 2], expr);
        i -= 2;
      }

      return expr;
    }

    // 11.12 Conditional Operator

    function parseConditionalExpression() {
      var expr, consequent, alternate;

      expr = parseBinaryExpression();

      if (match('?')) {
        lex();
        consequent = parseConditionalExpression();
        expect(':');
        alternate = parseConditionalExpression();

        expr = finishConditionalExpression(expr, consequent, alternate);
      }

      return expr;
    }

    // 11.14 Comma Operator

    function parseExpression() {
      var expr = parseConditionalExpression();

      if (match(',')) {
        throw new Error(DISABLED); // no sequence expressions
      }

      return expr;
    }

    function parse(code) {
      source = code;
      index = 0;
      length = source.length;
      lookahead = null;

      peek();

      var expr = parseExpression();

      if (lookahead.type !== TokenEOF) {
        throw new Error("Unexpect token after expression.");
      }
      return expr;
    }

    function getName(node) {
        const name = [];
        if (node.type === 'Identifier') {
            return [node.name];
        }
        if (node.type === 'Literal') {
            return [node.value];
        }
        if (node.type === 'MemberExpression') {
            name.push(...getName(node.object));
            name.push(...getName(node.property));
        }
        return name;
    }
    function startsWithDatum(node) {
        if (node.object.type === 'MemberExpression') {
            return startsWithDatum(node.object);
        }
        return node.object.name === 'datum';
    }
    function getDependentFields(expression) {
        const ast = parse(expression);
        const dependents = new Set();
        ast.visit((node) => {
            if (node.type === 'MemberExpression' && startsWithDatum(node)) {
                dependents.add(getName(node)
                    .slice(1)
                    .join('.'));
            }
        });
        return dependents;
    }

    /**
     * We don't know what a calculate node depends on so we should never move it beyond anything that produces fields.
     */
    class CalculateNode extends DataFlowNode {
        constructor(parent, transform) {
            super(parent);
            this.transform = transform;
            this._dependentFields = getDependentFields(this.transform.calculate);
        }
        clone() {
            return new CalculateNode(null, duplicate(this.transform));
        }
        static parseAllForSortIndex(parent, model) {
            // get all the encoding with sort fields from model
            model.forEachFieldDef((fieldDef, channel) => {
                if (!isScaleFieldDef(fieldDef)) {
                    return;
                }
                if (isSortArray(fieldDef.sort)) {
                    const { field, timeUnit } = fieldDef;
                    const sort = fieldDef.sort;
                    // generate `datum["a"] === val0 ? 0 : datum["a"] === val1 ? 1 : ... : n` via FieldEqualPredicate
                    const calculate = sort
                        .map((sortValue, i) => {
                        return `${fieldFilterExpression({ field, timeUnit, equal: sortValue })} ? ${i} : `;
                    })
                        .join('') + sort.length;
                    parent = new CalculateNode(parent, {
                        calculate,
                        as: sortArrayIndexField(fieldDef, channel, { forAs: true })
                    });
                }
            });
            return parent;
        }
        producedFields() {
            return new Set([this.transform.as]);
        }
        dependentFields() {
            return this._dependentFields;
        }
        assemble() {
            return {
                type: 'formula',
                expr: this.transform.calculate,
                as: this.transform.as
            };
        }
        hash() {
            return `Calculate ${hash(this.transform)}`;
        }
    }
    function sortArrayIndexField(fieldDef, channel, opt) {
        return vgField(fieldDef, Object.assign({ prefix: channel, suffix: 'sort_index' }, (opt || {})));
    }

    /**
     * Get header channel, which can be different from facet channel when orient is specified or when the facet channel is facet.
     */
    function getHeaderChannel(channel, orient) {
        if (contains(['top', 'bottom'], orient)) {
            return 'column';
        }
        else if (contains(['left', 'right'], orient)) {
            return 'row';
        }
        return channel === 'row' ? 'row' : 'column';
    }
    function getHeaderProperty(prop, facetFieldDef, config, channel) {
        const headerSpecificConfig = channel === 'row' ? config.headerRow : channel === 'column' ? config.headerColumn : config.headerFacet;
        return getFirstDefined(facetFieldDef && facetFieldDef.header ? facetFieldDef.header[prop] : undefined, headerSpecificConfig[prop], config.header[prop]);
    }
    function getHeaderProperties(properties, facetFieldDef, config, channel) {
        const props = {};
        for (const prop of properties) {
            const value = getHeaderProperty(prop, facetFieldDef, config, channel);
            if (value !== undefined) {
                props[prop] = value;
            }
        }
        return props;
    }

    const HEADER_CHANNELS = ['row', 'column'];
    const HEADER_TYPES = ['header', 'footer'];

    // TODO: rename to assembleHeaderTitleGroup
    function assembleTitleGroup(model, channel) {
        const title = model.component.layoutHeaders[channel].title;
        const config = model.config ? model.config : undefined;
        const facetFieldDef = model.component.layoutHeaders[channel].facetFieldDef
            ? model.component.layoutHeaders[channel].facetFieldDef
            : undefined;
        const { titleAnchor, titleAngle, titleOrient } = getHeaderProperties(['titleAnchor', 'titleAngle', 'titleOrient'], facetFieldDef, config, channel);
        const headerChannel = getHeaderChannel(channel, titleOrient);
        return {
            name: `${channel}-title`,
            type: 'group',
            role: `${headerChannel}-title`,
            title: Object.assign({ text: title }, (channel === 'row' ? { orient: 'left' } : {}), { style: 'guide-title' }, defaultHeaderGuideBaseline(titleAngle, headerChannel), defaultHeaderGuideAlign(headerChannel, titleAngle, titleAnchor), assembleHeaderProperties(config, facetFieldDef, channel, HEADER_TITLE_PROPERTIES, HEADER_TITLE_PROPERTIES_MAP))
        };
    }
    function defaultHeaderGuideAlign(headerChannel, angle, anchor = 'middle') {
        switch (anchor) {
            case 'start':
                return { align: 'left' };
            case 'end':
                return { align: 'right' };
        }
        const align = defaultLabelAlign(angle, headerChannel === 'row' ? 'left' : 'top');
        return align ? { align } : {};
    }
    function defaultHeaderGuideBaseline(angle, channel) {
        const baseline = defaultLabelBaseline(angle, channel === 'row' ? 'left' : 'top');
        return baseline ? { baseline } : {};
    }
    function assembleHeaderGroups(model, channel) {
        const layoutHeader = model.component.layoutHeaders[channel];
        const groups = [];
        for (const headerType of HEADER_TYPES) {
            if (layoutHeader[headerType]) {
                for (const headerCmpt of layoutHeader[headerType]) {
                    groups.push(assembleHeaderGroup(model, channel, headerType, layoutHeader, headerCmpt));
                }
            }
        }
        return groups;
    }
    function getSort(facetFieldDef, channel) {
        const { sort } = facetFieldDef;
        if (isSortField(sort)) {
            return {
                field: vgField(sort, { expr: 'datum' }),
                order: sort.order || 'ascending'
            };
        }
        else if (isArray(sort)) {
            return {
                field: sortArrayIndexField(facetFieldDef, channel, { expr: 'datum' }),
                order: 'ascending'
            };
        }
        else {
            return {
                field: vgField(facetFieldDef, { expr: 'datum' }),
                order: sort || 'ascending'
            };
        }
    }
    function assembleLabelTitle(facetFieldDef, channel, config) {
        const { format, labelAngle, labelAnchor, labelOrient } = getHeaderProperties(['format', 'labelAngle', 'labelAnchor', 'labelOrient'], facetFieldDef, config, channel);
        const headerChannel = getHeaderChannel(channel, labelOrient);
        return Object.assign({ text: formatSignalRef(facetFieldDef, format, 'parent', config) }, (channel === 'row' ? { orient: 'left' } : {}), { style: 'guide-label', frame: 'group' }, defaultHeaderGuideBaseline(labelAngle, headerChannel), defaultHeaderGuideAlign(headerChannel, labelAngle, labelAnchor), assembleHeaderProperties(config, facetFieldDef, channel, HEADER_LABEL_PROPERTIES, HEADER_LABEL_PROPERTIES_MAP));
    }
    function assembleHeaderGroup(model, channel, headerType, layoutHeader, headerCmpt) {
        if (headerCmpt) {
            let title = null;
            const { facetFieldDef } = layoutHeader;
            const config = model.config ? model.config : undefined;
            if (facetFieldDef && headerCmpt.labels) {
                const { labelOrient } = getHeaderProperties(['labelOrient'], facetFieldDef, config, channel);
                // Include label title in the header if orient aligns with the channel
                if ((channel === 'row' && !contains(['top', 'bottom'], labelOrient)) ||
                    (channel === 'column' && !contains(['left', 'right'], labelOrient))) {
                    title = assembleLabelTitle(facetFieldDef, channel, config);
                }
            }
            const isFacetWithoutRowCol = isFacetModel(model) && !isFacetMapping(model.facet);
            const axes = headerCmpt.axes;
            const hasAxes = axes && axes.length > 0;
            if (title || hasAxes) {
                const sizeChannel = channel === 'row' ? 'height' : 'width';
                return Object.assign({ name: model.getName(`${channel}_${headerType}`), type: 'group', role: `${channel}-${headerType}` }, (layoutHeader.facetFieldDef
                    ? {
                        from: { data: model.getName(channel + '_domain') },
                        sort: getSort(facetFieldDef, channel)
                    }
                    : {}), (hasAxes && isFacetWithoutRowCol
                    ? {
                        from: { data: model.getName(`facet_domain_${channel}`) }
                    }
                    : {}), (title ? { title } : {}), (headerCmpt.sizeSignal
                    ? {
                        encode: {
                            update: {
                                [sizeChannel]: headerCmpt.sizeSignal
                            }
                        }
                    }
                    : {}), (hasAxes ? { axes } : {}));
            }
        }
        return null;
    }
    const LAYOUT_TITLE_BAND = {
        column: {
            start: 0,
            end: 1
        },
        row: {
            start: 1,
            end: 0
        }
    };
    function getLayoutTitleBand(titleAnchor, headerChannel) {
        return LAYOUT_TITLE_BAND[headerChannel][titleAnchor];
    }
    function assembleLayoutTitleBand(headerComponentIndex, config) {
        const titleBand = {};
        for (const channel of FACET_CHANNELS) {
            const headerComponent = headerComponentIndex[channel];
            if (headerComponent && headerComponent.facetFieldDef) {
                const { titleAnchor, titleOrient } = getHeaderProperties(['titleAnchor', 'titleOrient'], headerComponent.facetFieldDef, config, channel);
                const headerChannel = getHeaderChannel(channel, titleOrient);
                const band = getLayoutTitleBand(titleAnchor, headerChannel);
                if (band !== undefined) {
                    titleBand[headerChannel] = band;
                }
            }
        }
        return keys(titleBand).length > 0 ? titleBand : undefined;
    }
    function assembleHeaderProperties(config, facetFieldDef, channel, properties, propertiesMap) {
        const props = {};
        for (const prop of properties) {
            if (!propertiesMap[prop]) {
                continue;
            }
            const value = getHeaderProperty(prop, facetFieldDef, config, channel);
            if (value !== undefined) {
                props[propertiesMap[prop]] = value;
            }
        }
        return props;
    }

    function assembleLayoutSignals(model) {
        return [...sizeSignals(model, 'width'), ...sizeSignals(model, 'height')];
    }
    function sizeSignals(model, sizeType) {
        const channel = sizeType === 'width' ? 'x' : 'y';
        const size = model.component.layoutSize.get(sizeType);
        if (!size || size === 'merged') {
            return [];
        }
        // Read size signal name from name map, just in case it is the top-level size signal that got renamed.
        const name = model.getSizeSignalRef(sizeType).signal;
        if (size === 'range-step') {
            const scaleComponent = model.getScaleComponent(channel);
            if (scaleComponent) {
                const type = scaleComponent.get('type');
                const range = scaleComponent.get('range');
                if (hasDiscreteDomain(type) && isVgRangeStep(range)) {
                    const scaleName = model.scaleName(channel);
                    if (isFacetModel(model.parent)) {
                        // If parent is facet and this is an independent scale, return only signal signal
                        // as the width/height will be calculated using the cardinality from
                        // facet's aggregate rather than reading from scale domain
                        const parentResolve = model.parent.component.resolve;
                        if (parentResolve.scale[channel] === 'independent') {
                            return [stepSignal(scaleName, range)];
                        }
                    }
                    return [
                        stepSignal(scaleName, range),
                        {
                            name,
                            update: sizeExpr(scaleName, scaleComponent, `domain('${scaleName}').length`)
                        }
                    ];
                }
            }
            /* istanbul ignore next: Condition should not happen -- only for warning in development. */
            throw new Error('layout size is range step although there is no rangeStep.');
        }
        else {
            return [
                {
                    name,
                    value: size
                }
            ];
        }
    }
    function stepSignal(scaleName, range) {
        return {
            name: scaleName + '_step',
            value: range.step
        };
    }
    function sizeExpr(scaleName, scaleComponent, cardinality) {
        const type = scaleComponent.get('type');
        const padding = scaleComponent.get('padding');
        const paddingOuter = getFirstDefined(scaleComponent.get('paddingOuter'), padding);
        let paddingInner = scaleComponent.get('paddingInner');
        paddingInner =
            type === 'band'
                ? // only band has real paddingInner
                    paddingInner !== undefined
                        ? paddingInner
                        : padding
                : // For point, as calculated in https://github.com/vega/vega-scale/blob/master/src/band.js#L128,
                    // it's equivalent to have paddingInner = 1 since there is only n-1 steps between n points.
                    1;
        return `bandspace(${cardinality}, ${paddingInner}, ${paddingOuter}) * ${scaleName}_step`;
    }

    /**
     * Parse an event selector string.
     * Returns an array of event stream definitions.
     */
    function parseSelector(selector, source, marks) {
      DEFAULT_SOURCE = source || VIEW;
      MARKS = marks || DEFAULT_MARKS;
      return parseMerge(selector.trim()).map(parseSelector$1);
    }

    var VIEW    = 'view',
        LBRACK  = '[',
        RBRACK  = ']',
        LBRACE  = '{',
        RBRACE  = '}',
        COLON   = ':',
        COMMA   = ',',
        NAME    = '@',
        GT      = '>',
        ILLEGAL$1 = /[[\]{}]/,
        DEFAULT_SOURCE,
        MARKS,
        DEFAULT_MARKS = {
          '*': 1,
          arc: 1,
          area: 1,
          group: 1,
          image: 1,
          line: 1,
          path: 1,
          rect: 1,
          rule: 1,
          shape: 1,
          symbol: 1,
          text: 1,
          trail: 1
        };

    function isMarkType(type) {
      return MARKS.hasOwnProperty(type);
    }

    function find(s, i, endChar, pushChar, popChar) {
      var count = 0,
          n = s.length,
          c;
      for (; i<n; ++i) {
        c = s[i];
        if (!count && c === endChar) return i;
        else if (popChar && popChar.indexOf(c) >= 0) --count;
        else if (pushChar && pushChar.indexOf(c) >= 0) ++count;
      }
      return i;
    }

    function parseMerge(s) {
      var output = [],
          start = 0,
          n = s.length,
          i = 0;

      while (i < n) {
        i = find(s, i, COMMA, LBRACK + LBRACE, RBRACK + RBRACE);
        output.push(s.substring(start, i).trim());
        start = ++i;
      }

      if (output.length === 0) {
        throw 'Empty event selector: ' + s;
      }
      return output;
    }

    function parseSelector$1(s) {
      return s[0] === '['
        ? parseBetween(s)
        : parseStream(s);
    }

    function parseBetween(s) {
      var n = s.length,
          i = 1,
          b, stream;

      i = find(s, i, RBRACK, LBRACK, RBRACK);
      if (i === n) {
        throw 'Empty between selector: ' + s;
      }

      b = parseMerge(s.substring(1, i));
      if (b.length !== 2) {
        throw 'Between selector must have two elements: ' + s;
      }

      s = s.slice(i + 1).trim();
      if (s[0] !== GT) {
        throw 'Expected \'>\' after between selector: ' + s;
      }

      b = b.map(parseSelector$1);

      stream = parseSelector$1(s.slice(1).trim());
      if (stream.between) {
        return {
          between: b,
          stream: stream
        };
      } else {
        stream.between = b;
      }

      return stream;
    }

    function parseStream(s) {
      var stream = {source: DEFAULT_SOURCE},
          source = [],
          throttle = [0, 0],
          markname = 0,
          start = 0,
          n = s.length,
          i = 0, j,
          filter;

      // extract throttle from end
      if (s[n-1] === RBRACE) {
        i = s.lastIndexOf(LBRACE);
        if (i >= 0) {
          try {
            throttle = parseThrottle(s.substring(i+1, n-1));
          } catch (e) {
            throw 'Invalid throttle specification: ' + s;
          }
          s = s.slice(0, i).trim();
          n = s.length;
        } else throw 'Unmatched right brace: ' + s;
        i = 0;
      }

      if (!n) throw s;

      // set name flag based on first char
      if (s[0] === NAME) markname = ++i;

      // extract first part of multi-part stream selector
      j = find(s, i, COLON);
      if (j < n) {
        source.push(s.substring(start, j).trim());
        start = i = ++j;
      }

      // extract remaining part of stream selector
      i = find(s, i, LBRACK);
      if (i === n) {
        source.push(s.substring(start, n).trim());
      } else {
        source.push(s.substring(start, i).trim());
        filter = [];
        start = ++i;
        if (start === n) throw 'Unmatched left bracket: ' + s;
      }

      // extract filters
      while (i < n) {
        i = find(s, i, RBRACK);
        if (i === n) throw 'Unmatched left bracket: ' + s;
        filter.push(s.substring(start, i).trim());
        if (i < n-1 && s[++i] !== LBRACK) throw 'Expected left bracket: ' + s;
        start = ++i;
      }

      // marshall event stream specification
      if (!(n = source.length) || ILLEGAL$1.test(source[n-1])) {
        throw 'Invalid event selector: ' + s;
      }

      if (n > 1) {
        stream.type = source[1];
        if (markname) {
          stream.markname = source[0].slice(1);
        } else if (isMarkType(source[0])) {
          stream.marktype = source[0];
        } else {
          stream.source = source[0];
        }
      } else {
        stream.type = source[0];
      }
      if (stream.type.slice(-1) === '!') {
        stream.consume = true;
        stream.type = stream.type.slice(0, -1);
      }
      if (filter != null) stream.filter = filter;
      if (throttle[0]) stream.throttle = throttle[0];
      if (throttle[1]) stream.debounce = throttle[1];

      return stream;
    }

    function parseThrottle(s) {
      var a = s.split(COMMA);
      if (!s.length || a.length > 2) throw s;
      return a.map(function(_) {
        var x = +_;
        if (x !== x) throw s;
        return x;
      });
    }

    class TimeUnitNode extends DataFlowNode {
        constructor(parent, formula) {
            super(parent);
            this.formula = formula;
        }
        clone() {
            return new TimeUnitNode(null, duplicate(this.formula));
        }
        static makeFromEncoding(parent, model) {
            const formula = model.reduceFieldDef((timeUnitComponent, fieldDef) => {
                if (fieldDef.timeUnit) {
                    const f = vgField(fieldDef, { forAs: true });
                    timeUnitComponent[f] = {
                        as: f,
                        timeUnit: fieldDef.timeUnit,
                        field: fieldDef.field
                    };
                }
                return timeUnitComponent;
            }, {});
            if (keys(formula).length === 0) {
                return null;
            }
            return new TimeUnitNode(parent, formula);
        }
        static makeFromTransform(parent, t) {
            return new TimeUnitNode(parent, {
                [t.field]: {
                    as: t.as,
                    timeUnit: t.timeUnit,
                    field: t.field
                }
            });
        }
        merge(other) {
            this.formula = Object.assign({}, this.formula, other.formula);
            other.remove();
        }
        producedFields() {
            return new Set(vals(this.formula).map(f => f.as));
        }
        dependentFields() {
            return new Set(vals(this.formula).map(f => f.field));
        }
        hash() {
            return `TimeUnit ${hash(this.formula)}`;
        }
        assemble() {
            return vals(this.formula).map(c => {
                return {
                    type: 'formula',
                    as: c.as,
                    expr: fieldExpr(c.timeUnit, c.field)
                };
            });
        }
    }

    const scaleBindings = {
        has: selCmpt => {
            return selCmpt.type === 'interval' && selCmpt.resolve === 'global' && selCmpt.bind && selCmpt.bind === 'scales';
        },
        parse: (model, selDef, selCmpt) => {
            const name = varName(selCmpt.name);
            const bound = (selCmpt.scales = []);
            for (const proj of selCmpt.project.items) {
                const channel = proj.channel;
                if (!isScaleChannel(channel)) {
                    continue;
                }
                const scale = model.getScaleComponent(channel);
                const scaleType = scale ? scale.get('type') : undefined;
                if (!scale || !hasContinuousDomain(scaleType)) {
                    warn(message.SCALE_BINDINGS_CONTINUOUS);
                    continue;
                }
                scale.set('domainRaw', { signal: accessPathWithDatum(proj.field, name) }, true);
                bound.push(proj);
                // Bind both x/y for diag plot of repeated views.
                if (model.repeater && model.repeater.row === model.repeater.column) {
                    const scale2 = model.getScaleComponent(channel === X ? Y : X);
                    scale2.set('domainRaw', { signal: accessPathWithDatum(proj.field, name) }, true);
                }
            }
        },
        topLevelSignals: (model, selCmpt, signals) => {
            const bound = selCmpt.scales.filter(proj => !signals.filter(s => s.name === proj.signals.data).length);
            // Top-level signals are only needed for multiview displays and if this
            // view's top-level signals haven't already been generated.
            if (!model.parent || !bound.length) {
                return signals;
            }
            // vlSelectionResolve does not account for the behavior of bound scales in
            // multiview displays. Each unit view adds a tuple to the store, but the
            // state of the selection is the unit selection most recently updated. This
            // state is captured by the top-level signals that we insert and "push
            // outer" to from within the units. We need to reassemble this state into
            // the top-level named signal, except no single selCmpt has a global view.
            const namedSg = signals.filter(s => s.name === selCmpt.name)[0];
            const update = namedSg.update;
            if (update.indexOf(VL_SELECTION_RESOLVE) >= 0) {
                namedSg.update = `{${bound.map(proj => `${$(proj.field)}: ${proj.signals.data}`).join(', ')}}`;
            }
            else {
                for (const proj of bound) {
                    const mapping = `, ${$(proj.field)}: ${proj.signals.data}`;
                    if (update.indexOf(mapping) < 0) {
                        namedSg.update = update.substring(0, update.length - 1) + mapping + '}';
                    }
                }
            }
            return signals.concat(bound.map(proj => ({ name: proj.signals.data })));
        },
        signals: (model, selCmpt, signals) => {
            // Nested signals need only push to top-level signals with multiview displays.
            if (model.parent) {
                for (const proj of selCmpt.scales) {
                    const signal = signals.filter(s => s.name === proj.signals.data)[0];
                    signal.push = 'outer';
                    delete signal.value;
                    delete signal.update;
                }
            }
            return signals;
        }
    };
    function domain(model, channel) {
        const scale = $(model.scaleName(channel));
        return `domain(${scale})`;
    }

    const TUPLE_FIELDS = '_tuple_fields';
    class SelectionProjectionComponent {
        constructor(...items) {
            this.items = items;
            this.has = {};
        }
    }
    const project = {
        has: () => {
            return true; // This transform handles its own defaults, so always run parse.
        },
        parse: (model, selDef, selCmpt) => {
            const name = selCmpt.name;
            const proj = selCmpt.project || (selCmpt.project = new SelectionProjectionComponent());
            const parsed = {};
            const timeUnits = {};
            const signals = new Set();
            const signalName = (p, range) => {
                const suffix = range === 'visual' ? p.channel : p.field;
                let sg = varName(`${name}_${suffix}`);
                for (let counter = 1; signals.has(sg); counter++) {
                    sg = varName(`${name}_${suffix}_${counter}`);
                }
                signals.add(sg);
                return { [range]: sg };
            };
            // If no explicit projection (either fields or encodings) is specified, set some defaults.
            // If an initial value is set, try to infer projections.
            // Otherwise, use the default configuration.
            if (!selDef.fields && !selDef.encodings) {
                const cfg = model.config.selection[selDef.type];
                if (selDef.init) {
                    for (const init of array(selDef.init)) {
                        for (const key of keys(init)) {
                            if (isSingleDefUnitChannel(key)) {
                                (selDef.encodings || (selDef.encodings = [])).push(key);
                            }
                            else {
                                if (isIntervalSelection(selDef)) {
                                    warn('Interval selections should be initialized using "x" and/or "y" keys.');
                                    selDef.encodings = cfg.encodings;
                                }
                                else {
                                    (selDef.fields || (selDef.fields = [])).push(key);
                                }
                            }
                        }
                    }
                }
                else {
                    selDef.encodings = cfg.encodings;
                    selDef.fields = cfg.fields;
                }
            }
            // TODO: find a possible channel mapping for these fields.
            for (const field of selDef.fields || []) {
                const p = { type: 'E', field };
                p.signals = Object.assign({}, signalName(p, 'data'));
                proj.items.push(p);
            }
            for (const channel of selDef.encodings || []) {
                const fieldDef = model.fieldDef(channel);
                if (fieldDef) {
                    let field = fieldDef.field;
                    if (fieldDef.timeUnit) {
                        field = model.vgField(channel);
                        // Construct TimeUnitComponents which will be combined into a
                        // TimeUnitNode. This node may need to be inserted into the
                        // dataflow if the selection is used across views that do not
                        // have these time units defined.
                        timeUnits[field] = {
                            as: field,
                            field: fieldDef.field,
                            timeUnit: fieldDef.timeUnit
                        };
                    }
                    // Prevent duplicate projections on the same field.
                    // TODO: what if the same field is bound to multiple channels (e.g., SPLOM diag).
                    if (!parsed[field]) {
                        // Determine whether the tuple will store enumerated or ranged values.
                        // Interval selections store ranges for continuous scales, and enumerations otherwise.
                        // Single/multi selections store ranges for binned fields, and enumerations otherwise.
                        let type = 'E';
                        if (selCmpt.type === 'interval') {
                            const scaleType = model.getScaleComponent(channel).get('type');
                            if (hasContinuousDomain(scaleType)) {
                                type = 'R';
                            }
                        }
                        else if (fieldDef.bin) {
                            type = 'R-RE';
                        }
                        const p = { field, channel, type };
                        p.signals = Object.assign({}, signalName(p, 'data'), signalName(p, 'visual'));
                        proj.items.push((parsed[field] = p));
                        proj.has[channel] = parsed[field];
                    }
                }
                else {
                    warn(message.cannotProjectOnChannelWithoutField(channel));
                }
            }
            if (selDef.init) {
                if (scaleBindings.has(selCmpt)) {
                    warn(message.NO_INIT_SCALE_BINDINGS);
                }
                else {
                    const parseInit = (i) => {
                        return proj.items.map(p => (i[p.channel] !== undefined ? i[p.channel] : i[p.field]));
                    };
                    if (isIntervalSelection(selDef)) {
                        selCmpt.init = parseInit(selDef.init);
                    }
                    else {
                        const init = isArray(selDef.init) ? selDef.init : [selDef.init];
                        selCmpt.init = init.map(parseInit);
                    }
                }
            }
            if (keys(timeUnits).length) {
                proj.timeUnit = new TimeUnitNode(null, timeUnits);
            }
        },
        signals: (model, selCmpt, allSignals) => {
            const name = selCmpt.name + TUPLE_FIELDS;
            const hasSignal = allSignals.filter(s => s.name === name);
            return hasSignal.length
                ? allSignals
                : allSignals.concat({
                    name,
                    value: selCmpt.project.items.map(proj => {
                        const rest = __rest(proj, ["signals"]);
                        return rest;
                    })
                });
        }
    };

    const BRUSH = '_brush';
    const SCALE_TRIGGER = '_scale_trigger';
    const interval = {
        signals: (model, selCmpt) => {
            const name = selCmpt.name;
            const fieldsSg = name + TUPLE_FIELDS;
            const hasScales = scaleBindings.has(selCmpt);
            const signals = [];
            const dataSignals = [];
            const scaleTriggers = [];
            if (selCmpt.translate && !hasScales) {
                const filterExpr = `!event.item || event.item.mark.name !== ${$(name + BRUSH)}`;
                events(selCmpt, (_, evt) => {
                    const filters = evt.between[0].filter || (evt.between[0].filter = []);
                    if (filters.indexOf(filterExpr) < 0) {
                        filters.push(filterExpr);
                    }
                });
            }
            selCmpt.project.items.forEach((proj, i) => {
                const channel = proj.channel;
                if (channel !== X && channel !== Y) {
                    warn('Interval selections only support x and y encoding channels.');
                    return;
                }
                const init = selCmpt.init ? selCmpt.init[i] : null;
                const cs = channelSignals(model, selCmpt, proj, init);
                const dname = proj.signals.data;
                const vname = proj.signals.visual;
                const scaleName = $(model.scaleName(channel));
                const scaleType = model.getScaleComponent(channel).get('type');
                const toNum = hasContinuousDomain(scaleType) ? '+' : '';
                signals.push(...cs);
                dataSignals.push(dname);
                scaleTriggers.push({
                    scaleName: model.scaleName(channel),
                    expr: `(!isArray(${dname}) || ` +
                        `(${toNum}invert(${scaleName}, ${vname})[0] === ${toNum}${dname}[0] && ` +
                        `${toNum}invert(${scaleName}, ${vname})[1] === ${toNum}${dname}[1]))`
                });
            });
            // Proxy scale reactions to ensure that an infinite loop doesn't occur
            // when an interval selection filter touches the scale.
            if (!hasScales) {
                signals.push({
                    name: name + SCALE_TRIGGER,
                    value: {},
                    on: [
                        {
                            events: scaleTriggers.map(t => ({ scale: t.scaleName })),
                            update: scaleTriggers.map(t => t.expr).join(' && ') + ` ? ${name + SCALE_TRIGGER} : {}`
                        }
                    ]
                });
            }
            // Only add an interval to the store if it has valid data extents. Data extents
            // are set to null if pixel extents are equal to account for intervals over
            // ordinal/nominal domains which, when inverted, will still produce a valid datum.
            const init = selCmpt.init;
            const update = `unit: ${unitName(model)}, fields: ${fieldsSg}, values`;
            return signals.concat(Object.assign({ name: name + TUPLE }, (init ? { init: `{${update}: ${assembleInit(init)}}` } : {}), { on: [
                    {
                        events: [{ signal: dataSignals.join(' || ') }],
                        update: dataSignals.join(' && ') + ` ? {${update}: [${dataSignals}]} : null`
                    }
                ] }));
        },
        modifyExpr: (model, selCmpt) => {
            const tpl = selCmpt.name + TUPLE;
            return tpl + ', ' + (selCmpt.resolve === 'global' ? 'true' : `{unit: ${unitName(model)}}`);
        },
        marks: (model, selCmpt, marks) => {
            const name = selCmpt.name;
            const { x, y } = selCmpt.project.has;
            const xvname = x && x.signals.visual;
            const yvname = y && y.signals.visual;
            const store = `data(${$(selCmpt.name + STORE)})`;
            // Do not add a brush if we're binding to scales.
            if (scaleBindings.has(selCmpt)) {
                return marks;
            }
            const update = {
                x: x !== undefined ? { signal: `${xvname}[0]` } : { value: 0 },
                y: y !== undefined ? { signal: `${yvname}[0]` } : { value: 0 },
                x2: x !== undefined ? { signal: `${xvname}[1]` } : { field: { group: 'width' } },
                y2: y !== undefined ? { signal: `${yvname}[1]` } : { field: { group: 'height' } }
            };
            // If the selection is resolved to global, only a single interval is in
            // the store. Wrap brush mark's encodings with a production rule to test
            // this based on the `unit` property. Hide the brush mark if it corresponds
            // to a unit different from the one in the store.
            if (selCmpt.resolve === 'global') {
                for (const key of keys(update)) {
                    update[key] = [
                        Object.assign({ test: `${store}.length && ${store}[0].unit === ${unitName(model)}` }, update[key]),
                        { value: 0 }
                    ];
                }
            }
            // Two brush marks ensure that fill colors and other aesthetic choices do
            // not interefere with the core marks, but that the brushed region can still
            // be interacted with (e.g., dragging it around).
            const _a = selCmpt.mark, { fill, fillOpacity } = _a, stroke = __rest(_a, ["fill", "fillOpacity"]);
            const vgStroke = keys(stroke).reduce((def, k) => {
                def[k] = [
                    {
                        test: [x !== undefined && `${xvname}[0] !== ${xvname}[1]`, y !== undefined && `${yvname}[0] !== ${yvname}[1]`]
                            .filter(t => t)
                            .join(' && '),
                        value: stroke[k]
                    },
                    { value: null }
                ];
                return def;
            }, {});
            return [
                {
                    name: name + BRUSH + '_bg',
                    type: 'rect',
                    clip: true,
                    encode: {
                        enter: {
                            fill: { value: fill },
                            fillOpacity: { value: fillOpacity }
                        },
                        update: update
                    }
                },
                ...marks,
                {
                    name: name + BRUSH,
                    type: 'rect',
                    clip: true,
                    encode: {
                        enter: {
                            fill: { value: 'transparent' }
                        },
                        update: Object.assign({}, update, vgStroke)
                    }
                }
            ];
        }
    };
    /**
     * Returns the visual and data signals for an interval selection.
     */
    function channelSignals(model, selCmpt, proj, init) {
        const channel = proj.channel;
        const vname = proj.signals.visual;
        const dname = proj.signals.data;
        const hasScales = scaleBindings.has(selCmpt);
        const scaleName = $(model.scaleName(channel));
        const scale = model.getScaleComponent(channel);
        const scaleType = scale ? scale.get('type') : undefined;
        const scaled = (str) => `scale(${scaleName}, ${str})`;
        const size = model.getSizeSignalRef(channel === X ? 'width' : 'height').signal;
        const coord = `${channel}(unit)`;
        const on = events(selCmpt, (def, evt) => {
            return [
                ...def,
                { events: evt.between[0], update: `[${coord}, ${coord}]` },
                { events: evt, update: `[${vname}[0], clamp(${coord}, 0, ${size})]` } // Brush End
            ];
        });
        // React to pan/zooms of continuous scales. Non-continuous scales
        // (band, point) cannot be pan/zoomed and any other changes
        // to their domains (e.g., filtering) should clear the brushes.
        on.push({
            events: { signal: selCmpt.name + SCALE_TRIGGER },
            update: hasContinuousDomain(scaleType) ? `[${scaled(`${dname}[0]`)}, ${scaled(`${dname}[1]`)}]` : `[0, 0]`
        });
        return hasScales
            ? [{ name: dname, on: [] }]
            : [
                Object.assign({ name: vname }, (init ? { init: assembleInit(init, scaled) } : { value: [] }), { on: on }),
                Object.assign({ name: dname }, (init ? { init: assembleInit(init) } : {}), { on: [
                        {
                            events: { signal: vname },
                            update: `${vname}[0] === ${vname}[1] ? null : invert(${scaleName}, ${vname})`
                        }
                    ] })
            ];
    }
    function events(selCmpt, cb) {
        return selCmpt.events.reduce((on, evt) => {
            if (!evt.between) {
                warn(`${evt} is not an ordered event stream for interval selections`);
                return on;
            }
            return cb(on, evt);
        }, []);
    }

    function singleOrMultiSignals(model, selCmpt) {
        const name = selCmpt.name;
        const fieldsSg = name + TUPLE_FIELDS;
        const proj = selCmpt.project;
        const datum = '(item().isVoronoi ? datum.datum : datum)';
        const values = proj.items
            .map(p => {
            const fieldDef = model.fieldDef(p.channel);
            // Binned fields should capture extents, for a range test against the raw field.
            return fieldDef && fieldDef.bin
                ? `[${accessPathWithDatum(model.vgField(p.channel, {}), datum)}, ` +
                    `${accessPathWithDatum(model.vgField(p.channel, { binSuffix: 'end' }), datum)}]`
                : `${accessPathWithDatum(p.field, datum)}`;
        })
            .join(', ');
        // Only add a discrete selection to the store if a datum is present _and_
        // the interaction isn't occurring on a group mark. This guards against
        // polluting interactive state with invalid values in faceted displays
        // as the group marks are also data-driven. We force the update to account
        // for constant null states but varying toggles (e.g., shift-click in
        // whitespace followed by a click in whitespace; the store should only
        // be cleared on the second click).
        const update = `unit: ${unitName(model)}, fields: ${fieldsSg}, values`;
        const signals = [
            {
                name: name + TUPLE,
                on: [
                    {
                        events: selCmpt.events,
                        update: `datum && item().mark.marktype !== 'group' ? {${update}: [${values}]} : null`,
                        force: true
                    }
                ]
            }
        ];
        if (selCmpt.init) {
            const insert = selCmpt.init.map((i) => {
                const str = assembleInit(i);
                return `{${update}: ${str}}`;
            });
            signals.push({
                name: `${name}_init`,
                init: `modify(${$(selCmpt.name + STORE)}, [${insert}])`
            });
        }
        return signals;
    }
    const multi = {
        signals: singleOrMultiSignals,
        modifyExpr: (model, selCmpt) => {
            const tpl = selCmpt.name + TUPLE;
            return tpl + ', ' + (selCmpt.resolve === 'global' ? 'null' : `{unit: ${unitName(model)}}`);
        }
    };

    const single = {
        signals: singleOrMultiSignals,
        modifyExpr: (model, selCmpt) => {
            const tpl = selCmpt.name + TUPLE;
            return tpl + ', ' + (selCmpt.resolve === 'global' ? 'true' : `{unit: ${unitName(model)}}`);
        }
    };

    const STORE = '_store';
    const TUPLE = '_tuple';
    const MODIFY = '_modify';
    const SELECTION_DOMAIN = '_selection_domain_';
    const VL_SELECTION_RESOLVE = 'vlSelectionResolve';
    const compilers = { single, multi, interval };
    function forEachSelection(model, cb) {
        const selections = model.component.selection;
        for (const name in selections) {
            if (selections.hasOwnProperty(name)) {
                const sel = selections[name];
                cb(sel, compilers[sel.type]);
            }
        }
    }
    function getFacetModel(model) {
        let parent = model.parent;
        while (parent) {
            if (isFacetModel(parent)) {
                break;
            }
            parent = parent.parent;
        }
        return parent;
    }
    function unitName(model) {
        let name = $(model.name);
        const facetModel = getFacetModel(model);
        if (facetModel) {
            const { facet } = facetModel;
            for (const channel of FACET_CHANNELS) {
                if (facet[channel]) {
                    name += ` + '__facet_${channel}_' + (${accessPathWithDatum(facetModel.vgField(channel), 'facet')})`;
                }
            }
        }
        return name;
    }
    function requiresSelectionId(model) {
        let identifier = false;
        forEachSelection(model, selCmpt => {
            identifier = identifier || selCmpt.project.items.some(proj => proj.field === SELECTION_ID);
        });
        return identifier;
    }
    function isRawSelectionDomain(domainRaw) {
        return domainRaw.signal.indexOf(SELECTION_DOMAIN) >= 0;
    }

    const VORONOI = 'voronoi';
    const nearest = {
        has: selCmpt => {
            return selCmpt.type !== 'interval' && selCmpt.nearest;
        },
        marks: (model, selCmpt, marks) => {
            const { x, y } = selCmpt.project.has;
            const markType = model.mark;
            if (isPathMark(markType)) {
                warn(message.nearestNotSupportForContinuous(markType));
                return marks;
            }
            const cellDef = {
                name: model.getName(VORONOI),
                type: 'path',
                from: { data: model.getName('marks') },
                encode: {
                    update: Object.assign({ fill: { value: 'transparent' }, strokeWidth: { value: 0.35 }, stroke: { value: 'transparent' }, isVoronoi: { value: true } }, tooltip(model, { reactiveGeom: true }))
                },
                transform: [
                    {
                        type: 'voronoi',
                        x: { expr: x || (!x && !y) ? 'datum.datum.x || 0' : '0' },
                        y: { expr: y || (!x && !y) ? 'datum.datum.y || 0' : '0' },
                        size: [model.getSizeSignalRef('width'), model.getSizeSignalRef('height')]
                    }
                ]
            };
            let index = 0;
            let exists = false;
            marks.forEach((mark, i) => {
                const name = mark.name || '';
                if (name === model.component.mark[0].name) {
                    index = i;
                }
                else if (name.indexOf(VORONOI) >= 0) {
                    exists = true;
                }
            });
            if (!exists) {
                marks.splice(index + 1, 0, cellDef);
            }
            return marks;
        }
    };

    const inputBindings = {
        has: selCmpt => {
            return selCmpt.type === 'single' && selCmpt.resolve === 'global' && selCmpt.bind && selCmpt.bind !== 'scales';
        },
        topLevelSignals: (model, selCmpt, signals) => {
            const name = selCmpt.name;
            const proj = selCmpt.project;
            const bind = selCmpt.bind;
            const init = selCmpt.init && selCmpt.init[0]; // Can only exist on single selections (one initial value).
            const datum = nearest.has(selCmpt) ? '(item().isVoronoi ? datum.datum : datum)' : 'datum';
            proj.items.forEach((p, i) => {
                const sgname = varName(`${name}_${p.field}`);
                const hasSignal = signals.filter(s => s.name === sgname);
                if (!hasSignal.length) {
                    signals.unshift(Object.assign({ name: sgname }, (init ? { init: assembleInit(init[i]) } : { value: null }), { on: [
                            {
                                events: selCmpt.events,
                                update: `datum && item().mark.marktype !== 'group' ? ${accessPathWithDatum(p.field, datum)} : null`
                            }
                        ], bind: bind[p.field] || bind[p.channel] || bind }));
                }
            });
            return signals;
        },
        signals: (model, selCmpt, signals) => {
            const name = selCmpt.name;
            const proj = selCmpt.project;
            const signal = signals.filter(s => s.name === name + TUPLE)[0];
            const fields = name + TUPLE_FIELDS;
            const values = proj.items.map(p => varName(`${name}_${p.field}`));
            const valid = values.map(v => `${v} !== null`).join(' && ');
            if (values.length) {
                signal.update = `${valid} ? {fields: ${fields}, values: [${values.join(', ')}]} : null`;
            }
            delete signal.value;
            delete signal.on;
            return signals;
        }
    };

    const TOGGLE = '_toggle';
    const toggle = {
        has: selCmpt => {
            return selCmpt.type === 'multi' && selCmpt.toggle;
        },
        signals: (model, selCmpt, signals) => {
            return signals.concat({
                name: selCmpt.name + TOGGLE,
                value: false,
                on: [{ events: selCmpt.events, update: selCmpt.toggle }]
            });
        },
        modifyExpr: (model, selCmpt) => {
            const tpl = selCmpt.name + TUPLE;
            const signal = selCmpt.name + TOGGLE;
            return (`${signal} ? null : ${tpl}, ` +
                (selCmpt.resolve === 'global' ? `${signal} ? null : true, ` : `${signal} ? null : {unit: ${unitName(model)}}, `) +
                `${signal} ? ${tpl} : null`);
        }
    };

    const clear = {
        has: selCmpt => {
            return selCmpt.clear !== false;
        },
        parse: (model, selDef, selCmpt) => {
            if (selDef.clear) {
                selCmpt.clear = parseSelector(selDef.clear, 'scope');
            }
        },
        topLevelSignals: (model, selCmpt, signals) => {
            if (inputBindings.has(selCmpt)) {
                selCmpt.project.items.forEach(proj => {
                    const idx = signals.findIndex(n => n.name === varName(`${selCmpt.name}_${proj.field}`));
                    if (idx !== -1) {
                        signals[idx].on.push({ events: selCmpt.clear, update: 'null' });
                    }
                });
            }
            return signals;
        },
        signals: (model, selCmpt, signals) => {
            function addClear(idx, update) {
                if (idx !== -1 && signals[idx].on) {
                    signals[idx].on.push({ events: selCmpt.clear, update });
                }
            }
            // Be as minimalist as possible when adding clear triggers to minimize dataflow execution.
            if (selCmpt.type === 'interval') {
                selCmpt.project.items.forEach(proj => {
                    const vIdx = signals.findIndex(n => n.name === proj.signals.visual);
                    addClear(vIdx, '[0, 0]');
                    if (vIdx === -1) {
                        const dIdx = signals.findIndex(n => n.name === proj.signals.data);
                        addClear(dIdx, 'null');
                    }
                });
            }
            else {
                let tIdx = signals.findIndex(n => n.name === selCmpt.name + TUPLE);
                addClear(tIdx, 'null');
                if (toggle.has(selCmpt)) {
                    tIdx = signals.findIndex(n => n.name === selCmpt.name + TOGGLE);
                    addClear(tIdx, 'false');
                }
            }
            return signals;
        }
    };

    const ANCHOR = '_translate_anchor';
    const DELTA = '_translate_delta';
    const translate = {
        has: selCmpt => {
            return selCmpt.type === 'interval' && selCmpt.translate;
        },
        signals: (model, selCmpt, signals) => {
            const name = selCmpt.name;
            const hasScales = scaleBindings.has(selCmpt);
            const anchor = name + ANCHOR;
            const { x, y } = selCmpt.project.has;
            let events = parseSelector(selCmpt.translate, 'scope');
            if (!hasScales) {
                events = events.map(e => ((e.between[0].markname = name + BRUSH), e));
            }
            signals.push({
                name: anchor,
                value: {},
                on: [
                    {
                        events: events.map(e => e.between[0]),
                        update: '{x: x(unit), y: y(unit)' +
                            (x !== undefined ? ', extent_x: ' + (hasScales ? domain(model, X) : `slice(${x.signals.visual})`) : '') +
                            (y !== undefined ? ', extent_y: ' + (hasScales ? domain(model, Y) : `slice(${y.signals.visual})`) : '') +
                            '}'
                    }
                ]
            }, {
                name: name + DELTA,
                value: {},
                on: [
                    {
                        events: events,
                        update: `{x: ${anchor}.x - x(unit), y: ${anchor}.y - y(unit)}`
                    }
                ]
            });
            if (x !== undefined) {
                onDelta(model, selCmpt, x, 'width', signals);
            }
            if (y !== undefined) {
                onDelta(model, selCmpt, y, 'height', signals);
            }
            return signals;
        }
    };
    function onDelta(model, selCmpt, proj, size, signals) {
        const name = selCmpt.name;
        const anchor = name + ANCHOR;
        const delta = name + DELTA;
        const channel = proj.channel;
        const hasScales = scaleBindings.has(selCmpt);
        const signal = signals.filter(s => s.name === proj.signals[hasScales ? 'data' : 'visual'])[0];
        const sizeSg = model.getSizeSignalRef(size).signal;
        const scaleCmpt = model.getScaleComponent(channel);
        const scaleType = scaleCmpt.get('type');
        const sign = hasScales && channel === X ? '-' : ''; // Invert delta when panning x-scales.
        const extent = `${anchor}.extent_${channel}`;
        const offset = `${sign}${delta}.${channel} / ` + (hasScales ? `${sizeSg}` : `span(${extent})`);
        const panFn = !hasScales
            ? 'panLinear'
            : scaleType === 'log'
                ? 'panLog'
                : scaleType === 'pow'
                    ? 'panPow'
                    : 'panLinear';
        const update = `${panFn}(${extent}, ${offset}` +
            (hasScales && scaleType === 'pow' ? `, ${scaleCmpt.get('exponent') || 1}` : '') +
            ')';
        signal.on.push({
            events: { signal: delta },
            update: hasScales ? update : `clampRange(${update}, 0, ${sizeSg})`
        });
    }

    const ANCHOR$1 = '_zoom_anchor';
    const DELTA$1 = '_zoom_delta';
    const zoom = {
        has: selCmpt => {
            return selCmpt.type === 'interval' && selCmpt.zoom;
        },
        signals: (model, selCmpt, signals) => {
            const name = selCmpt.name;
            const hasScales = scaleBindings.has(selCmpt);
            const delta = name + DELTA$1;
            const { x, y } = selCmpt.project.has;
            const sx = $(model.scaleName(X));
            const sy = $(model.scaleName(Y));
            let events = parseSelector(selCmpt.zoom, 'scope');
            if (!hasScales) {
                events = events.map(e => ((e.markname = name + BRUSH), e));
            }
            signals.push({
                name: name + ANCHOR$1,
                on: [
                    {
                        events: events,
                        update: !hasScales
                            ? `{x: x(unit), y: y(unit)}`
                            : '{' +
                                [sx ? `x: invert(${sx}, x(unit))` : '', sy ? `y: invert(${sy}, y(unit))` : '']
                                    .filter(expr => !!expr)
                                    .join(', ') +
                                '}'
                    }
                ]
            }, {
                name: delta,
                on: [
                    {
                        events: events,
                        force: true,
                        update: 'pow(1.001, event.deltaY * pow(16, event.deltaMode))'
                    }
                ]
            });
            if (x !== undefined) {
                onDelta$1(model, selCmpt, x, 'width', signals);
            }
            if (y !== undefined) {
                onDelta$1(model, selCmpt, y, 'height', signals);
            }
            return signals;
        }
    };
    function onDelta$1(model, selCmpt, proj, size, signals) {
        const name = selCmpt.name;
        const channel = proj.channel;
        const hasScales = scaleBindings.has(selCmpt);
        const signal = signals.filter(s => s.name === proj.signals[hasScales ? 'data' : 'visual'])[0];
        const sizeSg = model.getSizeSignalRef(size).signal;
        const scaleCmpt = model.getScaleComponent(channel);
        const scaleType = scaleCmpt.get('type');
        const base = hasScales ? domain(model, channel) : signal.name;
        const delta = name + DELTA$1;
        const anchor = `${name}${ANCHOR$1}.${channel}`;
        const zoomFn = !hasScales
            ? 'zoomLinear'
            : scaleType === 'log'
                ? 'zoomLog'
                : scaleType === 'pow'
                    ? 'zoomPow'
                    : 'zoomLinear';
        const update = `${zoomFn}(${base}, ${anchor}, ${delta}` +
            (hasScales && scaleType === 'pow' ? `, ${scaleCmpt.get('exponent') || 1}` : '') +
            ')';
        signal.on.push({
            events: { signal: delta },
            update: hasScales ? update : `clampRange(${update}, 0, ${sizeSg})`
        });
    }

    const compilers$1 = [project, toggle, scaleBindings, translate, zoom, inputBindings, nearest, clear];
    function forEachTransform(selCmpt, cb) {
        for (const t of compilers$1) {
            if (t.has(selCmpt)) {
                cb(t);
            }
        }
    }

    function assembleInit(init, wrap = identity) {
        if (isArray(init)) {
            const str = init.map(v => assembleInit(v, wrap)).join(', ');
            return `[${str}]`;
        }
        else if (isDateTime(init)) {
            return wrap(dateTimeExpr(init));
        }
        return wrap(JSON.stringify(init));
    }
    function assembleUnitSelectionSignals(model, signals) {
        forEachSelection(model, (selCmpt, selCompiler) => {
            const name = selCmpt.name;
            let modifyExpr = selCompiler.modifyExpr(model, selCmpt);
            signals.push(...selCompiler.signals(model, selCmpt));
            forEachTransform(selCmpt, txCompiler => {
                if (txCompiler.signals) {
                    signals = txCompiler.signals(model, selCmpt, signals);
                }
                if (txCompiler.modifyExpr) {
                    modifyExpr = txCompiler.modifyExpr(model, selCmpt, modifyExpr);
                }
            });
            signals.push({
                name: name + MODIFY,
                update: `modify(${$(selCmpt.name + STORE)}, ${modifyExpr})`
            });
        });
        return signals;
    }
    function assembleFacetSignals(model, signals) {
        if (model.component.selection && keys(model.component.selection).length) {
            const name = $(model.getName('cell'));
            signals.unshift({
                name: 'facet',
                value: {},
                on: [
                    {
                        events: parseSelector('mousemove', 'scope'),
                        update: `isTuple(facet) ? facet : group(${name}).datum`
                    }
                ]
            });
        }
        return signals;
    }
    function assembleTopLevelSignals(model, signals) {
        let hasSelections = false;
        forEachSelection(model, (selCmpt, selCompiler) => {
            const name = selCmpt.name;
            const store = $(name + STORE);
            const hasSg = signals.filter(s => s.name === name);
            if (!hasSg.length) {
                signals.push({
                    name: selCmpt.name,
                    update: `${VL_SELECTION_RESOLVE}(${store}` +
                        (selCmpt.resolve === 'global' ? ')' : `, ${$(selCmpt.resolve)})`)
                });
            }
            hasSelections = true;
            if (selCompiler.topLevelSignals) {
                signals = selCompiler.topLevelSignals(model, selCmpt, signals);
            }
            forEachTransform(selCmpt, txCompiler => {
                if (txCompiler.topLevelSignals) {
                    signals = txCompiler.topLevelSignals(model, selCmpt, signals);
                }
            });
        });
        if (hasSelections) {
            const hasUnit = signals.filter(s => s.name === 'unit');
            if (!hasUnit.length) {
                signals.unshift({
                    name: 'unit',
                    value: {},
                    on: [{ events: 'mousemove', update: 'isTuple(group()) ? group() : unit' }]
                });
            }
        }
        return signals;
    }
    function assembleUnitSelectionData(model, data) {
        forEachSelection(model, selCmpt => {
            const contains = data.filter(d => d.name === selCmpt.name + STORE);
            if (!contains.length) {
                data.push({ name: selCmpt.name + STORE });
            }
        });
        return data;
    }
    function assembleUnitSelectionMarks(model, marks) {
        forEachSelection(model, (selCmpt, selCompiler) => {
            marks = selCompiler.marks ? selCompiler.marks(model, selCmpt, marks) : marks;
            forEachTransform(selCmpt, txCompiler => {
                if (txCompiler.marks) {
                    marks = txCompiler.marks(model, selCmpt, marks);
                }
            });
        });
        return marks;
    }
    function assembleLayerSelectionMarks(model, marks) {
        for (const child of model.children) {
            if (isUnitModel(child)) {
                marks = assembleUnitSelectionMarks(child, marks);
            }
        }
        return marks;
    }
    function assembleSelectionPredicate(model, selections, dfnode) {
        const stores = [];
        function expr(name) {
            const vname = varName(name);
            const selCmpt = model.getSelectionComponent(vname, name);
            const store = $(vname + STORE);
            if (selCmpt.project.timeUnit) {
                const child = dfnode || model.component.data.raw;
                const tunode = selCmpt.project.timeUnit.clone();
                if (child.parent) {
                    tunode.insertAsParentOf(child);
                }
                else {
                    child.parent = tunode;
                }
            }
            if (selCmpt.empty !== 'none') {
                stores.push(store);
            }
            return (`vlSelectionTest(${store}, datum` + (selCmpt.resolve === 'global' ? ')' : `, ${$(selCmpt.resolve)})`));
        }
        const predicateStr = logicalExpr(selections, expr);
        return ((stores.length ? '!(' + stores.map(s => `length(data(${s}))`).join(' || ') + ') || ' : '') + `(${predicateStr})`);
    }
    // Selections are parsed _after_ scales. If a scale domain is set to
    // use a selection, the SELECTION_DOMAIN constant is used as the
    // domainRaw.signal during scale.parse and then replaced with the necessary
    // selection expression function during scale.assemble. To not pollute the
    // type signatures to account for this setup, the selection domain definition
    // is coerced to a string and appended to SELECTION_DOMAIN.
    function assembleSelectionScaleDomain(model, domainRaw) {
        const selDomain = JSON.parse(domainRaw.signal.replace(SELECTION_DOMAIN, ''));
        const name = varName(selDomain.selection);
        const encoding = selDomain.encoding;
        let field = selDomain.field;
        let selCmpt = model.component.selection && model.component.selection[name];
        if (selCmpt) {
            warn('Use "bind": "scales" to setup a binding for scales and selections within the same view.');
        }
        else {
            selCmpt = model.getSelectionComponent(name, selDomain.selection);
            if (!encoding && !field) {
                field = selCmpt.project.items[0].field;
                if (selCmpt.project.items.length > 1) {
                    warn('A "field" or "encoding" must be specified when using a selection as a scale domain. ' +
                        `Using "field": ${$(field)}.`);
                }
            }
            else if (encoding && !field) {
                const encodings = selCmpt.project.items.filter(p => p.channel === encoding);
                if (!encodings.length || encodings.length > 1) {
                    field = selCmpt.project.items[0].field;
                    warn((!encodings.length ? 'No ' : 'Multiple ') +
                        `matching ${$(encoding)} encoding found for selection ${$(selDomain.selection)}. ` +
                        `Using "field": ${$(field)}.`);
                }
                else {
                    field = encodings[0].field;
                }
            }
            return { signal: accessPathWithDatum(field, name) };
        }
        return { signal: 'null' };
    }

    /**
     * Converts a predicate into an expression.
     */
    // model is only used for selection filters.
    function expression(model, filterOp, node) {
        return logicalExpr(filterOp, (predicate) => {
            if (isString(predicate)) {
                return predicate;
            }
            else if (isSelectionPredicate(predicate)) {
                return assembleSelectionPredicate(model, predicate.selection, node);
            }
            else {
                // Filter Object
                return fieldFilterExpression(predicate);
            }
        });
    }

    function midPointWithPositionInvalidTest(params) {
        const { channel, channelDef, mark, scale } = params;
        const ref = midPoint(params);
        // Wrap to check if the positional value is invalid, if so, plot the point on the min value
        if (
        // Only this for field def without counting aggregate (as count wouldn't be null)
        isFieldDef(channelDef) &&
            !isCountingAggregateOp(channelDef.aggregate) &&
            // and only for continuous scale without zero (otherwise, null / invalid will be interpreted as zero, which doesn't cause layout problem)
            scale &&
            isContinuousToContinuous(scale.get('type')) &&
            scale.get('zero') === false) {
            return wrapPositionInvalidTest({
                fieldDef: channelDef,
                channel,
                mark,
                ref
            });
        }
        return ref;
    }
    function wrapPositionInvalidTest({ fieldDef, channel, mark, ref }) {
        if (!isPathMark(mark)) {
            // Only do this for non-path mark (as path marks will already use "defined" to skip points)
            return [fieldInvalidTestValueRef(fieldDef, channel), ref];
        }
        return ref;
    }
    function fieldInvalidTestValueRef(fieldDef, channel) {
        const test = fieldInvalidPredicate(fieldDef, true);
        const mainChannel = getMainRangeChannel(channel);
        const zeroValueRef = mainChannel === 'x' ? { value: 0 } : { field: { group: 'height' } };
        return Object.assign({ test }, zeroValueRef);
    }
    function fieldInvalidPredicate(field, invalid = true) {
        field = isString(field) ? field : vgField(field, { expr: 'datum' });
        const op = invalid ? '||' : '&&';
        const eq = invalid ? '===' : '!==';
        return `${field} ${eq} null ${op} ${invalid ? '' : '!'}isNaN(${field})`;
    }
    // TODO: we need to find a way to refactor these so that scaleName is a part of scale
    // but that's complicated.  For now, this is a huge step moving forward.
    /**
     * @return Vega ValueRef for normal x- or y-position without projection
     */
    function position(params) {
        const { channel, channelDef, scaleName, stack, offset } = params;
        if (isFieldDef(channelDef) && stack && channel === stack.fieldChannel) {
            // x or y use stack_end so that stacked line's point mark use stack_end too.
            return fieldRef(channelDef, scaleName, { suffix: 'end' }, { offset });
        }
        return midPointWithPositionInvalidTest(params);
    }
    /**
     * @return Vega ValueRef for normal x2- or y2-position without projection
     */
    function position2({ channel, channelDef, channel2Def, scaleName, scale, stack, mark, offset, defaultRef }) {
        if (isFieldDef(channelDef) &&
            stack &&
            // If fieldChannel is X and channel is X2 (or Y and Y2)
            channel.charAt(0) === stack.fieldChannel.charAt(0)) {
            return fieldRef(channelDef, scaleName, { suffix: 'start' }, { offset });
        }
        return midPointWithPositionInvalidTest({
            channel,
            channelDef: channel2Def,
            scaleName,
            scale,
            stack,
            mark,
            offset,
            defaultRef
        });
    }
    function getOffset(channel, markDef) {
        const offsetChannel = (channel + 'Offset'); // Need to cast as the type can't be inferred automatically
        // TODO: in the future read from encoding channel too
        const markDefOffsetValue = markDef[offsetChannel];
        if (markDefOffsetValue) {
            return markDefOffsetValue;
        }
        return undefined;
    }
    /**
     * Value Ref for binned fields
     */
    function bin$1({ channel, fieldDef, scaleName, mark, side, offset }) {
        const binSuffix = side === 'start' ? undefined : 'end';
        const ref = fieldRef(fieldDef, scaleName, { binSuffix }, offset ? { offset } : {});
        return wrapPositionInvalidTest({
            fieldDef,
            channel,
            mark,
            ref
        });
    }
    function fieldRef(fieldDef, scaleName, opt, mixins) {
        const ref = Object.assign({}, (scaleName ? { scale: scaleName } : {}), { field: vgField(fieldDef, opt) });
        if (mixins) {
            const { offset, band } = mixins;
            return Object.assign({}, ref, (offset ? { offset } : {}), (band ? { band } : {}));
        }
        return ref;
    }
    function bandRef(scaleName, band = true) {
        return {
            scale: scaleName,
            band: band
        };
    }
    /**
     * Signal that returns the middle of a bin from start and end field. Should only be used with x and y.
     */
    function binMidSignal({ scaleName, fieldDef, fieldDef2, offset }) {
        const start = vgField(fieldDef, { expr: 'datum' });
        const end = fieldDef2 !== undefined
            ? vgField(fieldDef2, { expr: 'datum' })
            : vgField(fieldDef, { binSuffix: 'end', expr: 'datum' });
        return Object.assign({ signal: `scale("${scaleName}", (${start} + ${end}) / 2)` }, (offset ? { offset } : {}));
    }
    /**
     * @returns {VgValueRef} Value Ref for xc / yc or mid point for other channels.
     */
    function midPoint({ channel, channelDef, channel2Def, scaleName, scale, stack, offset, defaultRef }) {
        // TODO: datum support
        if (channelDef) {
            /* istanbul ignore else */
            if (isFieldDef(channelDef)) {
                if (isTypedFieldDef(channelDef)) {
                    if (isBinning(channelDef.bin)) {
                        // Use middle only for x an y to place marks in the center between start and end of the bin range.
                        // We do not use the mid point for other channels (e.g. size) so that properties of legends and marks match.
                        if (contains([X, Y], channel) && channelDef.type === QUANTITATIVE) {
                            if (stack && stack.impute) {
                                // For stack, we computed bin_mid so we can impute.
                                return fieldRef(channelDef, scaleName, { binSuffix: 'mid' }, { offset });
                            }
                            // For non-stack, we can just calculate bin mid on the fly using signal.
                            return binMidSignal({ scaleName, fieldDef: channelDef, offset });
                        }
                        return fieldRef(channelDef, scaleName, binRequiresRange(channelDef, channel) ? { binSuffix: 'range' } : {}, {
                            offset
                        });
                    }
                    else if (isBinned(channelDef.bin)) {
                        if (isFieldDef(channel2Def)) {
                            return binMidSignal({ scaleName, fieldDef: channelDef, fieldDef2: channel2Def, offset });
                        }
                        else {
                            const channel2 = channel === X ? X2 : Y2;
                            warn(message.channelRequiredForBinned(channel2));
                        }
                    }
                }
                if (scale) {
                    const scaleType = scale.get('type');
                    if (hasDiscreteDomain(scaleType)) {
                        if (scaleType === 'band') {
                            // For band, to get mid point, need to offset by half of the band
                            return fieldRef(channelDef, scaleName, { binSuffix: 'range' }, { band: 0.5, offset });
                        }
                        return fieldRef(channelDef, scaleName, { binSuffix: 'range' }, { offset });
                    }
                }
                return fieldRef(channelDef, scaleName, {}, { offset }); // no need for bin suffix
            }
            else if (isValueDef(channelDef)) {
                const value = channelDef.value;
                const offsetMixins = offset ? { offset } : {};
                if (contains(['x', 'x2'], channel) && value === 'width') {
                    return Object.assign({ field: { group: 'width' } }, offsetMixins);
                }
                else if (contains(['y', 'y2'], channel) && value === 'height') {
                    return Object.assign({ field: { group: 'height' } }, offsetMixins);
                }
                return Object.assign({ value }, offsetMixins);
            }
            // If channelDef is neither field def or value def, it's a condition-only def.
            // In such case, we will use default ref.
        }
        return isFunction(defaultRef) ? defaultRef() : defaultRef;
    }
    function tooltipForEncoding(encoding, config, { reactiveGeom }) {
        const keyValues = [];
        const usedKey = {};
        function add(fieldDef, channel) {
            const mainChannel = getMainRangeChannel(channel);
            if (channel !== mainChannel) {
                fieldDef = Object.assign({}, fieldDef, { type: encoding[mainChannel].type });
            }
            const key = title(fieldDef, config, { allowDisabling: false });
            const value = text(fieldDef, config, reactiveGeom ? 'datum.datum' : 'datum').signal;
            if (!usedKey[key]) {
                keyValues.push(`${$(key)}: ${value}`);
            }
            usedKey[key] = true;
        }
        forEach(encoding, (channelDef, channel) => {
            if (isFieldDef(channelDef)) {
                add(channelDef, channel);
            }
            else if (hasConditionalFieldDef(channelDef)) {
                add(channelDef.condition, channel);
            }
        });
        return keyValues.length ? { signal: `{${keyValues.join(', ')}}` } : undefined;
    }
    function text(channelDef, config, expr = 'datum') {
        // text
        if (channelDef) {
            if (isValueDef(channelDef)) {
                return { value: channelDef.value };
            }
            if (isTypedFieldDef(channelDef)) {
                return formatSignalRef(channelDef, format(channelDef), expr, config);
            }
        }
        return undefined;
    }
    function mid(sizeRef) {
        return Object.assign({}, sizeRef, { mult: 0.5 });
    }
    function positionDefault({ markDef, config, defaultRef, channel, scaleName, scale, mark, checkBarAreaWithoutZero: checkBarAreaWithZero }) {
        return () => {
            const mainChannel = getMainRangeChannel(channel);
            const definedValueOrConfig = getFirstDefined(markDef[channel], getMarkConfig(channel, markDef, config));
            if (definedValueOrConfig !== undefined) {
                return { value: definedValueOrConfig };
            }
            if (isString(defaultRef)) {
                if (scaleName) {
                    const scaleType = scale.get('type');
                    if (contains([ScaleType.LOG, ScaleType.TIME, ScaleType.UTC], scaleType)) {
                        // Log scales cannot have zero.
                        // Zero in time scale is arbitrary, and does not affect ratio.
                        // (Time is an interval level of measurement, not ratio).
                        // See https://en.wikipedia.org/wiki/Level_of_measurement for more info.
                        if (checkBarAreaWithZero && (mark === 'bar' || mark === 'area')) {
                            warn(message.nonZeroScaleUsedWithLengthMark(mark, mainChannel, { scaleType }));
                        }
                    }
                    else {
                        if (scale.domainDefinitelyIncludesZero) {
                            return {
                                scale: scaleName,
                                value: 0
                            };
                        }
                        if (checkBarAreaWithZero && (mark === 'bar' || mark === 'area')) {
                            warn(message.nonZeroScaleUsedWithLengthMark(mark, mainChannel, { zeroFalse: scale.explicit.zero === false }));
                        }
                    }
                }
                if (defaultRef === 'zeroOrMin') {
                    return mainChannel === 'x' ? { value: 0 } : { field: { group: 'height' } };
                }
                else {
                    // zeroOrMax
                    return mainChannel === 'x' ? { field: { group: 'width' } } : { value: 0 };
                }
            }
            return defaultRef;
        };
    }

    var ref = /*#__PURE__*/Object.freeze({
        fieldInvalidTestValueRef: fieldInvalidTestValueRef,
        fieldInvalidPredicate: fieldInvalidPredicate,
        position: position,
        position2: position2,
        getOffset: getOffset,
        bin: bin$1,
        fieldRef: fieldRef,
        bandRef: bandRef,
        midPoint: midPoint,
        tooltipForEncoding: tooltipForEncoding,
        text: text,
        mid: mid,
        positionDefault: positionDefault
    });

    function isVisible(c) {
        return c !== 'transparent' && c !== null && c !== undefined;
    }
    function color(model) {
        const { markDef, encoding, config } = model;
        const { filled, type: markType } = markDef;
        const configValue = {
            fill: getMarkConfig('fill', markDef, config),
            stroke: getMarkConfig('stroke', markDef, config),
            color: getMarkConfig('color', markDef, config)
        };
        const transparentIfNeeded = contains(['bar', 'point', 'circle', 'square', 'geoshape'], markType)
            ? 'transparent'
            : undefined;
        const defaultFill = getFirstDefined(markDef.fill, configValue.fill, 
        // If there is no fill, always fill symbols, bar, geoshape
        // with transparent fills https://github.com/vega/vega-lite/issues/1316
        transparentIfNeeded);
        const defaultStroke = getFirstDefined(markDef.stroke, configValue.stroke);
        const colorVgChannel = filled ? 'fill' : 'stroke';
        const fillStrokeMarkDefAndConfig = Object.assign({}, (defaultFill ? { fill: { value: defaultFill } } : {}), (defaultStroke ? { stroke: { value: defaultStroke } } : {}));
        if (encoding.fill || encoding.stroke) {
            // ignore encoding.color, markDef.color, config.color
            if (markDef.color) {
                // warn for markDef.color  (no need to warn encoding.color as it will be dropped in normalized already)
                warn(message.droppingColor('property', { fill: 'fill' in encoding, stroke: 'stroke' in encoding }));
            }
            return Object.assign({}, nonPosition('fill', model, { defaultValue: getFirstDefined(defaultFill, transparentIfNeeded) }), nonPosition('stroke', model, { defaultValue: defaultStroke }));
        }
        else if (encoding.color) {
            return Object.assign({}, fillStrokeMarkDefAndConfig, nonPosition('color', model, {
                vgChannel: colorVgChannel,
                // apply default fill/stroke first, then color config, then transparent if needed.
                defaultValue: getFirstDefined(markDef[colorVgChannel], markDef.color, configValue[colorVgChannel], configValue.color, filled ? transparentIfNeeded : undefined)
            }));
        }
        else if (isVisible(markDef.fill) || isVisible(markDef.stroke)) {
            // Ignore markDef.color
            if (markDef.color) {
                warn(message.droppingColor('property', { fill: 'fill' in markDef, stroke: 'stroke' in markDef }));
            }
            return fillStrokeMarkDefAndConfig;
        }
        else if (markDef.color) {
            return Object.assign({}, fillStrokeMarkDefAndConfig, { 
                // override config with markDef.color
                [colorVgChannel]: { value: markDef.color } });
        }
        else if (isVisible(configValue.fill) || isVisible(configValue.stroke)) {
            // ignore config.color
            return fillStrokeMarkDefAndConfig;
        }
        else if (configValue.color) {
            return Object.assign({}, (transparentIfNeeded ? { fill: { value: 'transparent' } } : {}), { [colorVgChannel]: { value: configValue.color } });
        }
        return {};
    }
    function baseEncodeEntry(model, ignore) {
        const { fill, stroke } = color(model);
        return Object.assign({}, markDefProperties(model.markDef, ignore), wrapAllFieldsInvalid(model, 'fill', fill), wrapAllFieldsInvalid(model, 'stroke', stroke), nonPosition('opacity', model), nonPosition('fillOpacity', model), nonPosition('strokeOpacity', model), nonPosition('strokeWidth', model), tooltip(model), text$1(model, 'href'));
    }
    function wrapAllFieldsInvalid(model, channel, valueRef) {
        const { config, mark } = model;
        if (config.invalidValues === 'hide' && valueRef && !isPathMark(mark)) {
            // For non-path marks, we have to exclude invalid values (null and NaN) for scales with continuous domains.
            // For path marks, we will use "defined" property and skip these values instead.
            const test = allFieldsInvalidPredicate(model, { invalid: true, channels: SCALE_CHANNELS });
            if (test) {
                return {
                    [channel]: [
                        // prepend the invalid case
                        // TODO: support custom value
                        { test, value: null },
                        ...array(valueRef)
                    ]
                };
            }
        }
        return valueRef ? { [channel]: valueRef } : {};
    }
    function markDefProperties(mark, ignore) {
        return VG_MARK_CONFIGS.reduce((m, prop) => {
            if (mark[prop] !== undefined && ignore[prop] !== 'ignore') {
                m[prop] = { value: mark[prop] };
            }
            return m;
        }, {});
    }
    function valueIfDefined(prop, value) {
        if (value !== undefined) {
            return { [prop]: { value: value } };
        }
        return undefined;
    }
    function allFieldsInvalidPredicate(model, { invalid = false, channels }) {
        const filterIndex = channels.reduce((aggregator, channel) => {
            const scaleComponent = model.getScaleComponent(channel);
            if (scaleComponent) {
                const scaleType = scaleComponent.get('type');
                const field = model.vgField(channel, { expr: 'datum' });
                // While discrete domain scales can handle invalid values, continuous scales can't.
                if (field && hasContinuousDomain(scaleType)) {
                    aggregator[field] = true;
                }
            }
            return aggregator;
        }, {});
        const fields = keys(filterIndex);
        if (fields.length > 0) {
            const op = invalid ? '||' : '&&';
            return fields.map(field => fieldInvalidPredicate(field, invalid)).join(` ${op} `);
        }
        return undefined;
    }
    function defined(model) {
        if (model.config.invalidValues) {
            const signal = allFieldsInvalidPredicate(model, { channels: ['x', 'y'] });
            if (signal) {
                return { defined: { signal } };
            }
        }
        return {};
    }
    /**
     * Return mixins for non-positional channels with scales.  (Text doesn't have scale.)
     */
    function nonPosition(channel, model, opt = {}) {
        const { markDef, encoding, config } = model;
        const { vgChannel = channel } = opt;
        let { defaultRef, defaultValue } = opt;
        if (defaultRef === undefined) {
            // prettier-ignore
            defaultValue = defaultValue ||
                (vgChannel === channel
                    ? // When vl channel is the same as Vega's, no need to read from config as Vega will apply them correctly
                        markDef[channel]
                    : // However, when they are different (e.g, vl's text size is vg fontSize), need to read "size" from configs
                        getFirstDefined(markDef[channel], markDef[vgChannel], getMarkConfig(channel, markDef, config, { vgChannel })));
            defaultRef = defaultValue ? { value: defaultValue } : undefined;
        }
        const channelDef = encoding[channel];
        return wrapCondition(model, channelDef, vgChannel, cDef => {
            return midPoint({
                channel,
                channelDef: cDef,
                scaleName: model.scaleName(channel),
                scale: model.getScaleComponent(channel),
                stack: null,
                defaultRef
            });
        });
    }
    /**
     * Return a mixin that include a Vega production rule for a Vega-Lite conditional channel definition.
     * or a simple mixin if channel def has no condition.
     */
    function wrapCondition(model, channelDef, vgChannel, refFn) {
        const condition = channelDef && channelDef.condition;
        const valueRef = refFn(channelDef);
        if (condition) {
            const conditions = isArray(condition) ? condition : [condition];
            const vgConditions = conditions.map(c => {
                const conditionValueRef = refFn(c);
                const test = isConditionalSelection(c)
                    ? assembleSelectionPredicate(model, c.selection)
                    : expression(model, c.test);
                return Object.assign({ test }, conditionValueRef);
            });
            return {
                [vgChannel]: [...vgConditions, ...(valueRef !== undefined ? [valueRef] : [])]
            };
        }
        else {
            return valueRef !== undefined ? { [vgChannel]: valueRef } : {};
        }
    }
    function tooltip(model, opt = {}) {
        const { encoding, markDef, config } = model;
        const channelDef = encoding.tooltip;
        if (isArray(channelDef)) {
            return { tooltip: tooltipForEncoding({ tooltip: channelDef }, config, opt) };
        }
        else {
            return wrapCondition(model, channelDef, 'tooltip', cDef => {
                // use valueRef based on channelDef first
                const tooltipRefFromChannelDef = text(cDef, model.config, opt.reactiveGeom ? 'datum.datum' : 'datum');
                if (tooltipRefFromChannelDef) {
                    return tooltipRefFromChannelDef;
                }
                if (cDef === null) {
                    // Allow using encoding.tooltip = null to disable tooltip
                    return undefined;
                }
                // If tooltipDef does not exist, then use value from markDef or config
                const markTooltip = getFirstDefined(markDef.tooltip, getMarkConfig('tooltip', markDef, config));
                if (isString(markTooltip)) {
                    return { value: markTooltip };
                }
                else if (isObject(markTooltip)) {
                    // `tooltip` is `{fields: 'encodings' | 'fields'}`
                    if (markTooltip.content === 'encoding') {
                        return tooltipForEncoding(encoding, config, opt);
                    }
                    else {
                        return { signal: 'datum' };
                    }
                }
                return undefined;
            });
        }
    }
    function text$1(model, channel = 'text') {
        const channelDef = model.encoding[channel];
        return wrapCondition(model, channelDef, channel, cDef => text(cDef, model.config));
    }
    function bandPosition(fieldDef, channel, model, defaultSizeRef) {
        const scaleName = model.scaleName(channel);
        const sizeChannel = channel === 'x' ? 'width' : 'height';
        if (model.encoding.size ||
            model.markDef.size !== undefined ||
            (defaultSizeRef && defaultSizeRef.value !== undefined)) {
            const orient = model.markDef.orient;
            if (orient) {
                const centeredBandPositionMixins = {
                    // Use xc/yc and place the mark at the middle of the band
                    // This way we never have to deal with size's condition for x/y position.
                    [channel + 'c']: fieldRef(fieldDef, scaleName, {}, { band: 0.5 })
                };
                if (getTypedFieldDef(model.encoding.size)) {
                    return Object.assign({}, centeredBandPositionMixins, nonPosition('size', model, { vgChannel: sizeChannel }));
                }
                else if (isValueDef(model.encoding.size)) {
                    return Object.assign({}, centeredBandPositionMixins, nonPosition('size', model, { vgChannel: sizeChannel }));
                }
                else if (model.markDef.size !== undefined) {
                    return Object.assign({}, centeredBandPositionMixins, { [sizeChannel]: { value: model.markDef.size } });
                }
                else if (defaultSizeRef && defaultSizeRef.value !== undefined) {
                    return Object.assign({}, centeredBandPositionMixins, { [sizeChannel]: defaultSizeRef });
                }
            }
            else {
                warn(message.cannotApplySizeToNonOrientedMark(model.markDef.type));
            }
        }
        return {
            // FIXME: make offset works correctly here when we support group bar (https://github.com/vega/vega-lite/issues/396)
            [channel]: fieldRef(fieldDef, scaleName, { binSuffix: 'range' }, {}),
            [sizeChannel]: defaultSizeRef || bandRef(scaleName)
        };
    }
    function centeredPointPositionWithSize(channel, model, defaultPosRef, defaultSizeRef) {
        const centerChannel = channel === 'x' ? 'xc' : 'yc';
        const sizeChannel = channel === 'x' ? 'width' : 'height';
        return Object.assign({}, pointPosition(channel, model, defaultPosRef, centerChannel), nonPosition('size', model, { defaultRef: defaultSizeRef, vgChannel: sizeChannel }));
    }
    function binPosition({ fieldDef, fieldDef2, channel, scaleName, mark, spacing = 0, reverse }) {
        const binSpacing = {
            x: reverse ? spacing : 0,
            x2: reverse ? 0 : spacing,
            y: reverse ? 0 : spacing,
            y2: reverse ? spacing : 0
        };
        const channel2 = channel === X ? X2 : Y2;
        if (isBinning(fieldDef.bin)) {
            return {
                [channel2]: bin$1({
                    channel,
                    fieldDef,
                    scaleName,
                    mark,
                    side: 'start',
                    offset: binSpacing[`${channel}2`]
                }),
                [channel]: bin$1({ channel, fieldDef, scaleName, mark, side: 'end', offset: binSpacing[channel] })
            };
        }
        else if (isBinned(fieldDef.bin) && isFieldDef(fieldDef2)) {
            return {
                [channel2]: fieldRef(fieldDef, scaleName, {}, { offset: binSpacing[`${channel}2`] }),
                [channel]: fieldRef(fieldDef2, scaleName, {}, { offset: binSpacing[channel] })
            };
        }
        else {
            warn(message.channelRequiredForBinned(channel2));
            return undefined;
        }
    }
    /**
     * Return mixins for point (non-band) position channels.
     */
    function pointPosition(channel, model, defaultRef, vgChannel) {
        // TODO: refactor how refer to scale as discussed in https://github.com/vega/vega-lite/pull/1613
        const { encoding, mark, markDef, config, stack } = model;
        const channelDef = encoding[channel];
        const channel2Def = encoding[channel === X ? X2 : Y2];
        const scaleName = model.scaleName(channel);
        const scale = model.getScaleComponent(channel);
        const offset = getOffset(channel, model.markDef);
        const valueRef = !channelDef && (encoding.latitude || encoding.longitude)
            ? // use geopoint output if there are lat/long and there is no point position overriding lat/long.
                { field: model.getName(channel) }
            : position({
                channel,
                channelDef,
                channel2Def,
                scaleName,
                scale,
                stack,
                mark,
                offset,
                defaultRef: positionDefault({
                    markDef,
                    config,
                    defaultRef,
                    channel,
                    scaleName,
                    scale,
                    mark,
                    checkBarAreaWithoutZero: !channel2Def // only check for non-ranged marks
                })
            });
        return {
            [vgChannel || channel]: valueRef
        };
    }
    /**
     * Return mixins for x2, y2.
     * If channel is not specified, return one channel based on orientation.
     */
    function pointPosition2(model, defaultRef, channel) {
        const { encoding, mark, markDef, stack, config } = model;
        const baseChannel = channel === 'x2' ? 'x' : 'y';
        const channelDef = encoding[baseChannel];
        const scaleName = model.scaleName(baseChannel);
        const scale = model.getScaleComponent(baseChannel);
        const offset = getOffset(channel, model.markDef);
        const valueRef = !channelDef && (encoding.latitude || encoding.longitude)
            ? // use geopoint output if there are lat2/long2 and there is no point position2 overriding lat2/long2.
                { field: model.getName(channel) }
            : position2({
                channel,
                channelDef,
                channel2Def: encoding[channel],
                scaleName,
                scale,
                stack,
                mark,
                offset,
                defaultRef: positionDefault({
                    markDef,
                    config,
                    defaultRef,
                    channel,
                    scaleName,
                    scale,
                    mark,
                    checkBarAreaWithoutZero: !encoding[channel] // only check for non-ranged marks
                })
            });
        return { [channel]: valueRef };
    }

    var mixins = /*#__PURE__*/Object.freeze({
        color: color,
        baseEncodeEntry: baseEncodeEntry,
        valueIfDefined: valueIfDefined,
        defined: defined,
        nonPosition: nonPosition,
        wrapCondition: wrapCondition,
        tooltip: tooltip,
        text: text$1,
        bandPosition: bandPosition,
        centeredPointPositionWithSize: centeredPointPositionWithSize,
        binPosition: binPosition,
        pointPosition: pointPosition,
        pointPosition2: pointPosition2
    });

    function guideEncodeEntry(encoding, model) {
        return keys(encoding).reduce((encode, channel) => {
            const valueDef = encoding[channel];
            return Object.assign({}, encode, wrapCondition(model, valueDef, channel, (x) => ({ value: x.value })));
        }, {});
    }

    function defaultScaleResolve(channel, model) {
        if (isLayerModel(model) || isFacetModel(model)) {
            return 'shared';
        }
        else if (isConcatModel(model) || isRepeatModel(model)) {
            return contains(POSITION_SCALE_CHANNELS, channel) ? 'independent' : 'shared';
        }
        /* istanbul ignore next: should never reach here. */
        throw new Error('invalid model type for resolve');
    }
    function parseGuideResolve(resolve, channel) {
        const channelScaleResolve = resolve.scale[channel];
        const guide = contains(POSITION_SCALE_CHANNELS, channel) ? 'axis' : 'legend';
        if (channelScaleResolve === 'independent') {
            if (resolve[guide][channel] === 'shared') {
                warn(message.independentScaleMeansIndependentGuide(channel));
            }
            return 'independent';
        }
        return resolve[guide][channel] || 'shared';
    }

    /**
     * Generic class for storing properties that are explicitly specified
     * and implicitly determined by the compiler.
     * This is important for scale/axis/legend merging as
     * we want to prioritize properties that users explicitly specified.
     */
    class Split {
        constructor(explicit = {}, implicit = {}) {
            this.explicit = explicit;
            this.implicit = implicit;
        }
        clone() {
            return new Split(duplicate(this.explicit), duplicate(this.implicit));
        }
        combine() {
            // FIXME remove "as any".
            // Add "as any" to avoid an error "Spread types may only be created from object types".
            return Object.assign({}, this.explicit, this.implicit);
        }
        get(key) {
            // Explicit has higher precedence
            return getFirstDefined(this.explicit[key], this.implicit[key]);
        }
        getWithExplicit(key) {
            // Explicit has higher precedence
            if (this.explicit[key] !== undefined) {
                return { explicit: true, value: this.explicit[key] };
            }
            else if (this.implicit[key] !== undefined) {
                return { explicit: false, value: this.implicit[key] };
            }
            return { explicit: false, value: undefined };
        }
        setWithExplicit(key, value) {
            if (value.value !== undefined) {
                this.set(key, value.value, value.explicit);
            }
        }
        set(key, value, explicit) {
            delete this[explicit ? 'implicit' : 'explicit'][key];
            this[explicit ? 'explicit' : 'implicit'][key] = value;
            return this;
        }
        copyKeyFromSplit(key, s) {
            // Explicit has higher precedence
            if (s.explicit[key] !== undefined) {
                this.set(key, s.explicit[key], true);
            }
            else if (s.implicit[key] !== undefined) {
                this.set(key, s.implicit[key], false);
            }
        }
        copyKeyFromObject(key, s) {
            // Explicit has higher precedence
            if (s[key] !== undefined) {
                this.set(key, s[key], true);
            }
        }
        /**
         * Merge split object into this split object. Properties from the other split
         * overwrite properties from this split.
         */
        copyAll(other) {
            for (const key of keys(other.combine())) {
                const val = other.getWithExplicit(key);
                this.setWithExplicit(key, val);
            }
        }
    }
    function makeExplicit(value) {
        return {
            explicit: true,
            value
        };
    }
    function makeImplicit(value) {
        return {
            explicit: false,
            value
        };
    }
    function tieBreakByComparing(compare) {
        return (v1, v2, property, propertyOf) => {
            const diff = compare(v1.value, v2.value);
            if (diff > 0) {
                return v1;
            }
            else if (diff < 0) {
                return v2;
            }
            return defaultTieBreaker(v1, v2, property, propertyOf);
        };
    }
    function defaultTieBreaker(v1, v2, property, propertyOf) {
        if (v1.explicit && v2.explicit) {
            warn(message.mergeConflictingProperty(property, propertyOf, v1.value, v2.value));
        }
        // If equal score, prefer v1.
        return v1;
    }
    function mergeValuesWithExplicit(v1, v2, property, propertyOf, tieBreaker = defaultTieBreaker) {
        if (v1 === undefined || v1.value === undefined) {
            // For first run
            return v2;
        }
        if (v1.explicit && !v2.explicit) {
            return v1;
        }
        else if (v2.explicit && !v1.explicit) {
            return v2;
        }
        else if (stringify(v1.value) === stringify(v2.value)) {
            return v1;
        }
        else {
            return tieBreaker(v1, v2, property, propertyOf);
        }
    }

    class LegendComponent extends Split {
    }

    function values$1(legend, fieldDef) {
        const vals = legend.values;
        if (vals) {
            return valueArray(fieldDef, vals);
        }
        return undefined;
    }
    function defaultSymbolType(mark) {
        return mark === 'line' ? 'stroke' : 'circle';
    }
    function clipHeight(legendType) {
        if (legendType === 'gradient') {
            return 20;
        }
        return undefined;
    }
    function type(params) {
        const { legend } = params;
        return getFirstDefined(legend.type, defaultType$1(params));
    }
    function defaultType$1({ channel, timeUnit, scaleType, alwaysReturn }) {
        // Following the logic in https://github.com/vega/vega-parser/blob/master/src/parsers/legend.js
        if (isColorChannel(channel)) {
            if (contains(['quarter', 'month', 'day'], timeUnit)) {
                return 'symbol';
            }
            if (isContinuousToContinuous(scaleType)) {
                return alwaysReturn ? 'gradient' : undefined;
            }
        }
        return alwaysReturn ? 'symbol' : undefined;
    }
    function direction({ legend, legendConfig, timeUnit, channel, scaleType }) {
        const orient = getFirstDefined(legend.orient, legendConfig.orient, 'right');
        const legendType = type({ legend, channel, timeUnit, scaleType, alwaysReturn: true });
        return getFirstDefined(legend.direction, legendConfig[legendType ? 'gradientDirection' : 'symbolDirection'], defaultDirection(orient, legendType));
    }
    function defaultDirection(orient, legendType) {
        switch (orient) {
            case 'top':
            case 'bottom':
                return 'horizontal';
            case 'left':
            case 'right':
            case 'none':
            case undefined: // undefined = "right" in Vega
                return undefined; // vertical is Vega's default
            default:
                // top-left / ...
                // For inner legend, uses compact layout like Tableau
                return legendType === 'gradient' ? 'horizontal' : undefined;
        }
    }
    function defaultGradientLength({ legend, legendConfig, model, channel, scaleType }) {
        const { gradientHorizontalMaxLength, gradientHorizontalMinLength, gradientVerticalMaxLength, gradientVerticalMinLength } = legendConfig;
        const dir = direction({ legend, legendConfig, channel, scaleType });
        if (dir === 'horizontal') {
            const orient = getFirstDefined(legend.orient, legendConfig.orient);
            if (orient === 'top' || orient === 'bottom') {
                return gradientLengthSignal(model, 'width', gradientHorizontalMinLength, gradientHorizontalMaxLength);
            }
            else {
                return gradientHorizontalMinLength;
            }
        }
        else {
            // vertical / undefined (Vega uses vertical by default)
            return gradientLengthSignal(model, 'height', gradientVerticalMinLength, gradientVerticalMaxLength);
        }
    }
    function gradientLengthSignal(model, sizeType, min, max) {
        const sizeSignal = model.getSizeSignalRef(sizeType).signal;
        return { signal: `clamp(${sizeSignal}, ${min}, ${max})` };
    }
    function defaultLabelOverlap$1(scaleType) {
        if (contains(['quantile', 'threshold', 'log'], scaleType)) {
            return 'greedy';
        }
        return undefined;
    }

    var properties$1 = /*#__PURE__*/Object.freeze({
        values: values$1,
        defaultSymbolType: defaultSymbolType,
        clipHeight: clipHeight,
        type: type,
        defaultType: defaultType$1,
        direction: direction,
        defaultGradientLength: defaultGradientLength,
        defaultLabelOverlap: defaultLabelOverlap$1
    });

    function type$1(legendCmp, model, channel) {
        const scaleType = model.getScaleComponent(channel).get('type');
        return getFirstDefined(legendCmp.get('type'), defaultType$1({ channel, scaleType, alwaysReturn: true }));
    }
    function symbols(fieldDef, symbolsSpec, model, channel, legendCmp) {
        if (type$1(legendCmp, model, channel) !== 'symbol') {
            return undefined;
        }
        let out = Object.assign({}, applyMarkConfig({}, model, FILL_STROKE_CONFIG), color(model)); // FIXME: remove this when VgEncodeEntry is compatible with SymbolEncodeEntry
        switch (model.mark) {
            case BAR:
            case TICK:
            case TEXT:
                out.shape = { value: 'square' };
                break;
            case CIRCLE:
            case SQUARE:
                out.shape = { value: model.mark };
                break;
            case POINT:
            case LINE:
            case GEOSHAPE:
            case AREA:
                // use default circle
                break;
        }
        const { markDef, encoding, config } = model;
        const filled = markDef.filled;
        const opacity = getMaxValue(encoding.opacity) || markDef.opacity;
        if (out.fill) {
            // for fill legend, we don't want any fill in symbol
            if (channel === 'fill' || (filled && channel === COLOR)) {
                delete out.fill;
            }
            else {
                if (out.fill['field']) {
                    // For others, set fill to some opaque value (or nothing if a color is already set)
                    if (legendCmp.get('symbolFillColor')) {
                        delete out.fill;
                    }
                    else {
                        out.fill = { value: config.legend.symbolBaseFillColor || 'black' };
                        out.fillOpacity = { value: opacity || 1 };
                    }
                }
                else if (isArray(out.fill)) {
                    const fill = getFirstConditionValue(encoding.fill || encoding.color) ||
                        markDef.fill ||
                        (filled && markDef.color);
                    if (fill) {
                        out.fill = { value: fill };
                    }
                }
            }
        }
        if (out.stroke) {
            if (channel === 'stroke' || (!filled && channel === COLOR)) {
                delete out.stroke;
            }
            else {
                if (out.stroke['field']) {
                    // For others, remove stroke field
                    delete out.stroke;
                }
                else if (isArray(out.stroke)) {
                    const stroke = getFirstDefined(getFirstConditionValue(encoding.stroke || encoding.color), markDef.stroke, filled ? markDef.color : undefined);
                    if (stroke) {
                        out.stroke = { value: stroke };
                    }
                }
            }
        }
        if (channel !== SHAPE) {
            const shape = getFirstConditionValue(encoding.shape) || markDef.shape;
            if (shape) {
                out.shape = { value: shape };
            }
        }
        if (channel !== OPACITY) {
            if (opacity) {
                // only apply opacity if it is neither zero or undefined
                out.opacity = { value: opacity };
            }
        }
        out = Object.assign({}, out, symbolsSpec);
        return keys(out).length > 0 ? out : undefined;
    }
    function gradient(fieldDef, gradientSpec, model, channel, legendCmp) {
        if (type$1(legendCmp, model, channel) !== 'gradient') {
            return undefined;
        }
        let out = {};
        const opacity = getMaxValue(model.encoding.opacity) || model.markDef.opacity;
        if (opacity) {
            // only apply opacity if it is neither zero or undefined
            out.opacity = { value: opacity };
        }
        out = Object.assign({}, out, gradientSpec);
        return keys(out).length > 0 ? out : undefined;
    }
    function labels(fieldDef, labelsSpec, model, channel) {
        const legend = model.legend(channel);
        const config = model.config;
        let out = {};
        if (isTimeFormatFieldDef(fieldDef)) {
            const isUTCScale = model.getScaleComponent(channel).get('type') === ScaleType.UTC;
            const expr = timeFormatExpression('datum.value', fieldDef.timeUnit, legend.format, config.legend.shortTimeLabels, config.timeFormat, isUTCScale);
            labelsSpec = Object.assign({}, (expr ? { text: { signal: expr } } : {}), labelsSpec);
        }
        out = Object.assign({}, out, labelsSpec);
        return keys(out).length > 0 ? out : undefined;
    }
    function getMaxValue(channelDef) {
        return getConditionValue(channelDef, (v, conditionalDef) => Math.max(v, conditionalDef.value));
    }
    function getFirstConditionValue(channelDef) {
        return getConditionValue(channelDef, (v, conditionalDef) => {
            return getFirstDefined(v, conditionalDef.value);
        });
    }
    function getConditionValue(channelDef, reducer) {
        if (hasConditionalValueDef(channelDef)) {
            return (isArray(channelDef.condition) ? channelDef.condition : [channelDef.condition]).reduce(reducer, channelDef.value);
        }
        else if (isValueDef(channelDef)) {
            return channelDef.value;
        }
        return undefined;
    }

    var encode = /*#__PURE__*/Object.freeze({
        symbols: symbols,
        gradient: gradient,
        labels: labels
    });

    function parseLegend(model) {
        if (isUnitModel(model)) {
            model.component.legends = parseUnitLegend(model);
        }
        else {
            model.component.legends = parseNonUnitLegend(model);
        }
    }
    function parseUnitLegend(model) {
        const { encoding } = model;
        return [COLOR, FILL, STROKE, STROKEWIDTH, SIZE, SHAPE, OPACITY, FILLOPACITY, STROKEOPACITY].reduce((legendComponent, channel) => {
            const def = encoding[channel];
            if (model.legend(channel) &&
                model.getScaleComponent(channel) &&
                !(isFieldDef(def) && (channel === SHAPE && def.type === GEOJSON))) {
                legendComponent[channel] = parseLegendForChannel(model, channel);
            }
            return legendComponent;
        }, {});
    }
    function getLegendDefWithScale(model, channel) {
        const scale = model.scaleName(COLOR);
        if (channel === 'color') {
            return model.markDef.filled ? { fill: scale } : { stroke: scale };
        }
        return { [channel]: model.scaleName(channel) };
    }
    function isExplicit(value, property, legend, fieldDef) {
        switch (property) {
            case 'values':
                // specified legend.values is already respected, but may get transformed.
                return !!legend.values;
            case 'title':
                // title can be explicit if fieldDef.title is set
                if (property === 'title' && value === fieldDef.title) {
                    return true;
                }
        }
        // Otherwise, things are explicit if the returned value matches the specified property
        return value === legend[property];
    }
    function parseLegendForChannel(model, channel) {
        const fieldDef = model.fieldDef(channel);
        const legend = model.legend(channel);
        const legendCmpt = new LegendComponent({}, getLegendDefWithScale(model, channel));
        for (const property of LEGEND_PROPERTIES) {
            const value = getProperty(property, legend, channel, model);
            if (value !== undefined) {
                const explicit = isExplicit(value, property, legend, fieldDef);
                if (explicit || model.config.legend[property] === undefined) {
                    legendCmpt.set(property, value, explicit);
                }
            }
        }
        const legendEncoding = legend.encoding || {};
        const legendEncode = ['labels', 'legend', 'title', 'symbols', 'gradient'].reduce((e, part) => {
            const legendEncodingPart = guideEncodeEntry(legendEncoding[part] || {}, model);
            const value = encode[part]
                ? encode[part](fieldDef, legendEncodingPart, model, channel, legendCmpt) // apply rule
                : legendEncodingPart; // no rule -- just default values
            if (value !== undefined && keys(value).length > 0) {
                e[part] = { update: value };
            }
            return e;
        }, {});
        if (keys(legendEncode).length > 0) {
            legendCmpt.set('encode', legendEncode, !!legend.encoding);
        }
        return legendCmpt;
    }
    function getProperty(property, legend, channel, model) {
        const { encoding, mark } = model;
        const fieldDef = getTypedFieldDef(encoding[channel]);
        const legendConfig = model.config.legend;
        const { timeUnit } = fieldDef;
        const scaleType = model.getScaleComponent(channel).get('type');
        switch (property) {
            // TODO: enable when https://github.com/vega/vega/issues/1351 is fixed
            // case 'clipHeight':
            //   return getFirstDefined(specifiedLegend.clipHeight, properties.clipHeight(properties.type(...)));
            case 'direction':
                return direction({ legend, legendConfig, timeUnit, channel, scaleType });
            case 'format':
                // We don't include temporal field here as we apply format in encode block
                if (isTimeFormatFieldDef(fieldDef)) {
                    return undefined;
                }
                return numberFormat(fieldDef, legend.format, model.config);
            case 'formatType':
                // Same as format, We don't include temporal field here as we apply format in encode block
                if (isTimeFormatFieldDef(fieldDef)) {
                    return undefined;
                }
                return legend.formatType;
            case 'gradientLength':
                return getFirstDefined(
                // do specified gradientLength first
                legend.gradientLength, legendConfig.gradientLength, 
                // Otherwise, use smart default based on plot height
                defaultGradientLength({
                    model,
                    legend,
                    legendConfig,
                    channel,
                    scaleType
                }));
            case 'labelOverlap':
                return getFirstDefined(legend.labelOverlap, defaultLabelOverlap$1(scaleType));
            case 'symbolType':
                return getFirstDefined(legend.symbolType, defaultSymbolType(mark));
            case 'title':
                return title(fieldDef, model.config, { allowDisabling: true }) || undefined;
            case 'type':
                return type({ legend, channel, timeUnit, scaleType, alwaysReturn: false });
            case 'values':
                return values$1(legend, fieldDef);
        }
        // Otherwise, return specified property.
        return legend[property];
    }
    function parseNonUnitLegend(model) {
        const { legends, resolve } = model.component;
        for (const child of model.children) {
            parseLegend(child);
            keys(child.component.legends).forEach((channel) => {
                resolve.legend[channel] = parseGuideResolve(model.component.resolve, channel);
                if (resolve.legend[channel] === 'shared') {
                    // If the resolve says shared (and has not been overridden)
                    // We will try to merge and see if there is a conflict
                    legends[channel] = mergeLegendComponent(legends[channel], child.component.legends[channel]);
                    if (!legends[channel]) {
                        // If merge returns nothing, there is a conflict so we cannot make the legend shared.
                        // Thus, mark legend as independent and remove the legend component.
                        resolve.legend[channel] = 'independent';
                        delete legends[channel];
                    }
                }
            });
        }
        keys(legends).forEach((channel) => {
            for (const child of model.children) {
                if (!child.component.legends[channel]) {
                    // skip if the child does not have a particular legend
                    continue;
                }
                if (resolve.legend[channel] === 'shared') {
                    // After merging shared legend, make sure to remove legend from child
                    delete child.component.legends[channel];
                }
            }
        });
        return legends;
    }
    function mergeLegendComponent(mergedLegend, childLegend) {
        if (!mergedLegend) {
            return childLegend.clone();
        }
        const mergedOrient = mergedLegend.getWithExplicit('orient');
        const childOrient = childLegend.getWithExplicit('orient');
        if (mergedOrient.explicit && childOrient.explicit && mergedOrient.value !== childOrient.value) {
            // TODO: throw warning if resolve is explicit (We don't have info about explicit/implicit resolve yet.)
            // Cannot merge due to inconsistent orient
            return undefined;
        }
        let typeMerged = false;
        // Otherwise, let's merge
        for (const prop of VG_LEGEND_PROPERTIES) {
            const mergedValueWithExplicit = mergeValuesWithExplicit(mergedLegend.getWithExplicit(prop), childLegend.getWithExplicit(prop), prop, 'legend', 
            // Tie breaker function
            (v1, v2) => {
                switch (prop) {
                    case 'symbolType':
                        return mergeSymbolType(v1, v2);
                    case 'title':
                        return mergeTitleComponent(v1, v2);
                    case 'type':
                        // There are only two types. If we have different types, then prefer symbol over gradient.
                        typeMerged = true;
                        return makeImplicit('symbol');
                }
                return defaultTieBreaker(v1, v2, prop, 'legend');
            });
            mergedLegend.setWithExplicit(prop, mergedValueWithExplicit);
        }
        if (typeMerged) {
            if (((mergedLegend.implicit || {}).encode || {}).gradient) {
                deleteNestedProperty(mergedLegend.implicit, ['encode', 'gradient']);
            }
            if (((mergedLegend.explicit || {}).encode || {}).gradient) {
                deleteNestedProperty(mergedLegend.explicit, ['encode', 'gradient']);
            }
        }
        return mergedLegend;
    }
    function mergeSymbolType(st1, st2) {
        if (st2.value === 'circle') {
            // prefer "circle" over "stroke"
            return st2;
        }
        return st1;
    }

    function assembleLegends(model) {
        const legendComponentIndex = model.component.legends;
        const legendByDomain = {};
        for (const channel of keys(legendComponentIndex)) {
            const scaleComponent = model.getScaleComponent(channel);
            const domainHash = stringify(scaleComponent.domains);
            if (legendByDomain[domainHash]) {
                for (const mergedLegendComponent of legendByDomain[domainHash]) {
                    const merged = mergeLegendComponent(mergedLegendComponent, legendComponentIndex[channel]);
                    if (!merged) {
                        // If cannot merge, need to add this legend separately
                        legendByDomain[domainHash].push(legendComponentIndex[channel]);
                    }
                }
            }
            else {
                legendByDomain[domainHash] = [legendComponentIndex[channel].clone()];
            }
        }
        return flatten(vals(legendByDomain)).map((legendCmpt) => {
            const legend = legendCmpt.combine();
            if (legend.encode && legend.encode.symbols) {
                const out = legend.encode.symbols.update;
                if (out.fill && out.fill['value'] !== 'transparent' && !out.stroke && !legend.stroke) {
                    // For non color channel's legend, we need to override symbol stroke config from Vega config if stroke channel is not used.
                    out.stroke = { value: 'transparent' };
                }
                if (legend.fill) {
                    // If top-level fill is defined, for non color channel's legend, we need remove fill.
                    delete out.fill;
                }
            }
            return legend;
        });
    }

    function assembleProjections(model) {
        if (isLayerModel(model) || isConcatModel(model) || isRepeatModel(model)) {
            return assembleProjectionsForModelAndChildren(model);
        }
        else {
            return assembleProjectionForModel(model);
        }
    }
    function assembleProjectionsForModelAndChildren(model) {
        return model.children.reduce((projections, child) => {
            return projections.concat(child.assembleProjections());
        }, assembleProjectionForModel(model));
    }
    function assembleProjectionForModel(model) {
        const component = model.component.projection;
        if (!component || component.merged) {
            return [];
        }
        const projection = component.combine();
        const { name } = projection, rest = __rest(projection, ["name"]); // we need to extract name so that it is always present in the output and pass TS type validation
        if (!component.data) {
            // generate custom projection, no automatic fitting
            return [
                Object.assign({ name }, { translate: { signal: '[width / 2, height / 2]' } }, rest)
            ];
        }
        else {
            // generate projection that uses extent fitting
            const size = {
                signal: `[${component.size.map(ref => ref.signal).join(', ')}]`
            };
            const fit = component.data.reduce((sources, data) => {
                const source = isSignalRef(data) ? data.signal : `data('${model.lookupDataSource(data)}')`;
                if (!contains(sources, source)) {
                    // build a unique list of sources
                    sources.push(source);
                }
                return sources;
            }, []);
            if (fit.length <= 0) {
                throw new Error("Projection's fit didn't find any data sources");
            }
            return [
                Object.assign({ name,
                    size, fit: {
                        signal: fit.length > 1 ? `[${fit.join(', ')}]` : fit[0]
                    } }, rest)
            ];
        }
    }

    const PROJECTION_PROPERTIES = [
        'type',
        'clipAngle',
        'clipExtent',
        'center',
        'rotate',
        'precision',
        'reflectX',
        'reflectY',
        'coefficient',
        'distance',
        'fraction',
        'lobes',
        'parallel',
        'radius',
        'ratio',
        'spacing',
        'tilt'
    ];

    class ProjectionComponent extends Split {
        constructor(name, specifiedProjection, size, data) {
            super(Object.assign({}, specifiedProjection), // all explicit properties of projection
            { name } // name as initial implicit property
            );
            this.specifiedProjection = specifiedProjection;
            this.size = size;
            this.data = data;
            this.merged = false;
        }
        /**
         * Whether the projection parameters should fit provided data.
         */
        get isFit() {
            return !!this.data;
        }
    }

    function parseProjection(model) {
        model.component.projection = isUnitModel(model) ? parseUnitProjection(model) : parseNonUnitProjections(model);
    }
    function parseUnitProjection(model) {
        if (model.hasProjection) {
            const proj = model.specifiedProjection;
            const fit = !(proj && (proj.scale != null || proj.translate != null));
            const size = fit ? [model.getSizeSignalRef('width'), model.getSizeSignalRef('height')] : undefined;
            const data = fit ? gatherFitData(model) : undefined;
            return new ProjectionComponent(model.projectionName(true), Object.assign({}, (model.config.projection || {}), (proj || {})), size, data);
        }
        return undefined;
    }
    function gatherFitData(model) {
        const data = [];
        [[LONGITUDE, LATITUDE], [LONGITUDE2, LATITUDE2]].forEach(posssiblePair => {
            if (model.channelHasField(posssiblePair[0]) || model.channelHasField(posssiblePair[1])) {
                data.push({
                    signal: model.getName(`geojson_${data.length}`)
                });
            }
        });
        if (model.channelHasField(SHAPE) && model.fieldDef(SHAPE).type === GEOJSON) {
            data.push({
                signal: model.getName(`geojson_${data.length}`)
            });
        }
        if (data.length === 0) {
            // main source is geojson, so we can just use that
            data.push(model.requestDataName(MAIN));
        }
        return data;
    }
    function mergeIfNoConflict(first, second) {
        const allPropertiesShared = every(PROJECTION_PROPERTIES, prop => {
            // neither has the property
            if (!first.explicit.hasOwnProperty(prop) && !second.explicit.hasOwnProperty(prop)) {
                return true;
            }
            // both have property and an equal value for property
            if (first.explicit.hasOwnProperty(prop) &&
                second.explicit.hasOwnProperty(prop) &&
                // some properties might be signals or objects and require hashing for comparison
                stringify(first.get(prop)) === stringify(second.get(prop))) {
                return true;
            }
            return false;
        });
        const size = stringify(first.size) === stringify(second.size);
        if (size) {
            if (allPropertiesShared) {
                return first;
            }
            else if (stringify(first.explicit) === stringify({})) {
                return second;
            }
            else if (stringify(second.explicit) === stringify({})) {
                return first;
            }
        }
        // if all properties don't match, let each unit spec have its own projection
        return null;
    }
    function parseNonUnitProjections(model) {
        if (model.children.length === 0) {
            return undefined;
        }
        let nonUnitProjection;
        // parse all children first
        model.children.forEach(child => parseProjection(child));
        // analyze parsed projections, attempt to merge
        const mergable = every(model.children, child => {
            const projection = child.component.projection;
            if (!projection) {
                // child layer does not use a projection
                return true;
            }
            else if (!nonUnitProjection) {
                // cached 'projection' is null, cache this one
                nonUnitProjection = projection;
                return true;
            }
            else {
                const merge = mergeIfNoConflict(nonUnitProjection, projection);
                if (merge) {
                    nonUnitProjection = merge;
                }
                return !!merge;
            }
        });
        // if cached one and all other children share the same projection,
        if (nonUnitProjection && mergable) {
            // so we can elevate it to the layer level
            const name = model.projectionName(true);
            const modelProjection = new ProjectionComponent(name, nonUnitProjection.specifiedProjection, nonUnitProjection.size, duplicate(nonUnitProjection.data));
            // rename and assign all others as merged
            model.children.forEach(child => {
                const projection = child.component.projection;
                if (projection) {
                    if (projection.isFit) {
                        modelProjection.data.push(...child.component.projection.data);
                    }
                    child.renameProjection(projection.get('name'), name);
                    projection.merged = true;
                }
            });
            return modelProjection;
        }
        return undefined;
    }

    class SourceNode extends DataFlowNode {
        constructor(data) {
            super(null); // source cannot have parent
            data = data || { name: 'source' };
            let format;
            if (!isGenerator(data)) {
                format = data.format ? Object.assign({}, omit(data.format, ['parse'])) : {};
            }
            if (isInlineData(data)) {
                this._data = { values: data.values };
            }
            else if (isUrlData(data)) {
                this._data = { url: data.url };
                if (!format.type) {
                    // Extract extension from URL using snippet from
                    // http://stackoverflow.com/questions/680929/how-to-extract-extension-from-filename-string-in-javascript
                    let defaultExtension = /(?:\.([^.]+))?$/.exec(data.url)[1];
                    if (!contains(['json', 'csv', 'tsv', 'dsv', 'topojson'], defaultExtension)) {
                        defaultExtension = 'json';
                    }
                    // defaultExtension has type string but we ensure that it is DataFormatType above
                    format.type = defaultExtension;
                }
            }
            else if (isSphereGenerator(data)) {
                // hardwire GeoJSON sphere data into output specification
                this._data = { values: [{ type: 'Sphere' }] };
            }
            else if (isNamedData(data) || isGenerator(data)) {
                this._data = {};
            }
            // set flag to check if generator
            this._generator = isGenerator(data);
            // any dataset can be named
            if (data.name) {
                this._name = data.name;
            }
            if (format && keys(format).length > 0) {
                this._data.format = format;
            }
        }
        get data() {
            return this._data;
        }
        hasName() {
            return !!this._name;
        }
        get isGenerator() {
            return this._generator;
        }
        get dataName() {
            return this._name;
        }
        set dataName(name) {
            this._name = name;
        }
        set parent(parent) {
            throw new Error('Source nodes have to be roots.');
        }
        remove() {
            throw new Error('Source nodes are roots and cannot be removed.');
        }
        hash() {
            throw new Error('Cannot hash sources');
        }
        assemble() {
            return Object.assign({ name: this._name }, this._data, { transform: [] });
        }
    }

    /**
     * Iterates over a dataflow graph and checks whether all links are consistent.
     */
    function checkLinks(nodes) {
        for (const node of nodes) {
            for (const child of node.children) {
                if (child.parent !== node) {
                    console.error('Dataflow graph is inconsistent.', node, child);
                    return false;
                }
            }
            if (!checkLinks(node.children)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Abstract base class for BottomUpOptimizer and TopDownOptimizer.
     * Contains only mutation handling logic. Subclasses need to implement iteration logic.
     */
    class OptimizerBase {
        constructor() {
            this._mutated = false;
        }
        // Once true, _mutated is never set to false
        setMutated() {
            this._mutated = true;
        }
        get mutatedFlag() {
            return this._mutated;
        }
    }
    /**
     * Starts from a node and runs the optimization function(the "run" method) upwards to the root,
     * depending on the continueFlag and mutatedFlag values returned by the optimization function.
     */
    class BottomUpOptimizer extends OptimizerBase {
        constructor() {
            super();
            this._continue = false;
        }
        setContinue() {
            this._continue = true;
        }
        get continueFlag() {
            return this._continue;
        }
        get flags() {
            return { continueFlag: this.continueFlag, mutatedFlag: this.mutatedFlag };
        }
        set flags({ continueFlag, mutatedFlag }) {
            if (continueFlag) {
                this.setContinue();
            }
            if (mutatedFlag) {
                this.setMutated();
            }
        }
        /**
         * Reset the state of the optimizer after it has completed a run from the bottom of the tree to the top.
         */
        reset() {
            // do nothing
        }
        optimizeNextFromLeaves(node) {
            if (node instanceof SourceNode) {
                return false;
            }
            const next = node.parent;
            const { continueFlag } = this.run(node);
            if (continueFlag) {
                this.optimizeNextFromLeaves(next);
            }
            return this.mutatedFlag;
        }
    }
    /**
     * The optimizer function( the "run" method), is invoked on the given node and then continues recursively.
     */
    class TopDownOptimizer extends OptimizerBase {
    }

    function addDimension(dims, channel, fieldDef, model) {
        if (isTypedFieldDef(fieldDef) && isBinning(fieldDef.bin)) {
            dims.add(vgField(fieldDef, {}));
            dims.add(vgField(fieldDef, { binSuffix: 'end' }));
            if (binRequiresRange(fieldDef, channel)) {
                dims.add(vgField(fieldDef, { binSuffix: 'range' }));
            }
        }
        else if (isGeoPositionChannel(channel)) {
            const posChannel = getPositionChannelFromLatLong(channel);
            dims.add(model.getName(posChannel));
        }
        else {
            dims.add(vgField(fieldDef));
        }
        return dims;
    }
    function mergeMeasures(parentMeasures, childMeasures) {
        for (const field of keys(childMeasures)) {
            // when we merge a measure, we either have to add an aggregation operator or even a new field
            const ops = childMeasures[field];
            for (const op of keys(ops)) {
                if (field in parentMeasures) {
                    // add operator to existing measure field
                    parentMeasures[field][op] = new Set([...(parentMeasures[field][op] || []), ...ops[op]]);
                }
                else {
                    parentMeasures[field] = { [op]: ops[op] };
                }
            }
        }
    }
    class AggregateNode extends DataFlowNode {
        /**
         * @param dimensions string set for dimensions
         * @param measures dictionary mapping field name => dict of aggregation functions and names to use
         */
        constructor(parent, dimensions, measures) {
            super(parent);
            this.dimensions = dimensions;
            this.measures = measures;
        }
        clone() {
            return new AggregateNode(null, new Set(this.dimensions), duplicate(this.measures));
        }
        get groupBy() {
            return this.dimensions;
        }
        static makeFromEncoding(parent, model) {
            let isAggregate = false;
            model.forEachFieldDef(fd => {
                if (fd.aggregate) {
                    isAggregate = true;
                }
            });
            const meas = {};
            const dims = new Set();
            if (!isAggregate) {
                // no need to create this node if the model has no aggregation
                return null;
            }
            model.forEachFieldDef((fieldDef, channel) => {
                const { aggregate, field } = fieldDef;
                if (aggregate) {
                    if (aggregate === 'count') {
                        meas['*'] = meas['*'] || {};
                        meas['*']['count'] = new Set([vgField(fieldDef, { forAs: true })]);
                    }
                    else {
                        if (isArgminDef(aggregate) || isArgmaxDef(aggregate)) {
                            const op = isArgminDef(aggregate) ? 'argmin' : 'argmax';
                            const argField = aggregate[op];
                            meas[argField] = meas[argField] || {};
                            meas[argField][op] = new Set([vgField({ op, field: argField }, { forAs: true })]);
                        }
                        else {
                            meas[field] = meas[field] || {};
                            meas[field][aggregate] = new Set([vgField(fieldDef, { forAs: true })]);
                        }
                        // For scale channel with domain === 'unaggregated', add min/max so we can use their union as unaggregated domain
                        if (isScaleChannel(channel) && model.scaleDomain(channel) === 'unaggregated') {
                            meas[field] = meas[field] || {};
                            meas[field]['min'] = new Set([vgField({ field, aggregate: 'min' }, { forAs: true })]);
                            meas[field]['max'] = new Set([vgField({ field, aggregate: 'max' }, { forAs: true })]);
                        }
                    }
                }
                else {
                    addDimension(dims, channel, fieldDef, model);
                }
            });
            if (dims.size + keys(meas).length === 0) {
                return null;
            }
            return new AggregateNode(parent, dims, meas);
        }
        static makeFromTransform(parent, t) {
            const dims = new Set();
            const meas = {};
            for (const s of t.aggregate) {
                const { op, field, as } = s;
                if (op) {
                    if (op === 'count') {
                        meas['*'] = meas['*'] || {};
                        meas['*']['count'] = new Set([as ? as : vgField(s, { forAs: true })]);
                    }
                    else {
                        meas[field] = meas[field] || {};
                        meas[field][op] = new Set([as ? as : vgField(s, { forAs: true })]);
                    }
                }
            }
            for (const s of t.groupby || []) {
                dims.add(s);
            }
            if (dims.size + keys(meas).length === 0) {
                return null;
            }
            return new AggregateNode(parent, dims, meas);
        }
        merge(other) {
            if (setEqual(this.dimensions, other.dimensions)) {
                mergeMeasures(this.measures, other.measures);
                return true;
            }
            else {
                debug('different dimensions, cannot merge');
                return false;
            }
        }
        addDimensions(fields) {
            fields.forEach(this.dimensions.add, this.dimensions);
        }
        dependentFields() {
            return new Set([...this.dimensions, ...keys(this.measures)]);
        }
        producedFields() {
            const out = new Set();
            for (const field of keys(this.measures)) {
                for (const op of keys(this.measures[field])) {
                    const m = this.measures[field][op];
                    if (m.size === 0) {
                        out.add(`${op}_${field}`);
                    }
                    else {
                        m.forEach(out.add, out);
                    }
                }
            }
            return out;
        }
        hash() {
            return `Aggregate ${hash({ dimensions: this.dimensions, measures: this.measures })}`;
        }
        assemble() {
            const ops = [];
            const fields = [];
            const as = [];
            for (const field of keys(this.measures)) {
                for (const op of keys(this.measures[field])) {
                    for (const alias of this.measures[field][op]) {
                        as.push(alias);
                        ops.push(op);
                        fields.push(field === '*' ? null : replacePathInField(field));
                    }
                }
            }
            const result = {
                type: 'aggregate',
                groupby: [...this.dimensions],
                ops,
                fields,
                as
            };
            return result;
        }
    }

    function rangeFormula(model, fieldDef, channel, config) {
        if (binRequiresRange(fieldDef, channel)) {
            // read format from axis or legend, if there is no format then use config.numberFormat
            const guide = isUnitModel(model) ? model.axis(channel) || model.legend(channel) || {} : {};
            const startField = vgField(fieldDef, { expr: 'datum' });
            const endField = vgField(fieldDef, { expr: 'datum', binSuffix: 'end' });
            return {
                formulaAs: vgField(fieldDef, { binSuffix: 'range', forAs: true }),
                formula: binFormatExpression(startField, endField, guide.format, config)
            };
        }
        return {};
    }
    function binKey(bin, field) {
        return `${binToString(bin)}_${field}`;
    }
    function getSignalsFromModel(model, key) {
        return {
            signal: model.getName(`${key}_bins`),
            extentSignal: model.getName(`${key}_extent`)
        };
    }
    function isBinTransform(t) {
        return 'as' in t;
    }
    function createBinComponent(t, bin, model) {
        let as;
        if (isBinTransform(t)) {
            as = isString(t.as) ? [t.as, `${t.as}_end`] : [t.as[0], t.as[1]];
        }
        else {
            as = [vgField(t, { forAs: true }), vgField(t, { binSuffix: 'end', forAs: true })];
        }
        const normalizedBin = normalizeBin(bin, undefined) || {};
        const key = binKey(normalizedBin, t.field);
        const { signal, extentSignal } = getSignalsFromModel(model, key);
        const binComponent = Object.assign({ bin: normalizedBin, field: t.field, as: [as] }, (signal ? { signal } : {}), (extentSignal ? { extentSignal } : {}));
        return { key, binComponent };
    }
    class BinNode extends DataFlowNode {
        constructor(parent, bins) {
            super(parent);
            this.bins = bins;
        }
        clone() {
            return new BinNode(null, duplicate(this.bins));
        }
        static makeFromEncoding(parent, model) {
            const bins = model.reduceFieldDef((binComponentIndex, fieldDef, channel) => {
                if (isTypedFieldDef(fieldDef) && isBinning(fieldDef.bin)) {
                    const { key, binComponent } = createBinComponent(fieldDef, fieldDef.bin, model);
                    binComponentIndex[key] = Object.assign({}, binComponent, binComponentIndex[key], rangeFormula(model, fieldDef, channel, model.config));
                }
                return binComponentIndex;
            }, {});
            if (keys(bins).length === 0) {
                return null;
            }
            return new BinNode(parent, bins);
        }
        /**
         * Creates a bin node from BinTransform.
         * The optional parameter should provide
         */
        static makeFromTransform(parent, t, model) {
            const { key, binComponent } = createBinComponent(t, t.bin, model);
            return new BinNode(parent, {
                [key]: binComponent
            });
        }
        /**
         * Merge bin nodes. This method either integrates the bin config from the other node
         * or if this node already has a bin config, renames the corresponding signal in the model.
         */
        merge(other, renameSignal) {
            for (const key of keys(other.bins)) {
                if (key in this.bins) {
                    renameSignal(other.bins[key].signal, this.bins[key].signal);
                    // Ensure that we don't have duplicate names for signal pairs
                    this.bins[key].as = unique([...this.bins[key].as, ...other.bins[key].as], hash);
                }
                else {
                    this.bins[key] = other.bins[key];
                }
            }
            for (const child of other.children) {
                other.removeChild(child);
                child.parent = this;
            }
            other.remove();
        }
        producedFields() {
            return new Set(flatten(flatten(vals(this.bins).map(c => c.as))));
        }
        dependentFields() {
            return new Set(vals(this.bins).map(c => c.field));
        }
        hash() {
            return `Bin ${hash(this.bins)}`;
        }
        assemble() {
            return flatten(vals(this.bins).map(bin => {
                const transform = [];
                const [binAs, ...remainingAs] = bin.as;
                const binTrans = Object.assign({ type: 'bin', field: bin.field, as: binAs, signal: bin.signal }, bin.bin);
                if (!bin.bin.extent && bin.extentSignal) {
                    transform.push({
                        type: 'extent',
                        field: bin.field,
                        signal: bin.extentSignal
                    });
                    binTrans.extent = { signal: bin.extentSignal };
                }
                transform.push(binTrans);
                for (const as of remainingAs) {
                    for (let i = 0; i < 2; i++) {
                        transform.push({
                            type: 'formula',
                            expr: vgField({ field: binAs[i] }, { expr: 'datum' }),
                            as: as[i]
                        });
                    }
                }
                if (bin.formula) {
                    transform.push({
                        type: 'formula',
                        expr: bin.formula,
                        as: bin.formulaAs
                    });
                }
                return transform;
            }));
        }
    }

    /**
     * A node that helps us track what fields we are faceting by.
     */
    class FacetNode extends DataFlowNode {
        /**
         * @param model The facet model.
         * @param name The name that this facet source will have.
         * @param data The source data for this facet data.
         */
        constructor(parent, model, name, data) {
            super(parent);
            this.model = model;
            this.name = name;
            this.data = data;
            for (const channel of FACET_CHANNELS) {
                const fieldDef = model.facet[channel];
                if (fieldDef) {
                    const { bin, sort } = fieldDef;
                    this[channel] = Object.assign({ name: model.getName(`${channel}_domain`), fields: [vgField(fieldDef), ...(isBinning(bin) ? [vgField(fieldDef, { binSuffix: 'end' })] : [])] }, (isSortField(sort)
                        ? { sortField: sort }
                        : isArray(sort)
                            ? { sortIndexField: sortArrayIndexField(fieldDef, channel) }
                            : {}));
                }
            }
            this.childModel = model.child;
        }
        hash() {
            let out = `Facet`;
            for (const channel of FACET_CHANNELS) {
                if (this[channel]) {
                    out += ` ${channel.charAt(0)}:${hash(this[channel])}`;
                }
            }
            return out;
        }
        get fields() {
            const f = [];
            for (const channel of FACET_CHANNELS) {
                if (this[channel] && this[channel].fields) {
                    f.push(...this[channel].fields);
                }
            }
            return f;
        }
        /**
         * The name to reference this source is its name.
         */
        getSource() {
            return this.name;
        }
        getChildIndependentFieldsWithStep() {
            const childIndependentFieldsWithStep = {};
            for (const channel of ['x', 'y']) {
                const childScaleComponent = this.childModel.component.scales[channel];
                if (childScaleComponent && !childScaleComponent.merged) {
                    // independent scale
                    const type = childScaleComponent.get('type');
                    const range = childScaleComponent.get('range');
                    if (hasDiscreteDomain(type) && isVgRangeStep(range)) {
                        const domain = assembleDomain(this.childModel, channel);
                        const field = getFieldFromDomain(domain);
                        if (field) {
                            childIndependentFieldsWithStep[channel] = field;
                        }
                        else {
                            warn('Unknown field for ${channel}.  Cannot calculate view size.');
                        }
                    }
                }
            }
            return childIndependentFieldsWithStep;
        }
        assembleRowColumnHeaderData(channel, crossedDataName, childIndependentFieldsWithStep) {
            const childChannel = { row: 'y', column: 'x' }[channel];
            const fields = [];
            const ops = [];
            const as = [];
            if (childIndependentFieldsWithStep && childIndependentFieldsWithStep[childChannel]) {
                if (crossedDataName) {
                    // If there is a crossed data, calculate max
                    fields.push(`distinct_${childIndependentFieldsWithStep[childChannel]}`);
                    ops.push('max');
                }
                else {
                    // If there is no crossed data, just calculate distinct
                    fields.push(childIndependentFieldsWithStep[childChannel]);
                    ops.push('distinct');
                }
                // Although it is technically a max, just name it distinct so it's easier to refer to it
                as.push(`distinct_${childIndependentFieldsWithStep[childChannel]}`);
            }
            const { sortField, sortIndexField } = this[channel];
            if (sortField) {
                const { op = DEFAULT_SORT_OP, field } = sortField;
                fields.push(field);
                ops.push(op);
                as.push(vgField(sortField, { forAs: true }));
            }
            else if (sortIndexField) {
                fields.push(sortIndexField);
                ops.push('max');
                as.push(sortIndexField);
            }
            return {
                name: this[channel].name,
                // Use data from the crossed one if it exist
                source: crossedDataName || this.data,
                transform: [
                    Object.assign({ type: 'aggregate', groupby: this[channel].fields }, (fields.length
                        ? {
                            fields,
                            ops,
                            as
                        }
                        : {}))
                ]
            };
        }
        assembleFacetHeaderData(childIndependentFieldsWithStep) {
            const { columns } = this.model.layout;
            const { layoutHeaders } = this.model.component;
            const data = [];
            const hasSharedAxis = {};
            for (const headerChannel of HEADER_CHANNELS) {
                for (const headerType of HEADER_TYPES) {
                    const headers = (layoutHeaders[headerChannel] && layoutHeaders[headerChannel][headerType]) || [];
                    for (const header of headers) {
                        if (header.axes && header.axes.length > 0) {
                            hasSharedAxis[headerChannel] = true;
                            break;
                        }
                    }
                }
                if (hasSharedAxis[headerChannel]) {
                    const cardinality = `length(data("${this.facet.name}"))`;
                    const stop = headerChannel === 'row'
                        ? columns
                            ? { signal: `ceil(${cardinality} / ${columns})` }
                            : 1
                        : columns
                            ? { signal: `min(${cardinality}, ${columns})` }
                            : { signal: cardinality };
                    data.push({
                        name: `${this.facet.name}_${headerChannel}`,
                        transform: [
                            {
                                type: 'sequence',
                                start: 0,
                                stop
                            }
                        ]
                    });
                }
            }
            const { row, column } = hasSharedAxis;
            if (row || column) {
                data.unshift(this.assembleRowColumnHeaderData('facet', null, childIndependentFieldsWithStep));
            }
            return data;
        }
        assemble() {
            const data = [];
            let crossedDataName = null;
            const childIndependentFieldsWithStep = this.getChildIndependentFieldsWithStep();
            const { column, row, facet } = this;
            if (column && row && (childIndependentFieldsWithStep.x || childIndependentFieldsWithStep.y)) {
                // Need to create a cross dataset to correctly calculate cardinality
                crossedDataName = `cross_${this.column.name}_${this.row.name}`;
                const fields = [].concat(childIndependentFieldsWithStep.x || [], childIndependentFieldsWithStep.y || []);
                const ops = fields.map(() => 'distinct');
                data.push({
                    name: crossedDataName,
                    source: this.data,
                    transform: [
                        {
                            type: 'aggregate',
                            groupby: this.fields,
                            fields,
                            ops
                        }
                    ]
                });
            }
            for (const channel of [COLUMN, ROW]) {
                if (this[channel]) {
                    data.push(this.assembleRowColumnHeaderData(channel, crossedDataName, childIndependentFieldsWithStep));
                }
            }
            if (facet) {
                const facetData = this.assembleFacetHeaderData(childIndependentFieldsWithStep);
                if (facetData) {
                    data.push(...facetData);
                }
            }
            return data;
        }
    }

    class FilterNode extends DataFlowNode {
        constructor(parent, model, filter) {
            super(parent);
            this.model = model;
            this.filter = filter;
            // TODO: refactor this to not take a node and
            // then add a static function makeFromOperand and make the constructor take only an expression
            this.expr = expression(this.model, this.filter, this);
            this._dependentFields = getDependentFields(this.expr);
        }
        clone() {
            return new FilterNode(null, this.model, duplicate(this.filter));
        }
        dependentFields() {
            return this._dependentFields;
        }
        assemble() {
            return {
                type: 'filter',
                expr: this.expr
            };
        }
        hash() {
            return `Filter ${this.expr}`;
        }
    }

    /**
     * Remove quotes from a string.
     */
    function unquote(pattern) {
        if ((pattern[0] === "'" && pattern[pattern.length - 1] === "'") ||
            (pattern[0] === '"' && pattern[pattern.length - 1] === '"')) {
            return pattern.slice(1, -1);
        }
        return pattern;
    }
    /**
     * @param field The field.
     * @param parse What to parse the field as.
     */
    function parseExpression$1(field, parse) {
        const f = accessPathWithDatum(field);
        if (parse === 'number') {
            return `toNumber(${f})`;
        }
        else if (parse === 'boolean') {
            return `toBoolean(${f})`;
        }
        else if (parse === 'string') {
            return `toString(${f})`;
        }
        else if (parse === 'date') {
            return `toDate(${f})`;
        }
        else if (parse === 'flatten') {
            return f;
        }
        else if (parse.indexOf('date:') === 0) {
            const specifier = unquote(parse.slice(5, parse.length));
            return `timeParse(${f},'${specifier}')`;
        }
        else if (parse.indexOf('utc:') === 0) {
            const specifier = unquote(parse.slice(4, parse.length));
            return `utcParse(${f},'${specifier}')`;
        }
        else {
            warn(message.unrecognizedParse(parse));
            return null;
        }
    }
    class ParseNode extends DataFlowNode {
        clone() {
            return new ParseNode(null, duplicate(this._parse));
        }
        constructor(parent, parse) {
            super(parent);
            this._parse = parse;
        }
        hash() {
            return `Parse ${hash(this._parse)}`;
        }
        /**
         * Creates a parse node from a data.format.parse and updates ancestorParse.
         */
        static makeExplicit(parent, model, ancestorParse) {
            // Custom parse
            let explicit = {};
            const data = model.data;
            if (!isGenerator(data) && data && data.format && data.format.parse) {
                explicit = data.format.parse;
            }
            return this.makeWithAncestors(parent, explicit, {}, ancestorParse);
        }
        static makeImplicitFromFilterTransform(parent, transform, ancestorParse) {
            const parse = {};
            forEachLeaf(transform.filter, filter => {
                if (isFieldPredicate(filter)) {
                    // Automatically add a parse node for filters with filter objects
                    let val = null;
                    // For EqualFilter, just use the equal property.
                    // For RangeFilter and OneOfFilter, all array members should have
                    // the same type, so we only use the first one.
                    if (isFieldEqualPredicate(filter)) {
                        val = filter.equal;
                    }
                    else if (isFieldRangePredicate(filter)) {
                        val = filter.range[0];
                    }
                    else if (isFieldOneOfPredicate(filter)) {
                        val = (filter.oneOf || filter['in'])[0];
                    } // else -- for filter expression, we can't infer anything
                    if (val) {
                        if (isDateTime(val)) {
                            parse[filter.field] = 'date';
                        }
                        else if (isNumber(val)) {
                            parse[filter.field] = 'number';
                        }
                        else if (isString(val)) {
                            parse[filter.field] = 'string';
                        }
                    }
                    if (filter.timeUnit) {
                        parse[filter.field] = 'date';
                    }
                }
            });
            if (keys(parse).length === 0) {
                return null;
            }
            return this.makeWithAncestors(parent, {}, parse, ancestorParse);
        }
        /**
         * Creates a parse node for implicit parsing from a model and updates ancestorParse.
         */
        static makeImplicitFromEncoding(parent, model, ancestorParse) {
            const implicit = {};
            function add(fieldDef) {
                if (isTimeFormatFieldDef(fieldDef)) {
                    implicit[fieldDef.field] = 'date';
                }
                else if (isNumberFieldDef(fieldDef) && isMinMaxOp(fieldDef.aggregate)) {
                    implicit[fieldDef.field] = 'number';
                }
                else if (accessPathDepth(fieldDef.field) > 1) {
                    // For non-date/non-number (strings and booleans), derive a flattened field for a referenced nested field.
                    // (Parsing numbers / dates already flattens numeric and temporal fields.)
                    if (!(fieldDef.field in implicit)) {
                        implicit[fieldDef.field] = 'flatten';
                    }
                }
                else if (isScaleFieldDef(fieldDef) && isSortField(fieldDef.sort) && accessPathDepth(fieldDef.sort.field) > 1) {
                    // Flatten fields that we sort by but that are not otherwise flattened.
                    if (!(fieldDef.sort.field in implicit)) {
                        implicit[fieldDef.sort.field] = 'flatten';
                    }
                }
            }
            if (isUnitModel(model) || isFacetModel(model)) {
                // Parse encoded fields
                model.forEachFieldDef((fieldDef, channel) => {
                    if (isTypedFieldDef(fieldDef)) {
                        add(fieldDef);
                    }
                    else {
                        const mainChannel = getMainRangeChannel(channel);
                        if (mainChannel !== channel) {
                            const mainFieldDef = model.fieldDef(mainChannel);
                            add(Object.assign({}, fieldDef, { type: mainFieldDef.type }));
                        }
                        else {
                            throw new Error(`Non-secondary channel ${channel} must have type in its field definition ${JSON.stringify(fieldDef)}`);
                        }
                    }
                });
            }
            return this.makeWithAncestors(parent, {}, implicit, ancestorParse);
        }
        /**
         * Creates a parse node from "explicit" parse and "implicit" parse and updates ancestorParse.
         */
        static makeWithAncestors(parent, explicit, implicit, ancestorParse) {
            // We should not parse what has already been parsed in a parent (explicitly or implicitly) or what has been derived (maked as "derived"). We also don't need to flatten a field that has already been parsed.
            for (const field of keys(implicit)) {
                const parsedAs = ancestorParse.getWithExplicit(field);
                if (parsedAs.value !== undefined) {
                    // We always ignore derived fields even if they are implicitly defined because we expect users to create the right types.
                    if (parsedAs.explicit ||
                        parsedAs.value === implicit[field] ||
                        parsedAs.value === 'derived' ||
                        implicit[field] === 'flatten') {
                        delete implicit[field];
                    }
                    else {
                        warn(message.differentParse(field, implicit[field], parsedAs.value));
                    }
                }
            }
            for (const field of keys(explicit)) {
                const parsedAs = ancestorParse.get(field);
                if (parsedAs !== undefined) {
                    // Don't parse a field again if it has been parsed with the same type already.
                    if (parsedAs === explicit[field]) {
                        delete explicit[field];
                    }
                    else {
                        warn(message.differentParse(field, explicit[field], parsedAs));
                    }
                }
            }
            const parse = new Split(explicit, implicit);
            // add the format parse from this model so that children don't parse the same field again
            ancestorParse.copyAll(parse);
            // copy only non-null parses
            const p = {};
            for (const key of keys(parse.combine())) {
                const val = parse.get(key);
                if (val !== null) {
                    p[key] = val;
                }
            }
            if (keys(p).length === 0 || ancestorParse.parseNothing) {
                return null;
            }
            return new ParseNode(parent, p);
        }
        get parse() {
            return this._parse;
        }
        merge(other) {
            this._parse = Object.assign({}, this._parse, other.parse);
            other.remove();
        }
        /**
         * Assemble an object for Vega's format.parse property.
         */
        assembleFormatParse() {
            const formatParse = {};
            for (const field of keys(this._parse)) {
                const p = this._parse[field];
                if (accessPathDepth(field) === 1) {
                    formatParse[field] = p;
                }
            }
            return formatParse;
        }
        // format parse depends and produces all fields in its parse
        producedFields() {
            return new Set(keys(this._parse));
        }
        dependentFields() {
            return new Set(keys(this._parse));
        }
        assembleTransforms(onlyNested = false) {
            return keys(this._parse)
                .filter(field => (onlyNested ? accessPathDepth(field) > 1 : true))
                .map(field => {
                const expr = parseExpression$1(field, this._parse[field]);
                if (!expr) {
                    return null;
                }
                const formula = {
                    type: 'formula',
                    expr,
                    as: removePathFromField(field) // Vega output is always flattened
                };
                return formula;
            })
                .filter(t => t !== null);
        }
    }

    /**
     * A class for the join aggregate transform nodes.
     */
    class JoinAggregateTransformNode extends DataFlowNode {
        constructor(parent, transform) {
            super(parent);
            this.transform = transform;
        }
        clone() {
            return new JoinAggregateTransformNode(null, duplicate(this.transform));
        }
        addDimensions(fields) {
            this.transform.groupby = unique(this.transform.groupby.concat(fields), d => d);
        }
        dependentFields() {
            const out = new Set();
            this.transform.groupby.forEach(f => out.add(f));
            this.transform.joinaggregate
                .map(w => w.field)
                .filter(f => f !== undefined)
                .forEach(f => out.add(f));
            return out;
        }
        producedFields() {
            return new Set(this.transform.joinaggregate.map(this.getDefaultName));
        }
        getDefaultName(joinAggregateFieldDef) {
            return joinAggregateFieldDef.as || vgField(joinAggregateFieldDef);
        }
        hash() {
            return `JoinAggregateTransform ${hash(this.transform)}`;
        }
        assemble() {
            const fields = [];
            const ops = [];
            const as = [];
            for (const joinaggregate of this.transform.joinaggregate) {
                ops.push(joinaggregate.op);
                as.push(this.getDefaultName(joinaggregate));
                fields.push(joinaggregate.field === undefined ? null : joinaggregate.field);
            }
            const groupby = this.transform.groupby;
            return Object.assign({ type: 'joinaggregate', as,
                ops,
                fields }, (groupby !== undefined ? { groupby } : {}));
        }
    }

    function getStackByFields(model) {
        return model.stack.stackBy.reduce((fields, by) => {
            const fieldDef = by.fieldDef;
            const _field = vgField(fieldDef);
            if (_field) {
                fields.push(_field);
            }
            return fields;
        }, []);
    }
    function isValidAsArray(as) {
        return isArray(as) && as.every(s => isString(s)) && as.length > 1;
    }
    class StackNode extends DataFlowNode {
        clone() {
            return new StackNode(null, duplicate(this._stack));
        }
        constructor(parent, stack) {
            super(parent);
            this._stack = stack;
        }
        static makeFromTransform(parent, stackTransform) {
            const { stack, groupby, as, offset = 'zero' } = stackTransform;
            const sortFields = [];
            const sortOrder = [];
            if (stackTransform.sort !== undefined) {
                for (const sortField of stackTransform.sort) {
                    sortFields.push(sortField.field);
                    sortOrder.push(getFirstDefined(sortField.order, 'ascending'));
                }
            }
            const sort = {
                field: sortFields,
                order: sortOrder
            };
            let normalizedAs;
            if (isValidAsArray(as)) {
                normalizedAs = as;
            }
            else if (isString(as)) {
                normalizedAs = [as, as + '_end'];
            }
            else {
                normalizedAs = [stackTransform.stack + '_start', stackTransform.stack + '_end'];
            }
            return new StackNode(parent, {
                stackField: stack,
                groupby,
                offset,
                sort,
                facetby: [],
                as: normalizedAs
            });
        }
        static makeFromEncoding(parent, model) {
            const stackProperties = model.stack;
            const { encoding } = model;
            if (!stackProperties) {
                return null;
            }
            let dimensionFieldDef;
            if (stackProperties.groupbyChannel) {
                const cDef = encoding[stackProperties.groupbyChannel];
                dimensionFieldDef = getTypedFieldDef(cDef);
            }
            const stackby = getStackByFields(model);
            const orderDef = model.encoding.order;
            let sort;
            if (isArray(orderDef) || isFieldDef(orderDef)) {
                sort = sortParams(orderDef);
            }
            else {
                // default = descending by stackFields
                // FIXME is the default here correct for binned fields?
                sort = stackby.reduce((s, field) => {
                    s.field.push(field);
                    s.order.push('descending');
                    return s;
                }, { field: [], order: [] });
            }
            return new StackNode(parent, {
                dimensionFieldDef,
                stackField: model.vgField(stackProperties.fieldChannel),
                facetby: [],
                stackby,
                sort,
                offset: stackProperties.offset,
                impute: stackProperties.impute,
                as: [
                    model.vgField(stackProperties.fieldChannel, { suffix: 'start', forAs: true }),
                    model.vgField(stackProperties.fieldChannel, { suffix: 'end', forAs: true })
                ]
            });
        }
        get stack() {
            return this._stack;
        }
        addDimensions(fields) {
            this._stack.facetby.push(...fields);
        }
        dependentFields() {
            const out = new Set();
            out.add(this._stack.stackField);
            this.getGroupbyFields().forEach(out.add);
            this._stack.facetby.forEach(out.add);
            const field = this._stack.sort.field;
            isArray(field) ? field.forEach(out.add) : out.add(field);
            return out;
        }
        producedFields() {
            return new Set(this._stack.as);
        }
        hash() {
            return `Stack ${hash(this._stack)}`;
        }
        getGroupbyFields() {
            const { dimensionFieldDef, impute, groupby } = this._stack;
            if (dimensionFieldDef) {
                if (dimensionFieldDef.bin) {
                    if (impute) {
                        // For binned group by field with impute, we calculate bin_mid
                        // as we cannot impute two fields simultaneously
                        return [vgField(dimensionFieldDef, { binSuffix: 'mid' })];
                    }
                    return [
                        // For binned group by field without impute, we need both bin (start) and bin_end
                        vgField(dimensionFieldDef, {}),
                        vgField(dimensionFieldDef, { binSuffix: 'end' })
                    ];
                }
                return [vgField(dimensionFieldDef)];
            }
            return groupby || [];
        }
        assemble() {
            const transform = [];
            const { facetby, dimensionFieldDef, stackField: field, stackby, sort, offset, impute, as } = this._stack;
            // Impute
            if (impute && dimensionFieldDef) {
                if (dimensionFieldDef.bin) {
                    // As we can only impute one field at a time, we need to calculate
                    // mid point for a binned field
                    transform.push({
                        type: 'formula',
                        expr: '(' +
                            vgField(dimensionFieldDef, { expr: 'datum' }) +
                            '+' +
                            vgField(dimensionFieldDef, { expr: 'datum', binSuffix: 'end' }) +
                            ')/2',
                        as: vgField(dimensionFieldDef, { binSuffix: 'mid', forAs: true })
                    });
                }
                transform.push({
                    type: 'impute',
                    field,
                    groupby: [...stackby, ...facetby],
                    key: vgField(dimensionFieldDef, { binSuffix: 'mid' }),
                    method: 'value',
                    value: 0
                });
            }
            // Stack
            transform.push({
                type: 'stack',
                groupby: [...this.getGroupbyFields(), ...facetby],
                field,
                sort,
                as,
                offset
            });
            return transform;
        }
    }

    /**
     * A class for the window transform nodes
     */
    class WindowTransformNode extends DataFlowNode {
        constructor(parent, transform) {
            super(parent);
            this.transform = transform;
        }
        clone() {
            return new WindowTransformNode(null, duplicate(this.transform));
        }
        addDimensions(fields) {
            this.transform.groupby = unique(this.transform.groupby.concat(fields), d => d);
        }
        dependentFields() {
            const out = new Set();
            this.transform.groupby.forEach(f => out.add(f));
            this.transform.sort.forEach(m => out.add(m.field));
            this.transform.window
                .map(w => w.field)
                .filter(f => f !== undefined)
                .forEach(f => out.add(f));
            return out;
        }
        producedFields() {
            return new Set(this.transform.window.map(this.getDefaultName));
        }
        getDefaultName(windowFieldDef) {
            return windowFieldDef.as || vgField(windowFieldDef);
        }
        hash() {
            return `WindowTransform ${hash(this.transform)}`;
        }
        assemble() {
            const fields = [];
            const ops = [];
            const as = [];
            const params = [];
            for (const window of this.transform.window) {
                ops.push(window.op);
                as.push(this.getDefaultName(window));
                params.push(window.param === undefined ? null : window.param);
                fields.push(window.field === undefined ? null : window.field);
            }
            const frame = this.transform.frame;
            const groupby = this.transform.groupby;
            if (frame && frame[0] === null && frame[1] === null && ops.every(o => isAggregateOp(o))) {
                // when the window does not rely on any particular window ops or frame, switch to a simpler and more efficient joinaggregate
                return Object.assign({ type: 'joinaggregate', as, ops: ops, fields }, (groupby !== undefined ? { groupby } : {}));
            }
            const sortFields = [];
            const sortOrder = [];
            if (this.transform.sort !== undefined) {
                for (const sortField of this.transform.sort) {
                    sortFields.push(sortField.field);
                    sortOrder.push(sortField.order || 'ascending');
                }
            }
            const sort = {
                field: sortFields,
                order: sortOrder
            };
            const ignorePeers = this.transform.ignorePeers;
            return Object.assign({ type: 'window', params,
                as,
                ops,
                fields,
                sort }, (ignorePeers !== undefined ? { ignorePeers } : {}), (groupby !== undefined ? { groupby } : {}), (frame !== undefined ? { frame } : {}));
        }
    }

    /**
     * Move parse nodes up to forks.
     */
    class MoveParseUp extends BottomUpOptimizer {
        run(node) {
            const parent = node.parent;
            // move parse up by merging or swapping
            if (node instanceof ParseNode) {
                if (parent instanceof SourceNode) {
                    return this.flags;
                }
                if (parent.numChildren() > 1) {
                    // don't move parse further up but continue with parent.
                    this.setContinue();
                    return this.flags;
                }
                if (parent instanceof ParseNode) {
                    this.setMutated();
                    parent.merge(node);
                }
                else {
                    // don't swap with nodes that produce something that the parse node depends on (e.g. lookup)
                    if (fieldIntersection(parent.producedFields(), node.dependentFields())) {
                        this.setContinue();
                        return this.flags;
                    }
                    this.setMutated();
                    node.swapWithParent();
                }
            }
            this.setContinue();
            return this.flags;
        }
    }
    /**
     * Merge identical nodes at forks by comparing hashes.
     *
     * Does not need to iterate from leaves so we implement this with recursion as it's a bit simpler.
     */
    class MergeIdenticalNodes extends TopDownOptimizer {
        mergeNodes(parent, nodes) {
            const mergedNode = nodes.shift();
            for (const node of nodes) {
                parent.removeChild(node);
                node.parent = mergedNode;
                node.remove();
            }
        }
        run(node) {
            const hashes = node.children.map(x => x.hash());
            const buckets = {};
            for (let i = 0; i < hashes.length; i++) {
                if (buckets[hashes[i]] === undefined) {
                    buckets[hashes[i]] = [node.children[i]];
                }
                else {
                    buckets[hashes[i]].push(node.children[i]);
                }
            }
            for (const k of keys(buckets)) {
                if (buckets[k].length > 1) {
                    this.setMutated();
                    this.mergeNodes(node, buckets[k]);
                }
            }
            for (const child of node.children) {
                this.run(child);
            }
            return this.mutatedFlag;
        }
    }
    /**
     * Repeatedly remove leaf nodes that are not output or facet nodes.
     * The reason is that we don't need subtrees that don't have any output nodes.
     * Facet nodes are needed for the row or column domains.
     */
    class RemoveUnusedSubtrees extends BottomUpOptimizer {
        run(node) {
            if (node instanceof OutputNode || node.numChildren() > 0 || node instanceof FacetNode) {
                // no need to continue with parent because it is output node or will have children (there was a fork)
                return this.flags;
            }
            else {
                this.setMutated();
                node.remove();
            }
            return this.flags;
        }
    }
    /**
     * Removes duplicate time unit nodes (as determined by the name of the
     * output field) that may be generated due to selections projected over
     * time units.
     */
    class RemoveDuplicateTimeUnits extends BottomUpOptimizer {
        constructor() {
            super(...arguments);
            this.fields = new Set();
        }
        run(node) {
            this.setContinue();
            if (node instanceof TimeUnitNode) {
                const pfields = node.producedFields();
                if (hasIntersection(pfields, this.fields)) {
                    this.setMutated();
                    node.remove();
                }
                else {
                    this.fields = new Set([...this.fields, ...pfields]);
                }
            }
            return this.flags;
        }
        reset() {
            this.fields.clear();
        }
    }
    /**
     * Clones the subtree and ignores output nodes except for the leaves, which are renamed.
     */
    function cloneSubtree(facet) {
        function clone(node) {
            if (!(node instanceof FacetNode)) {
                const copy = node.clone();
                if (copy instanceof OutputNode) {
                    const newName = FACET_SCALE_PREFIX + copy.getSource();
                    copy.setSource(newName);
                    facet.model.component.data.outputNodes[newName] = copy;
                }
                else if (copy instanceof AggregateNode ||
                    copy instanceof StackNode ||
                    copy instanceof WindowTransformNode ||
                    copy instanceof JoinAggregateTransformNode) {
                    copy.addDimensions(facet.fields);
                }
                flatten(node.children.map(clone)).forEach((n) => (n.parent = copy));
                return [copy];
            }
            return flatten(node.children.map(clone));
        }
        return clone;
    }
    /**
     * Move facet nodes down to the next fork or output node. Also pull the main output with the facet node.
     * After moving down the facet node, make a copy of the subtree and make it a child of the main output.
     */
    function moveFacetDown(node) {
        if (node instanceof FacetNode) {
            if (node.numChildren() === 1 && !(node.children[0] instanceof OutputNode)) {
                // move down until we hit a fork or output node
                const child = node.children[0];
                if (child instanceof AggregateNode ||
                    child instanceof StackNode ||
                    child instanceof WindowTransformNode ||
                    child instanceof JoinAggregateTransformNode) {
                    child.addDimensions(node.fields);
                }
                child.swapWithParent();
                moveFacetDown(node);
            }
            else {
                // move main to facet
                const facetMain = node.model.component.data.main;
                moveMainDownToFacet(facetMain);
                // replicate the subtree and place it before the facet's main node
                const cloner = cloneSubtree(node);
                const copy = flatten(node.children.map(cloner));
                for (const c of copy) {
                    c.parent = facetMain;
                }
            }
        }
        else {
            node.children.map(moveFacetDown);
        }
    }
    function moveMainDownToFacet(node) {
        if (node instanceof OutputNode && node.type === MAIN) {
            if (node.numChildren() === 1) {
                const child = node.children[0];
                if (!(child instanceof FacetNode)) {
                    child.swapWithParent();
                    moveMainDownToFacet(node);
                }
            }
        }
    }
    /**
     * Remove nodes that are not required starting from a root.
     */
    class RemoveUnnecessaryNodes extends TopDownOptimizer {
        run(node) {
            // remove output nodes that are not required
            if (node instanceof OutputNode && !node.isRequired()) {
                this.setMutated();
                node.remove();
            }
            for (const child of node.children) {
                this.run(child);
            }
            return this.mutatedFlag;
        }
    }
    /**
     * Inserts an Intermediate ParseNode containing all non-conflicting Parse fields and removes the empty ParseNodes
     */
    class MergeParse extends BottomUpOptimizer {
        run(node) {
            const parent = node.parent;
            const parseChildren = parent.children.filter((x) => x instanceof ParseNode);
            if (parseChildren.length > 1) {
                const commonParse = {};
                for (const parseNode of parseChildren) {
                    const parse = parseNode.parse;
                    for (const k of keys(parse)) {
                        if (commonParse[k] === undefined) {
                            commonParse[k] = parse[k];
                        }
                        else if (commonParse[k] !== parse[k]) {
                            delete commonParse[k];
                        }
                    }
                }
                if (keys(commonParse).length !== 0) {
                    this.setMutated();
                    const mergedParseNode = new ParseNode(parent, commonParse);
                    for (const parseNode of parseChildren) {
                        for (const key of keys(commonParse)) {
                            delete parseNode.parse[key];
                        }
                        parent.removeChild(parseNode);
                        parseNode.parent = mergedParseNode;
                        if (keys(parseNode.parse).length === 0) {
                            parseNode.remove();
                        }
                    }
                }
            }
            this.setContinue();
            return this.flags;
        }
    }
    class MergeAggregateNodes extends BottomUpOptimizer {
        run(node) {
            const parent = node.parent;
            const aggChildren = parent.children.filter((x) => x instanceof AggregateNode);
            // Object which we'll use to map the fields which an aggregate is grouped by to
            // the set of aggregates with that grouping. This is useful as only aggregates
            // with the same group by can be merged
            const groupedAggregates = {};
            // Build groupedAggregates
            for (const agg of aggChildren) {
                const groupBys = hash(keys(agg.groupBy).sort());
                if (!(groupBys in groupedAggregates)) {
                    groupedAggregates[groupBys] = [];
                }
                groupedAggregates[groupBys].push(agg);
            }
            // Merge aggregateNodes with same key in groupedAggregates
            for (const group of keys(groupedAggregates)) {
                const mergeableAggs = groupedAggregates[group];
                if (mergeableAggs.length > 1) {
                    const mergedAggs = mergeableAggs.pop();
                    for (const agg of mergeableAggs) {
                        if (mergedAggs.merge(agg)) {
                            parent.removeChild(agg);
                            agg.parent = mergedAggs;
                            agg.remove();
                            this.setMutated();
                        }
                    }
                }
            }
            this.setContinue();
            return this.flags;
        }
    }
    /**
     * Merge bin nodes and move bin nodes up through forks but stop at filters and parse as we want them to stay before the bin node.
     */
    class MergeBins extends BottomUpOptimizer {
        constructor(model) {
            super();
            this.model = model;
        }
        run(node) {
            const parent = node.parent;
            const moveBinsUp = !(parent instanceof SourceNode || parent instanceof FilterNode || parent instanceof ParseNode);
            const promotableBins = [];
            const remainingBins = [];
            for (const child of parent.children) {
                if (child instanceof BinNode) {
                    if (moveBinsUp && !fieldIntersection(parent.producedFields(), child.dependentFields())) {
                        promotableBins.push(child);
                    }
                    else {
                        remainingBins.push(child);
                    }
                }
            }
            if (promotableBins.length > 0) {
                const promotedBin = promotableBins.pop();
                for (const bin of promotableBins) {
                    promotedBin.merge(bin, this.model.renameSignal.bind(this.model));
                }
                this.setMutated();
                if (parent instanceof BinNode) {
                    parent.merge(promotedBin, this.model.renameSignal.bind(this.model));
                }
                else {
                    promotedBin.swapWithParent();
                }
            }
            if (remainingBins.length > 1) {
                const remainingBin = remainingBins.pop();
                for (const bin of remainingBins) {
                    remainingBin.merge(bin, this.model.renameSignal.bind(this.model));
                }
                this.setMutated();
            }
            this.setContinue();
            return this.flags;
        }
    }

    const FACET_SCALE_PREFIX = 'scale_';
    const MAX_OPTIMIZATION_RUNS = 5;
    /**
     * Return all leaf nodes.
     */
    function getLeaves(roots) {
        const leaves = [];
        function append(node) {
            if (node.numChildren() === 0) {
                leaves.push(node);
            }
            else {
                node.children.forEach(append);
            }
        }
        roots.forEach(append);
        return leaves;
    }
    function isTrue(x) {
        return x;
    }
    /**
     * Run the specified optimizer on the provided nodes.
     *
     * @param optimizer The optimizer instance to run.
     * @param nodes A set of nodes to optimize.
     * @param flag Flag that will be or'ed with return valued from optimization calls to the nodes.
     */
    function runOptimizer(optimizer, nodes, flag) {
        const flags = nodes.map(node => {
            if (optimizer instanceof BottomUpOptimizer) {
                const runFlags = optimizer.optimizeNextFromLeaves(node);
                optimizer.reset();
                return runFlags;
            }
            else {
                return optimizer.run(node);
            }
        });
        return flags.some(isTrue) || flag;
    }
    function optimizationDataflowHelper(dataComponent, model) {
        let roots = dataComponent.sources;
        let mutatedFlag = false;
        // mutatedFlag should always be on the right side otherwise short circuit logic might cause the mutating method to not execute
        mutatedFlag = runOptimizer(new RemoveUnnecessaryNodes(), roots, mutatedFlag);
        // remove source nodes that don't have any children because they also don't have output nodes
        roots = roots.filter(r => r.numChildren() > 0);
        mutatedFlag = runOptimizer(new RemoveUnusedSubtrees(), getLeaves(roots), mutatedFlag);
        roots = roots.filter(r => r.numChildren() > 0);
        mutatedFlag = runOptimizer(new MoveParseUp(), getLeaves(roots), mutatedFlag);
        mutatedFlag = runOptimizer(new MergeBins(model), getLeaves(roots), mutatedFlag);
        mutatedFlag = runOptimizer(new RemoveDuplicateTimeUnits(), getLeaves(roots), mutatedFlag);
        mutatedFlag = runOptimizer(new MergeParse(), getLeaves(roots), mutatedFlag);
        mutatedFlag = runOptimizer(new MergeAggregateNodes(), getLeaves(roots), mutatedFlag);
        mutatedFlag = runOptimizer(new MergeIdenticalNodes(), roots, mutatedFlag);
        dataComponent.sources = roots;
        return mutatedFlag;
    }
    /**
     * Optimizes the dataflow of the passed in data component.
     */
    function optimizeDataflow(data, model) {
        // check before optimizations
        checkLinks(data.sources);
        let firstPassCounter = 0;
        let secondPassCounter = 0;
        for (let i = 0; i < MAX_OPTIMIZATION_RUNS; i++) {
            if (!optimizationDataflowHelper(data, model)) {
                break;
            }
            firstPassCounter++;
        }
        // move facets down and make a copy of the subtree so that we can have scales at the top level
        data.sources.map(moveFacetDown);
        for (let i = 0; i < MAX_OPTIMIZATION_RUNS; i++) {
            if (!optimizationDataflowHelper(data, model)) {
                break;
            }
            secondPassCounter++;
        }
        // check after optimizations
        checkLinks(data.sources);
        if (Math.max(firstPassCounter, secondPassCounter) === MAX_OPTIMIZATION_RUNS) {
            warn(`Maximum optimization runs(${MAX_OPTIMIZATION_RUNS}) reached.`);
        }
    }

    /**
     * A class that behaves like a SignalRef but lazily generates the signal.
     * The provided generator function should use `Model.getSignalName` to use the correct signal name.
     */
    class SignalRefWrapper {
        constructor(exprGenerator) {
            Object.defineProperty(this, 'signal', {
                enumerable: true,
                get: exprGenerator
            });
        }
        static fromName(rename, signalName) {
            return new SignalRefWrapper(() => rename(signalName));
        }
    }

    function parseScaleDomain(model) {
        if (isUnitModel(model)) {
            parseUnitScaleDomain(model);
        }
        else {
            parseNonUnitScaleDomain(model);
        }
    }
    function parseUnitScaleDomain(model) {
        const scales = model.specifiedScales;
        const localScaleComponents = model.component.scales;
        keys(localScaleComponents).forEach((channel) => {
            const specifiedScale = scales[channel];
            const specifiedDomain = specifiedScale ? specifiedScale.domain : undefined;
            const domains = parseDomainForChannel(model, channel);
            const localScaleCmpt = localScaleComponents[channel];
            localScaleCmpt.domains = domains;
            if (isSelectionDomain(specifiedDomain)) {
                // As scale parsing occurs before selection parsing, we use a temporary
                // signal here and append the scale.domain definition. This is replaced
                // with the correct domainRaw signal during scale assembly.
                // For more information, see isRawSelectionDomain in selection.ts.
                // FIXME: replace this with a special property in the scaleComponent
                localScaleCmpt.set('domainRaw', {
                    signal: SELECTION_DOMAIN + hash(specifiedDomain)
                }, true);
            }
            if (model.component.data.isFaceted) {
                // get resolve from closest facet parent as this decides whether we need to refer to cloned subtree or not
                let facetParent = model;
                while (!isFacetModel(facetParent) && facetParent.parent) {
                    facetParent = facetParent.parent;
                }
                const resolve = facetParent.component.resolve.scale[channel];
                if (resolve === 'shared') {
                    for (const domain of domains) {
                        // Replace the scale domain with data output from a cloned subtree after the facet.
                        if (isDataRefDomain(domain)) {
                            // use data from cloned subtree (which is the same as data but with a prefix added once)
                            domain.data = FACET_SCALE_PREFIX + domain.data.replace(FACET_SCALE_PREFIX, '');
                        }
                    }
                }
            }
        });
    }
    function parseNonUnitScaleDomain(model) {
        for (const child of model.children) {
            parseScaleDomain(child);
        }
        const localScaleComponents = model.component.scales;
        keys(localScaleComponents).forEach((channel) => {
            let domains;
            let domainRaw = null;
            for (const child of model.children) {
                const childComponent = child.component.scales[channel];
                if (childComponent) {
                    if (domains === undefined) {
                        domains = childComponent.domains;
                    }
                    else {
                        domains = domains.concat(childComponent.domains);
                    }
                    const dr = childComponent.get('domainRaw');
                    if (domainRaw && dr && domainRaw.signal !== dr.signal) {
                        warn('The same selection must be used to override scale domains in a layered view.');
                    }
                    domainRaw = dr;
                }
            }
            localScaleComponents[channel].domains = domains;
            if (domainRaw) {
                localScaleComponents[channel].set('domainRaw', domainRaw, true);
            }
        });
    }
    /**
     * Remove unaggregated domain if it is not applicable
     * Add unaggregated domain if domain is not specified and config.scale.useUnaggregatedDomain is true.
     */
    function normalizeUnaggregatedDomain(domain, fieldDef, scaleType, scaleConfig) {
        if (domain === 'unaggregated') {
            const { valid, reason } = canUseUnaggregatedDomain(fieldDef, scaleType);
            if (!valid) {
                warn(reason);
                return undefined;
            }
        }
        else if (domain === undefined && scaleConfig.useUnaggregatedDomain) {
            // Apply config if domain is not specified.
            const { valid } = canUseUnaggregatedDomain(fieldDef, scaleType);
            if (valid) {
                return 'unaggregated';
            }
        }
        return domain;
    }
    function parseDomainForChannel(model, channel) {
        const scaleType = model.getScaleComponent(channel).get('type');
        const domain = normalizeUnaggregatedDomain(model.scaleDomain(channel), model.fieldDef(channel), scaleType, model.config.scale);
        if (domain !== model.scaleDomain(channel)) {
            model.specifiedScales[channel] = Object.assign({}, model.specifiedScales[channel], { domain });
        }
        // If channel is either X or Y then union them with X2 & Y2 if they exist
        if (channel === 'x' && model.channelHasField('x2')) {
            if (model.channelHasField('x')) {
                return parseSingleChannelDomain(scaleType, domain, model, 'x').concat(parseSingleChannelDomain(scaleType, domain, model, 'x2'));
            }
            else {
                return parseSingleChannelDomain(scaleType, domain, model, 'x2');
            }
        }
        else if (channel === 'y' && model.channelHasField('y2')) {
            if (model.channelHasField('y')) {
                return parseSingleChannelDomain(scaleType, domain, model, 'y').concat(parseSingleChannelDomain(scaleType, domain, model, 'y2'));
            }
            else {
                return parseSingleChannelDomain(scaleType, domain, model, 'y2');
            }
        }
        return parseSingleChannelDomain(scaleType, domain, model, channel);
    }
    function mapDomainToDataSignal(domain, type, timeUnit) {
        return domain.map(v => {
            const data = valueExpr(v, { timeUnit, type });
            return { signal: `{data: ${data}}` };
        });
    }
    function parseSingleChannelDomain(scaleType, domain, model, channel) {
        const fieldDef = model.fieldDef(channel);
        if (domain && domain !== 'unaggregated' && !isSelectionDomain(domain)) {
            // explicit value
            const { type, timeUnit } = fieldDef;
            if (type === 'temporal' || timeUnit) {
                return mapDomainToDataSignal(domain, type, timeUnit);
            }
            return [domain];
        }
        const stack = model.stack;
        if (stack && channel === stack.fieldChannel) {
            if (stack.offset === 'normalize') {
                return [[0, 1]];
            }
            const data = model.requestDataName(MAIN);
            return [
                {
                    data,
                    field: model.vgField(channel, { suffix: 'start' })
                },
                {
                    data,
                    field: model.vgField(channel, { suffix: 'end' })
                }
            ];
        }
        const sort = isScaleChannel(channel)
            ? domainSort(model, channel, scaleType)
            : undefined;
        if (domain === 'unaggregated') {
            const data = model.requestDataName(MAIN);
            const { field } = fieldDef;
            return [
                {
                    data,
                    field: vgField({ field, aggregate: 'min' })
                },
                {
                    data,
                    field: vgField({ field, aggregate: 'max' })
                }
            ];
        }
        else if (isBinning(fieldDef.bin)) {
            if (hasDiscreteDomain(scaleType)) {
                if (scaleType === 'bin-ordinal') {
                    // we can omit the domain as it is inferred from the `bins` property
                    return [];
                }
                // ordinal bin scale takes domain from bin_range, ordered by bin start
                // This is useful for both axis-based scale (x/y) and legend-based scale (other channels).
                return [
                    {
                        // If sort by aggregation of a specified sort field, we need to use RAW table,
                        // so we can aggregate values for the scale independently from the main aggregation.
                        data: isBoolean$1(sort) ? model.requestDataName(MAIN) : model.requestDataName(RAW),
                        // Use range if we added it and the scale does not support computing a range as a signal.
                        field: model.vgField(channel, binRequiresRange(fieldDef, channel) ? { binSuffix: 'range' } : {}),
                        // we have to use a sort object if sort = true to make the sort correct by bin start
                        sort: sort === true || !isObject(sort)
                            ? {
                                field: model.vgField(channel, {}),
                                op: 'min' // min or max doesn't matter since we sort by the start of the bin range
                            }
                            : sort
                    }
                ];
            }
            else {
                // continuous scales
                if (isBinning(fieldDef.bin)) {
                    const signalName = model.getName(vgField(fieldDef, { suffix: 'bins' }));
                    return [
                        new SignalRefWrapper(() => {
                            const signal = model.getSignalName(signalName);
                            return `[${signal}.start, ${signal}.stop]`;
                        })
                    ];
                }
                else {
                    return [
                        {
                            data: model.requestDataName(MAIN),
                            field: model.vgField(channel, {})
                        }
                    ];
                }
            }
        }
        else if (sort) {
            return [
                {
                    // If sort by aggregation of a specified sort field, we need to use RAW table,
                    // so we can aggregate values for the scale independently from the main aggregation.
                    data: isBoolean$1(sort) ? model.requestDataName(MAIN) : model.requestDataName(RAW),
                    field: model.vgField(channel),
                    sort: sort
                }
            ];
        }
        else {
            return [
                {
                    data: model.requestDataName(MAIN),
                    field: model.vgField(channel)
                }
            ];
        }
    }
    function normalizeSortField(sort, isStacked) {
        const { op, field, order } = sort;
        return Object.assign({ 
            // Apply default op
            op: op || (isStacked ? 'sum' : DEFAULT_SORT_OP) }, (field ? { field: replacePathInField(field) } : {}), (order ? { order } : {}));
    }
    function domainSort(model, channel, scaleType) {
        if (!hasDiscreteDomain(scaleType)) {
            return undefined;
        }
        // save to cast as the only exception is the geojson type for shape, which would not generate a scale
        const fieldDef = model.fieldDef(channel);
        const sort = fieldDef.sort;
        // if the sort is specified with array, use the derived sort index field
        if (isSortArray(sort)) {
            return {
                op: 'min',
                field: sortArrayIndexField(fieldDef, channel),
                order: 'ascending'
            };
        }
        const isStacked = model.stack !== null;
        // Sorted based on an aggregate calculation over a specified sort field (only for ordinal scale)
        if (isSortField(sort)) {
            return normalizeSortField(sort, isStacked);
        }
        else if (isSortByEncoding(sort)) {
            const { encoding, order } = sort;
            const { aggregate, field } = model.fieldDef(encoding);
            const sortField = {
                op: aggregate,
                field,
                order
            };
            return normalizeSortField(sortField, isStacked);
        }
        else if (sort === 'descending') {
            return {
                op: 'min',
                field: model.vgField(channel),
                order: 'descending'
            };
        }
        else if (contains(['ascending', undefined /* default =ascending*/], sort)) {
            return true;
        }
        // sort == null
        return undefined;
    }
    /**
     * Determine if a scale can use unaggregated domain.
     * @return {Boolean} Returns true if all of the following conditions apply:
     * 1. `scale.domain` is `unaggregated`
     * 2. Aggregation function is not `count` or `sum`
     * 3. The scale is quantitative or time scale.
     */
    function canUseUnaggregatedDomain(fieldDef, scaleType) {
        const { aggregate, type } = fieldDef;
        if (!aggregate) {
            return {
                valid: false,
                reason: message.unaggregateDomainHasNoEffectForRawField(fieldDef)
            };
        }
        if (isString(aggregate) && !SHARED_DOMAIN_OP_INDEX[aggregate]) {
            return {
                valid: false,
                reason: message.unaggregateDomainWithNonSharedDomainOp(aggregate)
            };
        }
        if (type === 'quantitative') {
            if (scaleType === 'log') {
                return {
                    valid: false,
                    reason: message.unaggregatedDomainWithLogScale(fieldDef)
                };
            }
        }
        return { valid: true };
    }
    /**
     * Converts an array of domains to a single Vega scale domain.
     */
    function mergeDomains(domains) {
        const uniqueDomains = unique(domains.map(domain => {
            // ignore sort property when computing the unique domains
            if (isDataRefDomain(domain)) {
                const domainWithoutSort = __rest(domain, ["sort"]);
                return domainWithoutSort;
            }
            return domain;
        }), hash);
        const sorts = unique(domains
            .map(d => {
            if (isDataRefDomain(d)) {
                const s = d.sort;
                if (s !== undefined && !isBoolean$1(s)) {
                    if (s.op === 'count') {
                        // let's make sure that if op is count, we don't use a field
                        delete s.field;
                    }
                    if (s.order === 'ascending') {
                        // drop order: ascending as it is the default
                        delete s.order;
                    }
                }
                return s;
            }
            return undefined;
        })
            .filter(s => s !== undefined), hash);
        if (uniqueDomains.length === 0) {
            return undefined;
        }
        else if (uniqueDomains.length === 1) {
            const domain = domains[0];
            if (isDataRefDomain(domain) && sorts.length > 0) {
                let sort = sorts[0];
                if (sorts.length > 1) {
                    warn(message.MORE_THAN_ONE_SORT);
                    sort = true;
                }
                return Object.assign({}, domain, { sort });
            }
            return domain;
        }
        // only keep simple sort properties that work with unioned domains
        const simpleSorts = unique(sorts.map(s => {
            if (isBoolean$1(s)) {
                return s;
            }
            if (s.op === 'count') {
                return s;
            }
            warn(message.domainSortDropped(s));
            return true;
        }), hash);
        let sort;
        if (simpleSorts.length === 1) {
            sort = simpleSorts[0];
        }
        else if (simpleSorts.length > 1) {
            warn(message.MORE_THAN_ONE_SORT);
            sort = true;
        }
        const allData = unique(domains.map(d => {
            if (isDataRefDomain(d)) {
                return d.data;
            }
            return null;
        }), x => x);
        if (allData.length === 1 && allData[0] !== null) {
            // create a union domain of different fields with a single data source
            const domain = Object.assign({ data: allData[0], fields: uniqueDomains.map(d => d.field) }, (sort ? { sort } : {}));
            return domain;
        }
        return Object.assign({ fields: uniqueDomains }, (sort ? { sort } : {}));
    }
    /**
     * Return a field if a scale single field.
     * Return `undefined` otherwise.
     *
     */
    function getFieldFromDomain(domain) {
        if (isDataRefDomain(domain) && isString(domain.field)) {
            return domain.field;
        }
        else if (isDataRefUnionedDomain(domain)) {
            let field;
            for (const nonUnionDomain of domain.fields) {
                if (isDataRefDomain(nonUnionDomain) && isString(nonUnionDomain.field)) {
                    if (!field) {
                        field = nonUnionDomain.field;
                    }
                    else if (field !== nonUnionDomain.field) {
                        warn('Detected faceted independent scales that union domain of multiple fields from different data sources.  We will use the first field.  The result view size may be incorrect.');
                        return field;
                    }
                }
            }
            warn('Detected faceted independent scales that union domain of identical fields from different source detected.  We will assume that this is the same field from a different fork of the same data source.  However, if this is not case, the result view size maybe incorrect.');
            return field;
        }
        else if (isFieldRefUnionDomain(domain)) {
            warn('Detected faceted independent scales that union domain of multiple fields from the same data source.  We will use the first field.  The result view size may be incorrect.');
            const field = domain.fields[0];
            return isString(field) ? field : undefined;
        }
        return undefined;
    }
    function assembleDomain(model, channel) {
        const scaleComponent = model.component.scales[channel];
        const domains = scaleComponent.domains.map(domain => {
            // Correct references to data as the original domain's data was determined
            // in parseScale, which happens before parseData. Thus the original data
            // reference can be incorrect.
            if (isDataRefDomain(domain)) {
                domain.data = model.lookupDataSource(domain.data);
            }
            return domain;
        });
        // domains is an array that has to be merged into a single vega domain
        return mergeDomains(domains);
    }

    function assembleScales(model) {
        if (isLayerModel(model) || isConcatModel(model) || isRepeatModel(model)) {
            // For concat / layer / repeat, include scales of children too
            return model.children.reduce((scales, child) => {
                return scales.concat(assembleScales(child));
            }, assembleScalesForModel(model));
        }
        else {
            // For facet, child scales would not be included in the parent's scope.
            // For unit, there is no child.
            return assembleScalesForModel(model);
        }
    }
    function assembleScalesForModel(model) {
        return keys(model.component.scales).reduce((scales, channel) => {
            const scaleComponent = model.component.scales[channel];
            if (scaleComponent.merged) {
                // Skipped merged scales
                return scales;
            }
            const scale = scaleComponent.combine();
            // need to separate const and non const object destruction
            let { domainRaw } = scale;
            const { name, type, domainRaw: _d, range: _r } = scale, otherScaleProps = __rest(scale, ["name", "type", "domainRaw", "range"]);
            const range = assembleScaleRange(scale.range, name, channel);
            // As scale parsing occurs before selection parsing, a temporary signal
            // is used for domainRaw. Here, we detect if this temporary signal
            // is set, and replace it with the correct domainRaw signal.
            // For more information, see isRawSelectionDomain in selection.ts.
            if (domainRaw && isRawSelectionDomain(domainRaw)) {
                domainRaw = assembleSelectionScaleDomain(model, domainRaw);
            }
            const domain = assembleDomain(model, channel);
            scales.push(Object.assign({ name,
                type }, (domain ? { domain } : {}), (domainRaw ? { domainRaw } : {}), { range: range }, otherScaleProps));
            return scales;
        }, []);
    }
    function assembleScaleRange(scaleRange, scaleName, channel) {
        // add signals to x/y range
        if (channel === 'x' || channel === 'y') {
            if (isVgRangeStep(scaleRange)) {
                // For x/y range step, use a signal created in layout assemble instead of a constant range step.
                return {
                    step: { signal: scaleName + '_step' }
                };
            }
        }
        return scaleRange;
    }

    class ScaleComponent extends Split {
        constructor(name, typeWithExplicit) {
            super({}, // no initial explicit property
            { name } // name as initial implicit property
            );
            this.merged = false;
            this.domains = [];
            this.setWithExplicit('type', typeWithExplicit);
        }
        /**
         * Whether the scale definitely includes zero in the domain
         */
        get domainDefinitelyIncludesZero() {
            if (this.get('zero') !== false) {
                return true;
            }
            const domains = this.domains;
            if (isArray(domains)) {
                return some(domains, d => isArray(d) && d.length === 2 && d[0] <= 0 && d[1] >= 0);
            }
            return false;
        }
    }

    const RANGE_PROPERTIES = ['range', 'rangeStep', 'scheme'];
    function getSizeType(channel) {
        return channel === 'x' ? 'width' : channel === 'y' ? 'height' : undefined;
    }
    function parseUnitScaleRange(model) {
        const localScaleComponents = model.component.scales;
        // use SCALE_CHANNELS instead of scales[channel] to ensure that x, y come first!
        SCALE_CHANNELS.forEach((channel) => {
            const localScaleCmpt = localScaleComponents[channel];
            if (!localScaleCmpt) {
                return;
            }
            const mergedScaleCmpt = model.getScaleComponent(channel);
            const specifiedScale = model.specifiedScales[channel];
            const fieldDef = model.fieldDef(channel);
            // Read if there is a specified width/height
            const sizeType = getSizeType(channel);
            let sizeSpecified = sizeType ? !!model.component.layoutSize.get(sizeType) : undefined;
            const scaleType = mergedScaleCmpt.get('type');
            // if autosize is fit, size cannot be data driven
            const rangeStep = contains(['point', 'band'], scaleType) || !!specifiedScale.rangeStep;
            if (sizeType && model.fit && !sizeSpecified && rangeStep) {
                warn(message.CANNOT_FIX_RANGE_STEP_WITH_FIT);
                sizeSpecified = true;
            }
            const xyRangeSteps = getXYRangeStep(model);
            const rangeWithExplicit = parseRangeForChannel(channel, model.getSignalName.bind(model), scaleType, fieldDef.type, specifiedScale, model.config, localScaleCmpt.get('zero'), model.mark, sizeSpecified, model.getName(sizeType), xyRangeSteps);
            localScaleCmpt.setWithExplicit('range', rangeWithExplicit);
        });
    }
    function getRangeStep(model, channel) {
        const scaleCmpt = model.getScaleComponent(channel);
        if (!scaleCmpt) {
            return undefined;
        }
        const scaleType = scaleCmpt.get('type');
        const fieldDef = model.fieldDef(channel);
        if (hasDiscreteDomain(scaleType)) {
            const range = scaleCmpt && scaleCmpt.get('range');
            if (range && isVgRangeStep(range) && isNumber(range.step)) {
                return range.step;
                // TODO: support the case without range step
            }
        }
        else if (fieldDef && fieldDef.bin && isBinning(fieldDef.bin)) {
            const binSignal = model.getName(vgField(fieldDef, { suffix: 'bins' }));
            // TODO: extract this to be range step signal
            const sizeType = getSizeType(channel);
            const sizeSignal = model.getName(sizeType);
            return new SignalRefWrapper(() => {
                const updatedName = model.getSignalName(binSignal);
                const binCount = `(${updatedName}.stop - ${updatedName}.start) / ${updatedName}.step`;
                return `${model.getSignalName(sizeSignal)} / (${binCount})`;
            });
        }
        return undefined;
    }
    function getXYRangeStep(model) {
        const steps = [];
        for (const channel of POSITION_SCALE_CHANNELS) {
            const step = getRangeStep(model, channel);
            if (step !== undefined) {
                steps.push(step);
            }
        }
        return steps;
    }
    /**
     * Return mixins that includes one of the range properties (range, rangeStep, scheme).
     */
    function parseRangeForChannel(channel, getSignalName, scaleType, type, specifiedScale, config, zero, mark, sizeSpecified, sizeSignal, xyRangeSteps) {
        const noRangeStep = sizeSpecified || specifiedScale.rangeStep === null;
        // Check if any of the range properties is specified.
        // If so, check if it is compatible and make sure that we only output one of the properties
        for (const property of RANGE_PROPERTIES) {
            if (specifiedScale[property] !== undefined) {
                const supportedByScaleType = scaleTypeSupportProperty(scaleType, property);
                const channelIncompatability = channelScalePropertyIncompatability(channel, property);
                if (!supportedByScaleType) {
                    warn(message.scalePropertyNotWorkWithScaleType(scaleType, property, channel));
                }
                else if (channelIncompatability) {
                    // channel
                    warn(channelIncompatability);
                }
                else {
                    switch (property) {
                        case 'range':
                            return makeExplicit(specifiedScale[property]);
                        case 'scheme':
                            return makeExplicit(parseScheme(specifiedScale[property]));
                        case 'rangeStep': {
                            const rangeStep = specifiedScale[property];
                            if (rangeStep !== null) {
                                if (!sizeSpecified) {
                                    return makeExplicit({ step: rangeStep });
                                }
                                else {
                                    // If top-level size is specified, we ignore specified rangeStep.
                                    warn(message.rangeStepDropped(channel));
                                }
                            }
                        }
                    }
                }
            }
        }
        return makeImplicit(defaultRange(channel, getSignalName, scaleType, type, config, zero, mark, sizeSignal, xyRangeSteps, noRangeStep, specifiedScale.domain));
    }
    function parseScheme(scheme) {
        if (isExtendedScheme(scheme)) {
            return Object.assign({ scheme: scheme.name }, omit(scheme, ['name']));
        }
        return { scheme: scheme };
    }
    function defaultRange(channel, getSignalName, scaleType, type, config, zero, mark, sizeSignal, xyRangeSteps, noRangeStep, domain) {
        switch (channel) {
            case X:
            case Y:
                if (contains(['point', 'band'], scaleType) && !noRangeStep) {
                    if (channel === X && mark === 'text') {
                        if (config.scale.textXRangeStep) {
                            return { step: config.scale.textXRangeStep };
                        }
                    }
                    else {
                        if (config.scale.rangeStep) {
                            return { step: config.scale.rangeStep };
                        }
                    }
                }
                // If range step is null, use zero to width or height.
                // Note that these range signals are temporary
                // as they can be merged and renamed.
                // (We do not have the right size signal here since parseLayoutSize() happens after parseScale().)
                // We will later replace these temporary names with
                // the final name in assembleScaleRange()
                if (channel === Y && hasContinuousDomain(scaleType)) {
                    // For y continuous scale, we have to start from the height as the bottom part has the max value.
                    return [SignalRefWrapper.fromName(getSignalName, sizeSignal), 0];
                }
                else {
                    return [0, SignalRefWrapper.fromName(getSignalName, sizeSignal)];
                }
            case SIZE: {
                // TODO: support custom rangeMin, rangeMax
                const rangeMin = sizeRangeMin(mark, zero, config);
                const rangeMax = sizeRangeMax(mark, xyRangeSteps, config);
                if (isContinuousToDiscrete(scaleType)) {
                    return interpolateRange(rangeMin, rangeMax, defaultContinuousToDiscreteCount(scaleType, config, domain, channel));
                }
                else {
                    return [rangeMin, rangeMax];
                }
            }
            case STROKEWIDTH:
                // TODO: support custom rangeMin, rangeMax
                return [config.scale.minStrokeWidth, config.scale.maxStrokeWidth];
            case SHAPE:
                return 'symbol';
            case COLOR:
            case FILL:
            case STROKE:
                if (scaleType === 'ordinal') {
                    // Only nominal data uses ordinal scale by default
                    return type === 'nominal' ? 'category' : 'ordinal';
                }
                else {
                    return mark === 'rect' || mark === 'geoshape' ? 'heatmap' : 'ramp';
                }
            case OPACITY:
            case FILLOPACITY:
            case STROKEOPACITY:
                // TODO: support custom rangeMin, rangeMax
                return [config.scale.minOpacity, config.scale.maxOpacity];
        }
        /* istanbul ignore next: should never reach here */
        throw new Error(`Scale range undefined for channel ${channel}`);
    }
    function defaultContinuousToDiscreteCount(scaleType, config, domain, channel) {
        switch (scaleType) {
            case 'quantile':
                return config.scale.quantileCount;
            case 'quantize':
                return config.scale.quantizeCount;
            case 'threshold':
                if (domain !== undefined && isArray(domain)) {
                    return domain.length + 1;
                }
                else {
                    warn(message.domainRequiredForThresholdScale(channel));
                    // default threshold boundaries for threshold scale since domain has cardinality of 2
                    return 3;
                }
        }
    }
    /**
     * Returns the linear interpolation of the range according to the cardinality
     *
     * @param rangeMin start of the range
     * @param rangeMax end of the range
     * @param cardinality number of values in the output range
     */
    function interpolateRange(rangeMin, rangeMax, cardinality) {
        // always return a signal since it's better to compute the sequence in Vega later
        const f = () => {
            const rMax = isSignalRef(rangeMax) ? rangeMax.signal : rangeMax;
            const step = `(${rMax} - ${rangeMin}) / (${cardinality} - 1)`;
            return `sequence(${rangeMin}, ${rangeMax} + ${step}, ${step})`;
        };
        if (isSignalRef(rangeMax)) {
            return new SignalRefWrapper(f);
        }
        else {
            return { signal: f() };
        }
    }
    function sizeRangeMin(mark, zero, config) {
        if (zero) {
            return 0;
        }
        switch (mark) {
            case 'bar':
            case 'tick':
                return config.scale.minBandSize;
            case 'line':
            case 'trail':
            case 'rule':
                return config.scale.minStrokeWidth;
            case 'text':
                return config.scale.minFontSize;
            case 'point':
            case 'square':
            case 'circle':
                return config.scale.minSize;
        }
        /* istanbul ignore next: should never reach here */
        // sizeRangeMin not implemented for the mark
        throw new Error(message.incompatibleChannel('size', mark));
    }
    const MAX_SIZE_RANGE_STEP_RATIO = 0.95;
    function sizeRangeMax(mark, xyRangeSteps, config) {
        const scaleConfig = config.scale;
        switch (mark) {
            case 'bar':
            case 'tick': {
                if (config.scale.maxBandSize !== undefined) {
                    return config.scale.maxBandSize;
                }
                const min = minXYRangeStep(xyRangeSteps, config.scale);
                if (isNumber(min)) {
                    return min - 1;
                }
                else {
                    return new SignalRefWrapper(() => `${min.signal} - 1`);
                }
            }
            case 'line':
            case 'trail':
            case 'rule':
                return config.scale.maxStrokeWidth;
            case 'text':
                return config.scale.maxFontSize;
            case 'point':
            case 'square':
            case 'circle': {
                if (config.scale.maxSize) {
                    return config.scale.maxSize;
                }
                const pointStep = minXYRangeStep(xyRangeSteps, scaleConfig);
                if (isNumber(pointStep)) {
                    return Math.pow(MAX_SIZE_RANGE_STEP_RATIO * pointStep, 2);
                }
                else {
                    return new SignalRefWrapper(() => `pow(${MAX_SIZE_RANGE_STEP_RATIO} * ${pointStep.signal}, 2)`);
                }
            }
        }
        /* istanbul ignore next: should never reach here */
        // sizeRangeMax not implemented for the mark
        throw new Error(message.incompatibleChannel('size', mark));
    }
    /**
     * @returns {number} Range step of x or y or minimum between the two if both are ordinal scale.
     */
    function minXYRangeStep(xyRangeSteps, scaleConfig) {
        if (xyRangeSteps.length > 0) {
            let min = Infinity;
            for (const step of xyRangeSteps) {
                if (isSignalRef(step)) {
                    min = undefined;
                }
                else {
                    if (min !== undefined && step < min) {
                        min = step;
                    }
                }
            }
            return min !== undefined
                ? min
                : new SignalRefWrapper(() => {
                    const exprs = xyRangeSteps.map(e => (isSignalRef(e) ? e.signal : e));
                    return `min(${exprs.join(', ')})`;
                });
        }
        if (scaleConfig.rangeStep) {
            return scaleConfig.rangeStep;
        }
        return 21; // FIXME: re-evaluate the default value here.
    }

    function parseScaleProperty(model, property) {
        if (isUnitModel(model)) {
            parseUnitScaleProperty(model, property);
        }
        else {
            parseNonUnitScaleProperty(model, property);
        }
    }
    function parseUnitScaleProperty(model, property) {
        const localScaleComponents = model.component.scales;
        keys(localScaleComponents).forEach((channel) => {
            const specifiedScale = model.specifiedScales[channel];
            const localScaleCmpt = localScaleComponents[channel];
            const mergedScaleCmpt = model.getScaleComponent(channel);
            const fieldDef = model.fieldDef(channel);
            const config = model.config;
            const specifiedValue = specifiedScale[property];
            const sType = mergedScaleCmpt.get('type');
            const supportedByScaleType = scaleTypeSupportProperty(sType, property);
            const channelIncompatability = channelScalePropertyIncompatability(channel, property);
            if (specifiedValue !== undefined) {
                // If there is a specified value, check if it is compatible with scale type and channel
                if (!supportedByScaleType) {
                    warn(message.scalePropertyNotWorkWithScaleType(sType, property, channel));
                }
                else if (channelIncompatability) {
                    // channel
                    warn(channelIncompatability);
                }
            }
            if (supportedByScaleType && channelIncompatability === undefined) {
                if (specifiedValue !== undefined) {
                    // copyKeyFromObject ensures type safety
                    localScaleCmpt.copyKeyFromObject(property, specifiedScale);
                }
                else {
                    const value = getDefaultValue(property, model, channel, fieldDef, mergedScaleCmpt.get('type'), mergedScaleCmpt.get('padding'), mergedScaleCmpt.get('paddingInner'), specifiedScale.domain, model.markDef, config);
                    if (value !== undefined) {
                        localScaleCmpt.set(property, value, false);
                    }
                }
            }
        });
    }
    // Note: This method is used in Voyager.
    function getDefaultValue(property, model, channel, fieldDef, scaleType, scalePadding, scalePaddingInner, specifiedDomain, markDef, config) {
        const scaleConfig = config.scale;
        const { type, sort } = fieldDef;
        // If we have default rule-base, determine default value first
        switch (property) {
            case 'bins':
                return bins(model, fieldDef);
            case 'interpolate':
                return interpolate(channel, type);
            case 'nice':
                return nice(scaleType, channel, fieldDef);
            case 'padding':
                return padding(channel, scaleType, scaleConfig, fieldDef, markDef, config.bar);
            case 'paddingInner':
                return paddingInner(scalePadding, channel, markDef.type, scaleConfig);
            case 'paddingOuter':
                return paddingOuter(scalePadding, channel, scaleType, markDef.type, scalePaddingInner, scaleConfig);
            case 'reverse':
                return reverse(scaleType, sort);
            case 'zero':
                return zero$1(channel, fieldDef, specifiedDomain, markDef, scaleType);
        }
        // Otherwise, use scale config
        return scaleConfig[property];
    }
    // This method is here rather than in range.ts to avoid circular dependency.
    function parseScaleRange(model) {
        if (isUnitModel(model)) {
            parseUnitScaleRange(model);
        }
        else {
            parseNonUnitScaleProperty(model, 'range');
        }
    }
    function parseNonUnitScaleProperty(model, property) {
        const localScaleComponents = model.component.scales;
        for (const child of model.children) {
            if (property === 'range') {
                parseScaleRange(child);
            }
            else {
                parseScaleProperty(child, property);
            }
        }
        keys(localScaleComponents).forEach((channel) => {
            let valueWithExplicit;
            for (const child of model.children) {
                const childComponent = child.component.scales[channel];
                if (childComponent) {
                    const childValueWithExplicit = childComponent.getWithExplicit(property);
                    valueWithExplicit = mergeValuesWithExplicit(valueWithExplicit, childValueWithExplicit, property, 'scale', tieBreakByComparing((v1, v2) => {
                        switch (property) {
                            case 'range':
                                // For range step, prefer larger step
                                if (v1.step && v2.step) {
                                    return v1.step - v2.step;
                                }
                                return 0;
                            // TODO: precedence rule for other properties
                        }
                        return 0;
                    }));
                }
            }
            localScaleComponents[channel].setWithExplicit(property, valueWithExplicit);
        });
    }
    function bins(model, fieldDef) {
        const bin = fieldDef.bin;
        if (isBinning(bin)) {
            const signal = model.getName(vgField(fieldDef, { suffix: 'bins' }));
            return new SignalRefWrapper(() => {
                return model.getSignalName(signal);
            });
        }
        else if (isBinned(bin) && isBinParams(bin) && bin.step !== undefined) {
            // start and stop will be determined from the scale domain
            return {
                step: bin.step
            };
        }
        return undefined;
    }
    function interpolate(channel, type) {
        if (contains([COLOR, FILL, STROKE], channel) && type !== 'nominal') {
            return 'hcl';
        }
        return undefined;
    }
    function nice(scaleType, channel, fieldDef) {
        if (fieldDef.bin || contains([ScaleType.TIME, ScaleType.UTC], scaleType)) {
            return undefined;
        }
        return contains([X, Y], channel) ? true : undefined;
    }
    function padding(channel, scaleType, scaleConfig, fieldDef, markDef, barConfig) {
        if (contains([X, Y], channel)) {
            if (isContinuousToContinuous(scaleType)) {
                if (scaleConfig.continuousPadding !== undefined) {
                    return scaleConfig.continuousPadding;
                }
                const { type, orient } = markDef;
                if (type === 'bar' && !fieldDef.bin) {
                    if ((orient === 'vertical' && channel === 'x') || (orient === 'horizontal' && channel === 'y')) {
                        return barConfig.continuousBandSize;
                    }
                }
            }
            if (scaleType === ScaleType.POINT) {
                return scaleConfig.pointPadding;
            }
        }
        return undefined;
    }
    function paddingInner(paddingValue, channel, mark, scaleConfig) {
        if (paddingValue !== undefined) {
            // If user has already manually specified "padding", no need to add default paddingInner.
            return undefined;
        }
        if (contains([X, Y], channel)) {
            // Padding is only set for X and Y by default.
            // Basically it doesn't make sense to add padding for color and size.
            // paddingOuter would only be called if it's a band scale, just return the default for bandScale.
            const { bandPaddingInner, barBandPaddingInner, rectBandPaddingInner } = scaleConfig;
            return getFirstDefined(bandPaddingInner, mark === 'bar' ? barBandPaddingInner : rectBandPaddingInner);
        }
        return undefined;
    }
    function paddingOuter(paddingValue, channel, scaleType, mark, paddingInnerValue, scaleConfig) {
        if (paddingValue !== undefined) {
            // If user has already manually specified "padding", no need to add default paddingOuter.
            return undefined;
        }
        if (contains([X, Y], channel)) {
            // Padding is only set for X and Y by default.
            // Basically it doesn't make sense to add padding for color and size.
            if (scaleType === ScaleType.BAND) {
                const { bandPaddingOuter, barBandPaddingOuter, rectBandPaddingOuter } = scaleConfig;
                return getFirstDefined(bandPaddingOuter, mark === 'bar' ? barBandPaddingOuter : rectBandPaddingOuter, 
                /* By default, paddingOuter is paddingInner / 2. The reason is that
                  size (width/height) = step * (cardinality - paddingInner + 2 * paddingOuter).
                  and we want the width/height to be integer by default.
                  Note that step (by default) and cardinality are integers.) */
                paddingInnerValue / 2);
            }
        }
        return undefined;
    }
    function reverse(scaleType, sort) {
        if (hasContinuousDomain(scaleType) && sort === 'descending') {
            // For continuous domain scales, Vega does not support domain sort.
            // Thus, we reverse range instead if sort is descending
            return true;
        }
        return undefined;
    }
    function zero$1(channel, fieldDef, specifiedDomain, markDef, scaleType) {
        // If users explicitly provide a domain range, we should not augment zero as that will be unexpected.
        const hasCustomDomain = !!specifiedDomain && specifiedDomain !== 'unaggregated';
        if (hasCustomDomain) {
            if (hasContinuousDomain(scaleType)) {
                if (isArray(specifiedDomain)) {
                    const first = specifiedDomain[0];
                    const last = specifiedDomain[specifiedDomain.length - 1];
                    if (first <= 0 && last >= 0) {
                        // if the domain includes zero, make zero remains true
                        return true;
                    }
                }
                return false;
            }
        }
        // If there is no custom domain, return true only for the following cases:
        // 1) using quantitative field with size
        // While this can be either ratio or interval fields, our assumption is that
        // ratio are more common. However, if the scaleType is discretizing scale, we want to return
        // false so that range doesn't start at zero
        if (channel === 'size' && fieldDef.type === 'quantitative' && !isContinuousToDiscrete(scaleType)) {
            return true;
        }
        // 2) non-binned, quantitative x-scale or y-scale
        // (For binning, we should not include zero by default because binning are calculated without zero.)
        if (!fieldDef.bin && contains([X, Y], channel)) {
            const { orient, type } = markDef;
            if (contains(['bar', 'area', 'line', 'trail'], type)) {
                if ((orient === 'horizontal' && channel === 'y') || (orient === 'vertical' && channel === 'x')) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /**
     * Determine if there is a specified scale type and if it is appropriate,
     * or determine default type if type is unspecified or inappropriate.
     */
    // NOTE: CompassQL uses this method.
    function scaleType(specifiedScale, channel, fieldDef, mark) {
        const defaultScaleType = defaultType$2(channel, fieldDef, mark);
        const { type } = specifiedScale;
        if (!isScaleChannel(channel)) {
            // There is no scale for these channels
            return null;
        }
        if (type !== undefined) {
            // Check if explicitly specified scale type is supported by the channel
            if (!channelSupportScaleType(channel, type)) {
                warn(message.scaleTypeNotWorkWithChannel(channel, type, defaultScaleType));
                return defaultScaleType;
            }
            // Check if explicitly specified scale type is supported by the data type
            if (!scaleTypeSupportDataType(type, fieldDef.type)) {
                warn(message.scaleTypeNotWorkWithFieldDef(type, defaultScaleType));
                return defaultScaleType;
            }
            return type;
        }
        return defaultScaleType;
    }
    /**
     * Determine appropriate default scale type.
     */
    // NOTE: Voyager uses this method.
    function defaultType$2(channel, fieldDef, mark) {
        switch (fieldDef.type) {
            case 'nominal':
            case 'ordinal':
                if (isColorChannel(channel) || rangeType(channel) === 'discrete') {
                    if (channel === 'shape' && fieldDef.type === 'ordinal') {
                        warn(message.discreteChannelCannotEncode(channel, 'ordinal'));
                    }
                    return 'ordinal';
                }
                if (contains(['x', 'y'], channel)) {
                    if (contains(['rect', 'bar', 'rule'], mark)) {
                        // The rect/bar mark should fit into a band.
                        // For rule, using band scale to make rule align with axis ticks better https://github.com/vega/vega-lite/issues/3429
                        return 'band';
                    }
                    if (mark === 'bar') {
                        return 'band';
                    }
                }
                // Otherwise, use ordinal point scale so we can easily get center positions of the marks.
                return 'point';
            case 'temporal':
                if (isColorChannel(channel)) {
                    return 'time';
                }
                else if (rangeType(channel) === 'discrete') {
                    warn(message.discreteChannelCannotEncode(channel, 'temporal'));
                    // TODO: consider using quantize (equivalent to binning) once we have it
                    return 'ordinal';
                }
                return 'time';
            case 'quantitative':
                if (isColorChannel(channel)) {
                    if (isBinning(fieldDef.bin)) {
                        return 'bin-ordinal';
                    }
                    return 'linear';
                }
                else if (rangeType(channel) === 'discrete') {
                    warn(message.discreteChannelCannotEncode(channel, 'quantitative'));
                    // TODO: consider using quantize (equivalent to binning) once we have it
                    return 'ordinal';
                }
                return 'linear';
            case 'geojson':
                return undefined;
        }
        /* istanbul ignore next: should never reach this */
        throw new Error(message.invalidFieldType(fieldDef.type));
    }

    function parseScales(model) {
        parseScaleCore(model);
        parseScaleDomain(model);
        for (const prop of NON_TYPE_DOMAIN_RANGE_VEGA_SCALE_PROPERTIES) {
            parseScaleProperty(model, prop);
        }
        // range depends on zero
        parseScaleRange(model);
    }
    function parseScaleCore(model) {
        if (isUnitModel(model)) {
            model.component.scales = parseUnitScaleCore(model);
        }
        else {
            model.component.scales = parseNonUnitScaleCore(model);
        }
    }
    /**
     * Parse scales for all channels of a model.
     */
    function parseUnitScaleCore(model) {
        const { encoding, mark } = model;
        return SCALE_CHANNELS.reduce((scaleComponents, channel) => {
            let fieldDef;
            let specifiedScale;
            const channelDef = encoding[channel];
            // Don't generate scale for shape of geoshape
            if (isFieldDef(channelDef) && mark === GEOSHAPE && channel === SHAPE && channelDef.type === GEOJSON) {
                return scaleComponents;
            }
            if (isFieldDef(channelDef)) {
                fieldDef = channelDef;
                specifiedScale = channelDef.scale;
            }
            else if (hasConditionalFieldDef(channelDef)) {
                fieldDef = channelDef.condition;
                specifiedScale = channelDef.condition['scale']; // We use ['scale'] since we know that channel here has scale for sure
            }
            if (fieldDef && specifiedScale !== null && specifiedScale !== false) {
                specifiedScale = specifiedScale || {};
                const sType = scaleType(specifiedScale, channel, fieldDef, mark);
                scaleComponents[channel] = new ScaleComponent(model.scaleName(channel + '', true), {
                    value: sType,
                    explicit: specifiedScale.type === sType
                });
            }
            return scaleComponents;
        }, {});
    }
    const scaleTypeTieBreaker = tieBreakByComparing((st1, st2) => scaleTypePrecedence(st1) - scaleTypePrecedence(st2));
    function parseNonUnitScaleCore(model) {
        const scaleComponents = (model.component.scales = {});
        const scaleTypeWithExplicitIndex = {};
        const resolve = model.component.resolve;
        // Parse each child scale and determine if a particular channel can be merged.
        for (const child of model.children) {
            parseScaleCore(child);
            // Instead of always merging right away -- check if it is compatible to merge first!
            keys(child.component.scales).forEach((channel) => {
                // if resolve is undefined, set default first
                resolve.scale[channel] = resolve.scale[channel] || defaultScaleResolve(channel, model);
                if (resolve.scale[channel] === 'shared') {
                    const explicitScaleType = scaleTypeWithExplicitIndex[channel];
                    const childScaleType = child.component.scales[channel].getWithExplicit('type');
                    if (explicitScaleType) {
                        if (scaleCompatible(explicitScaleType.value, childScaleType.value)) {
                            // merge scale component if type are compatible
                            scaleTypeWithExplicitIndex[channel] = mergeValuesWithExplicit(explicitScaleType, childScaleType, 'type', 'scale', scaleTypeTieBreaker);
                        }
                        else {
                            // Otherwise, update conflicting channel to be independent
                            resolve.scale[channel] = 'independent';
                            // Remove from the index so they don't get merged
                            delete scaleTypeWithExplicitIndex[channel];
                        }
                    }
                    else {
                        scaleTypeWithExplicitIndex[channel] = childScaleType;
                    }
                }
            });
        }
        // Merge each channel listed in the index
        keys(scaleTypeWithExplicitIndex).forEach((channel) => {
            // Create new merged scale component
            const name = model.scaleName(channel, true);
            const typeWithExplicit = scaleTypeWithExplicitIndex[channel];
            scaleComponents[channel] = new ScaleComponent(name, typeWithExplicit);
            // rename each child and mark them as merged
            for (const child of model.children) {
                const childScale = child.component.scales[channel];
                if (childScale) {
                    child.renameScale(childScale.get('name'), name);
                    childScale.merged = true;
                }
            }
        });
        return scaleComponents;
    }

    class NameMap {
        constructor() {
            this.nameMap = {};
        }
        rename(oldName, newName) {
            this.nameMap[oldName] = newName;
        }
        has(name) {
            return this.nameMap[name] !== undefined;
        }
        get(name) {
            // If the name appears in the _nameMap, we need to read its new name.
            // We have to loop over the dict just in case the new name also gets renamed.
            while (this.nameMap[name] && name !== this.nameMap[name]) {
                name = this.nameMap[name];
            }
            return name;
        }
    }
    /*
      We use type guards instead of `instanceof` as `instanceof` makes
      different parts of the compiler depend on the actual implementation of
      the model classes, which in turn depend on different parts of the compiler.
      Thus, `instanceof` leads to circular dependency problems.

      On the other hand, type guards only make different parts of the compiler
      depend on the type of the model classes, but not the actual implementation.
    */
    function isUnitModel(model) {
        return model && model.type === 'unit';
    }
    function isFacetModel(model) {
        return model && model.type === 'facet';
    }
    function isRepeatModel(model) {
        return model && model.type === 'repeat';
    }
    function isConcatModel(model) {
        return model && model.type === 'concat';
    }
    function isLayerModel(model) {
        return model && model.type === 'layer';
    }
    class Model {
        constructor(spec, type, parent, parentGivenName, config, repeater, resolve, view) {
            this.type = type;
            this.parent = parent;
            this.config = config;
            this.repeater = repeater;
            this.view = view;
            this.children = [];
            /**
             * Corrects the data references in marks after assemble.
             */
            this.correctDataNames = (mark) => {
                // TODO: make this correct
                // for normal data references
                if (mark.from && mark.from.data) {
                    mark.from.data = this.lookupDataSource(mark.from.data);
                }
                // for access to facet data
                if (mark.from && mark.from.facet && mark.from.facet.data) {
                    mark.from.facet.data = this.lookupDataSource(mark.from.facet.data);
                }
                return mark;
            };
            this.parent = parent;
            this.config = config;
            this.repeater = repeater;
            // If name is not provided, always use parent's givenName to avoid name conflicts.
            this.name = spec.name || parentGivenName;
            this.title = isString(spec.title) ? { text: spec.title } : spec.title;
            // Shared name maps
            this.scaleNameMap = parent ? parent.scaleNameMap : new NameMap();
            this.projectionNameMap = parent ? parent.projectionNameMap : new NameMap();
            this.signalNameMap = parent ? parent.signalNameMap : new NameMap();
            this.data = spec.data;
            this.description = spec.description;
            this.transforms = normalizeTransform(spec.transform || []);
            this.layout = isUnitSpec(spec) || isLayerSpec(spec) ? {} : extractCompositionLayout(spec, type, config);
            this.component = {
                data: {
                    sources: parent ? parent.component.data.sources : [],
                    outputNodes: parent ? parent.component.data.outputNodes : {},
                    outputNodeRefCounts: parent ? parent.component.data.outputNodeRefCounts : {},
                    // data is faceted if the spec is a facet spec or the parent has faceted data and no data is defined
                    isFaceted: isFacetSpec(spec) || (parent && parent.component.data.isFaceted && !spec.data)
                },
                layoutSize: new Split(),
                layoutHeaders: { row: {}, column: {}, facet: {} },
                mark: null,
                resolve: Object.assign({ scale: {}, axis: {}, legend: {} }, (resolve ? duplicate(resolve) : {})),
                selection: null,
                scales: null,
                projection: null,
                axes: {},
                legends: {}
            };
        }
        get width() {
            return this.getSizeSignalRef('width');
        }
        get height() {
            return this.getSizeSignalRef('height');
        }
        initSize(size) {
            const { width, height } = size;
            if (width) {
                this.component.layoutSize.set('width', width, true);
            }
            if (height) {
                this.component.layoutSize.set('height', height, true);
            }
        }
        parse() {
            this.parseScale();
            this.parseLayoutSize(); // depends on scale
            this.renameTopLevelLayoutSizeSignal();
            this.parseSelections();
            this.parseProjection();
            this.parseData(); // (pathorder) depends on markDef; selection filters depend on parsed selections; depends on projection because some transforms require the finalized projection name.
            this.parseAxesAndHeaders(); // depends on scale and layout size
            this.parseLegends(); // depends on scale, markDef
            this.parseMarkGroup(); // depends on data name, scale, layout size, axisGroup, and children's scale, axis, legend and mark.
        }
        parseScale() {
            parseScales(this);
        }
        parseProjection() {
            parseProjection(this);
        }
        /**
         * Rename top-level spec's size to be just width / height, ignoring model name.
         * This essentially merges the top-level spec's width/height signals with the width/height signals
         * to help us reduce redundant signals declaration.
         */
        renameTopLevelLayoutSizeSignal() {
            if (this.getName('width') !== 'width') {
                this.renameSignal(this.getName('width'), 'width');
            }
            if (this.getName('height') !== 'height') {
                this.renameSignal(this.getName('height'), 'height');
            }
        }
        parseLegends() {
            parseLegend(this);
        }
        assembleGroupStyle() {
            if (this.type === 'unit' || this.type === 'layer') {
                return (this.view && this.view.style) || 'cell';
            }
            return undefined;
        }
        assembleEncodeFromView(view) {
            // Exclude "style"
            const baseView = __rest(view, ["style"]);
            const e = {};
            for (const property in baseView) {
                if (baseView.hasOwnProperty(property)) {
                    const value = baseView[property];
                    if (value !== undefined) {
                        e[property] = { value };
                    }
                }
            }
            return e;
        }
        assembleGroupEncodeEntry(isTopLevel) {
            let encodeEntry = undefined;
            if (this.view) {
                encodeEntry = this.assembleEncodeFromView(this.view);
            }
            if (!isTopLevel) {
                // For top-level spec, we can set the global width and height signal to adjust the group size.
                // For other child specs, we have to manually set width and height in the encode entry.
                if (this.type === 'unit' || this.type === 'layer') {
                    return Object.assign({ width: this.getSizeSignalRef('width'), height: this.getSizeSignalRef('height') }, (encodeEntry || {}));
                }
            }
            return encodeEntry;
        }
        assembleLayout() {
            if (!this.layout) {
                return undefined;
            }
            const _a = this.layout, { spacing } = _a, layout = __rest(_a, ["spacing"]);
            const { component, config } = this;
            const titleBand = assembleLayoutTitleBand(component.layoutHeaders, config);
            return Object.assign({ padding: spacing }, this.assembleDefaultLayout(), layout, (titleBand ? { titleBand } : {}));
        }
        assembleDefaultLayout() {
            return {};
        }
        assembleHeaderMarks() {
            const { layoutHeaders } = this.component;
            let headerMarks = [];
            for (const channel of FACET_CHANNELS) {
                if (layoutHeaders[channel].title) {
                    headerMarks.push(assembleTitleGroup(this, channel));
                }
            }
            for (const channel of HEADER_CHANNELS) {
                headerMarks = headerMarks.concat(assembleHeaderGroups(this, channel));
            }
            return headerMarks;
        }
        assembleAxes() {
            return assembleAxes(this.component.axes, this.config);
        }
        assembleLegends() {
            return assembleLegends(this);
        }
        assembleProjections() {
            return assembleProjections(this);
        }
        assembleTitle() {
            const _a = this.title || {}, { encoding } = _a, titleNoEncoding = __rest(_a, ["encoding"]);
            const title = Object.assign({}, extractTitleConfig(this.config.title).nonMark, titleNoEncoding, (encoding ? { encode: { update: encoding } } : {}));
            if (title.text) {
                if (contains(['unit', 'layer'], this.type)) {
                    // Unit/Layer
                    if (contains(['middle', undefined], title.anchor)) {
                        title.frame = title.frame || 'group';
                    }
                }
                else {
                    // composition with Vega layout
                    // Set title = "start" by default for composition as "middle" does not look nice
                    // https://github.com/vega/vega/issues/960#issuecomment-471360328
                    title.anchor = title.anchor || 'start';
                }
                return keys(title).length > 0 ? title : undefined;
            }
            return undefined;
        }
        /**
         * Assemble the mark group for this model.  We accept optional `signals` so that we can include concat top-level signals with the top-level model's local signals.
         */
        assembleGroup(signals = []) {
            const group = {};
            signals = signals.concat(this.assembleSignals());
            if (signals.length > 0) {
                group.signals = signals;
            }
            const layout = this.assembleLayout();
            if (layout) {
                group.layout = layout;
            }
            group.marks = [].concat(this.assembleHeaderMarks(), this.assembleMarks());
            // Only include scales if this spec is top-level or if parent is facet.
            // (Otherwise, it will be merged with upper-level's scope.)
            const scales = !this.parent || isFacetModel(this.parent) ? assembleScales(this) : [];
            if (scales.length > 0) {
                group.scales = scales;
            }
            const axes = this.assembleAxes();
            if (axes.length > 0) {
                group.axes = axes;
            }
            const legends = this.assembleLegends();
            if (legends.length > 0) {
                group.legends = legends;
            }
            return group;
        }
        hasDescendantWithFieldOnChannel(channel) {
            for (const child of this.children) {
                if (isUnitModel(child)) {
                    if (child.channelHasField(channel)) {
                        return true;
                    }
                }
                else {
                    if (child.hasDescendantWithFieldOnChannel(channel)) {
                        return true;
                    }
                }
            }
            return false;
        }
        getName(text) {
            return varName((this.name ? this.name + '_' : '') + text);
        }
        /**
         * Request a data source name for the given data source type and mark that data source as required. This method should be called in parse, so that all used data source can be correctly instantiated in assembleData().
         */
        requestDataName(name) {
            const fullName = this.getName(name);
            // Increase ref count. This is critical because otherwise we won't create a data source.
            // We also increase the ref counts on OutputNode.getSource() calls.
            const refCounts = this.component.data.outputNodeRefCounts;
            refCounts[fullName] = (refCounts[fullName] || 0) + 1;
            return fullName;
        }
        getSizeSignalRef(sizeType) {
            if (isFacetModel(this.parent)) {
                const channel = sizeType === 'width' ? 'x' : 'y';
                const scaleComponent = this.component.scales[channel];
                if (scaleComponent && !scaleComponent.merged) {
                    // independent scale
                    const type = scaleComponent.get('type');
                    const range = scaleComponent.get('range');
                    if (hasDiscreteDomain(type) && isVgRangeStep(range)) {
                        const scaleName = scaleComponent.get('name');
                        const domain = assembleDomain(this, channel);
                        const field = getFieldFromDomain(domain);
                        if (field) {
                            const fieldRef = vgField({ aggregate: 'distinct', field }, { expr: 'datum' });
                            return {
                                signal: sizeExpr(scaleName, scaleComponent, fieldRef)
                            };
                        }
                        else {
                            warn('Unknown field for ${channel}.  Cannot calculate view size.');
                            return null;
                        }
                    }
                }
            }
            return {
                signal: this.signalNameMap.get(this.getName(sizeType))
            };
        }
        /**
         * Lookup the name of the datasource for an output node. You probably want to call this in assemble.
         */
        lookupDataSource(name) {
            const node = this.component.data.outputNodes[name];
            if (!node) {
                // Name not found in map so let's just return what we got.
                // This can happen if we already have the correct name.
                return name;
            }
            return node.getSource();
        }
        getSignalName(oldSignalName) {
            return this.signalNameMap.get(oldSignalName);
        }
        renameSignal(oldName, newName) {
            this.signalNameMap.rename(oldName, newName);
        }
        renameScale(oldName, newName) {
            this.scaleNameMap.rename(oldName, newName);
        }
        renameProjection(oldName, newName) {
            this.projectionNameMap.rename(oldName, newName);
        }
        /**
         * @return scale name for a given channel after the scale has been parsed and named.
         */
        scaleName(originalScaleName, parse) {
            if (parse) {
                // During the parse phase always return a value
                // No need to refer to rename map because a scale can't be renamed
                // before it has the original name.
                return this.getName(originalScaleName);
            }
            // If there is a scale for the channel, it should either
            // be in the scale component or exist in the name map
            if (
            // If there is a scale for the channel, there should be a local scale component for it
            (isChannel(originalScaleName) && isScaleChannel(originalScaleName) && this.component.scales[originalScaleName]) ||
                // in the scale name map (the scale get merged by its parent)
                this.scaleNameMap.has(this.getName(originalScaleName))) {
                return this.scaleNameMap.get(this.getName(originalScaleName));
            }
            return undefined;
        }
        /**
         * @return projection name after the projection has been parsed and named.
         */
        projectionName(parse) {
            if (parse) {
                // During the parse phase always return a value
                // No need to refer to rename map because a projection can't be renamed
                // before it has the original name.
                return this.getName('projection');
            }
            if ((this.component.projection && !this.component.projection.merged) ||
                this.projectionNameMap.has(this.getName('projection'))) {
                return this.projectionNameMap.get(this.getName('projection'));
            }
            return undefined;
        }
        /**
         * Traverse a model's hierarchy to get the scale component for a particular channel.
         */
        getScaleComponent(channel) {
            /* istanbul ignore next: This is warning for debugging test */
            if (!this.component.scales) {
                throw new Error('getScaleComponent cannot be called before parseScale().  Make sure you have called parseScale or use parseUnitModelWithScale().');
            }
            const localScaleComponent = this.component.scales[channel];
            if (localScaleComponent && !localScaleComponent.merged) {
                return localScaleComponent;
            }
            return this.parent ? this.parent.getScaleComponent(channel) : undefined;
        }
        /**
         * Traverse a model's hierarchy to get a particular selection component.
         */
        getSelectionComponent(variableName, origName) {
            let sel = this.component.selection[variableName];
            if (!sel && this.parent) {
                sel = this.parent.getSelectionComponent(variableName, origName);
            }
            if (!sel) {
                throw new Error(message.selectionNotFound(origName));
            }
            return sel;
        }
    }
    /** Abstract class for UnitModel and FacetModel.  Both of which can contain fieldDefs as a part of its own specification. */
    class ModelWithField extends Model {
        /** Get "field" reference for Vega */
        vgField(channel, opt = {}) {
            const fieldDef = this.fieldDef(channel);
            if (!fieldDef) {
                return undefined;
            }
            return vgField(fieldDef, opt);
        }
        reduceFieldDef(f, init, t) {
            return reduce(this.getMapping(), (acc, cd, c) => {
                const fieldDef = getFieldDef(cd);
                if (fieldDef) {
                    return f(acc, fieldDef, c);
                }
                return acc;
            }, init, t);
        }
        forEachFieldDef(f, t) {
            forEach(this.getMapping(), (cd, c) => {
                const fieldDef = getFieldDef(cd);
                if (fieldDef) {
                    f(fieldDef, c);
                }
            }, t);
        }
    }

    class FilterInvalidNode extends DataFlowNode {
        constructor(parent, fieldDefs) {
            super(parent);
            this.fieldDefs = fieldDefs;
        }
        clone() {
            return new FilterInvalidNode(null, Object.assign({}, this.fieldDefs));
        }
        static make(parent, model) {
            const { config, mark } = model;
            if (config.invalidValues !== 'filter') {
                return null;
            }
            const filter = model.reduceFieldDef((aggregator, fieldDef, channel) => {
                const scaleComponent = isScaleChannel(channel) && model.getScaleComponent(channel);
                if (scaleComponent) {
                    const scaleType = scaleComponent.get('type');
                    // While discrete domain scales can handle invalid values, continuous scales can't.
                    // Thus, for non-path marks, we have to filter null for scales with continuous domains.
                    // (For path marks, we will use "defined" property and skip these values instead.)
                    if (hasContinuousDomain(scaleType) && !fieldDef.aggregate && !isPathMark(mark)) {
                        aggregator[fieldDef.field] = fieldDef;
                    }
                }
                return aggregator;
            }, {});
            if (!keys(filter).length) {
                return null;
            }
            return new FilterInvalidNode(parent, filter);
        }
        get filter() {
            return this.fieldDefs;
        }
        // create the VgTransforms for each of the filtered fields
        assemble() {
            const filters = keys(this.filter).reduce((vegaFilters, field) => {
                const fieldDef = this.fieldDefs[field];
                const ref = vgField(fieldDef, { expr: 'datum' });
                if (fieldDef !== null) {
                    vegaFilters.push(`${ref} !== null`);
                    vegaFilters.push(`!isNaN(${ref})`);
                }
                return vegaFilters;
            }, []);
            return filters.length > 0
                ? {
                    type: 'filter',
                    expr: filters.join(' && ')
                }
                : null;
        }
    }

    /**
     * A class for flatten transform nodes
     */
    class FlattenTransformNode extends DataFlowNode {
        constructor(parent, transform) {
            super(parent);
            this.transform = transform;
            this.transform = duplicate(transform); // duplicate to prevent side effects
            const { flatten, as = [] } = this.transform;
            this.transform.as = flatten.map((f, i) => as[i] || f);
        }
        clone() {
            return new FlattenTransformNode(this.parent, duplicate(this.transform));
        }
        producedFields() {
            return new Set(this.transform.as);
        }
        hash() {
            return `FlattenTransform ${hash(this.transform)}`;
        }
        assemble() {
            const { flatten: fields, as } = this.transform;
            const result = {
                type: 'flatten',
                fields,
                as
            };
            return result;
        }
    }

    /**
     * A class for flatten transform nodes
     */
    class FoldTransformNode extends DataFlowNode {
        constructor(parent, transform) {
            super(parent);
            this.transform = transform;
            this.transform = duplicate(transform); // duplicate to prevent side effects
            const specifiedAs = this.transform.as || [undefined, undefined];
            this.transform.as = [specifiedAs[0] || 'key', specifiedAs[1] || 'value'];
        }
        clone() {
            return new FoldTransformNode(null, duplicate(this.transform));
        }
        producedFields() {
            return new Set(this.transform.as);
        }
        hash() {
            return `FoldTransform ${hash(this.transform)}`;
        }
        assemble() {
            const { fold, as } = this.transform;
            const result = {
                type: 'fold',
                fields: fold,
                as
            };
            return result;
        }
    }

    class GeoJSONNode extends DataFlowNode {
        constructor(parent, fields, geojson, signal) {
            super(parent);
            this.fields = fields;
            this.geojson = geojson;
            this.signal = signal;
        }
        clone() {
            return new GeoJSONNode(null, duplicate(this.fields), this.geojson, this.signal);
        }
        static parseAll(parent, model) {
            if (model.component.projection && !model.component.projection.isFit) {
                return parent;
            }
            let geoJsonCounter = 0;
            [[LONGITUDE, LATITUDE], [LONGITUDE2, LATITUDE2]].forEach((coordinates) => {
                const pair = coordinates.map(channel => model.channelHasField(channel)
                    ? model.fieldDef(channel).field
                    : isValueDef(model.encoding[channel])
                        ? { expr: model.encoding[channel].value + '' }
                        : undefined);
                if (pair[0] || pair[1]) {
                    parent = new GeoJSONNode(parent, pair, null, model.getName(`geojson_${geoJsonCounter++}`));
                }
            });
            if (model.channelHasField(SHAPE)) {
                const fieldDef = model.fieldDef(SHAPE);
                if (fieldDef.type === GEOJSON) {
                    parent = new GeoJSONNode(parent, null, fieldDef.field, model.getName(`geojson_${geoJsonCounter++}`));
                }
            }
            return parent;
        }
        assemble() {
            return Object.assign({ type: 'geojson' }, (this.fields ? { fields: this.fields } : {}), (this.geojson ? { geojson: this.geojson } : {}), { signal: this.signal });
        }
    }

    class GeoPointNode extends DataFlowNode {
        constructor(parent, projection, fields, as) {
            super(parent);
            this.projection = projection;
            this.fields = fields;
            this.as = as;
        }
        clone() {
            return new GeoPointNode(null, this.projection, duplicate(this.fields), duplicate(this.as));
        }
        static parseAll(parent, model) {
            if (!model.projectionName()) {
                return parent;
            }
            [[LONGITUDE, LATITUDE], [LONGITUDE2, LATITUDE2]].forEach((coordinates) => {
                const pair = coordinates.map(channel => model.channelHasField(channel)
                    ? model.fieldDef(channel).field
                    : isValueDef(model.encoding[channel])
                        ? { expr: model.encoding[channel].value + '' }
                        : undefined);
                const suffix = coordinates[0] === LONGITUDE2 ? '2' : '';
                if (pair[0] || pair[1]) {
                    parent = new GeoPointNode(parent, model.projectionName(), pair, [
                        model.getName('x' + suffix),
                        model.getName('y' + suffix)
                    ]);
                }
            });
            return parent;
        }
        assemble() {
            return {
                type: 'geopoint',
                projection: this.projection,
                fields: this.fields,
                as: this.as
            };
        }
    }

    class GraticuleNode extends DataFlowNode {
        constructor(parent, params) {
            super(parent);
            this.params = params;
        }
        clone() {
            return new GraticuleNode(null, this.params);
        }
        assemble() {
            return Object.assign({ type: 'graticule' }, (this.params === true ? {} : this.params));
        }
    }

    class IdentifierNode extends DataFlowNode {
        clone() {
            return new IdentifierNode(null);
        }
        constructor(parent) {
            super(parent);
        }
        producedFields() {
            return new Set([SELECTION_ID]);
        }
        hash() {
            return 'Identifier';
        }
        assemble() {
            return { type: 'identifier', as: SELECTION_ID };
        }
    }

    const area = {
        vgMark: 'area',
        encodeEntry: (model) => {
            return Object.assign({}, baseEncodeEntry(model, { size: 'ignore', orient: 'include' }), pointPosition('x', model, 'zeroOrMin'), pointPosition('y', model, 'zeroOrMin'), pointPosition2(model, 'zeroOrMin', model.markDef.orient === 'horizontal' ? 'x2' : 'y2'), defined(model));
        }
    };

    const bar = {
        vgMark: 'rect',
        encodeEntry: (model) => {
            return Object.assign({}, baseEncodeEntry(model, { size: 'ignore', orient: 'ignore' }), barPosition(model, 'x'), barPosition(model, 'y'));
        }
    };
    function barPosition(model, channel) {
        const { config, encoding, markDef } = model;
        const orient = markDef.orient;
        const sizeDef = encoding.size;
        const isBarLength = channel === 'x' ? orient === 'horizontal' : orient === 'vertical';
        const channel2 = channel === 'x' ? 'x2' : 'y2';
        const fieldDef = encoding[channel];
        const fieldDef2 = encoding[channel2];
        const scaleName = model.scaleName(channel);
        const scale = model.getScaleComponent(channel);
        const spacing = getFirstDefined(markDef.binSpacing, config.bar.binSpacing);
        const reverse = scale ? scale.get('reverse') : undefined;
        const mark = 'bar';
        // x, x2, and width -- we must specify two of these in all conditions
        if (isFieldDef(fieldDef) && isBinned(fieldDef.bin)) {
            return binPosition({ fieldDef, fieldDef2, channel, mark, scaleName, spacing, reverse });
        }
        else if (isBarLength || fieldDef2) {
            return Object.assign({}, pointPosition(channel, model, 'zeroOrMin'), pointPosition2(model, 'zeroOrMin', channel2));
        }
        else {
            const sizeChannel = channel === 'x' ? 'width' : 'height';
            // vertical
            if (isFieldDef(fieldDef)) {
                const scaleType = scale.get('type');
                if (isBinning(fieldDef.bin) && !sizeDef && !hasDiscreteDomain(scaleType)) {
                    return binPosition({ fieldDef, channel, scaleName, mark, spacing, reverse });
                }
                else {
                    if (scaleType === ScaleType.BAND) {
                        return bandPosition(fieldDef, channel, model, defaultSizeRef(markDef, sizeChannel, scaleName, scale, config));
                    }
                }
            }
            // sized bin, normal point-ordinal axis, quantitative x-axis, or no x
            return centeredPointPositionWithSize(channel, model, mid(model[sizeChannel]), defaultSizeRef(markDef, sizeChannel, scaleName, scale, config));
        }
    }
    function defaultSizeRef(markDef, sizeChannel, scaleName, scale, config) {
        const markPropOrConfig = getFirstDefined(markDef[sizeChannel], markDef.size, getMarkConfig('size', markDef, config, { vgChannel: sizeChannel }));
        if (markPropOrConfig !== undefined) {
            return { value: markPropOrConfig };
        }
        if (scale) {
            const scaleType = scale.get('type');
            if (scaleType === 'point' || scaleType === 'band') {
                if (config.bar.discreteBandSize !== undefined) {
                    return { value: config.bar.discreteBandSize };
                }
                if (scaleType === ScaleType.POINT) {
                    const scaleRange = scale.get('range');
                    if (isVgRangeStep(scaleRange) && isNumber(scaleRange.step)) {
                        return { value: scaleRange.step - 1 };
                    }
                    warn(message.BAR_WITH_POINT_SCALE_AND_RANGESTEP_NULL);
                }
                else {
                    // BAND
                    return bandRef(scaleName);
                }
            }
            else {
                // continuous scale
                return { value: config.bar.continuousBandSize };
            }
        }
        // No Scale
        const value = getFirstDefined(
        // No scale is like discrete bar (with one item)
        config.bar.discreteBandSize, config.scale.rangeStep ? config.scale.rangeStep - 1 : undefined, 
        // If somehow default rangeStep is set to null or undefined, use 20 as back up
        20);
        return { value };
    }

    const geoshape = {
        vgMark: 'shape',
        encodeEntry: (model) => {
            return Object.assign({}, baseEncodeEntry(model, { size: 'ignore', orient: 'ignore' }));
        },
        postEncodingTransform: (model) => {
            const { encoding } = model;
            const shapeDef = encoding.shape;
            const transform = Object.assign({ type: 'geoshape', projection: model.projectionName() }, (shapeDef && isFieldDef(shapeDef) && shapeDef.type === GEOJSON
                ? { field: vgField(shapeDef, { expr: 'datum' }) }
                : {}));
            return [transform];
        }
    };

    const line = {
        vgMark: 'line',
        encodeEntry: (model) => {
            const { width, height } = model;
            return Object.assign({}, baseEncodeEntry(model, { size: 'ignore', orient: 'ignore' }), pointPosition('x', model, mid(width)), pointPosition('y', model, mid(height)), nonPosition('size', model, {
                vgChannel: 'strokeWidth' // VL's line size is strokeWidth
            }), defined(model));
        }
    };
    const trail = {
        vgMark: 'trail',
        encodeEntry: (model) => {
            const { width, height } = model;
            return Object.assign({}, baseEncodeEntry(model, { size: 'include', orient: 'ignore' }), pointPosition('x', model, mid(width)), pointPosition('y', model, mid(height)), nonPosition('size', model), defined(model));
        }
    };

    function encodeEntry(model, fixedShape) {
        const { config, width, height } = model;
        return Object.assign({}, baseEncodeEntry(model, { size: 'include', orient: 'ignore' }), pointPosition('x', model, mid(width)), pointPosition('y', model, mid(height)), nonPosition('size', model), shapeMixins(model, config, fixedShape));
    }
    function shapeMixins(model, config, fixedShape) {
        if (fixedShape) {
            return { shape: { value: fixedShape } };
        }
        return nonPosition('shape', model);
    }
    const point = {
        vgMark: 'symbol',
        encodeEntry: (model) => {
            return encodeEntry(model);
        }
    };
    const circle = {
        vgMark: 'symbol',
        encodeEntry: (model) => {
            return encodeEntry(model, 'circle');
        }
    };
    const square = {
        vgMark: 'symbol',
        encodeEntry: (model) => {
            return encodeEntry(model, 'square');
        }
    };

    const rect = {
        vgMark: 'rect',
        encodeEntry: (model) => {
            return Object.assign({}, baseEncodeEntry(model, { size: 'ignore', orient: 'ignore' }), rectPosition(model, 'x'), rectPosition(model, 'y'));
        }
    };
    function rectPosition(model, channel) {
        const channel2 = channel === 'x' ? 'x2' : 'y2';
        const fieldDef = model.encoding[channel];
        const fieldDef2 = model.encoding[channel2];
        const scale = model.getScaleComponent(channel);
        const scaleType = scale ? scale.get('type') : undefined;
        const scaleName = model.scaleName(channel);
        if (isFieldDef(fieldDef) && (isBinning(fieldDef.bin) || isBinned(fieldDef.bin))) {
            return binPosition({
                fieldDef,
                fieldDef2,
                channel,
                mark: 'rect',
                scaleName,
                spacing: 0,
                reverse: scale.get('reverse')
            });
        }
        else if (isFieldDef(fieldDef) && scale && hasDiscreteDomain(scaleType)) {
            /* istanbul ignore else */
            if (scaleType === ScaleType.BAND) {
                return bandPosition(fieldDef, channel, model);
            }
            else {
                // We don't support rect mark with point/ordinal scale
                throw new Error(message.scaleTypeNotWorkWithMark(RECT, scaleType));
            }
        }
        else {
            // continuous scale or no scale
            return Object.assign({}, pointPosition(channel, model, 'zeroOrMax'), pointPosition2(model, 'zeroOrMin', channel2));
        }
    }

    const rule = {
        vgMark: 'rule',
        encodeEntry: (model) => {
            const { markDef, width, height } = model;
            const orient = markDef.orient;
            if (!model.encoding.x && !model.encoding.y && !model.encoding.latitude && !model.encoding.longitude) {
                // Show nothing if we have none of x, y, lat, and long.
                return {};
            }
            return Object.assign({}, baseEncodeEntry(model, { size: 'ignore', orient: 'ignore' }), pointPosition('x', model, orient === 'horizontal' ? 'zeroOrMin' : mid(width)), pointPosition('y', model, orient === 'vertical' ? 'zeroOrMin' : mid(height)), (orient !== 'vertical' ? pointPosition2(model, 'zeroOrMax', 'x2') : {}), (orient !== 'horizontal' ? pointPosition2(model, 'zeroOrMax', 'y2') : {}), nonPosition('size', model, {
                vgChannel: 'strokeWidth' // VL's rule size is strokeWidth
            }));
        }
    };

    const text$2 = {
        vgMark: 'text',
        encodeEntry: (model) => {
            const { config, encoding, width, height } = model;
            return Object.assign({}, baseEncodeEntry(model, { size: 'ignore', orient: 'ignore' }), pointPosition('x', model, mid(width)), pointPosition('y', model, mid(height)), text$1(model), nonPosition('size', model, {
                vgChannel: 'fontSize' // VL's text size is fontSize
            }), valueIfDefined('align', align(model.markDef, encoding, config)), valueIfDefined('baseline', baseline(model.markDef, encoding, config)));
        }
    };
    function align(markDef, encoding, config) {
        const a = markDef.align || getMarkConfig('align', markDef, config);
        if (a === undefined) {
            return 'center';
        }
        // If there is a config, Vega-parser will process this already.
        return undefined;
    }
    function baseline(markDef, encoding, config) {
        const b = markDef.baseline || getMarkConfig('baseline', markDef, config);
        if (b === undefined) {
            return 'middle';
        }
        // If there is a config, Vega-parser will process this already.
        return undefined;
    }

    const tick = {
        vgMark: 'rect',
        encodeEntry: (model) => {
            const { config, markDef, width, height } = model;
            const orient = markDef.orient;
            const vgSizeChannel = orient === 'horizontal' ? 'width' : 'height';
            const vgThicknessChannel = orient === 'horizontal' ? 'height' : 'width';
            return Object.assign({}, baseEncodeEntry(model, { size: 'ignore', orient: 'ignore' }), pointPosition('x', model, mid(width), 'xc'), pointPosition('y', model, mid(height), 'yc'), nonPosition('size', model, {
                defaultValue: defaultSize(model),
                vgChannel: vgSizeChannel
            }), { [vgThicknessChannel]: { value: getFirstDefined(markDef.thickness, config.tick.thickness) } });
        }
    };
    function defaultSize(model) {
        const { config, markDef } = model;
        const { orient } = markDef;
        const vgSizeChannel = orient === 'horizontal' ? 'width' : 'height';
        const scale = model.getScaleComponent(orient === 'horizontal' ? 'x' : 'y');
        const markPropOrConfig = getFirstDefined(markDef[vgSizeChannel], markDef.size, getMarkConfig('size', markDef, config, { vgChannel: vgSizeChannel }), config.tick.bandSize);
        if (markPropOrConfig !== undefined) {
            return markPropOrConfig;
        }
        else {
            const scaleRange = scale ? scale.get('range') : undefined;
            const rangeStep = scaleRange && isVgRangeStep(scaleRange) ? scaleRange.step : config.scale.rangeStep;
            if (typeof rangeStep !== 'number') {
                // FIXME consolidate this log
                throw new Error('Function does not handle non-numeric rangeStep');
            }
            return (rangeStep * 3) / 4;
        }
    }

    const markCompiler = {
        area,
        bar,
        circle,
        geoshape,
        line,
        point,
        rect,
        rule,
        square,
        text: text$2,
        tick,
        trail
    };
    function parseMarkGroups(model) {
        if (contains([LINE, AREA, TRAIL], model.mark)) {
            return parsePathMark(model);
        }
        else {
            return getMarkGroups(model);
        }
    }
    const FACETED_PATH_PREFIX = 'faceted_path_';
    function parsePathMark(model) {
        const details = pathGroupingFields(model.mark, model.encoding);
        const pathMarks = getMarkGroups(model, {
            // If has subfacet for line/area group, need to use faceted data from below.
            fromPrefix: details.length > 0 ? FACETED_PATH_PREFIX : ''
        });
        if (details.length > 0) {
            // have level of details - need to facet line into subgroups
            // TODO: for non-stacked plot, map order to zindex. (Maybe rename order for layer to zindex?)
            return [
                {
                    name: model.getName('pathgroup'),
                    type: 'group',
                    from: {
                        facet: {
                            name: FACETED_PATH_PREFIX + model.requestDataName(MAIN),
                            data: model.requestDataName(MAIN),
                            groupby: details
                        }
                    },
                    encode: {
                        update: {
                            width: { field: { group: 'width' } },
                            height: { field: { group: 'height' } }
                        }
                    },
                    marks: pathMarks
                }
            ];
        }
        else {
            return pathMarks;
        }
    }
    function getSort$1(model) {
        const { encoding, stack, mark, markDef, config } = model;
        const order = encoding.order;
        if ((!isArray(order) && isValueDef(order) && isNullOrFalse(order.value)) ||
            ((!order && isNullOrFalse(markDef.order)) || isNullOrFalse(getMarkConfig('order', markDef, config)))) {
            return undefined;
        }
        else if ((isArray(order) || isFieldDef(order)) && !stack) {
            // Sort by the order field if it is specified and the field is not stacked. (For stacked field, order specify stack order.)
            return sortParams(order, { expr: 'datum' });
        }
        else if (isPathMark(mark)) {
            // For both line and area, we sort values based on dimension by default
            const dimensionChannelDef = encoding[markDef.orient === 'horizontal' ? 'y' : 'x'];
            if (isFieldDef(dimensionChannelDef)) {
                const s = dimensionChannelDef.sort;
                const sortField = isSortField(s)
                    ? vgField({
                        // FIXME: this op might not already exist?
                        // FIXME: what if dimensionChannel (x or y) contains custom domain?
                        aggregate: isAggregate(model.encoding) ? s.op : undefined,
                        field: s.field
                    }, { expr: 'datum' })
                    : vgField(dimensionChannelDef, {
                        // For stack with imputation, we only have bin_mid
                        binSuffix: model.stack && model.stack.impute ? 'mid' : undefined,
                        expr: 'datum'
                    });
                return {
                    field: sortField,
                    order: 'descending'
                };
            }
            return undefined;
        }
        return undefined;
    }
    function getMarkGroups(model, opt = { fromPrefix: '' }) {
        const mark = model.mark;
        const clip = getFirstDefined(model.markDef.clip, scaleClip(model), projectionClip(model));
        const style = getStyles(model.markDef);
        const key = model.encoding.key;
        const sort = getSort$1(model);
        const postEncodingTransform = markCompiler[mark].postEncodingTransform
            ? markCompiler[mark].postEncodingTransform(model)
            : null;
        return [
            Object.assign({ name: model.getName('marks'), type: markCompiler[mark].vgMark }, (clip ? { clip: true } : {}), (style ? { style } : {}), (key ? { key: { field: key.field } } : {}), (sort ? { sort } : {}), { from: { data: opt.fromPrefix + model.requestDataName(MAIN) }, encode: {
                    update: markCompiler[mark].encodeEntry(model)
                } }, (postEncodingTransform
                ? {
                    transform: postEncodingTransform
                }
                : {}))
        ];
    }
    /**
     * Returns list of path grouping fields
     * that the model's spec contains.
     */
    function pathGroupingFields(mark, encoding) {
        return keys(encoding).reduce((details, channel) => {
            switch (channel) {
                // x, y, x2, y2, lat, long, lat1, long2, order, tooltip, href, cursor should not cause lines to group
                case 'x':
                case 'y':
                case 'order':
                case 'href':
                case 'x2':
                case 'y2':
                // falls through
                case 'latitude':
                case 'longitude':
                case 'latitude2':
                case 'longitude2':
                // TODO: case 'cursor':
                // text, shape, shouldn't be a part of line/trail/area [falls through]
                case 'text':
                case 'shape':
                // falls through
                // tooltip fields should not be added to group by [falls through]
                case 'tooltip':
                    return details;
                case 'detail':
                case 'key': {
                    const channelDef = encoding[channel];
                    if (isArray(channelDef) || isFieldDef(channelDef)) {
                        (isArray(channelDef) ? channelDef : [channelDef]).forEach(fieldDef => {
                            if (!fieldDef.aggregate) {
                                details.push(vgField(fieldDef, {}));
                            }
                        });
                    }
                    return details;
                }
                case 'size':
                    if (mark === 'trail') {
                        // For trail, size should not group trail lines.
                        return details;
                    }
                // For line, it should group lines.
                // falls through
                case 'color':
                case 'fill':
                case 'stroke':
                case 'opacity':
                case 'fillOpacity':
                case 'strokeOpacity':
                case 'strokeWidth': {
                    // TODO strokeDashOffset:
                    // falls through
                    const fieldDef = getTypedFieldDef(encoding[channel]);
                    if (fieldDef && !fieldDef.aggregate) {
                        details.push(vgField(fieldDef, {}));
                    }
                    return details;
                }
                default:
                    throw new Error(`Bug: Channel ${channel} unimplemented for line mark`);
            }
        }, []);
    }
    /**
     * If scales are bound to interval selections, we want to automatically clip
     * marks to account for panning/zooming interactions. We identify bound scales
     * by the domainRaw property, which gets added during scale parsing.
     */
    function scaleClip(model) {
        const xScale = model.getScaleComponent('x');
        const yScale = model.getScaleComponent('y');
        return (xScale && xScale.get('domainRaw')) || (yScale && yScale.get('domainRaw')) ? true : undefined;
    }
    /**
     * If we use a custom projection with auto-fitting to the geodata extent,
     * we need to clip to ensure the chart size doesn't explode.
     */
    function projectionClip(model) {
        const projection = model.component.projection;
        return projection && !projection.isFit ? true : undefined;
    }

    class ImputeNode extends DataFlowNode {
        constructor(parent, transform) {
            super(parent);
            this.transform = transform;
        }
        clone() {
            return new ImputeNode(null, duplicate(this.transform));
        }
        producedFields() {
            return new Set([this.transform.impute]);
        }
        processSequence(keyvals) {
            const { start = 0, stop, step } = keyvals;
            const result = [start, stop, ...(step ? [step] : [])].join(',');
            return { signal: `sequence(${result})` };
        }
        static makeFromTransform(parent, imputeTransform) {
            return new ImputeNode(parent, imputeTransform);
        }
        static makeFromEncoding(parent, model) {
            const encoding = model.encoding;
            const xDef = encoding.x;
            const yDef = encoding.y;
            if (isFieldDef(xDef) && isFieldDef(yDef)) {
                const imputedChannel = xDef.impute ? xDef : yDef.impute ? yDef : undefined;
                if (imputedChannel === undefined) {
                    return undefined;
                }
                const keyChannel = xDef.impute ? yDef : yDef.impute ? xDef : undefined;
                const { method, value, frame, keyvals } = imputedChannel.impute;
                const groupbyFields = pathGroupingFields(model.mark, encoding);
                return new ImputeNode(parent, Object.assign({ impute: imputedChannel.field, key: keyChannel.field }, (method ? { method } : {}), (value !== undefined ? { value } : {}), (frame ? { frame } : {}), (keyvals !== undefined ? { keyvals } : {}), (groupbyFields.length ? { groupby: groupbyFields } : {})));
            }
            return null;
        }
        hash() {
            return `Impute ${hash(this.transform)}`;
        }
        assemble() {
            const { impute, key, keyvals, method, groupby, value, frame = [null, null] } = this.transform;
            const initialImpute = Object.assign({ type: 'impute', field: impute, key }, (keyvals ? { keyvals: isImputeSequence(keyvals) ? this.processSequence(keyvals) : keyvals } : {}), { method: 'value' }, (groupby ? { groupby } : {}), { value: null });
            let setImputedField;
            if (method && method !== 'value') {
                const deriveNewField = Object.assign({ type: 'window', as: [`imputed_${impute}_value`], ops: [method], fields: [impute], frame, ignorePeers: false }, (groupby ? { groupby } : {}));
                const replaceOriginal = {
                    type: 'formula',
                    expr: `datum.${impute} === null ? datum.imputed_${impute}_value : datum.${impute}`,
                    as: impute
                };
                setImputedField = [deriveNewField, replaceOriginal];
            }
            else {
                const replaceWithValue = {
                    type: 'formula',
                    expr: `datum.${impute} === null ? ${value} : datum.${impute}`,
                    as: impute
                };
                setImputedField = [replaceWithValue];
            }
            return [initialImpute, ...setImputedField];
        }
    }

    /**
     * Class to track interesting properties (see https://15721.courses.cs.cmu.edu/spring2016/papers/graefe-ieee1995.pdf)
     * about how fields have been parsed or whether they have been derived in a transform. We use this to not parse the
     * same field again (or differently).
     */
    class AncestorParse extends Split {
        constructor(explicit = {}, implicit = {}, parseNothing = false) {
            super(explicit, implicit);
            this.explicit = explicit;
            this.implicit = implicit;
            this.parseNothing = parseNothing;
        }
        clone() {
            const clone = super.clone();
            clone.parseNothing = this.parseNothing;
            return clone;
        }
    }

    class LookupNode extends DataFlowNode {
        constructor(parent, transform, secondary) {
            super(parent);
            this.transform = transform;
            this.secondary = secondary;
        }
        clone() {
            return new LookupNode(null, duplicate(this.transform), this.secondary);
        }
        static make(parent, model, transform, counter) {
            const sources = model.component.data.sources;
            let fromSource = findSource(transform.from.data, sources);
            if (!fromSource) {
                fromSource = new SourceNode(transform.from.data);
                sources.push(fromSource);
            }
            const fromOutputName = model.getName(`lookup_${counter}`);
            const fromOutputNode = new OutputNode(fromSource, fromOutputName, 'lookup', model.component.data.outputNodeRefCounts);
            model.component.data.outputNodes[fromOutputName] = fromOutputNode;
            return new LookupNode(parent, transform, fromOutputNode.getSource());
        }
        producedFields() {
            return new Set(this.transform.from.fields || (this.transform.as instanceof Array ? this.transform.as : [this.transform.as]));
        }
        hash() {
            return `Lookup ${hash({ transform: this.transform, secondary: this.secondary })}`;
        }
        assemble() {
            let foreign;
            if (this.transform.from.fields) {
                // lookup a few fields and add create a flat output
                foreign = Object.assign({ values: this.transform.from.fields }, (this.transform.as ? { as: this.transform.as instanceof Array ? this.transform.as : [this.transform.as] } : {}));
            }
            else {
                // lookup full record and nest it
                let asName = this.transform.as;
                if (!isString(asName)) {
                    warn(message.NO_FIELDS_NEEDS_AS);
                    asName = '_lookup';
                }
                foreign = {
                    as: [asName]
                };
            }
            return Object.assign({ type: 'lookup', from: this.secondary, key: this.transform.from.key, fields: [this.transform.lookup] }, foreign, (this.transform.default ? { default: this.transform.default } : {}));
        }
    }

    /**
     * A class for the sample transform nodes
     */
    class SampleTransformNode extends DataFlowNode {
        constructor(parent, transform) {
            super(parent);
            this.transform = transform;
        }
        clone() {
            return new SampleTransformNode(null, duplicate(this.transform));
        }
        hash() {
            return `SampleTransform ${hash(this.transform)}`;
        }
        assemble() {
            return {
                type: 'sample',
                size: this.transform.sample
            };
        }
    }

    class SequenceNode extends DataFlowNode {
        constructor(parent, params) {
            super(parent);
            this.params = params;
        }
        clone() {
            return new SequenceNode(null, this.params);
        }
        assemble() {
            return Object.assign({ type: 'sequence' }, this.params);
        }
    }

    function makeWalkTree(data) {
        // to name datasources
        let datasetIndex = 0;
        /**
         * Recursively walk down the tree.
         */
        function walkTree(node, dataSource) {
            if (node instanceof SourceNode) {
                // If the source is a named data source or a data source with values, we need
                // to put it in a different data source. Otherwise, Vega may override the data.
                if (!node.isGenerator && !isUrlData(node.data)) {
                    data.push(dataSource);
                    const newData = {
                        name: null,
                        source: dataSource.name,
                        transform: []
                    };
                    dataSource = newData;
                }
            }
            if (node instanceof ParseNode) {
                if (node.parent instanceof SourceNode && !dataSource.source) {
                    // If node's parent is a root source and the data source does not refer to another data source, use normal format parse
                    dataSource.format = Object.assign({}, (dataSource.format || {}), { parse: node.assembleFormatParse() });
                    // add calculates for all nested fields
                    dataSource.transform.push(...node.assembleTransforms(true));
                }
                else {
                    // Otherwise use Vega expression to parse
                    dataSource.transform.push(...node.assembleTransforms());
                }
            }
            if (node instanceof FacetNode) {
                if (!dataSource.name) {
                    dataSource.name = `data_${datasetIndex++}`;
                }
                if (!dataSource.source || dataSource.transform.length > 0) {
                    data.push(dataSource);
                    node.data = dataSource.name;
                }
                else {
                    node.data = dataSource.source;
                }
                node.assemble().forEach(d => data.push(d));
                // break here because the rest of the tree has to be taken care of by the facet.
                return;
            }
            if (node instanceof GraticuleNode ||
                node instanceof SequenceNode ||
                node instanceof FilterInvalidNode ||
                node instanceof FilterNode ||
                node instanceof CalculateNode ||
                node instanceof GeoPointNode ||
                node instanceof GeoJSONNode ||
                node instanceof AggregateNode ||
                node instanceof LookupNode ||
                node instanceof WindowTransformNode ||
                node instanceof JoinAggregateTransformNode ||
                node instanceof FoldTransformNode ||
                node instanceof FlattenTransformNode ||
                node instanceof IdentifierNode ||
                node instanceof SampleTransformNode) {
                dataSource.transform.push(node.assemble());
            }
            if (node instanceof BinNode ||
                node instanceof TimeUnitNode ||
                node instanceof ImputeNode ||
                node instanceof StackNode) {
                dataSource.transform = dataSource.transform.concat(node.assemble());
            }
            if (node instanceof OutputNode) {
                if (dataSource.source && dataSource.transform.length === 0) {
                    node.setSource(dataSource.source);
                }
                else if (node.parent instanceof OutputNode) {
                    // Note that an output node may be required but we still do not assemble a
                    // separate data source for it.
                    node.setSource(dataSource.name);
                }
                else {
                    if (!dataSource.name) {
                        dataSource.name = `data_${datasetIndex++}`;
                    }
                    // Here we set the name of the datasource we generated. From now on
                    // other assemblers can use it.
                    node.setSource(dataSource.name);
                    // if this node has more than one child, we will add a datasource automatically
                    if (node.numChildren() === 1) {
                        data.push(dataSource);
                        const newData = {
                            name: null,
                            source: dataSource.name,
                            transform: []
                        };
                        dataSource = newData;
                    }
                }
            }
            switch (node.numChildren()) {
                case 0:
                    // done
                    if (node instanceof OutputNode && (!dataSource.source || dataSource.transform.length > 0)) {
                        // do not push empty datasources that are simply references
                        data.push(dataSource);
                    }
                    break;
                case 1:
                    walkTree(node.children[0], dataSource);
                    break;
                default: {
                    if (!dataSource.name) {
                        dataSource.name = `data_${datasetIndex++}`;
                    }
                    let source = dataSource.name;
                    if (!dataSource.source || dataSource.transform.length > 0) {
                        data.push(dataSource);
                    }
                    else {
                        source = dataSource.source;
                    }
                    node.children.forEach(child => {
                        const newData = {
                            name: null,
                            source: source,
                            transform: []
                        };
                        walkTree(child, newData);
                    });
                    break;
                }
            }
        }
        return walkTree;
    }
    /**
     * Assemble data sources that are derived from faceted data.
     */
    function assembleFacetData(root) {
        const data = [];
        const walkTree = makeWalkTree(data);
        root.children.forEach(child => walkTree(child, {
            source: root.name,
            name: null,
            transform: []
        }));
        return data;
    }
    /**
     * Create Vega Data array from a given compiled model and append all of them to the given array
     *
     * @param  model
     * @param  data array
     * @return modified data array
     */
    function assembleRootData(dataComponent, datasets) {
        const data = [];
        // dataComponent.sources.forEach(debug);
        // draw(dataComponent.sources);
        const walkTree = makeWalkTree(data);
        let sourceIndex = 0;
        dataComponent.sources.forEach(root => {
            // assign a name if the source does not have a name yet
            if (!root.hasName()) {
                root.dataName = `source_${sourceIndex++}`;
            }
            const newData = root.assemble();
            walkTree(root, newData);
        });
        // remove empty transform arrays for cleaner output
        data.forEach(d => {
            if (d.transform.length === 0) {
                delete d.transform;
            }
        });
        // move sources without transforms (the ones that are potentially used in lookups) to the beginning
        let whereTo = 0;
        for (const [i, d] of data.entries()) {
            if ((d.transform || []).length === 0 && !d.source) {
                data.splice(whereTo++, 0, data.splice(i, 1)[0]);
            }
        }
        // now fix the from references in lookup transforms
        for (const d of data) {
            for (const t of d.transform || []) {
                if (t.type === 'lookup') {
                    t.from = dataComponent.outputNodes[t.from].getSource();
                }
            }
        }
        // inline values for datasets that are in the datastore
        for (const d of data) {
            if (d.name in datasets) {
                d.values = datasets[d.name];
            }
        }
        return data;
    }

    function getHeaderType(orient) {
        if (orient === 'top' || orient === 'left') {
            return 'header';
        }
        return 'footer';
    }
    function parseFacetHeaders(model) {
        for (const channel of FACET_CHANNELS) {
            parseFacetHeader(model, channel);
        }
        mergeChildAxis(model, 'x');
        mergeChildAxis(model, 'y');
    }
    function parseFacetHeader(model, channel) {
        if (model.channelHasField(channel)) {
            const fieldDef = model.facet[channel];
            const titleConfig = getHeaderProperty('title', null, model.config, channel);
            let title$1 = title(fieldDef, model.config, {
                allowDisabling: true,
                includeDefault: titleConfig === undefined || !!titleConfig
            });
            if (model.child.component.layoutHeaders[channel].title) {
                // merge title with child to produce "Title / Subtitle / Sub-subtitle"
                title$1 += ' / ' + model.child.component.layoutHeaders[channel].title;
                model.child.component.layoutHeaders[channel].title = null;
            }
            const labelOrient = getHeaderProperty('labelOrient', fieldDef, model.config, channel);
            const headerType = contains(['bottom', 'right'], labelOrient) ? 'footer' : 'header';
            model.component.layoutHeaders[channel] = {
                title: title$1,
                facetFieldDef: fieldDef,
                [headerType]: channel === 'facet' ? [] : [makeHeaderComponent(model, channel, true)]
            };
        }
    }
    function makeHeaderComponent(model, channel, labels) {
        const sizeType = channel === 'row' ? 'height' : 'width';
        return {
            labels,
            sizeSignal: model.child.component.layoutSize.get(sizeType) ? model.child.getSizeSignalRef(sizeType) : undefined,
            axes: []
        };
    }
    function mergeChildAxis(model, channel) {
        const { child } = model;
        if (child.component.axes[channel]) {
            const { layoutHeaders, resolve } = model.component;
            resolve.axis[channel] = parseGuideResolve(resolve, channel);
            if (resolve.axis[channel] === 'shared') {
                // For shared axis, move the axes to facet's header or footer
                const headerChannel = channel === 'x' ? 'column' : 'row';
                const layoutHeader = layoutHeaders[headerChannel];
                for (const axisComponent of child.component.axes[channel]) {
                    const headerType = getHeaderType(axisComponent.get('orient'));
                    layoutHeader[headerType] = layoutHeader[headerType] || [makeHeaderComponent(model, headerChannel, false)];
                    // FIXME: assemble shouldn't be called here, but we do it this way so we only extract the main part of the axes
                    const mainAxis = assembleAxis(axisComponent, 'main', model.config, { header: true });
                    // LayoutHeader no longer keep track of property precedence, thus let's combine.
                    layoutHeader[headerType][0].axes.push(mainAxis);
                    axisComponent.mainExtracted = true;
                }
            }
        }
    }

    function parseLayerLayoutSize(model) {
        parseChildrenLayoutSize(model);
        const layoutSizeCmpt = model.component.layoutSize;
        layoutSizeCmpt.setWithExplicit('width', parseNonUnitLayoutSizeForChannel(model, 'width'));
        layoutSizeCmpt.setWithExplicit('height', parseNonUnitLayoutSizeForChannel(model, 'height'));
    }
    const parseRepeatLayoutSize = parseLayerLayoutSize;
    const SIZE_TYPE_TO_MERGE = {
        vconcat: 'width',
        hconcat: 'height'
    };
    function parseConcatLayoutSize(model) {
        parseChildrenLayoutSize(model);
        const layoutSizeCmpt = model.component.layoutSize;
        const sizeTypeToMerge = SIZE_TYPE_TO_MERGE[model.concatType];
        if (sizeTypeToMerge) {
            layoutSizeCmpt.setWithExplicit(sizeTypeToMerge, parseNonUnitLayoutSizeForChannel(model, sizeTypeToMerge));
        }
    }
    function parseChildrenLayoutSize(model) {
        for (const child of model.children) {
            child.parseLayoutSize();
        }
    }
    function parseNonUnitLayoutSizeForChannel(model, sizeType) {
        const channel = sizeType === 'width' ? 'x' : 'y';
        const resolve = model.component.resolve;
        let mergedSize;
        // Try to merge layout size
        for (const child of model.children) {
            const childSize = child.component.layoutSize.getWithExplicit(sizeType);
            const scaleResolve = resolve.scale[channel];
            if (scaleResolve === 'independent' && childSize.value === 'range-step') {
                // Do not merge independent scales with range-step as their size depends
                // on the scale domains, which can be different between scales.
                mergedSize = undefined;
                break;
            }
            if (mergedSize) {
                if (scaleResolve === 'independent' && mergedSize.value !== childSize.value) {
                    // For independent scale, only merge if all the sizes are the same.
                    // If the values are different, abandon the merge!
                    mergedSize = undefined;
                    break;
                }
                mergedSize = mergeValuesWithExplicit(mergedSize, childSize, sizeType, '');
            }
            else {
                mergedSize = childSize;
            }
        }
        if (mergedSize) {
            // If merged, rename size and set size of all children.
            for (const child of model.children) {
                model.renameSignal(child.getName(sizeType), model.getName(sizeType));
                child.component.layoutSize.set(sizeType, 'merged', false);
            }
            return mergedSize;
        }
        else {
            // Otherwise, there is no merged size.
            return {
                explicit: false,
                value: undefined
            };
        }
    }
    function parseUnitLayoutSize(model) {
        const layoutSizeComponent = model.component.layoutSize;
        if (!layoutSizeComponent.explicit.width) {
            const width = defaultUnitSize(model, 'width');
            layoutSizeComponent.set('width', width, false);
        }
        if (!layoutSizeComponent.explicit.height) {
            const height = defaultUnitSize(model, 'height');
            layoutSizeComponent.set('height', height, false);
        }
    }
    function defaultUnitSize(model, sizeType) {
        const channel = sizeType === 'width' ? 'x' : 'y';
        const config = model.config;
        const scaleComponent = model.getScaleComponent(channel);
        if (scaleComponent) {
            const scaleType = scaleComponent.get('type');
            const range = scaleComponent.get('range');
            if (hasDiscreteDomain(scaleType) && isVgRangeStep(range)) {
                // For discrete domain with range.step, use dynamic width/height
                return 'range-step';
            }
            else {
                return config.view[sizeType];
            }
        }
        else if (model.hasProjection) {
            return config.view[sizeType];
        }
        else {
            // No scale - set default size
            if (sizeType === 'width' && model.mark === 'text') {
                // width for text mark without x-field is a bit wider than typical range step
                return config.scale.textXRangeStep;
            }
            // Set width/height equal to rangeStep config or if rangeStep is null, use value from default scale config.
            return config.scale.rangeStep || defaultScaleConfig.rangeStep;
        }
    }

    function replaceRepeaterInFacet(facet, repeater) {
        if (isFacetMapping(facet)) {
            return replaceRepeater(facet, repeater);
        }
        return replaceRepeaterInFieldDef(facet, repeater);
    }
    function replaceRepeaterInEncoding(encoding, repeater) {
        return replaceRepeater(encoding, repeater);
    }
    /**
     * Replaces repeated value and returns if the repeated value is valid.
     */
    function replaceRepeat(o, repeater) {
        if (isRepeatRef(o.field)) {
            if (o.field.repeat in repeater) {
                // any needed to calm down ts compiler
                return Object.assign({}, o, { field: repeater[o.field.repeat] });
            }
            else {
                warn(message.noSuchRepeatedValue(o.field.repeat));
                return undefined;
            }
        }
        return o;
    }
    /**
     * Replace repeater values in a field def with the concrete field name.
     */
    function replaceRepeaterInFieldDef(fieldDef, repeater) {
        fieldDef = replaceRepeat(fieldDef, repeater);
        if (fieldDef === undefined) {
            // the field def should be ignored
            return undefined;
        }
        else if (fieldDef === null) {
            return null;
        }
        if (isSortableFieldDef(fieldDef) && isSortField(fieldDef.sort)) {
            const sort = replaceRepeat(fieldDef.sort, repeater);
            fieldDef = Object.assign({}, fieldDef, (sort ? { sort } : {}));
        }
        return fieldDef;
    }
    function replaceRepeaterInChannelDef(channelDef, repeater) {
        if (isFieldDef(channelDef)) {
            const fd = replaceRepeaterInFieldDef(channelDef, repeater);
            if (fd) {
                return fd;
            }
            else if (isConditionalDef(channelDef)) {
                return { condition: channelDef.condition };
            }
        }
        else {
            if (hasConditionalFieldDef(channelDef)) {
                const fd = replaceRepeaterInFieldDef(channelDef.condition, repeater);
                if (fd) {
                    return Object.assign({}, channelDef, { condition: fd });
                }
                else {
                    const channelDefWithoutCondition = __rest(channelDef, ["condition"]);
                    return channelDefWithoutCondition;
                }
            }
            return channelDef;
        }
        return undefined;
    }
    function replaceRepeater(mapping, repeater) {
        const out = {};
        for (const channel in mapping) {
            if (mapping.hasOwnProperty(channel)) {
                const channelDef = mapping[channel];
                if (isArray(channelDef)) {
                    // array cannot have condition
                    out[channel] = channelDef.map(cd => replaceRepeaterInChannelDef(cd, repeater)).filter(cd => cd);
                }
                else {
                    const cd = replaceRepeaterInChannelDef(channelDef, repeater);
                    if (cd !== undefined) {
                        out[channel] = cd;
                    }
                }
            }
        }
        return out;
    }

    function facetSortFieldName(fieldDef, sort, opt) {
        return vgField(sort, Object.assign({ suffix: `by_${vgField(fieldDef)}` }, (opt || {})));
    }
    class FacetModel extends ModelWithField {
        constructor(spec, parent, parentGivenName, repeater, config) {
            super(spec, 'facet', parent, parentGivenName, config, repeater, spec.resolve);
            this.child = buildModel(spec.spec, this, this.getName('child'), undefined, repeater, config, false);
            this.children = [this.child];
            const facet = replaceRepeaterInFacet(spec.facet, repeater);
            this.facet = this.initFacet(facet);
        }
        initFacet(facet) {
            // clone to prevent side effect to the original spec
            if (!isFacetMapping(facet)) {
                return { facet: normalize(facet, 'facet') };
            }
            return reduce(facet, (normalizedFacet, fieldDef, channel) => {
                if (!contains([ROW, COLUMN], channel)) {
                    // Drop unsupported channel
                    warn(message.incompatibleChannel(channel, 'facet'));
                    return normalizedFacet;
                }
                if (fieldDef.field === undefined) {
                    warn(message.emptyFieldDef(fieldDef, channel));
                    return normalizedFacet;
                }
                // Convert type to full, lowercase type, or augment the fieldDef with a default type if missing.
                normalizedFacet[channel] = normalize(fieldDef, channel);
                return normalizedFacet;
            }, {});
        }
        channelHasField(channel) {
            return !!this.facet[channel];
        }
        fieldDef(channel) {
            return this.facet[channel];
        }
        parseData() {
            this.component.data = parseData(this);
            this.child.parseData();
        }
        parseLayoutSize() {
            parseChildrenLayoutSize(this);
        }
        parseSelections() {
            // As a facet has a single child, the selection components are the same.
            // The child maintains its selections to assemble signals, which remain
            // within its unit.
            this.child.parseSelections();
            this.component.selection = this.child.component.selection;
        }
        parseMarkGroup() {
            this.child.parseMarkGroup();
        }
        parseAxesAndHeaders() {
            this.child.parseAxesAndHeaders();
            parseFacetHeaders(this);
        }
        assembleSelectionTopLevelSignals(signals) {
            return this.child.assembleSelectionTopLevelSignals(signals);
        }
        assembleSignals() {
            this.child.assembleSignals();
            return [];
        }
        assembleSelectionData(data) {
            return this.child.assembleSelectionData(data);
        }
        getHeaderLayoutMixins() {
            const layoutMixins = {};
            for (const channel of FACET_CHANNELS) {
                for (const headerType of HEADER_TYPES) {
                    const layoutHeaderComponent = this.component.layoutHeaders[channel];
                    const headerComponent = layoutHeaderComponent[headerType];
                    const { facetFieldDef } = layoutHeaderComponent;
                    if (facetFieldDef) {
                        const titleOrient = getHeaderProperty('titleOrient', facetFieldDef, this.config, channel);
                        if (contains(['right', 'bottom'], titleOrient)) {
                            const headerChannel = getHeaderChannel(channel, titleOrient);
                            layoutMixins.titleAnchor = layoutMixins.titleAnchor || {};
                            layoutMixins.titleAnchor[headerChannel] = 'end';
                        }
                    }
                    if (headerComponent && headerComponent[0]) {
                        // set header/footerBand
                        const sizeType = channel === 'row' ? 'height' : 'width';
                        const bandType = headerType === 'header' ? 'headerBand' : 'footerBand';
                        if (channel !== 'facet' && !this.child.component.layoutSize.get(sizeType)) {
                            // If facet child does not have size signal, then apply headerBand
                            layoutMixins[bandType] = layoutMixins[bandType] || {};
                            layoutMixins[bandType][channel] = 0.5;
                        }
                        if (layoutHeaderComponent.title) {
                            layoutMixins.offset = layoutMixins.offset || {};
                            layoutMixins.offset[channel === 'row' ? 'rowTitle' : 'columnTitle'] = 10;
                        }
                    }
                }
            }
            return layoutMixins;
        }
        assembleDefaultLayout() {
            const { column, row } = this.facet;
            const columns = column ? this.columnDistinctSignal() : row ? 1 : undefined;
            let align = 'all';
            // Do not align the cells if the scale corresponding to the direction is indepent.
            // We always align when we facet into both row and column.
            if (!row && this.component.resolve.scale.x === 'independent') {
                align = 'none';
            }
            else if (!column && this.component.resolve.scale.y === 'independent') {
                align = 'none';
            }
            return Object.assign({}, this.getHeaderLayoutMixins(), (columns ? { columns } : {}), { bounds: 'full', align });
        }
        assembleLayoutSignals() {
            // FIXME(https://github.com/vega/vega-lite/issues/1193): this can be incorrect if we have independent scales.
            return this.child.assembleLayoutSignals();
        }
        columnDistinctSignal() {
            if (this.parent && this.parent instanceof FacetModel) {
                // For nested facet, we will add columns to group mark instead
                // See discussion in https://github.com/vega/vega/issues/952
                // and https://github.com/vega/vega-view/releases/tag/v1.2.6
                return undefined;
            }
            else {
                // In facetNode.assemble(), the name is always this.getName('column') + '_layout'.
                const facetLayoutDataName = this.getName('column_domain');
                return { signal: `length(data('${facetLayoutDataName}'))` };
            }
        }
        assembleGroup(signals) {
            if (this.parent && this.parent instanceof FacetModel) {
                // Provide number of columns for layout.
                // See discussion in https://github.com/vega/vega/issues/952
                // and https://github.com/vega/vega-view/releases/tag/v1.2.6
                return Object.assign({}, (this.channelHasField('column')
                    ? {
                        encode: {
                            update: {
                                // TODO(https://github.com/vega/vega-lite/issues/2759):
                                // Correct the signal for facet of concat of facet_column
                                columns: { field: vgField(this.facet.column, { prefix: 'distinct' }) }
                            }
                        }
                    }
                    : {}), super.assembleGroup(signals));
            }
            return super.assembleGroup(signals);
        }
        /**
         * Aggregate cardinality for calculating size
         */
        getCardinalityAggregateForChild() {
            const fields = [];
            const ops = [];
            const as = [];
            if (this.child instanceof FacetModel) {
                if (this.child.channelHasField('column')) {
                    const field = vgField(this.child.facet.column);
                    fields.push(field);
                    ops.push('distinct');
                    as.push(`distinct_${field}`);
                }
            }
            else {
                for (const channel of ['x', 'y']) {
                    const childScaleComponent = this.child.component.scales[channel];
                    if (childScaleComponent && !childScaleComponent.merged) {
                        const type = childScaleComponent.get('type');
                        const range = childScaleComponent.get('range');
                        if (hasDiscreteDomain(type) && isVgRangeStep(range)) {
                            const domain = assembleDomain(this.child, channel);
                            const field = getFieldFromDomain(domain);
                            if (field) {
                                fields.push(field);
                                ops.push('distinct');
                                as.push(`distinct_${field}`);
                            }
                            else {
                                warn('Unknown field for ${channel}.  Cannot calculate view size.');
                            }
                        }
                    }
                }
            }
            return { fields, ops, as };
        }
        assembleFacet() {
            const { name, data } = this.component.data.facetRoot;
            const { row, column } = this.facet;
            const { fields, ops, as } = this.getCardinalityAggregateForChild();
            const groupby = [];
            for (const channel of FACET_CHANNELS) {
                const fieldDef = this.facet[channel];
                if (fieldDef) {
                    groupby.push(vgField(fieldDef));
                    const { bin, sort } = fieldDef;
                    if (isBinning(bin)) {
                        groupby.push(vgField(fieldDef, { binSuffix: 'end' }));
                    }
                    if (isSortField(sort)) {
                        const { field, op = DEFAULT_SORT_OP } = sort;
                        const outputName = facetSortFieldName(fieldDef, sort);
                        if (row && column) {
                            // For crossed facet, use pre-calculate field as it requires a different groupby
                            // For each calculated field, apply max and assign them to the same name as
                            // all values of the same group should be the same anyway.
                            fields.push(outputName);
                            ops.push('max');
                            as.push(outputName);
                        }
                        else {
                            fields.push(field);
                            ops.push(op);
                            as.push(outputName);
                        }
                    }
                    else if (isArray(sort)) {
                        const outputName = sortArrayIndexField(fieldDef, channel);
                        fields.push(outputName);
                        ops.push('max');
                        as.push(outputName);
                    }
                }
            }
            const cross = !!row && !!column;
            return Object.assign({ name,
                data,
                groupby }, (cross || fields.length
                ? {
                    aggregate: Object.assign({}, (cross ? { cross } : {}), (fields.length ? { fields, ops, as } : {}))
                }
                : {}));
        }
        facetSortFields(channel) {
            const { facet } = this;
            const fieldDef = facet[channel];
            if (fieldDef) {
                if (isSortField(fieldDef.sort)) {
                    return [facetSortFieldName(fieldDef, fieldDef.sort, { expr: 'datum' })];
                }
                else if (isArray(fieldDef.sort)) {
                    return [sortArrayIndexField(fieldDef, channel, { expr: 'datum' })];
                }
                return [vgField(fieldDef, { expr: 'datum' })];
            }
            return [];
        }
        facetSortOrder(channel) {
            const { facet } = this;
            const fieldDef = facet[channel];
            if (fieldDef) {
                const { sort } = fieldDef;
                const order = (isSortField(sort) ? sort.order : !isArray(sort) && sort) || 'ascending';
                return [order];
            }
            return [];
        }
        assembleLabelTitle() {
            const { facet, config } = this;
            if (facet.facet) {
                // Facet always uses title to display labels
                return assembleLabelTitle(facet.facet, 'facet', config);
            }
            const ORTHOGONAL_ORIENT = {
                row: ['top', 'bottom'],
                column: ['left', 'right']
            };
            for (const channel of HEADER_CHANNELS) {
                if (facet[channel]) {
                    const labelOrient = getHeaderProperty('labelOrient', facet[channel], config, channel);
                    if (contains(ORTHOGONAL_ORIENT[channel], labelOrient)) {
                        // Row/Column with orthogonal labelOrient must use title to display labels
                        return assembleLabelTitle(facet[channel], channel, config);
                    }
                }
            }
            return undefined;
        }
        assembleMarks() {
            const { child } = this;
            // If we facet by two dimensions, we need to add a cross operator to the aggregation
            // so that we create all groups
            const facetRoot = this.component.data.facetRoot;
            const data = assembleFacetData(facetRoot);
            const encodeEntry = child.assembleGroupEncodeEntry(false);
            const title = this.assembleLabelTitle() || child.assembleTitle();
            const style = child.assembleGroupStyle();
            const markGroup = Object.assign({ name: this.getName('cell'), type: 'group' }, (title ? { title } : {}), (style ? { style } : {}), { from: {
                    facet: this.assembleFacet()
                }, 
                // TODO: move this to after data
                sort: {
                    field: flatten(FACET_CHANNELS.map(c => this.facetSortFields(c))),
                    order: flatten(FACET_CHANNELS.map(c => this.facetSortOrder(c)))
                } }, (data.length > 0 ? { data: data } : {}), (encodeEntry ? { encode: { update: encodeEntry } } : {}), child.assembleGroup(assembleFacetSignals(this, [])));
            return [markGroup];
        }
        getMapping() {
            return this.facet;
        }
    }

    function makeJoinAggregateFromFacet(parent, facet) {
        const { row, column } = facet;
        if (row && column) {
            let newParent = null;
            // only need to make one for crossed facet
            for (const fieldDef of [row, column]) {
                if (isSortField(fieldDef.sort)) {
                    const { field, op = DEFAULT_SORT_OP } = fieldDef.sort;
                    parent = newParent = new JoinAggregateTransformNode(parent, {
                        joinaggregate: [
                            {
                                op,
                                field,
                                as: facetSortFieldName(fieldDef, fieldDef.sort, { forAs: true })
                            }
                        ],
                        groupby: [vgField(fieldDef)]
                    });
                }
            }
            return newParent;
        }
        return null;
    }

    function findSource(data, sources) {
        for (const other of sources) {
            const otherData = other.data;
            // if both datasets have a name defined, we cannot merge
            if (data.name && other.hasName() && data.name !== other.dataName) {
                continue;
            }
            // feature and mesh are mutually exclusive
            if (data['format'] && data['format'].mesh && otherData.format && otherData.format.feature) {
                continue;
            }
            if (isInlineData(data) && isInlineData(otherData)) {
                if (deepEqual(data.values, otherData.values)) {
                    return other;
                }
            }
            else if (isUrlData(data) && isUrlData(otherData)) {
                if (data.url === otherData.url) {
                    return other;
                }
            }
            else if (isNamedData(data)) {
                if (data.name === other.dataName) {
                    return other;
                }
            }
        }
        return null;
    }
    function parseRoot(model, sources) {
        if (model.data || !model.parent) {
            // if the model defines a data source or is the root, create a source node
            const existingSource = findSource(model.data, sources);
            if (existingSource) {
                if (!isGenerator(model.data)) {
                    existingSource.data.format = mergeDeep({}, model.data.format, existingSource.data.format);
                }
                // if the new source has a name but the existing one does not, we can set it
                if (!existingSource.hasName() && model.data.name) {
                    existingSource.dataName = model.data.name;
                }
                return existingSource;
            }
            else {
                const source = new SourceNode(model.data);
                sources.push(source);
                return source;
            }
        }
        else {
            // If we don't have a source defined (overriding parent's data), use the parent's facet root or main.
            return model.parent.component.data.facetRoot
                ? model.parent.component.data.facetRoot
                : model.parent.component.data.main;
        }
    }
    /**
     * Parses a transform array into a chain of connected dataflow nodes.
     */
    function parseTransformArray(head, model, ancestorParse) {
        let lookupCounter = 0;
        for (const t of model.transforms) {
            let derivedType = undefined;
            let transformNode;
            if (isCalculate(t)) {
                transformNode = head = new CalculateNode(head, t);
                derivedType = 'derived';
            }
            else if (isFilter(t)) {
                transformNode = head = ParseNode.makeImplicitFromFilterTransform(head, t, ancestorParse) || head;
                head = new FilterNode(head, model, t.filter);
            }
            else if (isBin(t)) {
                transformNode = head = BinNode.makeFromTransform(head, t, model);
                derivedType = 'number';
            }
            else if (isTimeUnit(t)) {
                transformNode = head = TimeUnitNode.makeFromTransform(head, t);
                derivedType = 'date';
                // Create parse node because the input to time unit is always date.
                const parsedAs = ancestorParse.getWithExplicit(t.field);
                if (parsedAs.value === undefined) {
                    head = new ParseNode(head, { [t.field]: derivedType });
                    ancestorParse.set(t.field, derivedType, false);
                }
            }
            else if (isAggregate$1(t)) {
                transformNode = head = AggregateNode.makeFromTransform(head, t);
                derivedType = 'number';
                if (requiresSelectionId(model)) {
                    head = new IdentifierNode(head);
                }
            }
            else if (isLookup(t)) {
                transformNode = head = LookupNode.make(head, model, t, lookupCounter++);
                derivedType = 'derived';
            }
            else if (isWindow(t)) {
                transformNode = head = new WindowTransformNode(head, t);
                derivedType = 'number';
            }
            else if (isJoinAggregate(t)) {
                transformNode = head = new JoinAggregateTransformNode(head, t);
                derivedType = 'number';
            }
            else if (isStack(t)) {
                transformNode = head = StackNode.makeFromTransform(head, t);
                derivedType = 'derived';
            }
            else if (isFold(t)) {
                transformNode = head = new FoldTransformNode(head, t);
                derivedType = 'derived';
            }
            else if (isFlatten(t)) {
                transformNode = head = new FlattenTransformNode(head, t);
                derivedType = 'derived';
            }
            else if (isSample(t)) {
                head = new SampleTransformNode(head, t);
            }
            else if (isImpute(t)) {
                transformNode = head = ImputeNode.makeFromTransform(head, t);
                derivedType = 'derived';
            }
            else {
                warn(message.invalidTransformIgnored(t));
                continue;
            }
            if (transformNode && derivedType !== undefined) {
                for (const field of transformNode.producedFields()) {
                    ancestorParse.set(field, derivedType, false);
                }
            }
        }
        return head;
    }
    /*
    Description of the dataflow (http://asciiflow.com/):
         +--------+
         | Source |
         +---+----+
             |
             v
         FormatParse
         (explicit)
             |
             v
         Transforms
    (Filter, Calculate, Binning, TimeUnit, Aggregate, Window, ...)
             |
             v
         FormatParse
         (implicit)
             |
             v
     Binning (in `encoding`)
             |
             v
     Timeunit (in `encoding`)
             |
             v
    Formula From Sort Array
             |
             v
          +--+--+
          | Raw |
          +-----+
             |
             v
      Aggregate (in `encoding`)
             |
             v
      Stack (in `encoding`)
             |
             v
      Invalid Filter
             |
             v
       +----------+
       |   Main   |
       +----------+
             |
             v
         +-------+
         | Facet |----> "column", "column-layout", and "row"
         +-------+
             |
             v
      ...Child data...
    */
    function parseData(model) {
        let head = parseRoot(model, model.component.data.sources);
        const { outputNodes, outputNodeRefCounts } = model.component.data;
        const ancestorParse = model.parent ? model.parent.component.data.ancestorParse.clone() : new AncestorParse();
        const data = model.data;
        if (isGenerator(data)) {
            // insert generator transform
            if (isSequenceGenerator(data)) {
                head = new SequenceNode(head, data.sequence);
            }
            else if (isGraticuleGenerator(data)) {
                head = new GraticuleNode(head, data.graticule);
            }
            // no parsing necessary for generator
            ancestorParse.parseNothing = true;
        }
        else if (data && data.format && data.format.parse === null) {
            // format.parse: null means disable parsing
            ancestorParse.parseNothing = true;
        }
        head = ParseNode.makeExplicit(head, model, ancestorParse) || head;
        // Default discrete selections require an identifier transform to
        // uniquely identify data points as the _id field is volatile. Add
        // this transform at the head of our pipeline such that the identifier
        // field is available for all subsequent datasets. Additional identifier
        // transforms will be necessary when new tuples are constructed
        // (e.g., post-aggregation).
        if (requiresSelectionId(model) &&
            // only add identifier to unit/layer models that do not have layer parents to avoid redundant identifier transforms
            ((isUnitModel(model) || isLayerModel(model)) && (!model.parent || !isLayerModel(model.parent)))) {
            head = new IdentifierNode(head);
        }
        // HACK: This is equivalent for merging bin extent for union scale.
        // FIXME(https://github.com/vega/vega-lite/issues/2270): Correctly merge extent / bin node for shared bin scale
        const parentIsLayer = model.parent && isLayerModel(model.parent);
        if (isUnitModel(model) || isFacetModel(model)) {
            if (parentIsLayer) {
                head = BinNode.makeFromEncoding(head, model) || head;
            }
        }
        if (model.transforms.length > 0) {
            head = parseTransformArray(head, model, ancestorParse);
        }
        head = ParseNode.makeImplicitFromEncoding(head, model, ancestorParse) || head;
        if (isUnitModel(model)) {
            head = GeoJSONNode.parseAll(head, model);
            head = GeoPointNode.parseAll(head, model);
        }
        if (isUnitModel(model) || isFacetModel(model)) {
            if (!parentIsLayer) {
                head = BinNode.makeFromEncoding(head, model) || head;
            }
            head = TimeUnitNode.makeFromEncoding(head, model) || head;
            head = CalculateNode.parseAllForSortIndex(head, model);
        }
        // add an output node pre aggregation
        const rawName = model.getName(RAW);
        const raw = new OutputNode(head, rawName, RAW, outputNodeRefCounts);
        outputNodes[rawName] = raw;
        head = raw;
        if (isUnitModel(model)) {
            const agg = AggregateNode.makeFromEncoding(head, model);
            if (agg) {
                head = agg;
                if (requiresSelectionId(model)) {
                    head = new IdentifierNode(head);
                }
            }
            head = ImputeNode.makeFromEncoding(head, model) || head;
            head = StackNode.makeFromEncoding(head, model) || head;
        }
        if (isUnitModel(model)) {
            head = FilterInvalidNode.make(head, model) || head;
        }
        // output node for marks
        const mainName = model.getName(MAIN);
        const main = new OutputNode(head, mainName, MAIN, outputNodeRefCounts);
        outputNodes[mainName] = main;
        head = main;
        // add facet marker
        let facetRoot = null;
        if (isFacetModel(model)) {
            const facetName = model.getName('facet');
            // Derive new sort index field for facet's sort array
            head = CalculateNode.parseAllForSortIndex(head, model);
            // Derive new aggregate for facet's sort field
            // augment data source with new fields for crossed facet
            head = makeJoinAggregateFromFacet(head, model.facet) || head;
            facetRoot = new FacetNode(head, model, facetName, main.getSource());
            outputNodes[facetName] = facetRoot;
            head = facetRoot;
        }
        return Object.assign({}, model.component.data, { outputNodes,
            outputNodeRefCounts,
            raw,
            main,
            facetRoot,
            ancestorParse });
    }

    class BaseConcatModel extends Model {
        constructor(spec, specType, parent, parentGivenName, config, repeater, resolve) {
            super(spec, specType, parent, parentGivenName, config, repeater, resolve);
        }
        parseData() {
            this.component.data = parseData(this);
            this.children.forEach(child => {
                child.parseData();
            });
        }
        parseSelections() {
            // Merge selections up the hierarchy so that they may be referenced
            // across unit specs. Persist their definitions within each child
            // to assemble signals which remain within output Vega unit groups.
            this.component.selection = {};
            for (const child of this.children) {
                child.parseSelections();
                keys(child.component.selection).forEach(key => {
                    this.component.selection[key] = child.component.selection[key];
                });
            }
        }
        parseMarkGroup() {
            for (const child of this.children) {
                child.parseMarkGroup();
            }
        }
        parseAxesAndHeaders() {
            for (const child of this.children) {
                child.parseAxesAndHeaders();
            }
            // TODO(#2415): support shared axes
        }
        assembleSelectionTopLevelSignals(signals) {
            return this.children.reduce((sg, child) => child.assembleSelectionTopLevelSignals(sg), signals);
        }
        assembleSignals() {
            this.children.forEach(child => child.assembleSignals());
            return [];
        }
        assembleLayoutSignals() {
            return this.children.reduce((signals, child) => {
                return [...signals, ...child.assembleLayoutSignals()];
            }, assembleLayoutSignals(this));
        }
        assembleSelectionData(data) {
            return this.children.reduce((db, child) => child.assembleSelectionData(db), data);
        }
        assembleMarks() {
            // only children have marks
            return this.children.map(child => {
                const title = child.assembleTitle();
                const style = child.assembleGroupStyle();
                const encodeEntry = child.assembleGroupEncodeEntry(false);
                return Object.assign({ type: 'group', name: child.getName('group') }, (title ? { title } : {}), (style ? { style } : {}), (encodeEntry ? { encode: { update: encodeEntry } } : {}), child.assembleGroup());
            });
        }
    }

    class ConcatModel extends BaseConcatModel {
        constructor(spec, parent, parentGivenName, repeater, config) {
            super(spec, 'concat', parent, parentGivenName, config, repeater, spec.resolve);
            if (spec.resolve && spec.resolve.axis && (spec.resolve.axis.x === 'shared' || spec.resolve.axis.y === 'shared')) {
                warn(message.CONCAT_CANNOT_SHARE_AXIS);
            }
            this.concatType = isVConcatSpec(spec) ? 'vconcat' : isHConcatSpec(spec) ? 'hconcat' : 'concat';
            this.children = this.getChildren(spec).map((child, i) => {
                return buildModel(child, this, this.getName('concat_' + i), undefined, repeater, config, false);
            });
        }
        getChildren(spec) {
            if (isVConcatSpec(spec)) {
                return spec.vconcat;
            }
            else if (isHConcatSpec(spec)) {
                return spec.hconcat;
            }
            return spec.concat;
        }
        parseLayoutSize() {
            parseConcatLayoutSize(this);
        }
        parseAxisGroup() {
            return null;
        }
        assembleDefaultLayout() {
            return Object.assign({}, (this.concatType === 'vconcat' ? { columns: 1 } : {}), { bounds: 'full', 
                // Use align each so it can work with multiple plots with different size
                align: 'each' });
        }
    }

    function isFalseOrNull(v) {
        return v === false || v === null;
    }
    class AxisComponent extends Split {
        constructor(explicit = {}, implicit = {}, mainExtracted = false) {
            super();
            this.explicit = explicit;
            this.implicit = implicit;
            this.mainExtracted = mainExtracted;
        }
        clone() {
            return new AxisComponent(duplicate(this.explicit), duplicate(this.implicit), this.mainExtracted);
        }
        hasAxisPart(part) {
            // FIXME(https://github.com/vega/vega-lite/issues/2552) this method can be wrong if users use a Vega theme.
            if (part === 'axis') {
                // always has the axis container part
                return true;
            }
            if (part === 'grid' || part === 'title') {
                return !!this.get(part);
            }
            // Other parts are enabled by default, so they should not be false or null.
            return !isFalseOrNull(this.get(part));
        }
    }

    function labels$1(model, channel, specifiedLabelsSpec) {
        const fieldDef = model.fieldDef(channel) ||
            (channel === 'x' ? model.fieldDef('x2') : channel === 'y' ? model.fieldDef('y2') : undefined);
        const axis = model.axis(channel);
        const config = model.config;
        let labelsSpec = {};
        // We use a label encoding instead of setting the `format` property because Vega does not let us determine how the format should be interpreted.
        if (isTimeFormatFieldDef(fieldDef)) {
            const isUTCScale = model.getScaleComponent(channel).get('type') === ScaleType.UTC;
            const expr = timeFormatExpression('datum.value', fieldDef.timeUnit, axis.format, config.axis.shortTimeLabels, null, isUTCScale);
            if (expr) {
                labelsSpec.text = { signal: expr };
            }
        }
        labelsSpec = Object.assign({}, labelsSpec, specifiedLabelsSpec);
        return keys(labelsSpec).length === 0 ? undefined : labelsSpec;
    }

    function parseUnitAxes(model) {
        return POSITION_SCALE_CHANNELS.reduce((axis, channel) => {
            if (model.component.scales[channel] && model.axis(channel)) {
                axis[channel] = [parseAxis(channel, model)];
            }
            return axis;
        }, {});
    }
    const OPPOSITE_ORIENT = {
        bottom: 'top',
        top: 'bottom',
        left: 'right',
        right: 'left'
    };
    function parseLayerAxes(model) {
        const { axes, resolve } = model.component;
        const axisCount = { top: 0, bottom: 0, right: 0, left: 0 };
        for (const child of model.children) {
            child.parseAxesAndHeaders();
            for (const channel of keys(child.component.axes)) {
                resolve.axis[channel] = parseGuideResolve(model.component.resolve, channel);
                if (resolve.axis[channel] === 'shared') {
                    // If the resolve says shared (and has not been overridden)
                    // We will try to merge and see if there is a conflict
                    axes[channel] = mergeAxisComponents(axes[channel], child.component.axes[channel]);
                    if (!axes[channel]) {
                        // If merge returns nothing, there is a conflict so we cannot make the axis shared.
                        // Thus, mark axis as independent and remove the axis component.
                        resolve.axis[channel] = 'independent';
                        delete axes[channel];
                    }
                }
            }
        }
        // Move axes to layer's axis component and merge shared axes
        for (const channel of [X, Y]) {
            for (const child of model.children) {
                if (!child.component.axes[channel]) {
                    // skip if the child does not have a particular axis
                    continue;
                }
                if (resolve.axis[channel] === 'independent') {
                    // If axes are independent, concat the axisComponent array.
                    axes[channel] = (axes[channel] || []).concat(child.component.axes[channel]);
                    // Automatically adjust orient
                    for (const axisComponent of child.component.axes[channel]) {
                        const { value: orient, explicit } = axisComponent.getWithExplicit('orient');
                        if (axisCount[orient] > 0 && !explicit) {
                            // Change axis orient if the number do not match
                            const oppositeOrient = OPPOSITE_ORIENT[orient];
                            if (axisCount[orient] > axisCount[oppositeOrient]) {
                                axisComponent.set('orient', oppositeOrient, false);
                            }
                        }
                        axisCount[orient]++;
                        // TODO(https://github.com/vega/vega-lite/issues/2634): automatically add extra offset?
                    }
                }
                // After merging, make sure to remove axes from child
                delete child.component.axes[channel];
            }
            // Suppress grid lines for dual axis charts (https://github.com/vega/vega-lite/issues/4676)
            if (resolve.axis[channel] === 'independent' && axes[channel] && axes[channel].length > 1) {
                for (const axisCmpt of axes[channel]) {
                    if (!!axisCmpt.get('grid') && !axisCmpt.explicit.grid) {
                        axisCmpt.implicit.grid = false;
                    }
                }
            }
        }
    }
    function mergeAxisComponents(mergedAxisCmpts, childAxisCmpts) {
        if (mergedAxisCmpts) {
            // FIXME: this is a bit wrong once we support multiple axes
            if (mergedAxisCmpts.length !== childAxisCmpts.length) {
                return undefined; // Cannot merge axis component with different number of axes.
            }
            const length = mergedAxisCmpts.length;
            for (let i = 0; i < length; i++) {
                const merged = mergedAxisCmpts[i];
                const child = childAxisCmpts[i];
                if (!!merged !== !!child) {
                    return undefined;
                }
                else if (merged && child) {
                    const mergedOrient = merged.getWithExplicit('orient');
                    const childOrient = child.getWithExplicit('orient');
                    if (mergedOrient.explicit && childOrient.explicit && mergedOrient.value !== childOrient.value) {
                        // TODO: throw warning if resolve is explicit (We don't have info about explicit/implicit resolve yet.)
                        // Cannot merge due to inconsistent orient
                        return undefined;
                    }
                    else {
                        mergedAxisCmpts[i] = mergeAxisComponent(merged, child);
                    }
                }
            }
        }
        else {
            // For first one, return a copy of the child
            return childAxisCmpts.map(axisComponent => axisComponent.clone());
        }
        return mergedAxisCmpts;
    }
    function mergeAxisComponent(merged, child) {
        for (const prop of VG_AXIS_PROPERTIES) {
            const mergedValueWithExplicit = mergeValuesWithExplicit(merged.getWithExplicit(prop), child.getWithExplicit(prop), prop, 'axis', 
            // Tie breaker function
            (v1, v2) => {
                switch (prop) {
                    case 'title':
                        return mergeTitleComponent(v1, v2);
                    case 'gridScale':
                        return {
                            explicit: v1.explicit,
                            value: getFirstDefined(v1.value, v2.value)
                        };
                }
                return defaultTieBreaker(v1, v2, prop, 'axis');
            });
            merged.setWithExplicit(prop, mergedValueWithExplicit);
        }
        return merged;
    }
    function getFieldDefTitle(model, channel) {
        const channel2 = channel === 'x' ? 'x2' : 'y2';
        const fieldDef = model.fieldDef(channel);
        const fieldDef2 = model.fieldDef(channel2);
        const title1 = fieldDef ? fieldDef.title : undefined;
        const title2 = fieldDef2 ? fieldDef2.title : undefined;
        if (title1 && title2) {
            return mergeTitle(title1, title2);
        }
        else if (title1) {
            return title1;
        }
        else if (title2) {
            return title2;
        }
        else if (title1 !== undefined) {
            // falsy value to disable config
            return title1;
        }
        else if (title2 !== undefined) {
            // falsy value to disable config
            return title2;
        }
        return undefined;
    }
    function isExplicit$1(value, property, axis, model, channel) {
        switch (property) {
            case 'titleAngle':
            case 'labelAngle':
                return value === normalizeAngle(axis[property]);
            case 'values':
                return !!axis.values;
            // specified axis.values is already respected, but may get transformed.
            case 'encode':
                // both VL axis.encoding and axis.labelAngle affect VG axis.encode
                return !!axis.encoding || !!axis.labelAngle;
            case 'title':
                // title can be explicit if fieldDef.title is set
                if (value === getFieldDefTitle(model, channel)) {
                    return true;
                }
        }
        // Otherwise, things are explicit if the returned value matches the specified property
        return value === axis[property];
    }
    function parseAxis(channel, model) {
        const axis = model.axis(channel);
        const axisComponent = new AxisComponent();
        // 1.2. Add properties
        VG_AXIS_PROPERTIES.forEach(property => {
            const value = getProperty$1(property, axis, channel, model);
            if (value !== undefined) {
                const explicit = isExplicit$1(value, property, axis, model, channel);
                const configValue = getAxisConfig(property, model.config, channel, axisComponent.get('orient'), model.getScaleComponent(channel).get('type'));
                // only set property if it is explicitly set or has no config value (otherwise we will accidentally override config)
                if (explicit || configValue === undefined) {
                    // Do not apply implicit rule if there is a config value
                    axisComponent.set(property, value, explicit);
                }
                else if (contains(['grid', 'orient'], property) && configValue) {
                    // - Grid is an exception because we need to set grid = true to generate another grid axis
                    // - Orient is not an axis config in Vega, so we need to set too.
                    axisComponent.set(property, configValue, false);
                }
            }
        });
        // 2) Add guide encode definition groups
        const axisEncoding = axis.encoding || {};
        const axisEncode = AXIS_PARTS.reduce((e, part) => {
            if (!axisComponent.hasAxisPart(part)) {
                // No need to create encode for a disabled part.
                return e;
            }
            const axisEncodingPart = guideEncodeEntry(axisEncoding[part] || {}, model);
            const value = part === 'labels' ? labels$1(model, channel, axisEncodingPart) : axisEncodingPart;
            if (value !== undefined && keys(value).length > 0) {
                e[part] = { update: value };
            }
            return e;
        }, {});
        // FIXME: By having encode as one property, we won't have fine grained encode merging.
        if (keys(axisEncode).length > 0) {
            axisComponent.set('encode', axisEncode, !!axis.encoding || axis.labelAngle !== undefined);
        }
        return axisComponent;
    }
    function getProperty$1(property, specifiedAxis, channel, model) {
        const fieldDef = model.fieldDef(channel);
        // Some properties depend on labelAngle so we have to declare it here.
        // Also, we don't use `getFirstDefined` for labelAngle
        // as we want to normalize specified value to be within [0,360)
        const labelAngle$1 = labelAngle(model, specifiedAxis, channel, fieldDef);
        const orient$1 = getFirstDefined(specifiedAxis.orient, orient(channel));
        switch (property) {
            case 'scale':
                return model.scaleName(channel);
            case 'gridScale':
                return gridScale(model, channel);
            case 'format':
                // We don't include temporal field here as we apply format in encode block
                if (isTimeFormatFieldDef(fieldDef)) {
                    return undefined;
                }
                return numberFormat(fieldDef, specifiedAxis.format, model.config);
            case 'formatType':
                // Same as format, We don't include temporal field here as we apply format in encode block
                if (isTimeFormatFieldDef(fieldDef)) {
                    return undefined;
                }
                return specifiedAxis.formatType;
            case 'grid': {
                if (isBinned(model.fieldDef(channel).bin)) {
                    return false;
                }
                else {
                    const scaleType = model.getScaleComponent(channel).get('type');
                    return getFirstDefined(specifiedAxis.grid, defaultGrid(scaleType, fieldDef));
                }
            }
            case 'labelAlign':
                return getFirstDefined(specifiedAxis.labelAlign, defaultLabelAlign(labelAngle$1, orient$1));
            case 'labelAngle':
                return labelAngle$1;
            case 'labelBaseline':
                return getFirstDefined(specifiedAxis.labelBaseline, defaultLabelBaseline(labelAngle$1, orient$1));
            case 'labelFlush':
                return getFirstDefined(specifiedAxis.labelFlush, defaultLabelFlush(fieldDef, channel));
            case 'labelOverlap': {
                const scaleType = model.getScaleComponent(channel).get('type');
                return getFirstDefined(specifiedAxis.labelOverlap, defaultLabelOverlap(fieldDef, scaleType));
            }
            case 'orient':
                return orient$1;
            case 'tickCount': {
                const scaleType = model.getScaleComponent(channel).get('type');
                const sizeType = channel === 'x' ? 'width' : channel === 'y' ? 'height' : undefined;
                const size = sizeType ? model.getSizeSignalRef(sizeType) : undefined;
                return getFirstDefined(specifiedAxis.tickCount, defaultTickCount({ fieldDef, scaleType, size }));
            }
            case 'title': {
                const channel2 = channel === 'x' ? 'x2' : 'y2';
                const fieldDef2 = model.fieldDef(channel2);
                // Keep undefined so we use default if title is unspecified.
                // For other falsy value, keep them so we will hide the title.
                return getFirstDefined(specifiedAxis.title, getFieldDefTitle(model, channel), // If title not specified, store base parts of fieldDef (and fieldDef2 if exists)
                mergeTitleFieldDefs([toFieldDefBase(fieldDef)], fieldDef2 ? [toFieldDefBase(fieldDef2)] : []));
            }
            case 'values':
                return values(specifiedAxis, model, fieldDef);
        }
        // Otherwise, return specified property.
        return isAxisProperty(property) ? specifiedAxis[property] : undefined;
    }

    function normalizeMarkDef(mark, encoding, config) {
        const markDef = isMarkDef(mark) ? Object.assign({}, mark) : { type: mark };
        // set orient, which can be overridden by rules as sometimes the specified orient is invalid.
        const specifiedOrient = markDef.orient || getMarkConfig('orient', markDef, config);
        markDef.orient = orient$1(markDef.type, encoding, specifiedOrient);
        if (specifiedOrient !== undefined && specifiedOrient !== markDef.orient) {
            warn(message.orientOverridden(markDef.orient, specifiedOrient));
        }
        // set opacity and filled if not specified in mark config
        const specifiedOpacity = getFirstDefined(markDef.opacity, getMarkConfig('opacity', markDef, config));
        if (specifiedOpacity === undefined) {
            markDef.opacity = opacity(markDef.type, encoding);
        }
        const specifiedFilled = markDef.filled;
        if (specifiedFilled === undefined) {
            markDef.filled = filled(markDef, config);
        }
        // set cursor, which should be pointer if href channel is present unless otherwise specified
        const specifiedCursor = markDef.cursor || getMarkConfig('cursor', markDef, config);
        if (specifiedCursor === undefined) {
            markDef.cursor = cursor(markDef, encoding, config);
        }
        return markDef;
    }
    function cursor(markDef, encoding, config) {
        if (encoding.href || markDef.href || getMarkConfig('href', markDef, config)) {
            return 'pointer';
        }
        return markDef.cursor;
    }
    function opacity(mark, encoding) {
        if (contains([POINT, TICK, CIRCLE, SQUARE], mark)) {
            // point-based marks
            if (!isAggregate(encoding)) {
                return 0.7;
            }
        }
        return undefined;
    }
    function filled(markDef, config) {
        const filledConfig = getMarkConfig('filled', markDef, config);
        const mark = markDef.type;
        return getFirstDefined(filledConfig, mark !== POINT && mark !== LINE && mark !== RULE);
    }
    function orient$1(mark, encoding, specifiedOrient) {
        switch (mark) {
            case POINT:
            case CIRCLE:
            case SQUARE:
            case TEXT:
            case RECT:
                // orient is meaningless for these marks.
                return undefined;
        }
        const { x, y, x2, y2 } = encoding;
        switch (mark) {
            case BAR:
                if (isFieldDef(x) && isBinned(x.bin)) {
                    return 'vertical';
                }
                if (isFieldDef(y) && isBinned(y.bin)) {
                    return 'horizontal';
                }
                if (y2 || x2) {
                    // Ranged bar does not always have clear orientation, so we allow overriding
                    if (specifiedOrient) {
                        return specifiedOrient;
                    }
                    // If y is range and x is non-range, non-bin Q, y is likely a prebinned field
                    if (!x2 && isFieldDef(x) && x.type === QUANTITATIVE && !isBinning(x.bin)) {
                        return 'horizontal';
                    }
                    // If x is range and y is non-range, non-bin Q, x is likely a prebinned field
                    if (!y2 && isFieldDef(y) && y.type === QUANTITATIVE && !isBinning(y.bin)) {
                        return 'vertical';
                    }
                }
            // falls through
            case RULE:
                // return undefined for line segment rule and bar with both axis ranged
                if (x2 && y2) {
                    return undefined;
                }
            // falls through
            case AREA:
                // If there are range for both x and y, y (vertical) has higher precedence.
                if (y2) {
                    if (isFieldDef(y) && isBinned(y.bin)) {
                        return 'horizontal';
                    }
                    else {
                        return 'vertical';
                    }
                }
                else if (x2) {
                    if (isFieldDef(x) && isBinned(x.bin)) {
                        return 'vertical';
                    }
                    else {
                        return 'horizontal';
                    }
                }
                else if (mark === RULE) {
                    if (encoding.x && !encoding.y) {
                        return 'vertical';
                    }
                    else if (encoding.y && !encoding.x) {
                        return 'horizontal';
                    }
                }
            // falls through
            case LINE:
            case TICK: {
                // Tick is opposite to bar, line, area and never have ranged mark.
                const xIsContinuous = isFieldDef(encoding.x) && isContinuous(encoding.x);
                const yIsContinuous = isFieldDef(encoding.y) && isContinuous(encoding.y);
                if (xIsContinuous && !yIsContinuous) {
                    return mark !== 'tick' ? 'horizontal' : 'vertical';
                }
                else if (!xIsContinuous && yIsContinuous) {
                    return mark !== 'tick' ? 'vertical' : 'horizontal';
                }
                else if (xIsContinuous && yIsContinuous) {
                    const xDef = encoding.x; // we can cast here since they are surely fieldDef
                    const yDef = encoding.y;
                    const xIsTemporal = xDef.type === TEMPORAL;
                    const yIsTemporal = yDef.type === TEMPORAL;
                    // temporal without timeUnit is considered continuous, but better serves as dimension
                    if (xIsTemporal && !yIsTemporal) {
                        return mark !== 'tick' ? 'vertical' : 'horizontal';
                    }
                    else if (!xIsTemporal && yIsTemporal) {
                        return mark !== 'tick' ? 'horizontal' : 'vertical';
                    }
                    if (!xDef.aggregate && yDef.aggregate) {
                        return mark !== 'tick' ? 'vertical' : 'horizontal';
                    }
                    else if (xDef.aggregate && !yDef.aggregate) {
                        return mark !== 'tick' ? 'horizontal' : 'vertical';
                    }
                    if (specifiedOrient) {
                        // When ambiguous, use user specified one.
                        return specifiedOrient;
                    }
                    return 'vertical';
                }
                else {
                    // Discrete x Discrete case
                    if (specifiedOrient) {
                        // When ambiguous, use user specified one.
                        return specifiedOrient;
                    }
                    return undefined;
                }
            }
        }
        return 'vertical';
    }

    function parseUnitSelection(model, selDefs) {
        const selCmpts = {};
        const selectionConfig = model.config.selection;
        if (selDefs) {
            selDefs = duplicate(selDefs); // duplicate to avoid side effects to original spec
        }
        for (let name in selDefs) {
            if (!selDefs.hasOwnProperty(name)) {
                continue;
            }
            const selDef = selDefs[name];
            const _a = selectionConfig[selDef.type], cfg = __rest(_a, ["fields", "encodings"]); // Project transform applies its defaults.
            // Set default values from config if a property hasn't been specified,
            // or if it is true. E.g., "translate": true should use the default
            // event handlers for translate. However, true may be a valid value for
            // a property (e.g., "nearest": true).
            for (const key in cfg) {
                // A selection should contain either `encodings` or `fields`, only use
                // default values for these two values if neither of them is specified.
                if ((key === 'encodings' && selDef.fields) || (key === 'fields' && selDef.encodings)) {
                    continue;
                }
                if (key === 'mark') {
                    selDef[key] = Object.assign({}, cfg[key], selDef[key]);
                }
                if (selDef[key] === undefined || selDef[key] === true) {
                    selDef[key] = cfg[key] || selDef[key];
                }
            }
            name = varName(name);
            const selCmpt = (selCmpts[name] = Object.assign({}, selDef, { name: name, events: isString(selDef.on) ? parseSelector(selDef.on, 'scope') : selDef.on }));
            forEachTransform(selCmpt, txCompiler => {
                if (txCompiler.parse) {
                    txCompiler.parse(model, selDef, selCmpt);
                }
            });
        }
        return selCmpts;
    }

    /**
     * Internal model of Vega-Lite specification for the compiler.
     */
    class UnitModel extends ModelWithField {
        constructor(spec, parent, parentGivenName, parentGivenSize = {}, repeater, config, fit) {
            super(spec, 'unit', parent, parentGivenName, config, repeater, undefined, spec.view);
            this.fit = fit;
            this.specifiedScales = {};
            this.specifiedAxes = {};
            this.specifiedLegends = {};
            this.specifiedProjection = {};
            this.selection = {};
            this.children = [];
            this.initSize(Object.assign({}, parentGivenSize, (spec.width ? { width: spec.width } : {}), (spec.height ? { height: spec.height } : {})));
            const mark = isMarkDef(spec.mark) ? spec.mark.type : spec.mark;
            const encoding = (this.encoding = normalizeEncoding(replaceRepeaterInEncoding(spec.encoding || {}, repeater), mark));
            this.markDef = normalizeMarkDef(spec.mark, encoding, config);
            // calculate stack properties
            this.stack = stack(mark, encoding, this.config.stack);
            this.specifiedScales = this.initScales(mark, encoding);
            this.specifiedAxes = this.initAxes(encoding);
            this.specifiedLegends = this.initLegend(encoding);
            this.specifiedProjection = spec.projection;
            // Selections will be initialized upon parse.
            this.selection = spec.selection;
        }
        get hasProjection() {
            const { encoding } = this;
            const isGeoShapeMark = this.mark === GEOSHAPE;
            const hasGeoPosition = encoding && GEOPOSITION_CHANNELS.some(channel => isFieldDef(encoding[channel]));
            return isGeoShapeMark || hasGeoPosition;
        }
        /**
         * Return specified Vega-lite scale domain for a particular channel
         * @param channel
         */
        scaleDomain(channel) {
            const scale = this.specifiedScales[channel];
            return scale ? scale.domain : undefined;
        }
        axis(channel) {
            return this.specifiedAxes[channel];
        }
        legend(channel) {
            return this.specifiedLegends[channel];
        }
        initScales(mark, encoding) {
            return SCALE_CHANNELS.reduce((scales, channel) => {
                let fieldDef;
                let specifiedScale;
                const channelDef = encoding[channel];
                if (isFieldDef(channelDef)) {
                    fieldDef = channelDef;
                    specifiedScale = channelDef.scale;
                }
                else if (hasConditionalFieldDef(channelDef)) {
                    fieldDef = channelDef.condition;
                    specifiedScale = channelDef.condition['scale'];
                }
                if (fieldDef) {
                    scales[channel] = specifiedScale || {};
                }
                return scales;
            }, {});
        }
        initAxes(encoding) {
            return [X, Y].reduce((_axis, channel) => {
                // Position Axis
                // TODO: handle ConditionFieldDef
                const channelDef = encoding[channel];
                if (isFieldDef(channelDef) ||
                    (channel === X && isFieldDef(encoding.x2)) ||
                    (channel === Y && isFieldDef(encoding.y2))) {
                    const axisSpec = isFieldDef(channelDef) ? channelDef.axis : null;
                    if (axisSpec !== null) {
                        _axis[channel] = Object.assign({}, axisSpec);
                    }
                }
                return _axis;
            }, {});
        }
        initLegend(encoding) {
            return NONPOSITION_SCALE_CHANNELS.reduce((_legend, channel) => {
                const channelDef = encoding[channel];
                if (channelDef) {
                    const legend = isFieldDef(channelDef)
                        ? channelDef.legend
                        : hasConditionalFieldDef(channelDef)
                            ? channelDef.condition['legend']
                            : null;
                    if (legend !== null && legend !== false && supportLegend(channel)) {
                        _legend[channel] = Object.assign({}, legend);
                    }
                }
                return _legend;
            }, {});
        }
        parseData() {
            this.component.data = parseData(this);
        }
        parseLayoutSize() {
            parseUnitLayoutSize(this);
        }
        parseSelections() {
            this.component.selection = parseUnitSelection(this, this.selection);
        }
        parseMarkGroup() {
            this.component.mark = parseMarkGroups(this);
        }
        parseAxesAndHeaders() {
            this.component.axes = parseUnitAxes(this);
        }
        assembleSelectionTopLevelSignals(signals) {
            return assembleTopLevelSignals(this, signals);
        }
        assembleSignals() {
            return [...assembleAxisSignals(this), ...assembleUnitSelectionSignals(this, [])];
        }
        assembleSelectionData(data) {
            return assembleUnitSelectionData(this, data);
        }
        assembleLayout() {
            return null;
        }
        assembleLayoutSignals() {
            return assembleLayoutSignals(this);
        }
        assembleMarks() {
            let marks = this.component.mark || [];
            // If this unit is part of a layer, selections should augment
            // all in concert rather than each unit individually. This
            // ensures correct interleaving of clipping and brushed marks.
            if (!this.parent || !isLayerModel(this.parent)) {
                marks = assembleUnitSelectionMarks(this, marks);
            }
            return marks.map(this.correctDataNames);
        }
        getMapping() {
            return this.encoding;
        }
        get mark() {
            return this.markDef.type;
        }
        channelHasField(channel) {
            return channelHasField(this.encoding, channel);
        }
        fieldDef(channel) {
            const channelDef = this.encoding[channel];
            return getTypedFieldDef(channelDef);
        }
    }

    class LayerModel extends Model {
        constructor(spec, parent, parentGivenName, parentGivenSize, repeater, config, fit) {
            super(spec, 'layer', parent, parentGivenName, config, repeater, spec.resolve, spec.view);
            const layoutSize = Object.assign({}, parentGivenSize, (spec.width ? { width: spec.width } : {}), (spec.height ? { height: spec.height } : {}));
            this.initSize(layoutSize);
            this.children = spec.layer.map((layer, i) => {
                if (isLayerSpec(layer)) {
                    return new LayerModel(layer, this, this.getName('layer_' + i), layoutSize, repeater, config, fit);
                }
                if (isUnitSpec(layer)) {
                    return new UnitModel(layer, this, this.getName('layer_' + i), layoutSize, repeater, config, fit);
                }
                throw new Error(message.INVALID_SPEC);
            });
        }
        parseData() {
            this.component.data = parseData(this);
            for (const child of this.children) {
                child.parseData();
            }
        }
        parseLayoutSize() {
            parseLayerLayoutSize(this);
        }
        parseSelections() {
            // Merge selections up the hierarchy so that they may be referenced
            // across unit specs. Persist their definitions within each child
            // to assemble signals which remain within output Vega unit groups.
            this.component.selection = {};
            for (const child of this.children) {
                child.parseSelections();
                keys(child.component.selection).forEach(key => {
                    this.component.selection[key] = child.component.selection[key];
                });
            }
        }
        parseMarkGroup() {
            for (const child of this.children) {
                child.parseMarkGroup();
            }
        }
        parseAxesAndHeaders() {
            parseLayerAxes(this);
        }
        assembleSelectionTopLevelSignals(signals) {
            return this.children.reduce((sg, child) => child.assembleSelectionTopLevelSignals(sg), signals);
        }
        // TODO: Support same named selections across children.
        assembleSignals() {
            return this.children.reduce((signals, child) => {
                return signals.concat(child.assembleSignals());
            }, assembleAxisSignals(this));
        }
        assembleLayoutSignals() {
            return this.children.reduce((signals, child) => {
                return signals.concat(child.assembleLayoutSignals());
            }, assembleLayoutSignals(this));
        }
        assembleSelectionData(data) {
            return this.children.reduce((db, child) => child.assembleSelectionData(db), data);
        }
        assembleTitle() {
            let title = super.assembleTitle();
            if (title) {
                return title;
            }
            // If title does not provide layer, look into children
            for (const child of this.children) {
                title = child.assembleTitle();
                if (title) {
                    return title;
                }
            }
            return undefined;
        }
        assembleLayout() {
            return null;
        }
        assembleMarks() {
            return assembleLayerSelectionMarks(this, flatten(this.children.map(child => {
                return child.assembleMarks();
            })));
        }
        assembleLegends() {
            return this.children.reduce((legends, child) => {
                return legends.concat(child.assembleLegends());
            }, assembleLegends(this));
        }
    }

    class RepeatModel extends BaseConcatModel {
        constructor(spec, parent, parentGivenName, repeatValues, config) {
            super(spec, 'repeat', parent, parentGivenName, config, repeatValues, spec.resolve);
            if (spec.resolve && spec.resolve.axis && (spec.resolve.axis.x === 'shared' || spec.resolve.axis.y === 'shared')) {
                warn(message.REPEAT_CANNOT_SHARE_AXIS);
            }
            this.repeat = spec.repeat;
            this.children = this._initChildren(spec, this.repeat, repeatValues, config);
        }
        _initChildren(spec, repeat, repeater, config) {
            const children = [];
            const row = (!isArray(repeat) && repeat.row) || [repeater ? repeater.row : null];
            const column = (!isArray(repeat) && repeat.column) || [repeater ? repeater.column : null];
            const repeatValues = (isArray(repeat) && repeat) || [repeater ? repeater.repeat : null];
            // cross product
            for (const repeatValue of repeatValues) {
                for (const rowValue of row) {
                    for (const columnValue of column) {
                        const name = (repeatValue ? `__repeat_repeat_${repeatValue}` : '') +
                            (rowValue ? `__repeat_row_${rowValue}` : '') +
                            (columnValue ? `__repeat_column_${columnValue}` : '');
                        const childRepeat = {
                            repeat: repeatValue,
                            row: rowValue,
                            column: columnValue
                        };
                        children.push(buildModel(spec.spec, this, this.getName('child' + name), undefined, childRepeat, config, false));
                    }
                }
            }
            return children;
        }
        parseLayoutSize() {
            parseRepeatLayoutSize(this);
        }
        assembleDefaultLayout() {
            const { repeat } = this;
            const columns = isArray(repeat) ? undefined : repeat.column ? repeat.column.length : 1;
            return Object.assign({}, (columns ? { columns } : {}), { bounds: 'full', align: 'all' });
        }
    }

    function buildModel(spec, parent, parentGivenName, unitSize, repeater, config, fit) {
        if (isFacetSpec(spec)) {
            return new FacetModel(spec, parent, parentGivenName, repeater, config);
        }
        if (isLayerSpec(spec)) {
            return new LayerModel(spec, parent, parentGivenName, unitSize, repeater, config, fit);
        }
        if (isUnitSpec(spec)) {
            return new UnitModel(spec, parent, parentGivenName, unitSize, repeater, config, fit);
        }
        if (isRepeatSpec(spec)) {
            return new RepeatModel(spec, parent, parentGivenName, repeater, config);
        }
        if (isAnyConcatSpec(spec)) {
            return new ConcatModel(spec, parent, parentGivenName, repeater, config);
        }
        throw new Error(message.INVALID_SPEC);
    }

    /**
     * Vega-Lite's main function, for compiling Vega-lite spec into Vega spec.
     *
     * At a high-level, we make the following transformations in different phases:
     *
     * Input spec
     *     |
     *     |  (Normalization)
     *     v
     * Normalized Spec (Row/Column channels in single-view specs becomes faceted specs, composite marks becomes layered specs.)
     *     |
     *     |  (Build Model)
     *     v
     * A model tree of the spec
     *     |
     *     |  (Parse)
     *     v
     * A model tree with parsed components (intermediate structure of visualization primitives in a format that can be easily merged)
     *     |
     *     | (Optimize)
     *     v
     * A model tree with parsed components with the data component optimized
     *     |
     *     | (Assemble)
     *     v
     * Vega spec
     */
    function compile(inputSpec, opt = {}) {
        // 0. Augment opt with default opts
        if (opt.logger) {
            // set the singleton logger to the provided logger
            set(opt.logger);
        }
        if (opt.fieldTitle) {
            // set the singleton field title formatter
            setTitleFormatter(opt.fieldTitle);
        }
        try {
            // 1. Initialize config by deep merging default config with the config provided via option and the input spec.
            const config = initConfig(mergeDeep({}, opt.config, inputSpec.config));
            // 2. Normalize: Convert input spec -> normalized spec
            // - Decompose all extended unit specs into composition of unit spec.  For example, a box plot get expanded into multiple layers of bars, ticks, and rules. The shorthand row/column channel is also expanded to a facet spec.
            const spec = normalize$1(inputSpec, config);
            // - Normalize autosize to be a autosize properties object.
            const autosize = normalizeAutoSize(inputSpec.autosize, config.autosize, isLayerSpec(spec) || isUnitSpec(spec));
            // 3. Build Model: normalized spec -> Model (a tree structure)
            // This phases instantiates the models with default config by doing a top-down traversal. This allows us to pass properties that child models derive from their parents via their constructors.
            // See the abstract `Model` class and its children (UnitModel, LayerModel, FacetModel, RepeatModel, ConcatModel) for different types of models.
            const model = buildModel(spec, null, '', undefined, undefined, config, autosize.type === 'fit');
            // 4 Parse: Model --> Model with components
            // Note that components = intermediate representations that are equivalent to Vega specs.
            // We need these intermediate representation because we need to merge many visualization "components" like projections, scales, axes, and legends.
            // We will later convert these components into actual Vega specs in the assemble phase.
            // In this phase, we do a bottom-up traversal over the whole tree to
            // parse for each type of components once (e.g., data, layout, mark, scale).
            // By doing bottom-up traversal, we start parsing components of unit specs and
            // then merge child components of parent composite specs.
            //
            // Please see inside model.parse() for order of different components parsed.
            model.parse();
            // draw(model.component.data.sources);
            // 5. Optimize the dataflow.  This will modify the data component of the model.
            optimizeDataflow(model.component.data, model);
            // 6. Assemble: convert model components --> Vega Spec.
            return assembleTopLevelModel(model, getTopLevelProperties(inputSpec, config, autosize), inputSpec.datasets, inputSpec.usermeta);
        }
        finally {
            // Reset the singleton logger if a logger is provided
            if (opt.logger) {
                reset();
            }
            // Reset the singleton field title formatter if provided
            if (opt.fieldTitle) {
                resetTitleFormatter();
            }
        }
    }
    function getTopLevelProperties(topLevelSpec, config, autosize) {
        return Object.assign({ autosize: keys(autosize).length === 1 && autosize.type ? autosize.type : autosize }, extractTopLevelProperties(config), extractTopLevelProperties(topLevelSpec));
    }
    /*
     * Assemble the top-level model.
     *
     * Note: this couldn't be `model.assemble()` since the top-level model
     * needs some special treatment to generate top-level properties.
     */
    function assembleTopLevelModel(model, topLevelProperties, datasets = {}, usermeta) {
        // Config with Vega-Lite only config removed.
        const vgConfig = model.config ? stripAndRedirectConfig(model.config) : undefined;
        const data = [].concat(model.assembleSelectionData([]), 
        // only assemble data in the root
        assembleRootData(model.component.data, datasets));
        const projections = model.assembleProjections();
        const title = model.assembleTitle();
        const style = model.assembleGroupStyle();
        const encodeEntry = model.assembleGroupEncodeEntry(true);
        let layoutSignals = model.assembleLayoutSignals();
        // move width and height signals with values to top level
        layoutSignals = layoutSignals.filter(signal => {
            if ((signal.name === 'width' || signal.name === 'height') && signal.value !== undefined) {
                topLevelProperties[signal.name] = +signal.value;
                return false;
            }
            return true;
        });
        const output = Object.assign({ $schema: 'https://vega.github.io/schema/vega/v5.json' }, (model.description ? { description: model.description } : {}), topLevelProperties, (title ? { title } : {}), (style ? { style } : {}), (encodeEntry ? { encode: { update: encodeEntry } } : {}), { data }, (projections.length > 0 ? { projections: projections } : {}), model.assembleGroup([...layoutSignals, ...model.assembleSelectionTopLevelSignals([])]), (vgConfig ? { config: vgConfig } : {}), (usermeta ? { usermeta } : {}));
        return {
            spec: output
            // TODO: add warning / errors here
        };
    }

    class TransformExtractMapper extends SpecMapper {
        mapUnit(spec, { config }) {
            if (spec.encoding) {
                const { encoding: oldEncoding, transform: oldTransforms } = spec;
                const { bins, timeUnits, aggregate, groupby, encoding } = extractTransformsFromEncoding(oldEncoding, config);
                const transform = [
                    ...(oldTransforms ? oldTransforms : []),
                    ...bins,
                    ...timeUnits,
                    ...(!aggregate.length ? [] : [{ aggregate, groupby }])
                ];
                return Object.assign({}, spec, (transform.length > 0 ? { transform } : {}), { encoding });
            }
            else {
                return spec;
            }
        }
    }
    const extractor = new TransformExtractMapper();
    /**
     * Modifies spec extracting transformations from encoding and moving them to the transforms array
     */
    function extractTransforms(spec, config) {
        return extractor.map(spec, { config });
    }

    const version$1 = pkg.version;

    exports.compile = compile;
    exports.extractTransforms = extractTransforms;
    exports.normalize = normalize$1;
    exports.version = version$1;

    Object.defineProperty(exports, '__esModule', { value: true });

}));
//# sourceMappingURL=vega-lite.js.map
