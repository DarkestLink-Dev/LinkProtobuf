![logo](./Resources/Icon128.png)
# LinkProtobuf
A Unreal Engine plugin that makes using Google Protocol Buffers (protobuf) in UnrealEngine Projects simple and consistent across platforms.

## Overview

- Lightweight integration of protobuf into Unreal build and runtime.
- C++ APIs ready for UE modules.

## Features

- One-click generation of .proto message definitions
  - Create proto schemas from selected USTRUCT/UPROPERTY metadata or ready-made templates
- One-click generation of .pb.cc/.pb.h C++ sources
  - Run protoc from the editor or as part of the build step for all target platforms
- Runtime conversion between protobuf binaries and UE UStructs
  - Serialize UStruct -> TArray<uint8> (protobuf bytes) and parse TArray<uint8> (protobuf bytes) -> UStruct

## Supported Platforms

| Platform | Architecture        | EditorDevelop | Packaged |
|:--------:|:--------------------|:------:|:--------:|
| Windows  | x86_64 (x64)        |   ✓    |    ✓     |
| Windows  | ARM64               |   –    | Planned  |
| Linux    | x86_64              | Planned |    ✓     |
| Linux    | ARM64 (aarch64)     |   –    | ✓ |
| macOS    | x86_64 (Intel)      |   –    | – |
| macOS    | arm64 (Apple Silicon)| Planned |    Planned    |
| iOS      | arm64                |   –    | Planned |
| Android  | arm64-v8a           |   –    |    ✓     |
| Android  | x86_64              |   –    | ✓ |
| Android  | armeabi-v7a         |   –    | ✓ |


## Supported Engine Versions

This plugin is compatible with the following Unreal Engine versions:

| Engine Version | Support Status            |
|:---------------|:--------------------------|
| UE 5.6         | ✓ Fully Supported         |
| UE 5.5         | - Planned                 |
| UE 5.4         | - Planned                 |
| UE 5.3         | - Planned                 |
| UE 5.2         | - Planned                 |
| UE 5.1         | - Planned                 |
| UE 5.0         | - Planned                 |
| UE 4.27 or Older       | ⚠️ Planed Limited Support |

**Note**: For UE 4.27, some features may be limited due to engine API differences. It is recommended to use UE 5.0 or later for the best experience.

## Supported Types (Proto <-> UStruct)
The Proto-to-UStruct conversion supports the following:

- UE base types (commonly used):
  - bool, int8/int16/int32/int64, uint8/uint16/uint32/uint64
  - float, double
  - FString, FName, FText
  - Nested UStructs
- Containers:
  - TArray<T> — mapped to protobuf repeated fields
  - TSet<T> — protobuf has no native Set; serialized as repeated with uniqueness enforced during import/export
  - TMap<TKey, TValue> — mapped to protobuf map with key restrictions (see below)
- Enums:
  - All user-defined UENUMs and most engine-provided enums (serialized as integers)

Notes and limitations for containers:

- Set: because protobuf doesn’t have a Set type, it is represented as a repeated field; uniqueness is maintained by the plugin when converting.
- Map: protobuf map key types are restricted by the protobuf specification:
  - Keys must be an integral scalar or string: one of int32/int64/uint32/uint64/sint32/sint64/fixed32/fixed64/sfixed32/sfixed64, or string.
  - Keys cannot be floating-point (float/double), bytes, enums, messages/structs, arrays, or other complex types.
  - If you need other key types, consider converting keys to string or wrapping data in a UStruct before serialization.

Not supported:

- UInterface (interfaces)
- UObject reference types (ObjectType, e.g., UObject and subclasses) for direct serialization


## Prerequisites

- A C++-based Unreal Engine project is required.
- If your project is Blueprint-only, convert it to a C++ project via:
  - In the editor, go to **File -> New C++ Class**, create an (empty) class, and let Unreal regenerate project files.
  - Reopen the project in your IDE and build once so the C++ toolchain is set up.

## Usage

To use the LinkProtobuf plugin in your Unreal Engine project, follow these steps:

1. Install the plugin by copying it into your project's `Plugins` folder.
2. Enable the plugin in the Unreal Engine editor by going to `Edit` -> `Plugins`, finding LinkProtobuf , and checking the "Enabled" box.
3. Restart the Unreal Engine editor to load the plugin.
4. Create your own `UStruct` definitions for the messages you need to pass. 
5. Go to `Edit` -> `ProjectSettings` -> `Plugins` -> `LinkProtobuf ` -> `User Defined Structs for Protobuf` Add Structs you created.
7. > Click The `Generate ProtoFiles From Settings` Then you will see **ProtoCpp Generate Success. Please recompile your project from IDE or use Live Coding in Editor(Sometimes not work).** in the OpenDialog, otherwise, check the Output Log for errors.
8. Rebuild your project to include the generated protobuf C++ files.
9. Use the provided BluePrints APIs to serialize and deserialize your `UStruct` instances to and from protobuf binary format.
In Blueprint: Use "**Convert Struct To Proto Binary Bytes**" and "**Convert Proto Binary Bytes To Struct**" nodes.
In Cpp: Use `ULinkProtobufFunctionLibrary::ConvertStructToBinaryProtoBytes` and `ULinkProtobufFunctionLibrary::ConvertProtoBinaryBytesToStruct` functions.
10. To Add new `UStruct` you want to use, repeat steps 5-8.

## Contributing

Contributions are welcome. You can:

- Report bugs or request features in Issues
- Submit PRs with fixes or improvements

Issues: https://github.com/DarkestLink-Dev/LinkProtobuf/issues

## License

Specify your project license here (e.g., MIT, Apache-2.0). If you need, I can add a `LICENSE` file and update this section accordingly.

## Acknowledgements

- [Google Protobuf Documentation](https://developers.google.com/protocol-buffers)

- [Unreal Engine Documentation](https://docs.unrealengine.com/en-US/index.html)