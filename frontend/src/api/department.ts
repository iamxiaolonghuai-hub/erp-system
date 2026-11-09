import { get, post } from '@/utils/request'
import type { Department, DepartmentEmployee } from '@/types'

// 部门列表
export function getDepartments(params?: object) {
  return get<Department[]>('/departments', params)
}

// 部门员工列表（领导默认本部门，管理员可指定 department 参数）
export function getDepartmentEmployees(department?: string) {
  return get<DepartmentEmployee[]>('/departments/employees', {
    department,
  })
}

// 全部用户列表（仅管理员，含部门，用于调度）
export function getAllUsers() {
  return get<DepartmentEmployee[]>('/departments/users')
}

// 部门仪表盘
export function getDepartmentDashboard(department?: string) {
  return get<Record<string, unknown>>('/departments/dashboard', {
    department,
  })
}

// 部门趋势统计（折线图）
export function getDepartmentTrend(department?: string, period = 'day') {
  return get<{
    period: string
    labels: string[]
    values: number[]
    series?: { name: string; values: number[] }[]
    unit: string
  }>('/departments/trend', { department, period })
}

// 修改用户状态（领导改本部门员工 / 管理员任意）
export function updateUserStatus(userId: number, status: number) {
  return post<{ user_id: number; status: number }>('/users/status', {
    user_id: userId,
    status,
  })
}

export interface HandoverGroup {
  department: string
  count: number
  candidates: { id: number; name: string }[]
}

export function getHandoverPreview(userId: number) {
  return get<HandoverGroup[]>('/departments/handover', { user_id: userId })
}

export function assignEmployee(userId: number, options: {
  department_id?: number
  warehouse_id?: number
  handover_only: boolean
  handover: Record<string, number>
}) {
  return post<{ transferred: number }>('/departments/assign', { user_id: userId, ...options })
}

// 分配仓库（管理员或仓库部领导，给仓库部员工设置所属仓库）
export function assignWarehouse(userId: number, warehouseId: number) {
  return post<{ user_id: number; warehouse_id: number }>(
    '/departments/assign-warehouse',
    { user_id: userId, warehouse_id: warehouseId },
  )
}
