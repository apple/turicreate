Int64FeatureType
________________________________________________________________________________

The 64-bit integer feature type.


.. code-block:: proto

	message Int64FeatureType {}






DoubleFeatureType
________________________________________________________________________________

The double-precision floating point number feature type.


.. code-block:: proto

	message DoubleFeatureType {}






StringFeatureType
________________________________________________________________________________

The string feature type.


.. code-block:: proto

	message StringFeatureType {}






SizeRange
________________________________________________________________________________




.. code-block:: proto

	message SizeRange {
	    uint64 lowerBound = 1;
	    int64 upperBound = 2; // negative value means unbound otherwise upperbound is included in range
	}






ImageFeatureType
________________________________________________________________________________

The image feature type.


.. code-block:: proto

	message ImageFeatureType {
	    // Assumes raw (decompressed) format
	    enum ColorSpace {
	        INVALID_COLOR_SPACE = 0;
	        GRAYSCALE = 10; //  8 bits per pixel
	        RGB = 20;       // 32 bits per pixel: RGBA with A channel ignored
	        BGR = 30;       // 32 bits per pixel: BGRA with A channel ignored
	    }
	
	    message ImageSize {
	        uint64 width = 1;
	        uint64 height = 2;
	    }
	
	    message EnumeratedImageSizes {
	        repeated ImageSize sizes = 1;
	    }
	
	    message ImageSizeRange {
	        SizeRange widthRange = 1;
	        SizeRange heightRange = 2;
	    }
	
	    // The required or default image size is width x height
	    //
	    // If specificationVersion <= 2 or SizeFlexibility is empty,
	    // width x height is the required fixed image size
	    //
	    // If SizeFlexibility is present, width x height indicate a "default"
	    // image size which must be consistent with the flexibilty specified
	
	    int64 width = 1;
	    int64 height = 2;
	
	    // For specification version >= 3 you can specify image size flexibility.
	
	    oneof SizeFlexibility {
	
	        // Use enumeratedSizes for a set of distinct fixed sizes
	        // e.g. portrait or landscape: [80 x 100, 100 x 8]
	        //
	        // If the width x height fields above are specified then they must be
	        // one of the sizes listed.
	        //
	        // If width and height are not specified above then the default width
	        // and height will be enumeratedSizes[0]
	        //
	        // Must be non-empty
	
	        EnumeratedImageSizes enumeratedSizes = 21;
	
	        // Use imageSizeRange to allow for ranges of values
	        // e.g. any image greater than 10 x 20: [10..<max] x [20..<max]
	        //
	        // If width and height are specified above they must fall in the range
	        // specified in imageSizeRange. They will be treated as the default size.
	        //
	        // If width and height are not specified above then the default width
	        // and height will be imageSizeRange.widthRange.lowerBound x imageSizeRange.heightRange.lowerBound
	
	        ImageSizeRange imageSizeRange = 31;
	    }
	
	    ColorSpace colorSpace = 3;
	}






ImageFeatureType.ImageSize
--------------------------------------------------------------------------------




.. code-block:: proto

	    message ImageSize {
	        uint64 width = 1;
	        uint64 height = 2;
	    }






ImageFeatureType.EnumeratedImageSizes
--------------------------------------------------------------------------------




.. code-block:: proto

	    message EnumeratedImageSizes {
	        repeated ImageSize sizes = 1;
	    }






ImageFeatureType.ImageSizeRange
--------------------------------------------------------------------------------




.. code-block:: proto

	    message ImageSizeRange {
	        SizeRange widthRange = 1;
	        SizeRange heightRange = 2;
	    }






ArrayFeatureType
________________________________________________________________________________

The array feature type.


