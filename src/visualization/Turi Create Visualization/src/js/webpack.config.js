const webpack = require('webpack');
const path = require('path');

const APP_DIR = path.resolve(__dirname, 'lib');
const BUILD_DIR = path.resolve(__dirname, 'build');

const WebpackConfig = {

    entry: APP_DIR + '/index.js',

    output: {
        path: BUILD_DIR,
        filename: 'index.js'
    },
    resolve: {
      extensions: ['.js', '.jsx', '.json', '.css']
    },
    module: {
        rules: [
            {
                loader: 'babel-loader',
                test: /.js$/,
                include : APP_DIR,
                options: {
                    presets: [ 'es2015', 'react' ]
                }
            },
            {
                test: /\.css$/,
                use:['style-loader','css-loader']
            }
        ],
        loaders: [
        {
          test: /\.jsx?$/,
          loaders: ["babel-loader"]
        },
      ]
    },
    externals: {
        'd3': 'd3'
    },
    plugins: [
        new webpack.DefinePlugin({
            'process.env.NODE_ENV': JSON.stringify('production')
        })
    ]
}

module.exports = WebpackConfig;
