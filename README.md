# 中小企业 ERP 管理系统

面向中小企业的进销存与财务管理系统。采购单经审批后分批入库并生成应付，销售单在备货时预占库存，放行后扣减库存并生成应收。采购退货、销售退换货、库存盘点和财务结算均保留可追溯的业务记录。

项目采用 **Vibe Coding（AI 协作开发）**。先明确审批角色、库存变动和记账时点，再借助 AI 迭代前后端实现，通过接口回归、并发测试及数据库核对验证关键流程。

## 功能

| 模块 | 功能 |
| --- | --- |
| 采购与销售 | 订单审批、分批收发货、关闭剩余数量、关联原单的退货和等价换货 |
| 仓储 | 待入库、待出库和退换货待办，库存预占与释放，安全库存预警，盘点与差异复核 |
| 批次与成本 | 按商品启用批次和效期，限制过期批次的普通销售出库，按商品和仓库计算移动加权平均成本 |
| 财务 | 应收应付、收付款、退款、抵扣与冲正，资金操作由另一位人员复核 |
| 组织与权限 | 按角色、部门和仓库控制访问；支持指定审批人、员工调度和待办移交 |
| 记录与运维 | 按处理时间、原单号或出入库批次号查询历史；客户与供应商 CSV 导入去重、数据备份与恢复 |

## 技术与设计

- **后端**：C++17 或更高版本、Drogon、CMake、OpenSSL、MySQL 8.0。JWT 用于身份认证，业务服务继续检查账号状态、权限和单据归属。
- **前端**：Vue 3、TypeScript、Vite、Pinia、Element Plus、Axios、ECharts。按采购、销售、仓库、财务和管理员职责提供操作入口。
- **一致性**：同一批收发涉及的库存、流水、收发明细和账目在同一事务中写入。关键路径使用行锁和幂等校验，防止并发超卖和重试导致重复过账。
- **部署**：Docker Compose 运行 Nginx 前端与 Drogon 后端。Nginx 转发 `/api`，后端通过 Docker 网络连接已有的 MySQL，上传文件保存在命名卷。

## 快速开始

准备 Linux 或 WSL、Docker Compose v2 和一个运行中的 MySQL 8.0 容器。创建 **独立空 ERP 数据库**，为应用分配仅可访问该库的账号，并授予初始化与迁移所需权限。

在仓库根目录执行：

```sh
cp .env.example .env
docker network inspect erp-shared >/dev/null 2>&1 || docker network create erp-shared
docker network connect erp-shared my-mysql
```

执行前将 `my-mysql` 替换为实际容器名。若 MySQL 已接入该网络，可跳过连接命令。编辑 `.env`，设置 `DB_HOST` 为 MySQL 容器名，填写 `DB_NAME`、`DB_USER`、`DB_PASSWORD`，并用 `openssl rand -hex 32` 生成 `JWT_SECRET`。使用其他已有 Docker 网络时，将其名称填入 `MYSQL_NETWORK`。

```sh
docker compose build

# 仅首次安装时执行，目标必须是新建的空 ERP 数据库
docker compose run --rm --no-deps backend sh -c '
  MYSQL_PWD="$DB_PASSWORD" mysql --default-character-set=utf8mb4 \
    -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" "$DB_NAME" < db/schema.sql
'
```

> `db/schema.sql` 包含 `DROP TABLE`。升级已有数据库时，先备份并暂停后端写入，再执行增量迁移。

按以下顺序执行迁移。已有库升级请参照[部署说明](deploy/README.md#升级已有数据库)中的预检步骤：

```sh
set -e
for migration in approvers warehouse_approver order_warehouse reliability \
  master_import operations batches lots costing backup_settings departments normalized_details
do
  docker compose run --rm --no-deps backend python3 "db/migrate_${migration}.py"
done

docker compose up -d --no-build
docker compose ps
curl -I http://127.0.0.1:8080/
```

访问 [http://localhost:8080](http://localhost:8080)。从仓库的初始化 SQL 建立的演示库可用 `admin / admin123` 登录。演示账号仅用于本地试用，正式使用前请更换或停用所有演示账号，并为入口配置 HTTPS。数据库和 Drogon 端口可留在 Docker 网络内。

## 开发与验证

前端单独开发时，在 `frontend/` 执行 `npm ci`、`npm run dev`，默认访问 [http://localhost:5173](http://localhost:5173)。Vite 将 `/api` 转发给本机的 `127.0.0.1:5555`。生成前端生产包使用 `npm run build`。

后端本地开发需要安装启用了 MySQL 支持的 Drogon 及其构建依赖。从 [配置模板](deploy/config.template.json) 创建根目录下的 `config.json`，填写已初始化的测试库、JWT 密钥，并按本地环境调整上传路径。在仓库根目录运行：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target erp_system -j2
./build/erp_system
```

`test/` 中的 Python 脚本覆盖真实登录、审批移交、仓库权限、分批收发、并发预占和财务复核。脚本默认通过根目录的 `config.json` 连接 MySQL，接口测试访问 `127.0.0.1:5555`。业务回归会创建测试数据，请在独立测试库中运行。可用 `python3 test/audit_business_consistency.py` 执行只读一致性检查。

## 目录

```text
controllers/   HTTP 接口
filters/       认证和角色检查
services/      审批、库存、收发、退货与财务逻辑
models/        数据库 ORM 模型
frontend/      Vue 工作台与管理页面
db/            初始化 SQL、增量迁移及可选演示数据
deploy/        镜像配置、打包及备份恢复工具
test/          接口回归、并发验证和一致性检查
docs/          功能说明与操作指南
```

更多说明：[角色功能与流程](docs/ERP角色功能模块说明.md) · [数据库规范化与部门管理](docs/数据库规范化与部门管理说明.md) · [Docker 部署](deploy/README.md) · [备份恢复](docs/backup-restore-guide.md)

公开源码时使用 `.env.example` 和配置模板。实际环境配置、数据库备份、初始账号清单、上传附件与构建产物由 `.gitignore` 排除。
