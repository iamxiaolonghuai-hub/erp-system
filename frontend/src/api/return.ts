import { withApprovals } from './approval'
import { get, post } from '@/utils/request'
import type { OrderItem } from '@/types'

// ===== 采购退货 =====
export interface PurchaseReturn {
  id: number
  return_no: string
  order_id: number
  supplier_id: number
  warehouse_id: number
  total_amount: string
  status: number
  remark: string
  logistics_company: string
  logistics_no: string
  supplier_name: string
  created_at: string
  [key: string]: unknown
}

export interface SalesReturn {
  id: number
  return_no: string
  return_type: string
  order_id: number
  customer_id: number
  warehouse_id: number
  total_amount: string
  status: number
  remark: string
  logistics_company: string
  logistics_no: string
  customer_name: string
  created_at: string
  [key: string]: unknown
}

export interface ReturnItem {
  id: number
  direction: string
  product_id: number
  quantity: number
  price: string
  amount: string
  product_name: string
  sku: string
  [key: string]: unknown
}

// 采购退货列表
export function getPurchaseReturns(params?: object) {
  return withApprovals(get<PurchaseReturn[]>('/purchase/returns', params), 'purchase_returns')
}

// 销售退货列表
export function getSalesReturns(params?: object) {
  return withApprovals(get<SalesReturn[]>('/sales/returns', params), 'sales_returns')
}

// 采购退货明细
export function getPurchaseReturnItems(returnId: number) {
  return get<ReturnItem[]>('/purchase/return/items', { return_id: returnId })
}

// 销售退货明细
export function getSalesReturnItems(returnId: number) {
  return get<ReturnItem[]>('/sales/return/items', { return_id: returnId })
}

// 发起采购退货
export function createPurchaseReturn(data: {
  order_id: number
  warehouse_id: number
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  logistics_company?: string
  logistics_no?: string
  items: OrderItem[]
}) {
  return post<{ return_id: number; return_no: string; status: number }>(
    '/purchase/return',
    data,
  )
}

// 审批采购退货（领导）
export function approvePurchaseReturn(data: {
  return_id: number
  approve: boolean
}) {
  return post<{ return_id: number; status: number }>(
    '/purchase/return/approve',
    data,
  )
}

// 仓库审核采购退货
export function warehousePurchaseReturn(data: {
  return_id: number
  approve: boolean
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  logistics_company?: string
  logistics_no?: string
}) {
  return post<{ return_id: number; status: number }>(
    '/purchase/return/warehouse',
    data,
  )
}

// 发起销售退货
export function createSalesReturn(data: {
  order_id: number
  warehouse_id: number
  return_type?: string
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  logistics_company?: string
  logistics_no?: string
  items: OrderItem[]
}) {
  return post<{ return_id: number; return_no: string; status: number }>(
    '/sales/return',
    data,
  )
}

// 审批销售退货（领导）
export function approveSalesReturn(data: {
  return_id: number
  approve: boolean
}) {
  return post<{ return_id: number; status: number }>('/sales/return/approve', data)
}

// 仓库审核销售退货
export function warehouseSalesReturn(data: {
  return_id: number
  approve: boolean
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  logistics_company?: string
  logistics_no?: string
}) {
  return post<{ return_id: number; status: number }>('/sales/return/warehouse', data)
}
