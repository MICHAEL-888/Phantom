# 灵蛇ARK (Phantom Anti-Rootkit)

一款r3层强大的Windows平台反Rootkit安全工具。

## 功能特性

- 进程管理
- 文件系统管理
- 注册表管理
- 系统服务检测

## 系统要求

- Windows 10/11 (x86/x64/arm64)

## 使用说明

1. 以管理员身份运行程序

## 技术架构

## 开发进度

- [ ] 用户界面开发
	- [ ] 窗口置顶
- [ ] 进程管理
	- [x] 枚举进程名、PID以及进程路径
	- [x] 暴力枚举pid，检测隐藏进程
	- [x] 支持x86,x64进程模块检测
	- [x] 强力结束进程
	- [ ] 句柄检测
	- [x] 线程枚举
	- [ ] 钩子扫描
- [ ] 文件系统管理
	- [ ] 签名校验以及安全扫描功能
- [ ] 注册表管理
- [ ] 额外功能
	- [x] 提权至SYSTEM 

## 开源协议

本项目采用 [MIT](LICENSE) 开源协议。

## 贡献指南

欢迎提交Issue和Pull Request参与项目开发。
