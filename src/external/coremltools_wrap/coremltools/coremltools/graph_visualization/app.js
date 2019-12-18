"use strict";

document.addEventListener('DOMContentLoaded', function() {
	var options = {
	  name: 'dagre',
      nodeSep: 3,
	  edgeSep: 5,
	  minLen: function( edge ){ return 1; },
	  edgeWeight: function( edge ){ return 2; },
	  fit: true,
	  spacingFactor: 1.1,
	  nodeDimensionsIncludeLabels: true
	};

	var nodeInfo = getGraphNodesAndEdges();
	nodeInfo.then(function(nodesArray) {
		var cy = window.cy = cytoscape({
			container: document.getElementById('cy'),
		    elements: nodesArray,
		    layout: options,
		    style: [
			{
			    selector: "node",
			    style: {
			        shape: 'roundrectangle',
			        label: 'data(name)',
                    'font-size' : 30,
			        'border-width': 3,
                    'border-color': 'black',
			        width: 'label',
			        'color': '#000000',
			        'text-valign': 'center',
					'background-image': 'icons/node.png',
			        padding: 10,
		    	}
		    },
			{
			    selector: "node.parent",
			    style: {
					'compound-sizing-wrt-labels': 'include',
                    'background-image' : 'icons/parent.png',
                    'text-rotation' : '90deg',
                    'text-margin-x' : 10
		    	}
		    },
            {
			    selector: "node.parent > node",
			    style: {
			        opacity : 0
		    	}
		    },
            {
			    selector: "node.arrayFeatureExtractor",
			    style: {
					'background-image': 'icons/arrayFeatureExtractor.png'
		    	}
		    },
            {
			    selector: "node.categoricalMapping",
			    style: {
					'background-image': 'icons/categoricalMapping.png'
		    	}
		    },
            {
			    selector: "node.dictVectorizer",
			    style: {
					'background-image': 'icons/dictVectorizer.png'
		    	}
		    },
			{
			    selector: "node.custom",
			    style: {
					'background-image': 'icons/custom.png'
		    	}
		    },
            {
			    selector: "node.featureVectorizer",
			    style: {
					'background-image': 'icons/featureVectorizer.png'
		    	}
		    },
            {
			    selector: "node.glmClassifier",
			    style: {
					'background-image': 'icons/glmClassifier.png'
		    	}
		    },
            {
			    selector: "node.glmRegressor",
			    style: {
					'background-image': 'icons/glmRegressor.png'
		    	}
		    },
            {
			    selector: "node.identity",
			    style: {
					'background-image': 'icons/identity.png'
		    	}
		    },
            {
			    selector: "node.imputer",
			    style: {
					'background-image': 'icons/imputer.png'
		    	}
		    },
            {
			    selector: "node.neuralNetwork",
			    style: {
					'background-image': 'icons/neuralNetwork.png'
		    	}
		    },
            {
			    selector: "node.neuralNetworkClassifier",
			    style: {
					'background-image': 'icons/neuralNetworkClassifier.png'
		    	}
		    },
            {
			    selector: "node.neuralNetworkRegressor",
			    style: {
					'background-image': 'icons/neuralNetworkRegressor.png'
		    	}
		    },
            {
			    selector: "node.normalizer",
			    style: {
					'background-image': 'icons/normalizer.png'
		    	}
		    },
            {
			    selector: "node.oneHotEncoder",
			    style: {
					'background-image': 'icons/oneHotEncoder.png'
		    	}
		    },
            {
			    selector: "node.scaler",
			    style: {
					'background-image': 'icons/scaler.png'
		    	}
		    },
            {
			    selector: "node.supportVectorClassifier",
			    style: {
					'background-image': 'icons/supportVectorClassifier.png'
		    	}
		    },
            {
			    selector: "node.supportVectorRegressor",
			    style: {
					'background-image': 'icons/supportVectorRegressor.png'
		    	}
		    },
            {
			    selector: "node.treeEnsembleClassifier",
			    style: {
					'background-image': 'icons/treeEnsembleClassifier.png'
		    	}
		    },
            {
			    selector: "node.treeEnsembleRegressor",
			    style: {
					'background-image': 'icons/treeEnsembleRegressor.png'
		    	}
		    },
            {
			    selector: "node.convolution",
			    style: {
					'color': 'white',
					'background-image': 'icons/convolution.png'
		    	}
		    },
			{
			    selector: "node.deconvolution",
			    style: {
					'color': 'white',
					'background-image': 'icons/convolution.png'
		    	}
		    },
		    {
			    selector: "node.pooling",
			    style: {
					'color': 'white',
					'background-image': 'icons/pooling.png'
		    	}
		    },
		    {
			    selector: "node.activation",
			    style: {
					'background-image': 'icons/activation.png'
		    	}
		    },
            {
			    selector: "node.add",
			    style: {
					'background-image': 'icons/add.png'
		    	}
		    },
            {
			    selector: "node.average",
			    style: {
					'background-image': 'icons/average.png'
		    	}
		    },
            {
			    selector: "node.batchnorm",
			    style: {
					'background-image': 'icons/batchnorm.png'
		    	}
		    },
            {
			    selector: "node.biDirectionalLSTM",
			    style: {
					'background-image': 'icons/biDirectionalLSTM.png'
		    	}
		    },
            {
			    selector: "node.bias",
			    style: {
					'background-image': 'icons/bias.png'
		    	}
		    },
            {
			    selector: "node.concat",
			    style: {
					'background-image': 'icons/concat.png'
		    	}
		    },
            {
			    selector: "node.crop",
			    style: {
					'background-image': 'icons/crop.png'
		    	}
		    },
            {
			    selector: "node.dot",
			    style: {
					'background-image': 'icons/dot.png'
		    	}
		    },
            {
			    selector: "node.embedding",
			    style: {
					'background-image': 'icons/embedding.png'
		    	}
		    },
            {
			    selector: "node.flatten",
			    style: {
					'background-image': 'icons/flatten.png'
		    	}
		    },
            {
			    selector: "node.gru",
			    style: {
					'background-image': 'icons/gru.png'
		    	}
		    },
            {
			    selector: "node.innerProduct",
			    style: {
					'background-image': 'icons/innerProduct.png'
		    	}
		    },
            {
			    selector: "node.input",
			    style: {
					'background-image': 'icons/input.png'
		    	}
		    },
            {
			    selector: "node.output",
			    style: {
					'background-image': 'icons/output.png'
		    	}
		    },
            {
			    selector: "node.l2normalize",
			    style: {
					'background-image': 'icons/l2normalize.png'
		    	}
		    },
            {
			    selector: "node.loadConstant",
			    style: {
					'background-image': 'icons/loadConstant.png'
		    	}
		    },
            {
			    selector: "node.lrn",
			    style: {
					'background-image': 'icons/lrn.png'
		    	}
		    },
            {
			    selector: "node.max",
			    style: {
					'background-image': 'icons/max.png'
		    	}
		    },
            {
			    selector: "node.min",
			    style: {
					'background-image': 'icons/min.png'
		    	}
		    },
            {
			    selector: "node.multiply",
			    style: {
					'background-image': 'icons/multiply.png'
		    	}
		    },
            {
			    selector: "node.mvn",
			    style: {
					'background-image': 'icons/mvn.png'
		    	}
		    },
            {
			    selector: "node.padding",
			    style: {
					'background-image': 'icons/padding.png'
		    	}
		    },
            {
			    selector: "node.permute",
			    style: {
					'background-image': 'icons/permute.png'
		    	}
		    },
            {
			    selector: "node.pooling",
			    style: {
					'background-image': 'icons/pooling.png'
		    	}
		    },
            {
			    selector: "node.reduce",
			    style: {
					'background-image': 'icons/reduce.png'
		    	}
		    },
            {
			    selector: "node.reorganizeData",
			    style: {
					'background-image': 'icons/reorganizeData.png'
		    	}
		    },
            {
			    selector: "node.reshape",
			    style: {
					'background-image': 'icons/reshape.png'
		    	}
		    },
            {
			    selector: "node.scale",
			    style: {
					'background-image': 'icons/scale.png'
		    	}
		    },
            {
			    selector: "node.sequenceRepeat",
			    style: {
					'background-image': 'icons/sequenceRepeat.png'
		    	}
		    },
            {
			    selector: "node.simpleRecurrent",
			    style: {
					'background-image': 'icons/simpleRecurrent.png'
		    	}
		    },
            {
			    selector: "node.slice",
			    style: {
					'background-image': 'icons/slice.png'
		    	}
		    },
            {
			    selector: "node.softmax",
			    style: {
					'background-image': 'icons/softmax.png'
		    	}
		    },
            {
			    selector: "node.split",
			    style: {
					'background-image': 'icons/split.png'
		    	}
		    },
            {
			    selector: "node.unary",
			    style: {
					'background-image': 'icons/unary.png'
		    	}
		    },
            {
			    selector: "node.uniDirectionalLSTM",
			    style: {
					'background-image': 'icons/uniDirectionalLSTM.png'
		    	}
		    },
            {
			    selector: "node.upsample",
			    style: {
					'background-image': 'icons/upsample.png'
		    	}
		    },
		    {
		    	selector: "edge",
		    	style: {
		    		'curve-style': 'bezier',
		    		'control-point-weights': 1,
		    		'line-color': '#111111',
					'color' : '#000000',
					'border-width': 5,
					'font-size': 20,
		    		'target-arrow-shape': 'triangle',
		    		'target-arrow-color': '#111111',
					 label: 'data(label)',
					'text-background-opacity': 0,
					'text-background-color': '#ffffff',
					'text-background-shape': 'rectangle',
					'text-border-style': 'solid',
					'text-border-opacity': 0,
					'text-border-width': '1px',
					'text-border-color': 'darkgray',
					'text-opacity': 0
		    	}
		    }
		    ]
		});
        cy.fit();
        var childNodeCollection = cy.elements("node.parent > node");
        var childEdges  = childNodeCollection.connectedEdges();
        childEdges.style({'opacity': 0});
		cy.$('node').on('mouseover', function(e){
            var ele = e.target;
		    var keys = Object.keys(ele.data('info'));
		    var div = document.getElementById('node-info');
		    var content = '<br />';
		    content += '<div class="subtitle" align="left">Parameters</div>';
		    content += '<br />';
			content += '<div align="left">';
		    for (var i = keys.length - 1; i >= 0; i--) {
		  	    if (keys[i] != 'desc') {
                    var val = ele.data('info')[keys[i]].replace(/["]+/g, '');
                    content += keys[i] + ' : ' + val.charAt(0).toUpperCase().replace(/(?:\r\n|\r|\n)/g, '<br>') + val.slice(1) + '<br />';
                }
            }
			content += '</div>';
            if (ele.data('info')["desc"] != undefined) {
                content += '<br /><br /><div class="subtitle" align="left">Description</div><br />';
                content += ele.data('info')["desc"].replace(/(?:\r\n|\r|\n)/g, '<br>') + '<br />';
            }
            div.innerHTML = content;
		});

		cy.on('tap', 'edge', function (evt) {
           var edge = evt.target;
           var edgeLabel = edge.data().source;
           edge.style({'label': edgeLabel});
           edge.animate({
           		style: {
           			'text-opacity': 1,
		           	'text-margin-x': 15,
		           	'text-border-opacity': 1,
		           	'text-background-opacity': 1
           		}
           });
        });

		cy.on('click', 'node.parent', function(evt){
		    var node = evt.target;
            node.children().style({'opacity': 1});
            node.style({'color' : '#d5e1df'});
            var selectedChildNodeCollection = node.children();
			var parentEdges = node.connectedEdges();
            var selectedChildEdges  = selectedChildNodeCollection.connectedEdges();
            selectedChildEdges.style({'opacity' : 1});

            var selectedChildEdgesTarget = [];

            for(var idx = 0; idx < selectedChildEdges.length; idx++) {
				if (selectedChildEdges[idx].data('target') != node.data().id) {
					selectedChildEdgesTarget.push(selectedChildEdges[idx].data('target'));
				}
			}
			parentEdges.style({'opacity' : 0});

			for(var idx = 0; idx < parentEdges.length; idx++) {
				if (parentEdges[idx].data('target') != node.data().id) {

					if (selectedChildEdgesTarget.includes(parentEdges[idx].data('target')) == false) {
						parentEdges[idx].style({'opacity' : "1"});
					}
				}
			}

            cy.animate({
                fit : {
                    eles : selectedChildNodeCollection,
                    padding : 20
                }
            }, {
                duration: 500
            });

		});

		$('#label-switch').on('click', function(e) {

			var edges = cy.$('edge');
			for(var idx = 0 ; idx < edges.length; idx++) {
				edges[idx].style({label: edges[idx].data().shape});
			}
			if (edges.style().textOpacity == 0) {
				edges.animate({
					style: {
					'text-opacity': 1,
					'text-background-opacity': 1,
					'text-border-opacity': 1
					}
				});
			}
			else {
				edges.animate({
					style: {
					'text-opacity': 0,
					'text-background-opacity': 0,
					'text-border-opacity': 0
					}
				});
			}
		});

		$('#reset-state').on('click', function (e) {
			var childNodes = cy.$("node.parent > node");
			childNodes.style({
					"opacity": 0
				});
			childNodes.connectedEdges().style({
				'opacity': 0
			});
			var parentNodes = cy.$("node.parent");
			parentNodes.style({
				'color': 'black'
			});
			parentNodes.connectedEdges().style({
				'opacity': 1
			});
			var edges = cy.edges();
			for(var idx = 0 ; idx < edges.length; idx++) {
				edges[idx].style({label: null});
			}
			edges.animate({
				style: {
				'text-opacity': 0,
				'text-background-opacity': 0,
				'text-border-opacity': 0
				}
			});
			cy.fit();
		});

	});

});


function getGraphNodesAndEdges() {
	var graphPromise  = $.ajax({
		url: 'model.json',
		type: 'GET',
		dataType: 'json',
		contentType: "application/json; charset=utf-8",
	})
	.then(function(msg) {
		return msg;
	}
	);
	return Promise.resolve(graphPromise);

}
