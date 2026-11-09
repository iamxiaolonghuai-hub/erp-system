-- 联动测试数据脚本（各部门联动演示）
USE erp_system;

-- ============ 1. 初始化库存（多商品多仓库） ============
-- 已有 inventory id=1: product1 默认仓库 60
INSERT INTO inventory (product_id, warehouse_id, quantity, min_quantity) VALUES
(1, 2, 200, 50),   -- 贴片电阻 成品仓
(2, 1, 500, 100),  -- STM32 默认仓库
(2, 2, 300, 100),  -- STM32 成品仓
(3, 1, 120, 30),   -- A4纸 默认仓库
(4, 1, 800, 200),  -- 电解电容 默认仓库
(5, 2, 150, 40);   -- USB线 成品仓

-- ============ 2. 采购链路（已完成 status=3，含明细+库存流水+应付账） ============
-- 采购单 PO-TEST-001（华强北电子，供应商2）
INSERT INTO purchase_orders (order_no, supplier_id, total_amount, status, remark, created_by, approved_by, created_at) VALUES
('PO-TEST-001', 2, 4300.00, 3, '采购 STM32 芯片一批', 9, 8, '2026-10-20 09:00:00'),
('PO-TEST-002', 3, 2450.00, 3, '采购电解电容', 9, 8, '2026-10-22 10:30:00'),
('PO-TEST-003', 4, 1500.00, 1, '采购 USB 数据线（待仓库入库）', 9, 8, '2026-10-27 14:00:00');

-- 采购明细
INSERT INTO purchase_order_items (order_id, product_id, quantity, price, amount, received_qty) VALUES
((SELECT id FROM purchase_orders WHERE order_no='PO-TEST-001'), 2, 500, 8.60, 4300.00, 500),
((SELECT id FROM purchase_orders WHERE order_no='PO-TEST-002'), 4, 1000, 2.45, 2450.00, 1000),
((SELECT id FROM purchase_orders WHERE order_no='PO-TEST-003'), 5, 300, 5.00, 1500.00, 0);

-- 采购入库流水（已入库的两笔）
INSERT INTO stock_records (product_id, warehouse_id, change_type, change_qty, before_qty, after_qty, biz_type, biz_no, operator_id, created_at) VALUES
(2, 1, 'in', 500, 0, 500, 'purchase_in', 'PO-TEST-001', 7, '2026-10-21 09:00:00'),
(4, 1, 'in', 1000, 0, 1000, 'purchase_in', 'PO-TEST-002', 7, '2026-10-23 10:30:00');

-- 应付账（两笔已完成的采购）
INSERT INTO accounts (account_type, biz_type, biz_no, partner_id, partner_name, amount, paid_amount, status, created_at) VALUES
('payable', 'purchase', 'PO-TEST-001', 2, '华强北电子', 4300.00, 4300.00, 1, '2026-10-21 09:00:00'),
('payable', 'purchase', 'PO-TEST-002', 3, '广州元件批发', 2450.00, 1000.00, 0, '2026-10-23 10:30:00');

-- ============ 3. 销售链路（已完成 status=3，含明细+库存流水+应收账） ============
INSERT INTO sales_orders (order_no, customer_id, total_amount, status, remark, created_by, approved_by, created_at) VALUES
('SO-TEST-001', 2, 1700.00, 3, '销售 STM32 芯片', 5, 4, '2026-10-24 11:00:00'),
('SO-TEST-002', 3, 88.00, 3, '销售 A4 打印纸', 5, 4, '2026-10-25 15:00:00'),
('SO-TEST-003', 4, 450.00, 1, '销售 USB 数据线（待仓库出库）', 5, 4, '2026-10-28 09:30:00');

INSERT INTO sales_order_items (order_id, product_id, quantity, price, amount, shipped_qty) VALUES
((SELECT id FROM sales_orders WHERE order_no='SO-TEST-001'), 2, 200, 8.50, 1700.00, 200),
((SELECT id FROM sales_orders WHERE order_no='SO-TEST-002'), 3, 4, 22.00, 88.00, 4),
((SELECT id FROM sales_orders WHERE order_no='SO-TEST-003'), 5, 75, 6.00, 450.00, 0);

