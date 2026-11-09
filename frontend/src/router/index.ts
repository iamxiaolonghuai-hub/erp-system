import { createRouter, createWebHistory, type RouteRecordRaw } from 'vue-router'
import { useUserStore } from '@/stores/user'

const routes: RouteRecordRaw[] = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/Login.vue'),
    meta: { title: '登录' },
  },
  // ===== 管理员完整后台 =====
  {
    path: '/',
    component: () => import('@/layouts/MainLayout.vue'),
    redirect: '/dashboard',
    meta: { roles: ['admin'] },
    children: [
      {
        path: 'dashboard',
        name: 'Dashboard',
        component: () => import('@/views/Dashboard.vue'),
        meta: { title: '首页', icon: 'HomeFilled' },
      },
      {
        path: 'products',
        name: 'Products',
        component: () => import('@/views/product/ProductList.vue'),
        meta: { title: '商品管理', icon: 'Goods' },
      },
      {
        path: 'base/suppliers',
        name: 'Suppliers',
        component: () => import('@/views/base/SupplierList.vue'),
        meta: { title: '供应商管理', icon: 'OfficeBuilding' },
      },
      {
        path: 'base/customers',
        name: 'Customers',
        component: () => import('@/views/base/CustomerList.vue'),
        meta: { title: '客户管理', icon: 'User' },
      },
      {
        path: 'base/employees',
        name: 'Employees',
        component: () => import('@/views/base/EmployeeList.vue'),
        meta: { title: '员工管理', icon: 'UserFilled' },
      },
      {
        path: 'base/warehouses',
        name: 'Warehouses',
        component: () => import('@/views/base/WarehouseList.vue'),
        meta: { title: '仓库管理', icon: 'Location' },
      },
      {
        path: 'base/categories',
        name: 'Categories',
        component: () => import('@/views/base/CategoryList.vue'),
        meta: { title: '分类管理', icon: 'Menu' },
      },
      {
        path: 'inventory',
        name: 'Inventory',
        component: () => import('@/views/inventory/InventoryList.vue'),
        meta: { title: '库存管理', icon: 'Box' },
      },
      { path: 'inventory/cost', name: 'AdminInventoryCost', component: () => import('@/views/inventory/InventoryCost.vue'), meta: { title: '库存成本' } },
      { path: 'inventory/lots', name: 'AdminLotInventory', component: () => import('@/views/inventory/LotInventory.vue'), meta: { title: '商品批次库存' } },
      { path: 'warehouse-returns/purchase', name: 'AdminPurchaseReturnQueue', component: () => import('@/views/return/ReturnList.vue'), meta: { title: '采购退货待出库', returnType: 'purchase', warehouseQueue: true } },
      { path: 'warehouse-returns/sales', name: 'AdminSalesReturnQueue', component: () => import('@/views/return/ReturnList.vue'), meta: { title: '销售退换货待处理', returnType: 'sales', warehouseQueue: true } },
      { path: 'inventory/fulfillment', name: 'AdminFulfillment', component: () => import('@/views/warehouse/WarehouseAudit.vue'), meta: { title: '分批收发审核' } },
      {
        path: 'inventory/operation',
        name: 'StockOperation',
        component: () => import('@/views/inventory/StockOperation.vue'),
        meta: { title: '库存调整', icon: 'Switch' },
      },
      {
        path: 'purchase/orders',
        name: 'PurchaseOrders',
        component: () => import('@/views/purchase/PurchaseOrderList.vue'),
        meta: { title: '采购管理', icon: 'ShoppingCart' },
      },
      {
        path: 'sales/orders',
        name: 'SalesOrders',
        component: () => import('@/views/sales/SalesOrderList.vue'),
        meta: { title: '销售管理', icon: 'Sell' },
      },
      {
        path: 'finance/accounts',
        name: 'Accounts',
        component: () => import('@/views/finance/AccountList.vue'),
        meta: { title: '财务管理', icon: 'Money' },
      },
      { path: 'finance/actions-admin', name: 'AdminFinanceActions', component: () => import('@/views/finance/FinanceActions.vue'), meta: { title: '财务复核' } },
      { path: 'finance/payments-admin', name: 'AdminPayments', component: () => import('@/views/finance/PaymentList.vue'), meta: { title: '收付款与冲正' } },
      {
        path: 'inventory/check',
        name: 'StockCheck',
        component: () => import('@/views/inventory/StockCheckList.vue'),
        meta: { title: '库存盘点', icon: 'Finished' },
      },
      {
        path: 'report',
        name: 'Report',
        component: () => import('@/views/report/ReportDashboard.vue'),
        meta: { title: '统计报表', icon: 'DataAnalysis' },
      },
      { path: 'system/backup', name: 'SystemBackup', component: () => import('@/views/system/BackupSettings.vue'), meta: { title: '异机备份设置' } },
      {
        path: 'system/users',
        name: 'SystemUsers',
        component: () => import('@/views/system/UserList.vue'),
        meta: { title: '用户管理', icon: 'User' },
      },
      {
        path: 'system/logs',
        name: 'SystemLogs',
        component: () => import('@/views/system/OperationLogList.vue'),
        meta: { title: '操作日志', icon: 'Document' },
      },
      {
        path: 'system/departments',
        name: 'SystemDepartments',
        component: () => import('@/views/system/DepartmentList.vue'),
        meta: { title: '部门管理', icon: 'OfficeBuilding' },
      },
      {
        path: 'system/dept-assign',
        name: 'SystemDeptAssign',
        component: () => import('@/views/dept/DeptAssign.vue'),
        meta: { title: '员工调度', icon: 'Switch' },
      },
    ],
  },
  // ===== 领导工作台 =====
  {
    path: '/leader',
    component: () => import('@/layouts/LeaderLayout.vue'),
    redirect: '/leader/home',
    meta: { roles: ['leader'] },
    children: [
      {
        path: 'home',
        name: 'LeaderHome',
        component: () => import('@/views/leader/LeaderDashboard.vue'),
        meta: { title: '部门仪表盘' },
      },
      {
        path: 'approval',
        name: 'LeaderApproval',
        component: () => import('@/views/leader/ApprovalList.vue'),
        meta: { title: '待审批单据' },
      },
      {
        path: 'employees',
        name: 'LeaderEmployees',
        component: () => import('@/views/dept/DeptEmployees.vue'),
        meta: { title: '本部门员工' },
      },
      {
        path: 'sales',
        name: 'LeaderSales',
        component: () => import('@/views/sales/SalesOrderList.vue'),
        meta: { title: '销售单' },
      },
      {
        path: 'application',
        name: 'LeaderApplication',
        component: () => import('@/views/leader/ApplicationApproval.vue'),
        meta: { title: '申报审批' },
      },
      {
        path: 'purchase',
        name: 'LeaderPurchase',
        component: () => import('@/views/purchase/PurchaseOrderList.vue'),
        meta: { title: '采购单' },
      },
      {
        path: 'purchase-return',
        name: 'LeaderPurchaseReturn',
        component: () => import('@/views/return/ReturnList.vue'),
        meta: { title: '采购退货', returnType: 'purchase' },
      },
      {
        path: 'sales-return',
        name: 'LeaderSalesReturn',
        component: () => import('@/views/return/ReturnList.vue'),
        meta: { title: '销售退货', returnType: 'sales' },
      },
      {
        path: 'inventory',
        name: 'LeaderInventory',
        component: () => import('@/views/inventory/InventoryList.vue'),
        meta: { title: '库存查询' },
      },
      {
        path: 'warehouse-audit',
        name: 'LeaderWarehouseAudit',
        component: () => import('@/views/warehouse/WarehouseAudit.vue'),
        meta: { title: '仓库审核' },
      },
      {
        path: 'inventory/operation',
        name: 'LeaderStockOperation',
        component: () => import('@/views/inventory/StockOperation.vue'),
        meta: { title: '库存调整审批' },
      },
      { path: 'warehouse-returns/purchase', name: 'LeaderPurchaseReturnQueue', component: () => import('@/views/return/ReturnList.vue'), meta: { title: '采购退货待出库', returnType: 'purchase', warehouseQueue: true } },
      { path: 'warehouse-returns/sales', name: 'LeaderSalesReturnQueue', component: () => import('@/views/return/ReturnList.vue'), meta: { title: '销售退换货待处理', returnType: 'sales', warehouseQueue: true } },
      { path: 'inventory/lots', name: 'LeaderLots', component: () => import('@/views/inventory/LotInventory.vue'), meta: { title: '商品批次库存' } },
      {
        path: 'inventory/check',
        name: 'LeaderStockCheck',
        component: () => import('@/views/inventory/StockCheckList.vue'),
        meta: { title: '盘点差异复核' },
      },
      {
        path: 'base/warehouses',
        name: 'LeaderWarehouses',
        component: () => import('@/views/base/WarehouseList.vue'),
        meta: { title: '仓库管理' },
      },
      {
        path: 'finance',
        name: 'LeaderFinance',
        component: () => import('@/views/finance/AccountList.vue'),
        meta: { title: '往来账' },
      },
    ],
  },
  // ===== 员工工作台 =====
  {
    path: '/staff',
    component: () => import('@/layouts/StaffLayout.vue'),
    redirect: '/staff/home',
    meta: { roles: ['staff'] },
    children: [
      {
        path: 'home',
        name: 'StaffHome',
        component: () => import('@/views/Workspace.vue'),
        meta: { title: '工作台首页' },
      },
      {
        path: 'sales/orders',
        name: 'StaffSales',
        component: () => import('@/views/sales/SalesOrderList.vue'),
        meta: { title: '销售单' },
      },
      {
        path: 'application',
        name: 'StaffApplication',
        component: () => import('@/views/staff/ApplicationList.vue'),
        meta: { title: '申报供应商/客户' },
      },
      {
        path: 'purchase/orders',
        name: 'StaffPurchase',
        component: () => import('@/views/purchase/PurchaseOrderList.vue'),
        meta: { title: '采购单' },
      },
      {
        path: 'purchase-return',
        name: 'StaffPurchaseReturn',
        component: () => import('@/views/return/ReturnList.vue'),
        meta: { title: '采购退货', returnType: 'purchase' },
      },
      {
        path: 'sales-return',
        name: 'StaffSalesReturn',
        component: () => import('@/views/return/ReturnList.vue'),
        meta: { title: '销售退货', returnType: 'sales' },
      },
      {
        path: 'inventory',
        name: 'StaffInventory',
        component: () => import('@/views/inventory/InventoryList.vue'),
        meta: { title: '库存查询' },
      },
      {
        path: 'inventory/operation',
        name: 'StaffStockOperation',
        component: () => import('@/views/inventory/StockOperation.vue'),
        meta: { title: '库存调整' },
      },
      { path: 'warehouse-returns/purchase', name: 'StaffPurchaseReturnQueue', component: () => import('@/views/return/ReturnList.vue'), meta: { title: '采购退货待出库', returnType: 'purchase', warehouseQueue: true } },
      { path: 'warehouse-returns/sales', name: 'StaffSalesReturnQueue', component: () => import('@/views/return/ReturnList.vue'), meta: { title: '销售退换货待处理', returnType: 'sales', warehouseQueue: true } },
      { path: 'inventory/lots', name: 'StaffLots', component: () => import('@/views/inventory/LotInventory.vue'), meta: { title: '商品批次库存' } },
      {
        path: 'inventory/check',
        name: 'StaffStockCheck',
        component: () => import('@/views/inventory/StockCheckList.vue'),
        meta: { title: '库存盘点' },
      },
      {
        path: 'warehouse-audit',
        name: 'StaffWarehouseAudit',
        component: () => import('@/views/warehouse/WarehouseAudit.vue'),
        meta: { title: '仓库审核' },
      },
      {
        path: 'finance/accounts',
        name: 'StaffFinance',
        component: () => import('@/views/finance/AccountList.vue'),
        meta: { title: '往来账' },
      },
      {
        path: 'my-docs',
        name: 'StaffMyDocs',
        component: () => import('@/views/staff/MyDocs.vue'),
        meta: { title: '我经手的单据' },
      },
    ],
  },
  // ===== 财务工作台（财务部不分上下级，扁平化） =====
  {
    path: '/finance',
    component: () => import('@/layouts/FinanceLayout.vue'),
    redirect: '/finance/home',
    meta: { roles: ['leader', 'staff'] },
    children: [
      { path: 'my-actions', name: 'FinanceMyActions', component: () => import('@/components/MyApprovalActions.vue'), meta: { title: '我复核的' } },
      { path: 'actions', name: 'FinanceActions', component: () => import('@/views/finance/FinanceActions.vue'), meta: { title: '财务复核' } },
      { path: 'cost', name: 'FinanceInventoryCost', component: () => import('@/views/inventory/InventoryCost.vue'), meta: { title: '库存成本' } },
      {
        path: 'home',
        name: 'FinanceHome',
        component: () => import('@/views/finance/FinanceDashboard.vue'),
        meta: { title: '财务首页' },
      },
      {
        path: 'accounts',
        name: 'FinanceAccounts',
        component: () => import('@/views/finance/AccountList.vue'),
        meta: { title: '往来账' },
      },
      {
        path: 'payments',
        name: 'FinancePayments',
        component: () => import('@/views/finance/PaymentList.vue'),
        meta: { title: '收付款记录' },
      },
    ],
  },
  {
    path: '/:pathMatch(.*)*',
    redirect: '/dashboard',
  },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

