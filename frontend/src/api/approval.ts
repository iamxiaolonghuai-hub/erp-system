import { get } from '@/utils/request'
import { useUserStore } from '@/stores/user'
import type { ApiResponse } from '@/types'
export async function withApprovals<T extends { id: number; status?: number }>(response: Promise<ApiResponse<T[]>>, table: string): Promise<ApiResponse<T[]>> {
  const [result, metadata] = await Promise.all([response, get<{ id:number; assigned_approver_id:number|null; assigned_approver_name:string }[]>('/approval/assignments', {table})])
  const lookup = new Map(metadata.data.map(row => [row.id,row]))
  const user = useUserStore()
  result.data = result.data.map(row => ({...row,...lookup.get(row.id)})).filter(row => {
    const assigned = lookup.get(row.id)?.assigned_approver_id
    return !user.isLeader || user.isAdmin || row.status !== 0 || !assigned || assigned === user.userInfo?.id
  })
  return result
}