-- 销售出库流水（已出库的两笔）
INSERT INTO stock_records (product_id, warehouse_id, change_type, change_qty, before_qty, after_qty, biz_type, biz_no, operator_id, created_at) VALUES
(2, 1, 'out', -200, 500, 300, 'sale_out', 'SO-TEST-001', 7, '2026-10-24 12:00:00'),
(3, 1, 'out', -4, 120, 116, 'sale_out', 'SO-TEST-002', 7, '2026-10-25 16:00:00');

-- 应收账
INSERT INTO accounts (account_type, biz_type, biz_no, partner_id, partner_name, amount, paid_amount, status, created_at) VALUES
('receivable', 'sale', 'SO-TEST-001', 2, '杭州电子厂', 1700.00, 1700.00, 1, '2026-10-24 12:00:00'),
('receivable', 'sale', 'SO-TEST-002', 3, '北京硬件公司', 88.00, 0.00, 0, '2026-10-25 16:00:00');

-- ============ 4. 收付款记录（财务部联动） ============
INSERT INTO payments (payment_no, account_id, payment_type, amount, pay_method, pay_date, remark, created_by, created_at) VALUES
('PAY-TEST-001', (SELECT id FROM accounts WHERE biz_no='PO-TEST-001'), 'pay', 4300.00, '银行转账', '2026-10-21', '采购付款', 10, '2026-10-21 10:00:00'),
('PAY-TEST-002', (SELECT id FROM accounts WHERE biz_no='PO-TEST-002'), 'pay', 1000.00, '银行转账', '2026-10-23', '采购首付款', 10, '2026-10-23 11:00:00'),
('PAY-TEST-003', (SELECT id FROM accounts WHERE biz_no='SO-TEST-001'), 'receive', 1700.00, '银行转账', '2026-10-24', '销售收款', 10, '2026-10-24 13:00:00');

-- ============ 5. 退货链路 ============
-- 采购退货（已完成 status=3）：退 STM32 50片
INSERT INTO purchase_returns (return_no, order_id, supplier_id, warehouse_id, total_amount, status, remark, created_by, approved_by, created_at) VALUES
('PR-TEST-001', (SELECT id FROM purchase_orders WHERE order_no='PO-TEST-001'), 2, 1, 430.00, 3, '部分芯片有质量问题退货', 9, 8, '2026-10-26 10:00:00');

INSERT INTO purchase_return_items (return_id, product_id, quantity, price, amount) VALUES
((SELECT id FROM purchase_returns WHERE return_no='PR-TEST-001'), 2, 50, 8.60, 430.00);

-- 采购退货出库流水
INSERT INTO stock_records (product_id, warehouse_id, change_type, change_qty, before_qty, after_qty, biz_type, biz_no, operator_id, created_at) VALUES
(2, 1, 'out', -50, 300, 250, 'purchase_return', 'PR-TEST-001', 7, '2026-10-26 11:00:00');

-- 采购退货冲减应付账（负数）
INSERT INTO accounts (account_type, biz_type, biz_no, partner_id, partner_name, amount, paid_amount, status, created_at) VALUES
('payable', 'purchase_return', 'PR-TEST-001', 2, '华强北电子', -430.00, 0.00, 0, '2026-10-26 11:00:00');

-- 销售退货（待仓库审核 status=1）：退 A4纸 2包
INSERT INTO sales_returns (return_no, order_id, customer_id, warehouse_id, total_amount, status, remark, created_by, approved_by, created_at) VALUES
('SR-TEST-001', (SELECT id FROM sales_orders WHERE order_no='SO-TEST-002'), 3, 1, 44.00, 1, '客户退回破损打印纸', 5, 4, '2026-10-28 10:00:00');

INSERT INTO sales_return_items (return_id, product_id, quantity, price, amount) VALUES
((SELECT id FROM sales_returns WHERE return_no='SR-TEST-001'), 3, 2, 22.00, 44.00);

-- ============ 6. 申报单（演示采购申报供应商 + 销售申报客户） ============
INSERT INTO applications (application_no, type, title, payload, status, applicant_id, approved_by, remark, created_at) VALUES
('APP-TEST-001', 'supplier', '东莞电子元件商行', '{"contact":"刘老板","phone":"13700000001","email":"dg@example.com","address":"东莞市"}', 0, 9, NULL, '', '2026-10-29 09:00:00'),
('APP-TEST-002', 'customer', '南京智能设备', '{"contact":"孙经理","phone":"13700000002","email":"nj@example.com","address":"南京市"}', 0, 5, NULL, '', '2026-10-29 09:30:00');

SELECT '联动测试数据导入完成' AS result;
