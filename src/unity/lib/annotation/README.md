# Annotation

The backend and message formats for TuriCreate Annotation are defined in this directory. 

## OVERVIEW: The Annotation API

The main methods of the annotation api are documented below:

- `void show(const std::string &path_to_client);`
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
protoc --proto_path="./format" --cpp_out="./build/format/cpp" --objc_out="./build/format/obj_c" "./format/annotate.proto" "./format/data.proto" "./format/meta.proto"
```

After you've regenerated the protobuf files you can now continue with the CMake build of TuriCreate.


## ANNOTATION CLASS

### annotation_base.hpp

Every annotation backend extends from the class declared in the `annotation_base.hpp` file.

The constructor takes a


