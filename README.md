# SigGen_Meas_System

## 1. 工程描述
- **硬件**: STM32F407ZGT6 (HSE: 25MHz)
- **环境**: Keil-MDK (Arm Compiler 5), 编码 GB2312
- **固件**: STM32Cube_FW_F4_V1.28.3, Keil.STM32F4xx_DFP.2.17.0
- **结构**: 内置 DSP 库；`BSP/` (底层驱动)；`APP/` (业务逻辑)

## 2. 获取源码
```bash
git clone https://github.com/404elf/SigGen_Meas_System.git
```

## 3. 协作规范

**分支工作流**：
```bash
git checkout -b <分支名>       # 新建分支
git push -u origin <分支名>    # 关联远程并推送
```

**日常提交**：
```bash
git pull                       # 1. 更新
git add .                      # 2. 暂存
git commit -m "feat/fix: 描述" # 3. 提交/存档
git push                       # 4. 推送
```

**单文件同步** (避免 `.ioc` 冲突)：
```bash
# 警告: 将覆盖本地对应文件，需先 commit 本地更改。
git fetch origin
git checkout <队友分支名> -- <文件路径>
```
> *注：若拉取了全新文件，需在 Keil 工程树中手动添加。*

## 4. 技术支持与问题排查
开发中如遇疑难问题，建议优先借助 AI 工具辅助分析，或直接向团队成员提问探讨解决。闲暇之余，也推荐向 AI 学习更多的 Git 进阶使用技巧。