# 商品、审批人与仪表盘更新

- 管理员新增/编辑商品必须选择启用供应商；supplier_id 与 has_serial 使用同一条 SQL 保存。
- 库存操作申请中的序列号必须非空、不重复，数量与商品数量一致（启用序列号管理的商品）。目前此校验覆盖库存操作单；采购/销售整单的逐件序列号追踪仍需单独完善。
- 六类单据支持 assigned_approver_id：采购、销售、采购退货、销售退换货、库存操作、基础资料申报。
- 不指定时进入部门公共审批队列；指定时仅所选领导或管理员可审批。重新提交沿用原审批人。
- 后端审批校验当前账号状态、当前部门/领导角色及指定审批人。前端待审批列表排除分配给其他领导的单据。
- 仪表盘统一标题区、留白、指标卡、表格样式，并适配窄屏侧栏与单列卡片。
- WSL 前端启用文件轮询，修复 Windows 文件修改后预览未更新的问题。

## 部署与验证

现有数据库运行 `python3 db/migrate_approvers.py`，仅增加缺失的可空列与索引；新数据库初始化使用更新后的 schema.sql。不要在已有数据库执行 schema.sql（包含 DROP TABLE）。

后端：`cmake -S . -B build_wsl && cmake --build build_wsl -j2`。
前端：`cd frontend && npm run build`。
集成验证：`python3 test/verify_product_approvals.py`。该脚本使用现有启用账号构建短期本地测试凭证，创建唯一命名的临时商品、采购/销售单与申报单，最终清理这些测试记录；不审批业务单、不进行库存或财务过账。请求审计日志保留。

运行预览：后端在 build_wsl 目录执行 `./erp_system`；前端执行 `npm run dev -- --host 0.0.0.0 --port 5174 --strictPort`。
