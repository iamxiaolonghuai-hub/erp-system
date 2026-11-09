import { get, post } from '@/utils/request'
import type { Account, Payment } from '@/types'

export type FinanceKind = 'payment' | 'refund' | 'offset' | 'reverse'
export interface FinanceAction {
  id: number; action_no: string; kind: FinanceKind; account_id: number; target_account_id?: number
  amount: string | number; status: number; created_by: number; assigned_approver_id?: number
  approved_by?: number; created_by_name?: string; approved_by_name?: string; assigned_approver_name?: string
  partner_name?: string; biz_no?: string; target_biz_no?: string; payment_no?: string
  remark?: string; review_remark?: string; pay_date?: string; approved_at?: string
  reversed_by?: number; created_at?: string
}
export const financeKindText = { payment: '收付款', refund: '退款', offset: '贷项抵扣', reverse: '冲正' }
export function getFinanceActions(params?: object) { return get<FinanceAction[]>('/finance/actions', params) }
export function getFinanceReviewers() { return get<{id: number; name: string}[]>('/finance/reviewers') }
export function submitFinanceAction(data: object) { return post<{action_id: number}>('/finance/actions', data) }
export function reviewFinanceAction(data: {action_id: number; approve: boolean; remark: string}) { return post('/finance/actions/review', data) }

// 往来账列表（对接 RestfulAccountsCtrl）
export function getAccounts(params?: object) {
  return get<Account[]>('/accounts', params)
}

// 收付款记录列表（对接 RestfulPaymentsCtrl）
export function getPayments(params?: object) {
  return get<Payment[]>('/payments', params)
}

// 收付款登记（更新往来账 + 写记录 + 自动结清）
export function addPayment(data: {
  account_id: number
  amount: number
  pay_method?: string
  pay_date?: string
  request_id?: string
  remark?: string
}) {
  return post<{ payment_id: number; paid_amount: string; total_amount: string; settled: boolean }>(
    '/finance/payments',
    data,
  )
}
