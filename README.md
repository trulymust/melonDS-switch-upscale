<img width="2500" height="1000" alt="banner" src="https://github.com/user-attachments/assets/bb31595c-5452-453e-8d30-0144d172b1af" />

<h2 align="center"><b>melonDS Switch Upscale</b></h2>
<p align="center">
<a href="https://github.com/trulymust/melonDS-switch-upscale/releases"><img src="https://img.shields.io/github/v/release/trulymust/melonDS-switch-upscale?label=release"></a>
<a href="https://www.gnu.org/licenses/gpl-3.0" alt="License: GPLv3"><img src="https://img.shields.io/badge/License-GPL%20v3-%23ff554d.svg"></a>
</p>

## English

This is a maintained Nintendo Switch fork of melonDS based on the `switch-new` codebase. The immediate goal of this fork is to keep the Switch build usable while adding practical visual and menu improvements that are not available in the abandoned upstream Switch branch.

### Current Status

The current build is experimental but usable for testing on real hardware. It includes the following changes on top of the previous Switch codebase:

- Experimental Deko3D internal 3D resolution scaling: `1x`, `2x`, and `4x`.
- A new `Sharp` screen filter option in addition to nearest and linear filtering.
- In-game Action Replay cheat management from the melonDS pause menu.
- Faster savestate backup/load handling for the Switch frontend.
- A Switch menu localization framework with English and Simplified Chinese UI text.
- Existing Switch frontend features from the base fork, including RetroAchievements support.

### Important Notes

- Internal resolution scaling currently targets the 3D renderer. Native-resolution 2D layers, capture effects, and games that heavily mix 2D/3D may not show a dramatic difference in every scene.
- `4x` is intentionally exposed for quick visual testing. It can be expensive and may be unstable or slow depending on the game.
- The cheat menu loads the existing melonDS Action Replay code database for the loaded game. Put a matching `.mch` file next to the ROM when needed.
- Simplified Chinese can be selected from `Display settings` -> `GUI` -> `Language`.
- This fork is not a complete replacement for desktop melonDS. DSi mode and some Switch-specific frontend behavior remain work in progress.

### Install

Copy `melonDS.nro` to:

```text
/switch/melonds/melonDS.nro
```

Required DS BIOS/firmware files should be placed in `/switch/melonds` or `/melonds`:

- `bios7.bin`: ARM7 BIOS, 16KB
- `bios9.bin`: ARM9 BIOS, 4KB
- `firmware.bin`: DS firmware, 128KB/256KB/512KB

Firmware boot requires a firmware dump from an original DS or DS Lite. DSi/3DS DS-mode firmware dumps are usually only suitable for direct game boot.

### Build

The local development build has been verified with the `devkitpro/devkita64` Docker image and Ninja:

```bash
git submodule update --init --recursive
mkdir -p build-switch-codex
docker run --rm -v "$PWD":/work -w /work/build-switch-codex devkitpro/devkita64 sh -lc \
  '. /opt/devkitpro/switchvars.sh; cmake .. -G Ninja -DENABLE_OGLRENDERER=OFF -DBUILD_QT_SDL=OFF -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-cross-Switch.cmake; ninja -j2'
```

The output is:

```text
build-switch-codex/melonDS.nro
```

### Credits

This fork builds on the work of the melonDS project and the Switch frontend work by RSDuck, Jpe230, Gheovgos, and contributors from the previous Switch forks. Original melonDS credits include:

- Martin for GBAtek documentation.
- Cydrak for extra 3D GPU research.
- limittox for the old icon.
- daedra000 for the NX icons and banner.
- Everyone testing melonDS, reporting issues, and suggesting improvements.

## 中文

这是一个面向 Nintendo Switch 的 melonDS 维护分支，基于 `switch-new` 代码继续开发。这个 fork 的目标是让 Switch 版继续可用，并加入更实际的显示、菜单和易用性改进。

### 当前状态

当前版本属于实验性版本，但已经可以在真实 Switch 硬件上测试使用。相对原 Switch 代码库，当前加入了这些改动：

- 实验性的 Deko3D 3D 内部分辨率提升：`1x`、`2x`、`4x`。
- 新增 `Sharp` 锐化显示滤镜，原有最近邻和线性滤镜仍然保留。
- 在游戏暂停菜单中加入 Action Replay 金手指管理。
- 优化 Switch 前端的即时存档备份和读档处理速度。
- 加入菜单本地化框架，目前支持英文和简体中文。
- 保留基础分支已有的 Switch 前端功能，包括 RetroAchievements 支持。

### 注意事项

- 内部分辨率提升目前主要作用于 3D 渲染。原生分辨率的 2D 图层、画面捕获效果，以及大量混合 2D/3D 的场景，不一定每个画面都能明显变清晰。
- `4x` 主要用于快速观察效果，性能和稳定性会因游戏而异。
- 金手指菜单使用 melonDS 现有的 Action Replay 数据库逻辑。需要时，把匹配游戏的 `.mch` 文件放在 ROM 旁边。
- 中文菜单路径：`Display settings` -> `GUI` -> `Language` -> `Simplified Chinese`。
- 这个 fork 不是桌面版 melonDS 的完整替代品。DSi 模式和部分 Switch 前端功能仍然在完善中。

### 安装

把 `melonDS.nro` 复制到：

```text
/switch/melonds/melonDS.nro
```

DS BIOS/firmware 文件放到 `/switch/melonds` 或 `/melonds`：

- `bios7.bin`：ARM7 BIOS，16KB
- `bios9.bin`：ARM9 BIOS，4KB
- `firmware.bin`：DS firmware，128KB/256KB/512KB

如果要从固件启动，需要原始 DS 或 DS Lite 的 firmware dump。DSi/3DS 的 DS 模式 firmware dump 通常只适合直接启动游戏。

### 构建

本地开发构建已用 `devkitpro/devkita64` Docker 镜像和 Ninja 验证：

```bash
git submodule update --init --recursive
mkdir -p build-switch-codex
docker run --rm -v "$PWD":/work -w /work/build-switch-codex devkitpro/devkita64 sh -lc \
  '. /opt/devkitpro/switchvars.sh; cmake .. -G Ninja -DENABLE_OGLRENDERER=OFF -DBUILD_QT_SDL=OFF -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-cross-Switch.cmake; ninja -j2'
```

生成文件位置：

```text
build-switch-codex/melonDS.nro
```

### 致谢

这个 fork 建立在 melonDS 项目以及 RSDuck、Jpe230、Gheovgos 和其他 Switch 分支贡献者的工作之上。原 melonDS 项目的致谢包括：

- Martin 提供的 GBAtek 文档。
- Cydrak 的额外 3D GPU 研究。
- limittox 提供的旧图标。
- daedra000 提供的 NX 图标和 banner。
- 所有测试 melonDS、报告问题并提出建议的人。

## License

[![GNU GPLv3 Image](https://www.gnu.org/graphics/gplv3-127x51.png)](http://www.gnu.org/licenses/gpl-3.0.en.html)

melonDS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
