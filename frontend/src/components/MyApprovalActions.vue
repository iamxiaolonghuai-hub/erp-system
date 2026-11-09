<template>
  <div>
    <div class="filters">
      <el-input v-model="number" clearable placeholder="原单号或出入库批次号" style="width: 220px" @keyup.enter="search" />
      <el-date-picker v-model="dates" type="daterange" value-format="YYYY-MM-DD" start-placeholder="处理起始日" end-placeholder="处理结束日" />
      <el-select v-model="action" clearable placeholder="全部处理动作" style="width: 155px">
        <el-option v-for="(label, key) in actionNames" :key="key" :label="label" :value="key" />
      </el-select>
      <el-button type="primary" @click="search">查询</el-button>
    </div>
    <el-table :data="rows" v-loading="loading" stripe>
      <el-table-column label="业务" width="120">
        <template #default="{row}">{{ types[row.document_type] || row.document_type }}</template>
      </el-table-column>
      <el-table-column prop="document_no" label="业务单号" min-width="170" />
      <el-table-column prop="batch_no" label="出入库批次号" min-width="170" />
      <el-table-column label="我的动作" width="125">
        <template #default="{row}">{{ actionNames[row.action] || row.action }}</template>
      </el-table-column>
      <el-table-column prop="initiator_name" label="业务发起人" min-width="110" />
      <el-table-column prop="handler_name" label="经办人" min-width="100" />
      <el-table-column prop="approver_name" label="批准或复核人" min-width="125" />
      <el-table-column label="实际处理时间" min-width="180">
        <template #default="{row}">{{ row.action_at.slice(0, 19) }}</template>
      </el-table-column>
    </el-table>
    <div class="filters pages">
      <el-button :disabled="loading || page === 1" @click="load(page - 1)">上一页</el-button>
      <span>第 {{ page }} 页</span>
      <el-button :disabled="loading || !hasMore" @click="load(page + 1)">下一页</el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { get } from '@/utils/request'

const props = defineProps<{ warehouse?: boolean }>()
const types: Record<string, string> = {
  purchase_orders: '采购单', sales_orders: '销售单', purchase_returns: '采购退货',
  sales_returns: '销售退换货', stock_operations: '库存调整', stock_checks: '库存盘点',
  applications: '资料申报', finance_actions: '财务申请',
}
const actionNames: Record<string, string> = {
  approve: '批准', reject: '驳回', review: '盘点复核', receive: '分批入库',
  prepare: '备货', ship: '出库放行', warehouse_complete: '仓库确认', warehouse_reject: '仓库打回',
}
interface ActionRow {
  document_type: string
  document_no: string
  batch_no: string | null
  action: string
  action_at: string
  initiator_name: string | null
  handler_name: string | null
  approver_name: string | null
}
const number = ref('')
const dates = ref<[string, string] | null>(null)
const action = ref('')
const page = ref(1)
const loading = ref(false)
const hasMore = ref(false)
const rows = ref<ActionRow[]>([])
let filters: { number: string; from?: string; to?: string; action: string }
let request = 0

async function load(target: number) {
  const current = ++request
  loading.value = true
  try {
    const result = await get<{rows: ActionRow[]; has_more: boolean}>('/approval/my-actions', {
      ...filters, page: target, mode: props.warehouse ? 'warehouse' : undefined,
    })
    if (current !== request) return
    rows.value = result.data.rows
    hasMore.value = result.data.has_more
    page.value = target
  } catch {
    // 请求层已提示。
  } finally {
    if (current === request) loading.value = false
  }
}
function search() {
  filters = { number: number.value.trim(), from: dates.value?.[0], to: dates.value?.[1], action: action.value }
  void load(1)
}
onMounted(search)
</script>

<style scoped>
.filters { display: flex; align-items: center; gap: 12px; flex-wrap: wrap; margin-bottom: 14px; }
.pages { justify-content: flex-end; margin: 12px 0 0; }
</style>
