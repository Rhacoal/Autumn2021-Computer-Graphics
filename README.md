# 计算机图形学基础 作业3

## 测试环境

Windows 10 x64 10.0.19041.1151  (NVIDIA RTX 2080Ti 11GB)  
macOS Big Sur 11.3 (AMD Radeon Pro 5500M 8GB)

OpenGL 4.1 (core profile)  
OpenCL 1.2

## 编译与运行

使用 CMake 构建。

在以下平台测试编译运行过

- Windows 10 Visual Studio 2019 16.11.5 MSVC 14.29.30133
- Windows 10 g++ 11.2.0
- macOS 11.3 clang 12.0.0

### 依赖库

- `glad`, `glfw`, `glm` 基础库
- `cl.hpp` OpenCL 的 C++ Wrapper
- `tinydialog` 跨平台的 native 文件打开对话框
- `nlohmann/json` json 序列化与反序列化
- `imgui` GUI 组件
- `stb_image` 图像加载
- `assimp` 资源加载

## 功能与实现

在两次作业中实现了一个简易的图形框架，支持：

- 对象层级结构
- 材质 (Blinn-Phong 和 Cock-Torrance)
- 三角形网格 并进行了一定的封装。代码见 `lib` 目录

并通过 OpenCL 实现了一个简单的路径追踪器，代码见 `lib/shaders/raytracing.cpp` (OpenCL 程序)
和 `lib/raytracing/rt.cpp` (C++ Wrapper)。也可以使用纯 CPU 渲染，在启动命令行中添加 `cpu` 即可，渲染分辨率控制见
`appmain.cpp:384`。

主程序位于 `src/appmain.cpp`

### 控制

- 按住 ALT 键移动鼠标旋转视角
    - 使用上下左右键也可以旋转视角
- 按住 ALT 键鼠标即可自由移动
    - 此时可操作左上方的面板 (使用 ImGui 实现)
- 左上方的面板右下角可以调整大小
- WASD 控制相机水平移动，Q 向上移动，E 向下移动
- 窗口可以调整大小
- 面板中，filters 下有三个后处理器可以开关
    - `invert color` 反相
    - `grayscale` 灰度
    - `gamma correction` 伽马校正 (默认开启)
- `skybox` 开关控制天空盒是否显示
- `export` 将路径追踪的图像输出到 `output.png` 文件
    - 反相和灰度效果不会被输出
    - 伽马校正一定会被应用

### 场景加载

可以加载自定义的 json 格式场景

```jsonc
{
  "materials": [              // 所有材质
    {
      "id": "mtl-ball",       // 材质的 ID，会被物体引用
      "type": "standard",
      "albedo": { "texture": "material/rustediron2/rustediron2_basecolor.png" },
      "metallic": { "texture": "material/rustediron2/rustediron2_metallic.png" },
      "roughness": { "texture": "material/rustediron2/rustediron2_roughness.png" },
      "normal": { "texture": "material/rustediron2/rustediron2_normal.png" }
    },
    // ...
  ],
  "objects": [                // 所有物体
    {
      "id": "ball1",
      "geometry": {           // 几何信息
        "type": "sphere",     // 支持 "box", "sphere", "plane" 和 "file"
        "radius": 1.0
      },
      "position": [3.0, 1.0, -2.0],
      "material": "mtl-ball"  // 引用的材质 ID，或材质信息结构体
    },
    {
      "id": "bunny",
      "geometry": {
        "type": "file",       // 从文件读取几何信息
        "file": "object/bun_zipper_res2.ply"
      },
      "position": [-2.0, 0.0, -2.0],
      "scale": [10.0, 10.0, 10.0],  // 四元数
      "material": "mtl-bunny"
    },
    {
      "id": "light",
      "isLight": true,        // 标记 isLight 的物体会在路径追踪时被作为光源处理
      "position": [0.0, 5.0, 0.0],
      "geometry": {
        "type": "plane",
        "size": [0.5, 0.5]
      },
      "material": "mtl-light"
    },
    // ...
  ]
}
```

在程序中，通过左上方 `load_scene...` 按钮选择场景文件进行加载

json 读取使用 `nlohmann` 库实现，见 `lib/importer/obj_loader.cpp`

ply 读取使用 `assimp` 库实现

### 滤镜

基于 FBO 实现了反相和灰度滤镜。

### 路径追踪实现

作业中实现了基于蒙特卡洛积分方法的路径追踪器，基于 Disney BSDF 光照模型

### 加速结构
加速采用了 BVH，虽然没有测试无加速结构下的速度，但是考虑到在场景中有 `dragon_vrip.ply` 和 `bun_zipper.ply`
两个原始模型的情况下，可以在 `1047ms` 内进行一个 spp 的计算，
