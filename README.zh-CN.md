![logo](./Resources/Icon128.png)
<p align="right">语言: <b>中文</b> | <a href="./README.en.md">English</a></p>
# LinkProtobuf

虚幻引擎插件，使您在虚幻引擎项目中使用 Google Protocol Buffers (protobuf) 变得简单且跨平台一致。

## 概览

- 轻量级的 protobuf 库集成到虚幻引擎Runtime。
- 为虚幻引擎准备的 C++ API 和蓝图接口。

## 功能特性

- 一键生成 .proto 消息定义
  - 从选定的 USTRUCT/UPROPERTY 元数据或现成模板创建 proto 消息结构
- 一键生成 .pb.cc/.pb.h C++ 源文件
  - 从编辑器运行 protoc，或作为构建步骤的一部分，支持所有目标平台
- 游戏运行时 protobuf 二进制文件与虚幻 UStruct 之间的转换
  - 序列化 UStruct → TArray<uint8>（protobuf 字节）且解析 TArray<uint8>（protobuf 字节）→ UStruct

## 支持的平台

| 平台     | 架构              | 编辑器开发 | 已打包 |
|:--------:|:------------------|:------:|:--------:|
| Windows  | x86_64 (x64)      |   ✓    |    ✓     |
| Windows  | ARM64             |   –    | ✓ |
| Linux    | x86_64            | 正在开发 |    ✓     |
| Linux    | ARM64 (aarch64)   |   –    | ✓ |
| macOS    | x86_64 (Intel)    |   –    | – |
| macOS    | arm64 (Apple Silicon) | 正在开发 | 正在开发 |
| iOS      | arm64             |   –    | 正在开发 |
| Android  | arm64-v8a         |   –    |    ✓     |
| Android  | x86_64            |   –    | ✓ |
| Android  | armeabi-v7a       |   –    | ✓ |

## 支持的引擎版本

本插件与以下虚幻引擎版本兼容：

| 引擎版本 | 支持状态            |
|:---------|:--------------------------|
| UE 5.6   | ✓ 完全支持         |
| UE 5.5   | ✓ 完全支持         |
| UE 5.4   | ✓ 完全支持         |
| UE 5.3   | ✓ 完全支持         |
| UE 5.2   | ✓ 完全支持         |
| UE 5.1   | - 正在开发           |
| UE 5.0   | - 正在开发           |
| UE 4.27 或更早 | ⚠️ 计划有限支持 |


## 支持的类型（Proto ↔ UStruct）

Proto 到 UStruct 的转换支持以下类型：

- 虚幻引擎基础类型（常用）：
  - bool、int8/int16/int32/int64、uint8/uint16/uint32/uint64
  - float、double
  - FString、FName、FText
  - FVector、FVector2D、FVector4、FRotator、FQuat、FTransform
  - FColor、FLinearColor
  - 嵌套 UStruct
- 容器类型：
  - TArray<T> — 映射到 protobuf repeated（重复）字段
  - TSet<T> — 因为 protobuf 没有 Set 类型，用 repeated 字段表示；插件在转换时维护唯一性
  - TMap<TKey, TValue> — 映射到 protobuf map，key 有限制（见下方说明）
- 枚举：
  - 所有用户定义的 UENUM 和大多数引擎提供的枚举（序列化为整数）

容器相关的注意事项与限制：

- Set：因为 protobuf 没有 Set 类型，它表示为 repeated 字段；唯一性由插件在转换时维护。
- Map：protobuf map key 类型受 protobuf 规范限制：
  - Key 必须是整数标量或字符串类型：int32/int64/uint32/uint64 或 string。
  - Value 可以是任何支持的标量或消息类型。
  - 如果需要其他 key 类型，考虑将 key 转换为字符串或在序列化前将数据包装在 UStruct 中。

不支持的类型：

- UInterface（接口）
- UObject 引用类型（ObjectType，如 UObject 及其子类）的直接序列化

## 前置条件

- 需要基于 C++ 的虚幻引擎项目。
- 如果你的项目是蓝图项目，请通过以下步骤转换为 C++ 项目：
  - 在编辑器中，进入**文件 → 新建 C++ 类**，创建一个（空）类，并让虚幻重新生成项目文件。
  - 在你的 IDE 中重新打开项目并构建一次，以便 C++ 工具链完成设置。
- 插件需要安装在你的项目的 `Plugins` 文件夹中。

**⚠️ 重要提示：如果从 FAB Marketplace 安装，你必须手动将插件复制到项目的 Plugins 文件夹以使其正常工作。**

## 使用方法

要在虚幻引擎项目中使用 LinkProtobuf 插件，请按照以下步骤操作：

1. 将插件复制到项目的 `Plugins` 文件夹中进行安装。
2. 在虚幻编辑器中启用插件：进入**编辑 → 插件**，找到 LinkProtobuf，并勾选"已启用"框。
3. 关闭虚幻编辑器，右键点击 .uproject 文件，选择"生成 Visual Studio 项目文件"。
4. 启动虚幻编辑器并为需要传递的消息创建你自己的 `UStruct` 定义。
5. 进入**编辑 → 项目设置 → 插件 → LinkProtobuf → 用户定义的 Protobuf 结构**，添加你创建的结构。
6. 点击**从设置生成 Proto 文件**，你将在对话框中看到 **Proto C++ 生成成功。请从 IDE 重新编译你的项目或使用编辑器中的 Live Coding（有时不工作）。**，否则检查输出日志以查看错误。
7. 重新构建你的项目以包含生成的 protobuf C++ 文件。
8. 使用提供的蓝图 API 将你的 `UStruct` 实例序列化和反序列化为 protobuf 二进制格式。
   - 在蓝图中：使用**将结构体转换为 Proto 二进制字节**和**将 Proto 二进制字节转换为结构体**节点。
   - 在 C++ 中：使用 `ULinkProtobufFunctionLibrary::ConvertStructToBinaryProtoBytes` 和 `ULinkProtobufFunctionLibrary::ConvertProtoBinaryBytesToStruct` 函数。

## 贡献

欢迎任何形式的贡献。你可以：

- 在 Issues 中报告缺陷或提交功能需求
- 提交修复或改进的 Pull Request（PR）

Issues: https://github.com/DarkestLink-Dev/LinkProtobuf/issues

## 许可证

Apache-2.0

## 致谢

- [Google Protobuf 文档](https://developers.google.com/protocol-buffers)

- [Unreal Engine 文档](https://docs.unrealengine.com/en-US/index.html)

