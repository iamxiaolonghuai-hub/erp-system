// 统一 API 响应类型
export interface ApiResponse<T = unknown> {
  code: number
  message: string
  data: T
}

// 登录请求/响应
export interface LoginRequest {
  username: string
  password: string
}

export interface LoginData {
  token: string
  user: UserInfo
}

// 用户信息
export interface UserInfo {
  id: number
  username: string
  real_name: string
  department?: string
  email?: string
  phone?: string
  status: number
  roles?: string[]
  created_at?: string
  [key: string]: unknown
}

// 通用分页请求
export interface PageQuery {
  page?: number
  page_size?: number
  [key: string]: unknown
}

// 通用分页响应
export interface PageResult<T> {
  list: T[]
  total: number
  page: number
  page_size: number
}

// 商品
export interface Product {
  batch_enabled?: number
  expiry_enabled?: number
  supplier_id?: number
  has_serial?: number
  id: number
  sku: string
  name: string
  category_id?: number
  unit?: string
  spec?: string
  price?: number
  cost_price?: number
  status?: number
  description?: string
  created_at?: string
  [key: string]: unknown
}

// 库存
export interface Inventory {
  reserved_qty?: number
  available_qty?: number
  id: number
  product_id: number
  warehouse_id: number
  quantity: number
  min_quantity: number
  [key: string]: unknown
}

// 供应商
export interface Supplier {
  id: number
  name: string
  contact?: string
  phone?: string
  email?: string
  address?: string
  status?: number
  remark?: string
  [key: string]: unknown
}

// 客户
export interface Customer {
  id: number
  name: string
  contact?: string
  phone?: string
  email?: string
  address?: string
  status?: number
  remark?: string
  [key: string]: unknown
}

// 员工
export interface Employee {
  department_id?:number | null
  id: number
  user_id?: number | null
  emp_no: string
  name: string
  department?: string
  position?: string
  phone?: string
  email?: string
  status?: number
  [key: string]: unknown
}

// 仓库
export interface Warehouse {
  id: number
  name: string
  code: string
  address?: string
  manager?: string
  status?: number
  [key: string]: unknown
}

// 商品分类
export interface Category {
  id: number
  parent_id?: number
  name: string
  sort_order?: number
  [key: string]: unknown
}

// 采购/销售单明细
export interface OrderItem {
  product_id: number
  quantity: number
  price: number
  direction?: string
}

// 采购单
export interface PurchaseOrder {
  id: number
  order_no: string
  supplier_id: number
  total_amount?: string | number
  status?: number
  remark?: string
  created_by?: number | null
  created_at?: string
  [key: string]: unknown
}

// 销售单
export interface SalesOrder {
  id: number
  order_no: string
  customer_id: number
  total_amount?: string | number
  status?: number
  remark?: string
  created_by?: number | null
  created_at?: string
  [key: string]: unknown
}

// 应收/应付往来账
export interface Account {
  offset_amount?: string | number
  id: number
  account_type: string
  biz_type: string
  biz_no: string
  partner_id: number
  partner_name?: string
  amount?: string | number
  paid_amount?: string | number
  status?: number
  due_date?: string | null
  created_at?: string
  [key: string]: unknown
}

// 收付款记录
export interface Payment {
  id: number
  payment_no: string
  account_id: number
  payment_type: string
  amount?: string | number
  pay_method?: string
  pay_date?: string
  remark?: string
  created_at?: string
  [key: string]: unknown
}

// 角色
export interface Role {
  id: number
  name: string
  code: string
  description?: string
  created_at?: string
  [key: string]: unknown
}

// 权限
export interface Permission {
  id: number
  name: string
  code: string
  module?: string
  description?: string
  [key: string]: unknown
}

// 操作日志
export interface OperationLog {
  id: number
  user_id?: number | null
  username?: string
  method?: string
  path?: string
  ip?: string
  status_code?: number
  cost_ms?: number
  created_at?: string
  [key: string]: unknown
}

// 盘点单
export interface StockCheck {
  counted_by?: number | null
  counter_name?: string
  created_by_name?: string
  reviewer_name?: string
  id: number
  check_no: string
  warehouse_id: number
  status?: number
  remark?: string
  created_by?: number | null
  created_at?: string
  [key: string]: unknown
}

// 盘点明细
export interface StockCheckItem {
  id: number
  check_id: number
  product_id: number
  book_qty: number
  actual_qty: number
  diff_qty: number
  [key: string]: unknown
}

// 部门
export interface Department {
  status?:number
  id: number
  name: string
  code: string
  description?: string
  [key: string]: unknown
}

// 部门员工
export interface DepartmentEmployee {
  department_id?:number | null
  id: number
  username: string
  real_name: string
  department: string
  email?: string
  phone?: string
  status: number
  role_code?: string
  [key: string]: unknown
}
