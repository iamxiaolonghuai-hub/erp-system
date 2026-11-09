import { withApprovals } from './approval'
import { get, post } from '@/utils/request'

// 申报单类型
export type ApplicationType = 'supplier' | 'customer' | 'product'

export interface ApplicationItem {
  id: number
  application_no: string
  type: ApplicationType
  title: string
  payload?: string
  status?: number
  applicant_id?: number
  approved_by?: number
  assigned_approver_id?: number
  request_id?: string
  remark?: string
  created_at?: string
  applicant_name?: string
  approver_name?: string
  [key: string]: unknown
}

// 创建申报单
export function createApplication(data: {
  type: ApplicationType
  title: string
  payload: Record<string, unknown>
  assigned_approver_id?: number
  request_id?: string
  remark?: string
}) {
  return post<{ application_no: string }>('/application/create', data)
}

// 审批申报单
export function approveApplication(data: {
  application_id: number
  approve: boolean
  assigned_approver_id?: number
  request_id?: string
  remark?: string
}) {
  return post<{ status: number }>('/application/approve', data)
}

// 申报单列表
export function getApplications(params?: { type?: string; status?: number }) {
  return withApprovals(get<ApplicationItem[]>('/application/list', params), 'applications')
}

// 被驳回的申报单修改后重新提交
export function resubmitApplication(data: {
  application_id: number
  title: string
  payload: Record<string, unknown>
}) {
  return post<{ application_id: number; status: number }>('/application/resubmit', data)
}
