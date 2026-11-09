-- =====================================================================
-- 基于 C++ Drogon 框架的中小企业 ERP 系统 —— 数据库表结构
-- 数据库: erp_system  字符集: utf8mb4  MySQL 8.0
-- 六大模块: 基础数据管理 / 采购管理 / 销售管理 / 库存管理 / 基础财务管理 / 系统管理
-- =====================================================================

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- =====================================================================
-- 一、系统管理模块（用户 / 角色 / 权限 / 操作日志）
-- =====================================================================

DROP TABLE IF EXISTS `departments`;
CREATE TABLE `departments` (
    `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '部门ID',
    `name`        VARCHAR(64) NOT NULL COMMENT '部门名称',
    `code`        VARCHAR(64) NOT NULL COMMENT '部门编码',
    `description` VARCHAR(255) NOT NULL DEFAULT '',
    `created_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_code` (`code`),
    UNIQUE KEY `uk_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='部门表';

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users` (
    `id`            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '用户ID',
    `username`      VARCHAR(64)  NOT NULL COMMENT '登录名',
    `password`      VARCHAR(255) NOT NULL COMMENT '密码（bcrypt 哈希）',
    `real_name`     VARCHAR(64)  NOT NULL DEFAULT '' COMMENT '真实姓名',
    `email`         VARCHAR(128) NOT NULL DEFAULT '' COMMENT '邮箱',
    `phone`         VARCHAR(32)  NOT NULL DEFAULT '' COMMENT '手机号',
    `status`        TINYINT      NOT NULL DEFAULT 1 COMMENT '状态: 1启用 0禁用',
    `last_login_at` DATETIME     NULL COMMENT '最后登录时间',
    `created_at`    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`    DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `department`    VARCHAR(64)  NOT NULL DEFAULT '' COMMENT '所属部门',
    `warehouse_id`  BIGINT UNSIGNED NULL COMMENT '所属仓库（仓库部员工）',
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_username` (`username`),
    KEY `idx_department` (`department`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户表';

DROP TABLE IF EXISTS `roles`;
CREATE TABLE `roles` (
    `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '角色ID',
    `name`        VARCHAR(64) NOT NULL COMMENT '角色名称',
    `code`        VARCHAR(64) NOT NULL COMMENT '角色编码',
    `description` VARCHAR(255) NOT NULL DEFAULT '',
    `created_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_code` (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色表';

DROP TABLE IF EXISTS `permissions`;
CREATE TABLE `permissions` (
    `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '权限ID',
    `name`        VARCHAR(64) NOT NULL COMMENT '权限名称',
    `code`        VARCHAR(128) NOT NULL COMMENT '权限编码',
    `module`      VARCHAR(64) NOT NULL DEFAULT '' COMMENT '所属模块',
    `description` VARCHAR(255) NOT NULL DEFAULT '',
    `created_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_code` (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='权限表';

DROP TABLE IF EXISTS `user_roles`;
CREATE TABLE `user_roles` (
    `user_id` BIGINT UNSIGNED NOT NULL,
    `role_id` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`user_id`, `role_id`),
    KEY `idx_role_id` (`role_id`),
    CONSTRAINT `fk_ur_user` FOREIGN KEY (`user_id`) REFERENCES `users` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_ur_role` FOREIGN KEY (`role_id`) REFERENCES `roles` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户角色关联表';

DROP TABLE IF EXISTS `role_permissions`;
CREATE TABLE `role_permissions` (
    `role_id`       BIGINT UNSIGNED NOT NULL,
    `permission_id` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`role_id`, `permission_id`),
    KEY `idx_permission_id` (`permission_id`),
    CONSTRAINT `fk_rp_role` FOREIGN KEY (`role_id`) REFERENCES `roles` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_rp_perm` FOREIGN KEY (`permission_id`) REFERENCES `permissions` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色权限关联表';

-- 操作日志表
DROP TABLE IF EXISTS `operation_logs`;
CREATE TABLE `operation_logs` (
    `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志ID',
    `user_id`     BIGINT UNSIGNED NULL COMMENT '操作人ID',
    `username`    VARCHAR(64) NOT NULL DEFAULT '' COMMENT '操作人登录名',
    `method`      VARCHAR(16) NOT NULL COMMENT 'HTTP方法',
    `path`        VARCHAR(255) NOT NULL COMMENT '请求路径',
    `ip`          VARCHAR(64) NOT NULL DEFAULT '' COMMENT '来源IP',
    `status_code` INT NOT NULL DEFAULT 0 COMMENT '响应状态码',
    `cost_ms`     BIGINT NOT NULL DEFAULT 0 COMMENT '耗时(毫秒)',
    `created_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_user_id` (`user_id`),
    KEY `idx_created_at` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='操作日志表';

-- =====================================================================
-- 二、基础数据管理模块（仓库 / 商品分类 / 商品 / 供应商 / 客户 / 员工）
-- =====================================================================

DROP TABLE IF EXISTS `warehouses`;
CREATE TABLE `warehouses` (
    `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '仓库ID',
    `name`       VARCHAR(64) NOT NULL COMMENT '仓库名称',
    `code`       VARCHAR(32) NOT NULL COMMENT '仓库编码',
    `address`    VARCHAR(255) NOT NULL DEFAULT '' COMMENT '仓库地址',
    `manager`    VARCHAR(64) NOT NULL DEFAULT '' COMMENT '负责人',
    `status`     TINYINT NOT NULL DEFAULT 1 COMMENT '状态: 1启用 0停用',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_code` (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='仓库表';

DROP TABLE IF EXISTS `categories`;
CREATE TABLE `categories` (
    `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '分类ID',
    `parent_id`  BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '父分类ID',
    `name`       VARCHAR(64) NOT NULL COMMENT '分类名称',
    `sort_order` INT NOT NULL DEFAULT 0,
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_parent_id` (`parent_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='商品分类表';

DROP TABLE IF EXISTS `products`;
CREATE TABLE `products` (
    `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '商品ID',
    `sku`         VARCHAR(64)  NOT NULL COMMENT '商品编码(SKU)',
    `name`        VARCHAR(128) NOT NULL COMMENT '商品名称',
    `category_id` BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '分类ID',
    `unit`        VARCHAR(16)  NOT NULL DEFAULT '件' COMMENT '计量单位',
    `spec`        VARCHAR(255) NOT NULL DEFAULT '' COMMENT '规格型号',
    `price`       DECIMAL(12,2) NOT NULL DEFAULT 0.00 COMMENT '销售参考价',
    `cost_price`  DECIMAL(12,2) NOT NULL DEFAULT 0.00 COMMENT '成本价',
    `status`      TINYINT NOT NULL DEFAULT 1 COMMENT '状态: 1上架 0下架',
    `description` TEXT COMMENT '商品描述',
    `image_url`   VARCHAR(255) NOT NULL DEFAULT '' COMMENT '商品图片URL',
    `has_serial`  TINYINT NOT NULL DEFAULT 0 COMMENT '是否启用序列号管理(0否1是)',
    `supplier_id` BIGINT UNSIGNED NULL COMMENT '供应商ID',
    `created_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_sku` (`sku`),
    KEY `idx_category_id` (`category_id`),
    KEY `idx_supplier_id` (`supplier_id`),
    KEY `idx_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='商品表';

DROP TABLE IF EXISTS `suppliers`;
CREATE TABLE `suppliers` (
    `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '供应商ID',
    `name`       VARCHAR(128) NOT NULL COMMENT '供应商名称',
    `contact`    VARCHAR(64)  NOT NULL DEFAULT '',
    `phone`      VARCHAR(32)  NOT NULL DEFAULT '',
    `email`      VARCHAR(128) NOT NULL DEFAULT '',
    `address`    VARCHAR(255) NOT NULL DEFAULT '',
    `status`     TINYINT NOT NULL DEFAULT 1 COMMENT '状态: 1合作中 0停用',
    `remark`     VARCHAR(255) NOT NULL DEFAULT '',
    `image_url`  VARCHAR(255) NOT NULL DEFAULT '' COMMENT '供应商资质图片URL',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='供应商表';

DROP TABLE IF EXISTS `customers`;
CREATE TABLE `customers` (
    `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '客户ID',
    `name`       VARCHAR(128) NOT NULL COMMENT '客户名称',
    `contact`    VARCHAR(64)  NOT NULL DEFAULT '',
    `phone`      VARCHAR(32)  NOT NULL DEFAULT '',
    `email`      VARCHAR(128) NOT NULL DEFAULT '',
    `address`    VARCHAR(255) NOT NULL DEFAULT '',
    `status`     TINYINT NOT NULL DEFAULT 1 COMMENT '状态: 1正常 0停用',
    `remark`     VARCHAR(255) NOT NULL DEFAULT '',
    `image_url`  VARCHAR(255) NOT NULL DEFAULT '' COMMENT '客户图片URL',
    `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='客户表';

-- 员工表（复用 users，此处为员工业务信息扩展）
DROP TABLE IF EXISTS `employees`;
CREATE TABLE `employees` (
    `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '员工ID',
    `user_id`     BIGINT UNSIGNED NULL COMMENT '关联登录账号(可为空)',
    `emp_no`      VARCHAR(32) NOT NULL COMMENT '工号',
    `name`        VARCHAR(64) NOT NULL COMMENT '姓名',
    `department`  VARCHAR(64) NOT NULL DEFAULT '' COMMENT '部门',
    `position`    VARCHAR(64) NOT NULL DEFAULT '' COMMENT '职位',
    `phone`       VARCHAR(32) NOT NULL DEFAULT '',
    `email`       VARCHAR(128) NOT NULL DEFAULT '',
    `status`      TINYINT NOT NULL DEFAULT 1 COMMENT '状态: 1在职 0离职',
    `created_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_emp_no` (`emp_no`),
    KEY `idx_user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='员工表';

-- =====================================================================
-- 三、库存管理模块（库存 / 库存流水 / 盘点）
-- =====================================================================

DROP TABLE IF EXISTS `inventory`;
CREATE TABLE `inventory` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '库存ID',
    `product_id`   BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    `warehouse_id` BIGINT UNSIGNED NOT NULL COMMENT '仓库ID',
    `quantity`     INT NOT NULL DEFAULT 0 COMMENT '当前库存数量',
    `min_quantity` INT NOT NULL DEFAULT 0 COMMENT '安全库存(预警阈值)',
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_product_warehouse` (`product_id`, `warehouse_id`),
    KEY `idx_warehouse_id` (`warehouse_id`),
    CONSTRAINT `fk_inv_product` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_inv_warehouse` FOREIGN KEY (`warehouse_id`) REFERENCES `warehouses` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='库存表';

-- 库存操作单（入库/出库/调拨统一，走审批）
DROP TABLE IF EXISTS `stock_operations`;
CREATE TABLE `stock_operations` (
    `id`                BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '操作单ID',
    `operation_no`      VARCHAR(64) NOT NULL COMMENT '操作单号',
    `type`              VARCHAR(16) NOT NULL COMMENT '类型: in入库 out出库 transfer调拨',
    `product_id`        BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    `from_warehouse_id` BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '出库仓库ID(出库/调拨用)',
    `to_warehouse_id`   BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '入库仓库ID(入库/调拨用)',
    `qty`               INT NOT NULL COMMENT '数量',
    `biz_type`          VARCHAR(32) NOT NULL DEFAULT '' COMMENT '业务类型',
    `biz_no`            VARCHAR(64) NOT NULL DEFAULT '' COMMENT '关联业务单号',
    `logistics_company` VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物流公司',
    `logistics_no`      VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物流单号',
    `status`            TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0待审批 1已批准 2已驳回',
    `applicant_id`      BIGINT UNSIGNED NULL COMMENT '申请人ID',
    `approved_by`       BIGINT UNSIGNED NULL COMMENT '审批人ID',
    `approved_at`       DATETIME NULL COMMENT '审批时间',
    `serial_nos`        TEXT NULL COMMENT '序列号列表(逗号分隔)',
    `remark`            VARCHAR(255) NOT NULL DEFAULT '',
    `created_at`        DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`        DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `assigned_approver_id` BIGINT UNSIGNED NULL COMMENT '指定审批领导',
    KEY `idx_assigned_approver` (`assigned_approver_id`, `status`),
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_operation_no` (`operation_no`),
    KEY `idx_product_id` (`product_id`),
    KEY `idx_type` (`type`),
    KEY `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='库存操作单表';

DROP TABLE IF EXISTS `stock_operation_logs`;
CREATE TABLE `stock_operation_logs` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志ID',
    `operation_id` BIGINT UNSIGNED NOT NULL COMMENT '操作单ID',
    `action`       VARCHAR(16) NOT NULL COMMENT '动作: submit提交 approve批准 reject驳回',
    `operator_id`  BIGINT UNSIGNED NOT NULL COMMENT '操作人ID',
    `remark`       VARCHAR(255) NOT NULL DEFAULT '' COMMENT '备注/驳回原因',
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '操作时间',
    PRIMARY KEY (`id`),
    KEY `idx_operation` (`operation_id`),
    KEY `idx_operator` (`operator_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='库存操作单流转日志（全生命周期操作人记录）';

DROP TABLE IF EXISTS `stock_records`;
CREATE TABLE `stock_records` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '流水ID',
    `product_id`   BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    `warehouse_id` BIGINT UNSIGNED NOT NULL COMMENT '仓库ID',
    `change_type`  VARCHAR(16) NOT NULL COMMENT '变动类型: in入库 out出库 adjust调整',
    `change_qty`   INT NOT NULL COMMENT '变动数量(正负)',
    `before_qty`   INT NOT NULL DEFAULT 0 COMMENT '变动前数量',
    `after_qty`    INT NOT NULL DEFAULT 0 COMMENT '变动后数量',
    `biz_type`     VARCHAR(32) NOT NULL DEFAULT '' COMMENT '业务类型',
    `biz_no`       VARCHAR(64) NOT NULL DEFAULT '' COMMENT '关联业务单号',
    `remark`       VARCHAR(255) NOT NULL DEFAULT '',
    `operator_id`  BIGINT UNSIGNED NULL COMMENT '操作人ID',
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_product_id` (`product_id`),
    KEY `idx_warehouse_id` (`warehouse_id`),
    KEY `idx_biz_no` (`biz_no`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='库存流水表';

-- 盘点单
DROP TABLE IF EXISTS `stock_checks`;
CREATE TABLE `stock_checks` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '盘点单ID',
    `check_no`     VARCHAR(64) NOT NULL COMMENT '盘点单号',
    `warehouse_id` BIGINT UNSIGNED NOT NULL COMMENT '仓库ID',
    `status`       TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0进行中 1已完成',
    `remark`       VARCHAR(255) NOT NULL DEFAULT '',
    `created_by`   BIGINT UNSIGNED NULL,
    `reviewed_by`  BIGINT UNSIGNED NULL COMMENT '盘点差异复核人',
    `counted_by`   BIGINT UNSIGNED NULL COMMENT '实际盘点人',
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_check_no` (`check_no`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='盘点单表';

DROP TABLE IF EXISTS `stock_check_items`;
CREATE TABLE `stock_check_items` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '盘点明细ID',
    `check_id`     BIGINT UNSIGNED NOT NULL COMMENT '盘点单ID',
    `product_id`   BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    `book_qty`     INT NOT NULL DEFAULT 0 COMMENT '账面数量',
    `actual_qty`   INT NOT NULL DEFAULT 0 COMMENT '实盘数量',
    `diff_qty`     INT NOT NULL DEFAULT 0 COMMENT '差异数量',
    PRIMARY KEY (`id`),
    KEY `idx_check_id` (`check_id`),
    CONSTRAINT `fk_sci_check` FOREIGN KEY (`check_id`) REFERENCES `stock_checks` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='盘点明细表';

-- =====================================================================
-- 四、采购管理模块（采购订单 / 明细 / 入库 / 退货）
-- =====================================================================

DROP TABLE IF EXISTS `purchase_orders`;
CREATE TABLE `purchase_orders` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '采购单ID',
    `order_no`     VARCHAR(64) NOT NULL COMMENT '采购单号',
    `supplier_id`  BIGINT UNSIGNED NOT NULL COMMENT '供应商ID',
    `warehouse_id` BIGINT UNSIGNED NULL COMMENT '业务归属仓库',
    `total_amount` DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '采购总额',
    `status`       TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0待审批 1已批准 2已驳回 3已完成',
    `remark`       VARCHAR(255) NOT NULL DEFAULT '',
    `created_by`   BIGINT UNSIGNED NULL COMMENT '创建人ID',
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `approved_by`  BIGINT UNSIGNED NULL COMMENT '审批人ID',
    `warehouse_confirmed_by` BIGINT UNSIGNED NULL COMMENT '仓管员入库确认人ID',
    `image_url`    VARCHAR(255) NOT NULL DEFAULT '' COMMENT '附件图片URL',
    `assigned_approver_id` BIGINT UNSIGNED NULL COMMENT '指定审批领导',
    KEY `idx_assigned_approver` (`assigned_approver_id`, `status`),
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_order_no` (`order_no`),
    KEY `idx_supplier_id` (`supplier_id`),
    CONSTRAINT `fk_po_supplier` FOREIGN KEY (`supplier_id`) REFERENCES `suppliers` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='采购单表';

DROP TABLE IF EXISTS `purchase_order_items`;
CREATE TABLE `purchase_order_items` (
    `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '明细ID',
    `order_id`   BIGINT UNSIGNED NOT NULL COMMENT '采购单ID',
    `product_id` BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    `quantity`   INT NOT NULL COMMENT '采购数量',
    `price`      DECIMAL(12,2) NOT NULL DEFAULT 0.00 COMMENT '采购单价',
    `amount`     DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '小计金额',
    `received_qty` INT NOT NULL DEFAULT 0 COMMENT '已入库数量',
    PRIMARY KEY (`id`),
    KEY `idx_order_id` (`order_id`),
    KEY `idx_product_id` (`product_id`),
    CONSTRAINT `fk_poi_order` FOREIGN KEY (`order_id`) REFERENCES `purchase_orders` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_poi_product` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='采购单明细表';

-- =====================================================================
-- 五、销售管理模块（销售订单 / 明细 / 出库 / 退货）
-- =====================================================================

DROP TABLE IF EXISTS `sales_orders`;
CREATE TABLE `sales_orders` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '销售单ID',
    `order_no`     VARCHAR(64) NOT NULL COMMENT '销售单号',
    `customer_id`  BIGINT UNSIGNED NOT NULL COMMENT '客户ID',
    `warehouse_id` BIGINT UNSIGNED NULL COMMENT '业务归属仓库',
    `total_amount` DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '销售总额',
    `status`       TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0待审批 1已批准待出库 2已驳回 3已完成 4打回待修改 5仓管员已确认待领导批准',
    `remark`       VARCHAR(255) NOT NULL DEFAULT '',
    `created_by`   BIGINT UNSIGNED NULL,
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `approved_by`  BIGINT UNSIGNED NULL COMMENT '审批人ID',
    `image_url`    VARCHAR(255) NOT NULL DEFAULT '' COMMENT '附件图片URL',
    `logistics_company` VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物流公司',
    `logistics_no`      VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物流单号',
    `warehouse_confirmed_by` BIGINT UNSIGNED NULL COMMENT '仓管员确认人ID',
    `warehouse_approved_by` BIGINT UNSIGNED NULL COMMENT '仓库出库批准人ID',
    `assigned_approver_id` BIGINT UNSIGNED NULL COMMENT '指定审批领导',
    KEY `idx_assigned_approver` (`assigned_approver_id`, `status`),
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_order_no` (`order_no`),
    KEY `idx_customer_id` (`customer_id`),
    CONSTRAINT `fk_so_customer` FOREIGN KEY (`customer_id`) REFERENCES `customers` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='销售单表';

DROP TABLE IF EXISTS `sales_order_items`;
CREATE TABLE `sales_order_items` (
    `id`          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '明细ID',
    `order_id`    BIGINT UNSIGNED NOT NULL COMMENT '销售单ID',
    `product_id`  BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    `quantity`    INT NOT NULL COMMENT '销售数量',
    `price`       DECIMAL(12,2) NOT NULL DEFAULT 0.00 COMMENT '销售单价',
    `amount`      DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '小计金额',
    `shipped_qty` INT NOT NULL DEFAULT 0 COMMENT '已出库数量',
    PRIMARY KEY (`id`),
    KEY `idx_order_id` (`order_id`),
    KEY `idx_product_id` (`product_id`),
    CONSTRAINT `fk_soi_order` FOREIGN KEY (`order_id`) REFERENCES `sales_orders` (`id`) ON DELETE CASCADE,
    CONSTRAINT `fk_soi_product` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='销售单明细表';

-- =====================================================================
-- 六、基础财务管理模块（往来账 / 收付款）
-- =====================================================================

-- 应收/应付往来账（由采购、销售单据自动生成）
DROP TABLE IF EXISTS `accounts`;
CREATE TABLE `accounts` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '往来账ID',
    `account_type` VARCHAR(16) NOT NULL COMMENT '类型: receivable应收 payable应付',
    `biz_type`     VARCHAR(32) NOT NULL COMMENT '业务类型: purchase采购 sale销售',
    `biz_no`       VARCHAR(64) NOT NULL COMMENT '关联业务单号',
    `partner_id`   BIGINT UNSIGNED NOT NULL COMMENT '往来单位ID(供应商/客户)',
    `partner_name` VARCHAR(128) NOT NULL DEFAULT '' COMMENT '往来单位名称',
    `amount`       DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '账目金额',
    `paid_amount`  DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '已收/已付金额',
    `offset_amount` DECIMAL(14,2) NOT NULL DEFAULT 0.00,
    `status`       TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0未结清 1已结清',
    `due_date`     DATE NULL COMMENT '到期日',
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_biz_no` (`biz_no`),
    KEY `idx_partner_id` (`partner_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='应收应付往来账表';

-- 收付款记录
DROP TABLE IF EXISTS `payments`;
CREATE TABLE `payments` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '收付款ID',
    `payment_no`   VARCHAR(64) NOT NULL COMMENT '收付款单号',
    `account_id`   BIGINT UNSIGNED NOT NULL COMMENT '关联往来账ID',
    `payment_type` VARCHAR(16) NOT NULL COMMENT '类型: receive收款 pay付款',
    `amount`       DECIMAL(14,2) NOT NULL COMMENT '收付款金额',
    `pay_method`   VARCHAR(32) NOT NULL DEFAULT '' COMMENT '支付方式',
    `pay_date`     DATE NOT NULL COMMENT '收付款日期',
    `remark`       VARCHAR(255) NOT NULL DEFAULT '',
    `created_by`   BIGINT UNSIGNED NULL,
    `created_at`   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_payment_no` (`payment_no`),
    KEY `idx_account_id` (`account_id`),
    CONSTRAINT `fk_pay_account` FOREIGN KEY (`account_id`) REFERENCES `accounts` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='收付款记录表';

-- 采购退货单
DROP TABLE IF EXISTS `purchase_returns`;
CREATE TABLE `purchase_returns` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '退货单ID',
    `return_no`    VARCHAR(64) NOT NULL COMMENT '退货单号',
    `order_id`     BIGINT UNSIGNED NOT NULL COMMENT '原采购单ID',
    `supplier_id`  BIGINT UNSIGNED NOT NULL COMMENT '供应商ID',
    `warehouse_id` BIGINT UNSIGNED NOT NULL COMMENT '退货出库仓库ID',
    `total_amount` DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '退货总额',
    `status`       TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0待审批 1待仓库审核 2已驳回 3已完成 4已打回',
    `remark`       VARCHAR(255) NOT NULL DEFAULT '',
    `logistics_company` VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物流公司',
    `logistics_no`  VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物流单号',
    `created_by`    BIGINT UNSIGNED NULL COMMENT '创建人ID',
    `approved_by`   BIGINT UNSIGNED NULL COMMENT '审批人ID',
    `warehouse_confirmed_by` BIGINT UNSIGNED NULL COMMENT '仓库确认人ID',
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `assigned_approver_id` BIGINT UNSIGNED NULL COMMENT '指定审批领导',
    KEY `idx_assigned_approver` (`assigned_approver_id`, `status`),
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_return_no` (`return_no`),
    KEY `idx_order_id` (`order_id`),
    KEY `idx_supplier_id` (`supplier_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='采购退货单表';

DROP TABLE IF EXISTS `purchase_return_items`;
CREATE TABLE `purchase_return_items` (
    `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '退货明细ID',
    `return_id`  BIGINT UNSIGNED NOT NULL COMMENT '退货单ID',
    `product_id` BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    `quantity`   INT NOT NULL COMMENT '退货数量',
    `price`      DECIMAL(12,2) NOT NULL DEFAULT 0.00 COMMENT '退货单价',
    `amount`     DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '小计金额',
    PRIMARY KEY (`id`),
    KEY `idx_return_id` (`return_id`),
    KEY `idx_product_id` (`product_id`),
    CONSTRAINT `fk_pri_return` FOREIGN KEY (`return_id`) REFERENCES `purchase_returns` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='采购退货明细表';

-- 销售退货单
DROP TABLE IF EXISTS `sales_returns`;
CREATE TABLE `sales_returns` (
    `id`           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '退货单ID',
    `return_no`    VARCHAR(64) NOT NULL COMMENT '退货/换货单号',
    `return_type`  VARCHAR(16) NOT NULL DEFAULT 'return' COMMENT '类型: return退货 exchange换货',
    `order_id`     BIGINT UNSIGNED NOT NULL COMMENT '原销售单ID',
    `customer_id`  BIGINT UNSIGNED NOT NULL COMMENT '客户ID',
    `warehouse_id` BIGINT UNSIGNED NOT NULL COMMENT '退货入库仓库ID',
    `total_amount` DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '退货总额',
    `status`       TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0待审批 1待仓库审核 2已驳回 3已完成 4已打回',
    `remark`       VARCHAR(255) NOT NULL DEFAULT '',
    `logistics_company` VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物流公司',
    `logistics_no`  VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物流单号',
    `created_by`    BIGINT UNSIGNED NULL COMMENT '创建人ID',
    `approved_by`   BIGINT UNSIGNED NULL COMMENT '审批人ID',
    `warehouse_confirmed_by` BIGINT UNSIGNED NULL COMMENT '仓库确认人ID',
    `created_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`    DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `assigned_approver_id` BIGINT UNSIGNED NULL COMMENT '指定审批领导',
    KEY `idx_assigned_approver` (`assigned_approver_id`, `status`),
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_return_no` (`return_no`),
    KEY `idx_order_id` (`order_id`),
    KEY `idx_customer_id` (`customer_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='销售退货单表';

DROP TABLE IF EXISTS `sales_return_items`;
CREATE TABLE `sales_return_items` (
    `id`         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '退货明细ID',
    `return_id`  BIGINT UNSIGNED NOT NULL COMMENT '退货/换货单ID',
    `direction`  VARCHAR(16) NOT NULL DEFAULT 'return' COMMENT '方向: return退入 replace换出',
    `product_id` BIGINT UNSIGNED NOT NULL COMMENT '商品ID',
    `quantity`   INT NOT NULL COMMENT '退货数量',
    `price`      DECIMAL(12,2) NOT NULL DEFAULT 0.00 COMMENT '退货单价',
    `amount`     DECIMAL(14,2) NOT NULL DEFAULT 0.00 COMMENT '小计金额',
    PRIMARY KEY (`id`),
    KEY `idx_return_id` (`return_id`),
    KEY `idx_product_id` (`product_id`),
    CONSTRAINT `fk_sri_return` FOREIGN KEY (`return_id`) REFERENCES `sales_returns` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='销售退货明细表';

-- 申报单（供应商/客户申报：采购部申报供应商，销售部申报客户）
DROP TABLE IF EXISTS `applications`;
CREATE TABLE `applications` (
    `id`             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '申报单ID',
    `application_no` VARCHAR(64) NOT NULL COMMENT '申报单号',
    `type`           VARCHAR(16) NOT NULL COMMENT '类型: supplier供应商 customer客户',
    `title`          VARCHAR(128) NOT NULL DEFAULT '' COMMENT '申报对象名称',
    `payload`        TEXT COMMENT '申报字段 JSON（contact/phone/email/address）',
    `status`         TINYINT NOT NULL DEFAULT 0 COMMENT '状态: 0待审批 1已批准 2已驳回',
    `applicant_id`   BIGINT UNSIGNED NULL COMMENT '申请人ID',
    `approved_by`    BIGINT UNSIGNED NULL COMMENT '审批人ID',
    `remark`         VARCHAR(255) NOT NULL DEFAULT '' COMMENT '备注/驳回原因',
    `created_at`     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `updated_at`     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    `assigned_approver_id` BIGINT UNSIGNED NULL COMMENT '指定审批领导',
    KEY `idx_assigned_approver` (`assigned_approver_id`, `status`),
    PRIMARY KEY (`id`),
    UNIQUE KEY `uk_application_no` (`application_no`),
    KEY `idx_type` (`type`),
    KEY `idx_status` (`status`),
    KEY `idx_applicant` (`applicant_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='申报单表';

SET FOREIGN_KEY_CHECKS = 1;

-- =====================================================================
-- 初始种子数据
-- =====================================================================

INSERT INTO `roles` (`name`, `code`, `description`) VALUES
('管理员', 'admin', '拥有全部权限（含系统管理）'),
('领导', 'leader', '只读查看本部门数据与报表，管理本部门员工'),
('员工', 'staff', '日常业务操作（出入库/采购/销售）');

-- 部门字典
INSERT INTO `departments` (`name`, `code`, `description`) VALUES
('销售部', 'sales', '负责销售订单与客户'),
('仓库部', 'warehouse', '负责库存与出入库'),
('采购部', 'purchase', '负责采购订单与供应商'),
('财务部', 'finance', '负责收付款与往来账');


INSERT INTO `permissions` (`name`, `code`, `module`, `description`) VALUES
('查看用户',   'user:read',      '系统管理', '查看用户'),
('管理用户',   'user:write',     '系统管理', '增删改用户'),
('查看角色',   'role:read',      '系统管理', '查看角色权限'),
('管理角色',   'role:write',     '系统管理', '增删改角色权限'),
('查看日志',   'log:read',       '系统管理', '查看操作日志'),
('查看基础数据', 'base:read',    '基础数据', '查看供应商/客户/商品/仓库/员工'),
('管理基础数据', 'base:write',   '基础数据', '增删改基础数据'),
('查看库存',   'inventory:read', '库存管理', '查看库存与流水'),
('调整库存',   'inventory:write','库存管理', '出入库调整与盘点'),
('查看采购单', 'purchase:read',  '采购管理', '查看采购单'),
('管理采购单', 'purchase:write', '采购管理', '增删改采购单与收货入库'),
('查看销售单', 'sales:read',     '销售管理', '查看销售单'),
('管理销售单', 'sales:write',    '销售管理', '增删改销售单与发货出库'),
('查看财务',   'finance:read',   '财务管理', '查看往来账与收付款'),
('管理财务',   'finance:write',  '财务管理', '收付款登记');

-- admin 拥有全部权限
INSERT INTO `role_permissions` (`role_id`, `permission_id`)
SELECT r.id, p.id FROM `roles` r, `permissions` p WHERE r.code = 'admin';

-- leader 拥有全部「只读」权限（查看类）
INSERT INTO `role_permissions` (`role_id`, `permission_id`)
SELECT r.id, p.id FROM `roles` r, `permissions` p
WHERE r.code = 'leader' AND p.code IN
  ('user:read','role:read','log:read','base:read','inventory:read','purchase:read','sales:read','finance:read');

-- staff 拥有日常业务操作权限（无财务、无系统管理写权限）
INSERT INTO `role_permissions` (`role_id`, `permission_id`)
SELECT r.id, p.id FROM `roles` r, `permissions` p
WHERE r.code = 'staff' AND p.code IN
  ('base:read','inventory:read','inventory:write','purchase:read','purchase:write','sales:read','sales:write');

-- 默认账号（密码：admin 用 admin123；其余 123456，加盐 SHA256 哈希，格式 salt$hash）
-- 统一 123456 哈希：a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b
INSERT INTO `users` (`username`, `password`, `real_name`, `department`, `status`) VALUES
('admin', 'f15063a40cb8f7f4ee585b26a185644d$01c3d52131905e96d8211863f91c8bc799e2f7f785c54004afc757c81aa9fbb9', '系统管理员', '', 1),
('leader', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '张领导', '销售部', 1),
('staff', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '李员工', '销售部', 1),
('sale_leader', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '销售经理', '销售部', 1),
('sale_staff', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '销售专员', '销售部', 1),
('warehouse_leader', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '仓库主管', '仓库部', 1),
('warehouse_staff', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '仓管员', '仓库部', 1),
('purchase_leader', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '采购主管', '采购部', 1),
('purchase_staff', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '采购专员', '采购部', 1),
('finance_leader', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '财务主管', '财务部', 1),
('finance_staff', 'a1b2c3d4e5f60718293a4b5c6d7e8f90$457a2ff654a76ac5a19ce5a4e04e5c54ad717e94712db4dc28d54c03d13c452b', '财务专员', '财务部', 1);

INSERT INTO `user_roles` (`user_id`, `role_id`)
SELECT u.id, r.id FROM `users` u, `roles` r WHERE u.username = 'admin' AND r.code = 'admin';

INSERT INTO `user_roles` (`user_id`, `role_id`)
SELECT u.id, r.id FROM `users` u, `roles` r WHERE u.username IN
  ('leader','sale_leader','warehouse_leader','purchase_leader','finance_leader') AND r.code = 'leader';

INSERT INTO `user_roles` (`user_id`, `role_id`)
SELECT u.id, r.id FROM `users` u, `roles` r WHERE u.username IN
  ('staff','sale_staff','warehouse_staff','purchase_staff','finance_staff') AND r.code = 'staff';

-- 员工档案（与账号关联）
INSERT INTO `employees` (`user_id`, `emp_no`, `name`, `department`, `position`, `phone`, `email`, `status`) VALUES
((SELECT id FROM users WHERE username='leader'), 'E001', '张领导', '销售部', '销售经理', '', '', 1),
((SELECT id FROM users WHERE username='staff'), 'E002', '李员工', '销售部', '销售专员', '', '', 1),
((SELECT id FROM users WHERE username='sale_leader'), 'E003', '销售经理', '销售部', '销售经理', '', '', 1),
((SELECT id FROM users WHERE username='sale_staff'), 'E004', '销售专员', '销售部', '销售专员', '', '', 1),
((SELECT id FROM users WHERE username='warehouse_leader'), 'E005', '仓库主管', '仓库部', '仓库主管', '', '', 1),
((SELECT id FROM users WHERE username='warehouse_staff'), 'E006', '仓管员', '仓库部', '仓管员', '', '', 1),
((SELECT id FROM users WHERE username='purchase_leader'), 'E007', '采购主管', '采购部', '采购主管', '', '', 1),
((SELECT id FROM users WHERE username='purchase_staff'), 'E008', '采购专员', '采购部', '采购专员', '', '', 1),
((SELECT id FROM users WHERE username='finance_leader'), 'E009', '财务主管', '财务部', '财务主管', '', '', 1),
((SELECT id FROM users WHERE username='finance_staff'), 'E010', '财务专员', '财务部', '财务专员', '', '', 1);

-- 默认仓库
INSERT INTO `warehouses` (`name`, `code`, `address`, `manager`) VALUES
('默认仓库', 'DEFAULT', '主仓库', '管理员'),
('成品仓', 'FINISHED', '成品仓库', ''),
('原料仓', 'RAW', '原料仓库', '');

-- 商品分类
INSERT INTO `categories` (`parent_id`, `name`, `sort_order`) VALUES
(0, '电子元器件', 1),
(0, '办公用品', 2),
(0, '原材料', 3),
(1, '电阻电容', 1),
(1, '芯片', 2);

-- 示例商品
INSERT INTO `products` (`sku`, `name`, `category_id`, `unit`, `spec`, `price`, `cost_price`, `status`, `description`) VALUES
('SKU0001', '贴片电阻 10KΩ', 4, '个', '0603 5%', 0.02, 0.008, 1, '常用贴片电阻'),
('SKU0002', 'STM32F103C8T6', 5, '片', 'LQFP48', 8.50, 6.20, 1, 'ARM Cortex-M3 主控芯片'),
('SKU0003', 'A4 打印纸', 2, '包', '500张/包', 22.00, 16.50, 1, '办公用复印纸'),
('SKU0004', '电解电容 100μF', 4, '个', '16V 铝电解', 0.15, 0.06, 1, '铝电解电容'),
('SKU0005', 'USB 数据线', 2, '条', 'Type-C 1m', 6.00, 2.80, 1, 'Type-C 数据线');

CREATE TABLE IF NOT EXISTS business_events (
 id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 document_type VARCHAR(40) NOT NULL, document_id BIGINT UNSIGNED NOT NULL,
 action VARCHAR(40) NOT NULL, operator_id BIGINT UNSIGNED NOT NULL,
 before_status INT NOT NULL, after_status INT NOT NULL,
 remark VARCHAR(1000) NOT NULL DEFAULT '',
 created_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
 KEY idx_document (document_type,document_id,id), KEY idx_operator (operator_id,created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS business_requests (
 user_id BIGINT UNSIGNED NOT NULL, request_key VARCHAR(128) NOT NULL,
 fingerprint CHAR(64) NOT NULL, response MEDIUMTEXT NULL,
 created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
 PRIMARY KEY(user_id,request_key)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS master_import_locks (
 kind VARCHAR(32) NOT NULL PRIMARY KEY
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
