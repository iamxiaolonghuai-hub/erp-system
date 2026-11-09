import { get, post, put, del } from '@/utils/request'
import type { Role, OperationLog, UserInfo } from '@/types'

// ============ 用户 ============
export function getUsers(params?: object) {
  return get<UserInfo[]>('/users', params)
}
export function updateUser(id: number, data: Partial<UserInfo>) {
  return put(`/users/${id}`, data)
}
export function deleteUser(id: number) {
  return del(`/users/${id}`)
}
// 创建用户（自动哈希密码 + 分配角色）
export function createUser(data: {
  username: string
  password: string
  real_name?: string
  email?: string
  phone?: string
  role_id?: number
}) {
  return post<UserInfo>('/users/create', data)
}
// 重置密码
export function resetPassword(userId: number, password: string) {
  return post('/users/reset_password', { user_id: userId, password })
}

// ============ 角色 ============
export function getRoles(params?: object) {
  return get<Role[]>('/roles', params)
}
export function createRole(data: Partial<Role>) {
  return post<Role>('/roles', data)
}
export function updateRole(id: number, data: Partial<Role>) {
  return put(`/roles/${id}`, data)
}
export function deleteRole(id: number) {
  return del(`/roles/${id}`)
}

// ============ 操作日志 ============
export function getOperationLogs(params?: object) {
  return get<OperationLog[]>('/operationlogs', params)
}
