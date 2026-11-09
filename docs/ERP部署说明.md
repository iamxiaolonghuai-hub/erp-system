# ERP 系统部署说明

版本日期：2026年10月31日  
适用环境：腾讯云 Linux 服务器，2核4G，已运行 Docker MySQL  
交付平台：Linux AMD64，服务器 `uname -m` 必须返回 `x86_64`

本文说明如何上传离线镜像、连接现有 MySQL、准备数据库、启动应用和处理常见故障。部署只启动 ERP 后端和前端两个容器，不会另外安装 MySQL。2核4G可用于小规模业务；当前未进行生产容量压测。

## 一 交付内容与系统结构

部署包为 `erp-server-linux-amd64-2026-10-31.tar.gz`，约53.5MB。解压后主要内容如下。

| 文件 | 用途 |
| --- | --- |
| erp-images.tar | 前后端 Docker 镜像，可离线加载 |
| compose.yaml | 启动 ERP 前后端，连接已有 MySQL 的外部网络 |
| .env.example | 环境变量模板，不含真实密码 |
| db/schema.sql | 全新空库的表结构和演示数据，包含删表语句 |
| db/migrate_*.py | 对已有 ERP 数据库补齐字段和辅助表 |
| BUILD-INFO.json | 镜像标识和构建方式 |
| deploy/SERVER-README.md | 随包部署操作说明 |

访问链路：浏览器 → 服务器 HTTP 端口 → Nginx 前端 → C++ Drogon 后端 → 已有 MySQL。

前端技术为 Vue，后端业务逻辑为 C++。Python 用于生成启动配置和执行数据库迁移，不是业务接口的主要实现语言。本次常规 Docker 多阶段编译因镜像仓库返回 EOF 失败，交付镜像采用已经编译验证的 WSL 后端及所需运行依赖封装；前端采用当前生产构建和缓存的 nginx:1.27-alpine。

部署包不包含本地业务数据库、真实账号密码或已上传附件。需要保留本地业务数据时，应单独备份数据库和附件并迁移；上传镜像不会自动带入这些数据。

## 二 部署前准备

在服务器执行：

```sh
uname -m
docker version
docker compose version
docker ps
df -h
free -h
```

确认架构为 x86_64，Docker 服务正常，有 Docker Compose v2，并记下现有 MySQL 容器名。若结果为 aarch64，需要重新制作 ARM64 镜像，不能直接使用本包。

准备好 MySQL 数据库名、数据库账号和密码。账号需允许 ERP 容器所在网络连接；迁移期间还需要 CREATE 和 ALTER 权限。已有其他应用的 MySQL 可以共用服务，但应使用独立的 ERP 数据库，避免初始化到其他业务库。

在腾讯云控制台为 ERP 的 HTTP 端口设置入站规则。默认是8080，使用其他端口时按实际配置放行。不需要为了 ERP 连接而开放公网3306，也不需要暴露后端5555端口。

## 三 上传并加载镜像

在服务器准备目录：

```sh
mkdir -p /opt/erp
```

将部署包上传到该目录。可以使用腾讯云文件传输工具，或在本地执行以下命令，将用户、IP和本地文件路径替换为实际值：

```sh
scp erp-server-linux-amd64-2026-10-31.tar.gz 用户@服务器IP:/opt/erp/
```

在服务器执行：

```sh
cd /opt/erp
tar -xzf erp-server-linux-amd64-2026-10-31.tar.gz
docker load -i erp-images.tar
docker image inspect erp-backend:2026-10-31 erp-frontend:2026-10-31
```

如同时上传校验文件，可在解压前执行 `sha256sum -c erp-server-linux-amd64-2026-10-31.tar.gz.sha256`。

## 四 配置数据库连接

首次部署复制模板，升级时保留原有配置，不要覆盖：

```sh
cp .env.example .env
chmod 600 .env
openssl rand -hex 32
```

最后一条命令生成随机密钥，用作 JWT_SECRET。编辑 `.env`，按下面的字段说明填写。以下仅为示例，不可直接作为真实密码使用。

```dotenv
DB_HOST=你的MySQL容器名
DB_PORT=3306
DB_NAME=erp_system
DB_USER=erp
DB_PASSWORD=替换为数据库密码
JWT_SECRET=替换为生成的随机密钥
MYSQL_NETWORK=erp-shared
ERP_HTTP_PORT=8080
```

| 配置项 | 填写规则 |
| --- | --- |
| DB_HOST | 现有 MySQL 的容器名或该网络内可解析的别名 |
| DB_PORT | MySQL 容器内部监听端口，通常3306；不是宿主机映射端口 |
| DB_NAME | ERP 专用数据库名，需事先创建或恢复 |
| DB_USER 与 DB_PASSWORD | 可访问该数据库的账号密码 |
| JWT_SECRET | 至少32个字符的随机密钥，不使用模板原值 |
| MYSQL_NETWORK | MySQL 与 ERP 后端共同接入的自定义 Docker 网络 |
| ERP_HTTP_PORT | 服务器对外 HTTP 端口，默认8080，避免与已有应用冲突 |

`.env` 使用 Compose 的环境文件语法。密码含空格、井号、美元符号等字符时，应正确加引号，并检查配置解析是否成功；不要将真实 `.env` 发给他人。

DB_HOST 不要填127.0.0.1，ERP 容器内它指向 ERP 自己。

## 五 将已有 MySQL 接入网络

如果 MySQL 已在自定义网络中，可以直接将该网络名填入 MYSQL_NETWORK，无需另建。若需要新建共享网络，执行：

```sh
docker network inspect erp-shared >/dev/null 2>&1 || docker network create erp-shared
docker network connect erp-shared 你的MySQL容器名
```

