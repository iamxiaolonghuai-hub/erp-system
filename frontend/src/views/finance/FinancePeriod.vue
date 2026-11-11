<template>
  <el-card shadow="never">
    <div class="toolbar">
      <el-date-picker v-model="month" type="month" value-format="YYYY-MM" placeholder="会计月份" @change="load" />
      <el-button :loading="loading" @click="load">查询</el-button>
      <el-button v-if="canClose && data?.can_close" type="warning" :loading="closing" @click="close">结账</el-button>
    </div>
    <p v-if="data">{{ data.period_start }} 至 {{ data.period_end }} · 已结账至 {{ data.closed_through || '—' }}</p>
    <el-table v-loading="loading" :data="data?.rows || []" stripe>
      <el-table-column prop="partner_name" label="往来单位" min-width="140" />
      <el-table-column prop="biz_no" label="单据号" min-width="160" />
      <el-table-column label="类型" width="90">
        <template #default="{row}">{{ row.account_type === 'payable' ? '应付' : '应收' }}</template>
      </el-table-column>
      <el-table-column prop="opening" label="期初余额" min-width="100" align="right" />
      <el-table-column prop="amount_change" label="新增及退货净额" min-width="140" align="right" />
      <el-table-column prop="cash_change" label="现金结算净额" min-width="130" align="right" />
      <el-table-column prop="offset_change" label="抵扣净额" min-width="100" align="right" />
      <el-table-column prop="closing" label="期末余额" min-width="100" align="right" />
    </el-table>
  </el-card>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useUserStore } from '@/stores/user'
import { get, post } from '@/utils/request'
import { newRequestId } from '@/utils/requestId'

interface PeriodReport {
  period_start: string
  period_end: string
  closed_through: string | null
  can_close: boolean
  rows: Record<string, unknown>[]
}

const user = useUserStore()
const canClose = computed(() => user.isAdmin || user.isLeader)
const month = ref(new Date().toLocaleDateString('sv-SE').slice(0, 7))
const data = ref<PeriodReport>()
const loading = ref(false)
const closing = ref(false)
let query = 0

async function load() {
  const current = ++query
  data.value = undefined
  if (!month.value) return
  loading.value = true
  try {
    const result = await get<PeriodReport>('/finance/period', { month: month.value })
    if (current === query) data.value = result.data
  } catch {
    // Request errors are shown by the shared client.
  } finally {
    if (current === query) loading.value = false
  }
}
async function close() {
  const selected = month.value
  try {
    await ElMessageBox.confirm(`确认结账 ${selected}？该期间的账目将冻结。`, '财务结账', { type: 'warning' })
    closing.value = true
    const result = await post('/finance/period/close', { month: selected, request_id: newRequestId() })
    ElMessage.success(result.message)
    await load()
  } catch {
    // Cancellation and request errors are handled by the dialog or client.
  } finally {
    closing.value = false
  }
}
onMounted(load)
</script>

<style scoped>
.toolbar { display: flex; gap: 12px; flex-wrap: wrap; }
</style>
