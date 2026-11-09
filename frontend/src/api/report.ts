import { get } from '@/utils/request'

export interface DashboardStats {
  product_count: number
  inventory_total: number
  low_stock: number
  purchase_total: number
  sales_total: number
  gross_profit: number | null
  receivable: number
  payable: number
}

export function getDashboard() {
  return get<DashboardStats>('/report/dashboard')
}

export function getPurchaseSummary() {
  return get<{ supplier_id: number; total_amount: number; order_count: number }[]>('/report/purchase')
}

export function getSalesSummary() {
  return get<{ customer_id: number; total_amount: number; order_count: number }[]>('/report/sales')
}

export function getInventorySummary() {
  return get<{ warehouse_id: number; product_count: number; total_quantity: number }[]>('/report/inventory')
}
