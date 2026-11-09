import { request } from '@/utils/request'
import { useUserStore } from '@/stores/user'

// 导出 CSV（触发浏览器下载）
export async function exportCsv(table: string) {
  const userStore = useUserStore()
  const token = userStore.token
  const resp = await fetch(`/api/export/${table}`, {
    headers: { Authorization: `Bearer ${token}` },
  })
  if (!resp.ok) {
    throw new Error('导出失败')
  }
  const blob = await resp.blob()
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `${table}.csv`
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
  URL.revokeObjectURL(url)
}

// 导入 CSV
export interface ImportResult {
  success: number; failed: number; skipped?: number; total?: number
  issues?: { row: number; name: string; type: string; message: string }[]
  issues_truncated?: boolean
}
export function importCsv(table: string, csv: string) {
  return request<ImportResult>({ method: 'post', url: `/import/${table}`, data: { csv }, timeout: 120000 })
}
