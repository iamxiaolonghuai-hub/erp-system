# Docker 构建与云服务器部署

应用包含后端与 Nginx 前端两个镜像，不包含 MySQL。现有数据库通过 Docker 外部网络连接。默认 HTTP 端口 8080，上传文件保存在命名卷中。

## 从源码部署

在 Linux 或 WSL 环境中按[项目快速开始](../README.md#快速开始)配置数据库网络、构建镜像、初始化空库和迁移。首次安装可以使用演示数据验证流程，正式部署时应更换或停用演示账号。

`DB_HOST` 填写 ERP 后端所在网络能解析的 MySQL 容器名，`ERP_HTTP_PORT` 默认 8080。正式域名可通过现有反向代理提供 HTTPS。上传附件随 Compose 命名卷持久化。

## 升级已有数据库

升级前备份 ERP 数据库和附件，并停止当前后端写入。按[迁移顺序](../README.md#快速开始)执行所需迁移。执行部门和规范化明细迁移前，分别运行只读预检：

```sh
docker compose stop backend
docker compose run --rm --no-deps backend python3 db/migrate_departments.py --check-only
docker compose run --rm --no-deps backend python3 db/migrate_normalized_details.py --check-only
```

老版本如缺少前序迁移，应先按顺序补齐前序迁移，再预检这两项。预检通过后，去掉 `--check-only` 按相同顺序执行，最后启动与新结构配套的后端。已有业务库只使用增量迁移。

## 本地构建镜像后上传

在可联网的 Docker 环境执行 `sh deploy/package.sh`，得到：

- `release/erp-images.tar.gz`：两个完整镜像。
- `release/erp-deploy.tar.gz`：Compose 与部署配置。

上传并在服务器加载：

```sh
scp release/erp-images.tar.gz release/erp-deploy.tar.gz 用户@服务器IP:/opt/erp/
# 服务器执行
cd /opt/erp
tar -xzf erp-deploy.tar.gz
gzip -dc erp-images.tar.gz | docker load
cp .env.example .env
# 编辑配置并连接 MySQL 网络、按前述步骤迁移数据库
docker compose up -d --no-build
```

默认构建架构与构建机器一致。若服务器为 ARM64，请在 ARM64 服务器构建，或使用 buildx 为目标平台构建；不要将 AMD64 镜像直接当作 ARM64 原生镜像。

## 停止与更新

`docker compose down` 停止应用，命名卷保留。不要添加 `-v`，否则会删除上传文件卷。更新前备份数据库和上传文件卷，加载新镜像后执行 `docker compose up -d --no-build`。

构建时基础镜像、Drogon 源码、系统与 npm 依赖均需要网络。如果 Docker Desktop 镜像代理返回 EOF，应先修复 Docker 的代理或镜像加速配置，或在可联网的服务器构建。
