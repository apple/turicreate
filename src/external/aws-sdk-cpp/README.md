# AWS SDK

---

## AWS SDK version

The version is `1.7.134`, which is the latest version (c9d2aeb1c)

If you upgrade the SDK, you should also modify the compile definitions,

```Cmake
target_compile_definitions(aws-cpp-sdk PRIVATE -DAWS_SDK_VERSION_MAJOR=1)
target_compile_definitions(aws-cpp-sdk PRIVATE -DAWS_SDK_VERSION_MINOR=7)
target_compile_definitions(aws-cpp-sdk PRIVATE -DAWS_SDK_VERSION_PATCH=143)
```

---

## AWS Stream Event Version

[v0.1.4][2] (32713d3)

---

## AWS C Common Version

d023c9cb10e22bace150550a7357eab36164af52

## AWS Checksums Version

519d6d9093819b6cf89ffff589a27ef8f83d0f65

[2] : https://github.com/awslabs/aws-c-event-stream/releases/tag/v0.1.4
