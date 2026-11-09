import { withApprovals } from './approval'
import { get, post } from '@/utils/request'
import type { SalesOrder, OrderItem } from '@/types'

// 销售单列表（对接 RestfulSalesOrdersCtrl）
export function getSalesOrders(params?: object) {
  return withApprovals(get<SalesOrder[]>('/salesorders', params), 'sales_orders')
}

// 销售单明细
export function getSalesOrderItems(orderId: number) {
  return get(`/salesorderitems?filter=${JSON.stringify({ order_id: orderId })}`)
}

// 创建销售单（下单即出库 + 生成应收账）
export function createSalesOrder(data: {
  customer_id: number
  warehouse_id: number
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  items: OrderItem[]
}) {
  return post<{ order_id: number; order_no: string; account_id: number }>(
    '/sales/orders',
    data,
  )
}

// 审批销售单（领导）
export function approveSalesOrder(data: {
  order_id: number
  approve: boolean
  warehouse_id?: number
}) {
  return post<{ order_id: number; status: number }>('/sales/approve', data)
}

// 仓管员确认出库（仓库部，1→5 填物流，待领导批准）
export function warehouseConfirm(data: {
  order_id: number
  approve: boolean
  warehouse_id?: number
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  logistics_company?: string
  logistics_no?: string
}) {
  return post<{ order_id: number; status: number }>('/sales/confirm', data)
}

// 仓库领导批准出库（仓库部领导，5→3 扣库存）
export function warehouseOutbound(data: {
  order_id: number
  approve: boolean
  warehouse_id?: number
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  logistics_company?: string
  logistics_no?: string
}) {
  return post<{ order_id: number; status: number }>('/sales/outbound', data)
}

// 员工修改后重新提交
export function resubmitSalesOrder(data: {
  order_id: number
  items: OrderItem[]
}) {
  return post<{ order_id: number; status: number }>('/sales/resubmit', data)
}