// 路由守卫：未登录跳转登录页；按角色分流
router.beforeEach(async (to, _from, next) => {
  const userStore = useUserStore()
  if(userStore.token){try{await userStore.refreshUserInfo()}catch{if(!userStore.token)next('/login');else next(false);return}}
  document.title = `${String(to.meta.title || 'ERP')} - 中小企业 ERP 系统`

  // 财务部成员统一进财务工作台
  const isFinance = userStore.department === '财务部' && !userStore.isAdmin

  // 未登录
  if (to.path !== '/login' && !userStore.token) {
    next('/login')
    return
  }
  // 已登录访问登录页 → 按角色跳转
  if (to.path === '/login' && userStore.token) {
    next(userStore.isAdmin ? '/dashboard' : isFinance ? '/finance' : userStore.isLeader ? '/leader' : '/staff')
    return
  }

  // 财务部成员强制进入财务工作台
  if (isFinance && !to.path.startsWith('/finance') && to.path !== '/login') {
    next('/finance')
    return
  }

  if (to.meta.warehouseQueue && !userStore.isAdmin && userStore.department !== '仓库部') {
    next(userStore.isLeader ? '/leader' : '/staff')
    return
  }

  // 角色权限校验（仅对配置了 roles 的路由生效）
  const allowedRoles = to.meta.roles as string[] | undefined
  if (allowedRoles && allowedRoles.length > 0) {
    const hasRole = allowedRoles.some((r) => (userStore.roles as string[]).includes(r))
    if (!hasRole) {
      // 无权限访问 → 跳回自己角色该去的首页
      next(userStore.isAdmin ? '/dashboard' : isFinance ? '/finance' : userStore.isLeader ? '/leader' : '/staff')
      return
    }
  }

  next()
})

export default router
