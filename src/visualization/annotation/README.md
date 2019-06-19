# Annotation

The backend and message formats for Turi Create Annotation are defined in this directory.

## OVERVIEW: The Annotation API

The main methods of the annotation api are documented below:

- `void annotate(const std::string &path_to_client);`
	- Handles the process creation and allocates a pointer to the pipe used to communicate the data.

- `annotate_spec::Data getItems(size_t start, size_t end);`
	- Retrieves objects to display. These will stream to a front-end.

- `annotate_spec::Annotations getAnnotations(size_t start, size_t end);`
	- Retrieves the annotations. These will stream to a front-end.

- `bool setAnnotations(const annotate_spec::Annotations &annotation);`
	- Saves the annotations coming in from the front end.

## PROTOCOL: The Annotation Message

The annotation message follows a specific format defined in the `format/annotate.proto` and is used to communicate with the front-end app. Use this proto file on the front-end to de-serialize the message.

*(Note: the annotation format that each toolkit expects does not currently follow this proto format. Those are tookit specific and have already been predefined. These may be refactored later)*

##### BUILDING: Making Changes to the Proto Format

If you make any changes to the protobuf format, then you have to recompile. This will not be done automatically by the CMake build. To recompile you will need the v3.3.0 of the Protocol Buffers, which you can download and install here:

```
https://github.com/protocolbuffers/protobuf/releases/tag/v3.3.0
```

When you've installed the package, regenerate these protobuf files use the command below in the current directory.

```bash
	make annotationProtobuf
```

After you've regenerated the protobuf files you can now continue with the CMake build of TuriCreate.


## ANNOTATION CLASS

### annotation_base.hpp

Every annotation backend extends from the class declared in the `annotation_base.hpp` file. This forces the annotation api to remain consistent across all implementations. When extending the `AnnotationBase` you must implement the following methods to get your custom annotation implementations working.

- `annotate_spec::MetaData metaData()`

This method returns the meta of the dataset to be annotated. Since annotations and data are streamed to the front end the UI needs to know things like the unique number of labels and numer of total datum in the dataset. Define a `MetaData` message in `format/meta.proto` and populate the message in this method. (Note: if you change the proto file, you'll have to recompile using the instructions above)

- `annotate_spec::Data getItems(size_t start, size_t end)`

This method retrieves the data to be annotated. In Image Classification this would be the images. Define a new message in `format/data.proto` and populate the message in this method. (Note: if you change the proto file, you'll have to recompile using the instructions above)

- `annotate_spec::Annotations getAnnotations(size_t start, size_t end)`

This method retrieves the annotations. In Image Classification this would be the string values corresponding the the images. Note that these annotations are tied to the images in the dataset via the `index` of the image in the SFrame. Since SFrames are immutable by nature, this works. Define a new message in `format/annotate.proto` and populate the message in this method. (Note: if you change the proto file, you'll have to recompile using the instructions above)

- `bool setAnnotations(const annotate_spec::Annotations &annotations)`

This method is responsible for setting the annotations coming from the client to this method. A return value of `true` means that the annotations we successfully captured by this class.
