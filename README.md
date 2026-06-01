# ailiasDesktopLayoutSaver

桌面布局保存与恢复工具。保存 Windows 桌面图标位置，支持 20 个独立存档位，一键恢复乱掉的桌面布局。

## 工作原理

Windows 将桌面图标位置信息存储在注册表中两个关键的路径下：

- `HKEY_CURRENT_USER\Software\Microsoft\Windows\Shell\Bags\1\Desktop`
- `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Streams\Desktop`

**保存**时，软件枚举这两个注册表键下的所有值，将其完整序列化为二进制存档文件（含屏幕分辨率以便后续校验）。

**恢复**时，软件先删除注册表中的现有布局数据，再写入存档中的值，最后通过重启 Explorer 进程让 Windows 重新读取注册表，刷新桌面布局。

## 功能

- 20 个独立存档位，每个可自定义命名
- 支持设置"主用布局"，最小化到托盘后右键一键恢复
- 双击存档条目快速恢复，右键菜单支持删除/重命名/设置为主用
- Windows 深色模式自适应
- 最小化到系统托盘，后台静默运行

## 下载

### 安装版（推荐）
运行 `ailiasDesktopLayoutSaver Setup.exe`，选择安装目录后自动完成安装。
- 创建开始菜单和桌面快捷方式
- 注册到控制面板「添加/删除程序」
- 自带卸载器

### 免安装版（绿色版）
直接运行 `ailiasDesktopLayoutSaver.exe`，无需安装。
- 数据保存在 `%APPDATA%\ailiasDesktopLayoutSaver\`
- 关闭窗口最小化到系统托盘，托盘右键可退出

## 使用说明

1. **保存布局**：选择一个空槽位，点击"保存当前布局"
2. **命名存档**：选中槽位，再次点击名称进入编辑
3. **设置主用**：右键槽位 → "设为主用布局"
4. **一键恢复**：点击顶部"一键恢复主用布局"按钮，或托盘右键菜单
5. **手动恢复**：双击任意已保存的槽位

## 系统要求

- Windows 10 / 11（64 位）
- 无需额外运行库（已静态编译）

## 构建

```bash
cl /O2 /MT main.cpp /Fe:ailiasDesktopLayoutSaver.exe \
   /link user32.lib gdi32.lib comctl32.lib shell32.lib \
   advapi32.lib dwmapi.lib uxtheme.lib
```
