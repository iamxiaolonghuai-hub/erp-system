import { withApprovals } from './approval'
import { get, post, put } from '@/utils/request'
import type { PageQuery, Product, Inventory } from '@/types'
export interface InventoryStats { product_count: number; inventory_count: number; total_quantity: number; low_stock: number; record_count: number }
export function getInventoryStats() { return get<InventoryStats>('/inventory/summary') }

// 商品相关 API（对接 RestfulProductsCtrl）
export function getProducts(params: PageQuery) {
  return get<Product[]>('/products', params)
}

export function getProduct(id: number) {
  return get<Product>(`/products/${id}`)
}

export function createProduct(data: Partial<Product>) {
  return post<Product>('/products', data)
}

export function updateProduct(id: number, data: Partial<Product>) {
  return put<Product>(`/products/${id}`, data)
}

// 商品序列号开关信息（id → has_serial）
export function getSerialInfo() {
  return get<{ id: number; name: string; has_serial: number; supplier_id?: number }[]>('/products/serial-info')
}

// 设置商品序列号开关
export function updateProductSerial(id: number, hasSerial: number) {
  return put<{ id: number; has_serial: number }>(`/products/${id}/serial`, {
    has_serial: hasSerial,
  })
}

// 库存相关 API（对接 RestfulInventoryCtrl + StockController）
export function getInventory(params: PageQuery) {
  return get<Inventory[]>('/inventory', params)
}

// 设置安全库存阈值（min_quantity）
export function updateInventoryMinQuantity(id: number, minQuantity: number) {
  return put<{ id: number; min_quantity: number }>(`/inventory/${id}/safety`, {
    min_quantity: minQuantity,
  })
}

// 入库（业务联动，自动登记流水）
export function stockIn(data: {
  product_id: number
  warehouse_id: number
  qty: number
  biz_type?: string
  biz_no?: string
}) {
  return post('/stock/in', data)
}

// 出库（业务联动，自动扣减库存）
export function stockOut(data: {
  product_id: number
  warehouse_id: number
  qty: number
  biz_type?: string
  biz_no?: string
}) {
  return post('/stock/out', data)
}

// ============ 库存操作单（入库/出库/调拨，走审批） ============
export interface StockOperation {
  id: number
  operation_no: string
  type: string
  product_id: number
  product_name: string
  sku: string
  from_warehouse_id: number
  to_warehouse_id: number
  from_warehouse_name: string
  to_warehouse_name: string
  qty: number
  biz_type: string
  biz_no: string
  logistics_company: string
  logistics_no: string
  status: number
  remark: string
  created_at: string
  [key: string]: unknown
}

// 提交库存操作单
export function createStockOperation(data: {
  unit_cost?: string
  lot_no?: string
  manufactured_on?: string
  expires_on?: string
  type: string
  product_id: number
  from_warehouse_id?: number
  to_warehouse_id?: number
  qty: number
  biz_type?: string
  biz_no?: string
  logistics_company?: string
  logistics_no?: string
  serial_nos?: string
  assigned_approver_id?: number
  request_id?: string
  remark?: string
}) {
  return post<{ operation_id: number; operation_no: string; status: number }>(
    '/stock/operation',
    data,
  )
}

// 审批库存操作单
export function approveStockOperation(data: {
  operation_id: number
  approve: boolean
  assigned_approver_id?: number
  request_id?: string
  remark?: string
}) {
  return post<{ operation_id: number; status: number }>(
    '/stock/operation/approve',
    data,
  )
}

// 库存操作单列表
export function getStockOperations(params?: { type?: string; status?: number }) {
  return withApprovals(get<StockOperation[]>('/stock/operations', params), 'stock_operations')
}
