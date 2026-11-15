# GitHub 更新

安装器的“GitHub 更新”按钮用于更新选中的已有完整资源版安装目录。安装完成后，也可双击安装目录的“检查更新.cmd”。首次资源安装仍从固定版本的 CDN 下载，GitHub Release 只分发训练器更新。

公开渠道不需要下载者登录。发布者在 `profiles/github-update.json` 配置 `repository`，值为 `owner/repository`。当前正式仓库为 `bigfragrance/valoranttrainer`，安装器及更新包均已配置该渠道。更换渠道后重建安装器及更新包，确保二者携带相同配置。

## 发布

1. 构建包含当前模块的完整资源版更新 ZIP，保留根目录 `patch-version.json` 与 `package-sha256.json`。
2. 执行 `python tools/build_github_update_manifest.py exports/ValorantTrainer-Update-20261115.zip`。
3. 创建正式 GitHub Release，tag 与包内 `release` 相同，可加 `v` 前缀。
4. 同一 Release 上传 ZIP、生成的 `trainer-update.json`、安装 EXE 和对应 SHA256 文件。全部上传完毕后发布 Release。
5. 用旧版本目录执行 `runtime/python.exe -I -B tools/github_update.py --repository owner/repository` 验证远程检查，再运行 `--install` 验证升级。

## 完整性与恢复

更新器获取 GitHub latest 正式 Release，核对版本、固定资源 manifest、资产大小、SHA256，再核对包内每份文件及原生源码快照。归档路径、重复文件、符号链接和展开大小受限制。SHA256 的信任来源是配置的 GitHub 仓库及 HTTPS，发布账户应限制写入者。

更新与安装共用 `.setup/install.lock`。替换产品文件前在 `.setup/update-transaction` 保存备份与 journal，遇到写入或校验错误回滚。进程中断后，重试更新会先恢复未完成事务。运行中的游戏或控制台可能锁定文件，应先关闭，随后重试。用户 `.local` 设置、资源和下载进度受包路径规则保护。

2026.11.15 已发布到公开仓库及正式 Release。本次验证覆盖受控元数据响应、完整更新 ZIP、损坏下载、部分写入故障回滚、未完成 journal 恢复和现有更新测试。另使用无账户凭据的更新器，从真实 GitHub Release 完成版本检查、下载 536 文件更新包、应用更新、保留设置与资源，再次检查确认已是最新版。公开安装 EXE 也已重新下载并核对 SHA256。跨机器完整资源安装仍待验收。
