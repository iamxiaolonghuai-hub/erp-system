import { withApprovals } from './approval'
import { get, post } from '@/utils/request'
import type { PurchaseOrder, OrderItem } from '@/types'

// 采购单列表（对接 RestfulPurchaseOrdersCtrl）
export function getPurchaseOrders(params?: object) {
  return withApprovals(get<PurchaseOrder[]>('/purchaseorders', params), 'purchase_orders')
}

// 采购单明细
export function getPurchaseOrderItems(orderId: number) {
  return get(`/purchaseorderitems?filter=${JSON.stringify({ order_id: orderId })}`)
}

// 创建采购单（下单即入库 + 生成应付账）
export function createPurchaseOrder(data: {
  supplier_id: number
  warehouse_id: number
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  items: OrderItem[]
}) {
  return post<{ order_id: number; order_no: string; account_id: number }>(
    '/purchase/orders',
    data,
  )
}

// 审批采购单（领导）
export function approvePurchaseOrder(data: {
  order_id: number
  approve: boolean
  warehouse_id?: number
}) {
  return post<{ order_id: number; status: number }>('/purchase/approve', data)
}

// 仓库入库审核（仓库部）
export function warehouseInbound(data: {
  order_id: number
  approve: boolean
  warehouse_id?: number
  assigned_approver_id?: number
  request_id?: string
  remark?: string
}) {
  return post<{ order_id: number; status: number }>('/purchase/inbound', data)
}

// 员工修改后重新提交
export function resubmitPurchaseOrder(data: {
  order_id: number
  items: OrderItem[]
}) {
  return post<{ order_id: number; status: number }>('/purchase/resubmit', data)
}