.. code-block:: proto

	message ArrayFeatureType {
	
	    enum ArrayDataType {
	        INVALID_ARRAY_DATA_TYPE = 0;
	        FLOAT32 = 65568; // 0x10000 | 32
	        DOUBLE = 65600;  // 0x10000 | 64
	        INT32 = 131104;  // 0x20000 | 32
	    }
	
	    // The required or default shape
	    //
	    // If specificationVersion <= 2 or ShapeFlexibility is empty,
	    // shape is the required fixed shape
	    //
	    // If ShapeFlexibility is present, shape indicate a "default"
	    // shape which must be consistent with the flexibilty specified
	
	    repeated int64 shape = 1;
	
	    ArrayDataType dataType = 2;
	
	    message Shape {
	        repeated int64 shape = 1;
	    }
	
	    message EnumeratedShapes {
	        repeated Shape shapes = 1;
	    }
	
	    message ShapeRange {
	        // sizeRanges.size() must be length 1 or 3
	        // sizeRanges[d] specifies the allowed range for dimension d
	        repeated SizeRange sizeRanges = 1;
	    }
	
	    // For specification version >= 3 you can specify image size flexibility.
	
	    oneof ShapeFlexibility {
	
	        // Use enumeratedShapes for a set of distinct fixed shapes
	        //
	        // If the shape field is specified then it must be
	        // one of the enumerated shapes.
	        // If shape is not specifed, the "default" shape will be considered
	        // enumeratedShapes[0]
	        //
	        // Must be non-empty
	
	        EnumeratedShapes enumeratedShapes = 21;
	
	        // Use shapeRange to allow the size of each dimension vary within
	        // indpendently specified ranges
	        //
	        // If you specify shape above it must fall in the range
	        // specified in shapeRanges. It will be treated as the default shape.
	        //
	        // If you don't specify shape above then the default shape will
	        // have shape[d] = shapeRange.sizeRanges[d].lowerBound
	
	        ShapeRange shapeRange = 31;
	
	    }
	
	    oneof defaultOptionalValue {
	        int32 intDefaultValue = 41;
	        float floatDefaultValue = 51;
	        double doubleDefaultValue = 61;
	    }
	
	}






ArrayFeatureType.Shape
--------------------------------------------------------------------------------




.. code-block:: proto

	    message Shape {
	        repeated int64 shape = 1;
	    }






ArrayFeatureType.EnumeratedShapes
--------------------------------------------------------------------------------




.. code-block:: proto

	    message EnumeratedShapes {
	        repeated Shape shapes = 1;
	    }






ArrayFeatureType.ShapeRange
--------------------------------------------------------------------------------




.. code-block:: proto

	    message ShapeRange {
	        // sizeRanges.size() must be length 1 or 3
	        // sizeRanges[d] specifies the allowed range for dimension d
	        repeated SizeRange sizeRanges = 1;
	    }






DictionaryFeatureType
________________________________________________________________________________

The dictionary feature type.


.. code-block:: proto

	message DictionaryFeatureType {
	    oneof KeyType {
	        Int64FeatureType int64KeyType = 1;
	        StringFeatureType stringKeyType = 2;
	    }
	}






SequenceFeatureType
________________________________________________________________________________

The Sequence feature type.


.. code-block:: proto

	message SequenceFeatureType {
	
	    oneof Type {
	        Int64FeatureType int64Type = 1;
	        StringFeatureType stringType = 3;
	    }
	
	    // Range of allowed size/length/count of sequence
	    SizeRange sizeRange = 101;
	}






FeatureType
________________________________________________________________________________

A feature, which may be optional.


.. code-block:: proto

	message FeatureType {
	    oneof Type {
	        Int64FeatureType int64Type = 1;
	        DoubleFeatureType doubleType = 2;
	        StringFeatureType stringType = 3;
	        ImageFeatureType imageType = 4;
	        ArrayFeatureType multiArrayType = 5;
	        DictionaryFeatureType dictionaryType = 6;
	        SequenceFeatureType sequenceType = 7;
	    }
	
	    bool isOptional = 1000;
	}










ArrayFeatureType.ArrayDataType
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum ArrayDataType {
	        INVALID_ARRAY_DATA_TYPE = 0;
	        FLOAT32 = 65568; // 0x10000 | 32
	        DOUBLE = 65600;  // 0x10000 | 64
	        INT32 = 131104;  // 0x20000 | 32
	    }



ImageFeatureType.ColorSpace
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum ColorSpace {
	        INVALID_COLOR_SPACE = 0;
	        GRAYSCALE = 10; //  8 bits per pixel
	        RGB = 20;       // 32 bits per pixel: RGBA with A channel ignored
	        BGR = 30;       // 32 bits per pixel: BGRA with A channel ignored
	    }