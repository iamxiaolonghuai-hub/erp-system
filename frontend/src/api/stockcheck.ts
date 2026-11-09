import { get, post } from '@/utils/request'
import type { StockCheck, StockCheckItem } from '@/types'

// 盘点单列表（对接 RestfulStockChecksCtrl）
export function getStockChecks(params?: object) {
  return get<StockCheck[]>('/stockchecks', params)
}

// 盘点明细
export function getStockCheckItems(checkId: number) {
  return get<StockCheckItem[]>('/stockcheckitems', { filter: JSON.stringify({ check_id: checkId }) })
}

// 创建盘点单
export function createStockCheck(data: {
  warehouse_id: number
  request_id?: string
  remark?: string
  items: { product_id: number; book_qty: number; actual_qty: number }[]
}) {
  return post<{ check_id: number; check_no: string }>('/stockcheck/create', data)
}

// 完成盘点（差异调整库存）
export function completeStockCheck(checkId: number) {
  return post<{ check_id: number; adjust_count: number }>('/stockcheck/complete', {
    check_id: checkId,
  })
}
