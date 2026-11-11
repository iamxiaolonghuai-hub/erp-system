# 腾讯云部署：2026-10-31

适用 Linux AMD64 / x86_64，2核4G可用于小规模业务，实际容量取决于业务量。本包包含 ERP 前后端镜像，不包含 MySQL 镜像、本地数据库数据、上传附件或真实密码。服务器已有 MySQL 时直接连接它。

## 1. 上传、解压、加载

上传本包至服务器（例如 /opt/erp），执行：

```sh
uname -m
# 必须是 x86_64；aarch64 需要另行构建 ARM64 镜像
tar -xzf erp-server-linux-amd64-2026-11-01-r3.tar.gz
docker load -i erp-images.tar
cp .env.example .env
```

编辑 `.env`，填入已有 MySQL 的容器名、数据库名、账号和密码，设置独立的 JWT_SECRET（可用 `openssl rand -hex 32` 生成）。DB_PORT 一般填容器内端口3306。ERP_HTTP_PORT 默认8080，如果已被其他应用使用则换端口。

## 2. 连接现有 MySQL

```sh
docker ps
docker network inspect erp-shared >/dev/null 2>&1 || docker network create erp-shared
docker network connect erp-shared 你的MySQL容器名
```

已加入网络时不用重复 connect。`.env` 中 DB_HOST 填上面的 MySQL 容器名，MYSQL_NETWORK 填 erp-shared。不要填127.0.0.1；容器内它指向 ERP 自己。也可使用 MySQL 现有的自定义网络，并在 MYSQL_NETWORK 中填写该网络名。数据库账号必须允许该网络的连接。

## 3. 准备 ERP 数据库

若服务器已有 ERP 业务库，先备份，停止ERP后端写入，再执行以下增量迁移。2026-11-01-r3会移除旧部门文字、序列号CSV和收发JSON明细等列，必须配套升级后端。MySQL及博客服务无需停止。

```sh
docker compose stop backend

docker compose run --rm --no-deps backend python3 db/migrate_approvers.py
docker compose run --rm --no-deps backend python3 db/migrate_warehouse_approver.py
docker compose run --rm --no-deps backend python3 db/migrate_order_warehouse.py
docker compose run --rm --no-deps backend python3 db/migrate_reliability.py
docker compose run --rm --no-deps backend python3 db/migrate_master_import.py
docker compose run --rm --no-deps backend python3 db/migrate_operations.py
docker compose run --rm --no-deps backend python3 db/migrate_batches.py
docker compose run --rm --no-deps backend python3 db/migrate_lots.py
docker compose run --rm --no-deps backend python3 db/migrate_costing.py
docker compose run --rm --no-deps backend python3 db/migrate_backup_settings.py

docker compose run --rm --no-deps backend python3 db/migrate_departments.py --check-only

docker compose run --rm --no-deps backend python3 db/migrate_normalized_details.py --check-only

docker compose run --rm --no-deps backend python3 db/migrate_departments.py

docker compose run --rm --no-deps backend python3 db/migrate_normalized_details.py
docker compose run --rm --no-deps backend python3 db/migrate_finance_periods.py
docker compose run --rm --no-deps backend python3 db/migrate_serial_inventory.py
```

迁移需要 ALTER/CREATE 权限。**不要在已有业务库运行 db/schema.sql，它包含 DROP TABLE。**

如果服务器只有 MySQL、还没有 ERP 库，应先创建专用空数据库（utf8mb4），再仅向这个空库导入 db/schema.sql。也可将本地 ERP 数据库备份恢复到服务器，之后执行上述增量迁移。全新库的演示账号密码见 schema.sql，正式使用前应修改。

## 4. 启动

```sh
docker compose up -d --no-build --pull never
docker compose ps
docker compose logs --tail=100 backend frontend
curl -I http://127.0.0.1:8080/
```

腾讯云防火墙或安全组放行 ERP_HTTP_PORT，访问 `http://服务器公网IP:8080`（使用自定义端口时相应替换）。外部数据库无需额外开放公网3306。后台不单独暴露5555端口。已有域名、HTTPS反向代理可转发到本机8080。

前后端镜像标签均为2026-11-01-r3。更新同一目录的部署时保留原 `.env` 和 uploads 命名卷，加载镜像、迁移后重新启动即可。不要执行 `docker compose down -v`。

## 构建说明

本次常规 Docker 多阶段编译被镜像仓库 EOF 阻断，因此交付镜像采用已编译验证的当前 WSL 后端及其运行依赖封装，前端为当前生产构建，基于本机缓存的 nginx:1.27-alpine。两者均为 Linux AMD64。容器内包含 Python 和 MySQL 客户端，支持上述迁移；不包含 MySQL 服务端。后端以非 root 用户运行。

源码构建 Dockerfile 仍保留在源码包中；网络可用时可以按 deploy/README.md 重新构建。容器冒烟测试使用独立临时 MySQL，覆盖前端、API代理、迁移以及供应商/客户导入导出。