MySQL 已接入时不要重复 connect。加入额外网络不要求停止 MySQL。默认 Compose 中 MySQL 网络为 external，部署时不会自动创建，因此应先完成此步骤。

## 六 准备数据库

### 情况一 服务器已有 ERP 业务库

先备份数据库，再执行增量迁移。以下操作以当前目录 `/opt/erp`、已配置好的 `.env` 为前提：

```sh
docker compose run --rm --no-deps backend python3 db/migrate_approvers.py
docker compose run --rm --no-deps backend python3 db/migrate_warehouse_approver.py
docker compose run --rm --no-deps backend python3 db/migrate_order_warehouse.py
docker compose run --rm --no-deps backend python3 db/migrate_reliability.py
docker compose run --rm --no-deps backend python3 db/migrate_master_import.py
```

这些脚本检查缺失项后增加字段或表，分别支持指定审批人、仓库批准人、订单仓库、业务轨迹和请求去重、盘点人员、资料导入锁。不会为历史盘点单凭空补填盘点人。

**已有 ERP 业务库不要执行 schema.sql，其中包含 DROP TABLE，会删除原有业务表及数据。**

### 情况二 服务器有 MySQL 但没有 ERP 数据库

可以选择以下一种方式：

1. 保留本地业务数据：将本地 ERP 数据库完整备份恢复到服务器的 ERP 专用库，再执行上述增量迁移；附件也要单独迁移。
2. 从空系统开始：创建专用空库，字符集 utf8mb4，仅向该空库导入 db/schema.sql。该脚本含演示数据，正式使用前更换演示账号密码并核对基础资料。

例如先将初始化文件复制进现有 MySQL 容器：

```sh
docker cp db/schema.sql 你的MySQL容器名:/tmp/erp-schema.sql
docker exec -it 你的MySQL容器名 sh
```

在 MySQL 容器的交互终端中，使用具备建库权限的账号执行，交互输入密码：

```sh
mysql -uroot -p
```

在 MySQL 终端中，仅在确认该库为新建空库时执行：

```sql
CREATE DATABASE erp_system CHARACTER SET utf8mb4;
USE erp_system;
SOURCE /tmp/erp-schema.sql;
EXIT;
```

如果创建数据库提示库已存在，应先核对内容和用途，不要直接继续初始化。为 ERP 配置合适的数据库账号后，退出容器终端，再执行上一节的迁移命令，以确认增量结构齐全。

## 七 启动与验收

```sh
cd /opt/erp
docker compose config --quiet
docker compose up -d --no-build --pull never
docker compose ps
docker compose logs --tail=100 backend frontend
curl -I http://127.0.0.1:8080/
```

改过 ERP_HTTP_PORT 的，应替换 curl 端口。浏览器访问 `http://服务器公网IP:8080`。正式域名可通过服务器现有反向代理配置 HTTPS。

验收时确认：能够登录；角色进入对应工作台；商品、供应商、客户正常显示；管理员能导入和导出 CSV；库存预警与低库存列表一致；采购、销售、盘点使用独立测试单据能按角色完成流转。正式库中不要用真实业务单据随意试扣库存或收付款。

本次镜像已在独立临时 MySQL 环境验证页面、API代理、数据库连接、五个迁移脚本、供应商及客户导入导出与自动去重；服务器部署后的网络、实际库结构、账号和数据仍需按上述步骤验收。

## 八 日常维护与更新

查看运行状态及日志：

```sh
docker compose ps
docker compose logs --tail=100 backend frontend
docker stats --no-stream
```

更新前备份 ERP 数据库、`.env` 和 uploads 命名卷中的附件。保持原部署目录和 Compose 项目名，以便继续使用原命名卷；将旧镜像标识和旧部署包保留到新版本验收完成。

更新步骤为：加载新镜像 → 按新版本说明迁移 → `docker compose up -d --no-build --pull never` → 验收。数据库变更后的回退不能只换旧镜像，应核对结构兼容性，必要时恢复升级前数据库备份。

`docker compose down` 会停止应用但保留命名卷；**不要添加 `-v`，它会删除包括附件在内的项目命名卷。** 不要手动清理现有 MySQL 的卷。

2核4G上应持续观察 MySQL 和 ERP 的内存、CPU、磁盘占用。容量或并发增长时，应先压测再调整配置，不能仅凭能启动就认定可承载生产峰值。

## 九 常见问题

| 现象 | 优先检查 |
| --- | --- |
| 页面打不开 | 容器状态、端口映射、端口冲突、腾讯云安全组和系统防火墙 |
| 页面有内容但 API 返回502 | backend 日志、MySQL连接、数据库结构、容器网络 |
| 无法解析 MySQL 容器名 | DB_HOST、MYSQL_NETWORK 和 MySQL 是否接入同一自定义网络 |
| Access denied | 数据库用户名密码、账号允许连接的来源、数据库授权 |
| 表或字段不存在 | 是否连接正确 ERP 数据库，是否已完成初始化或增量迁移 |
| external network not found | 先创建或填写已经存在的外部网络 |
| exec format error | CPU架构不匹配，核对 uname -m 和镜像平台 |
| 缺少环境变量或 JWT_SECRET 过短 | 检查 .env 是否齐全、密钥是否至少32字符 |
| 更新后需要重新登录 | JWT密钥变更或登录状态过期，重新登录；无必要时保留原密钥 |

## 十 对应配置与验证文件

部署配置：compose.yaml、.env.example、deploy/entrypoint.py、deploy/config.template.json、deploy/nginx.conf。数据库脚本：db/。镜像验证：test/verify_docker_package.py。该文档与2026年10月31日交付包对应，后续版本应按实际改动更新。
