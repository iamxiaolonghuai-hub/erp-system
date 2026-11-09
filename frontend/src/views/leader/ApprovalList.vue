<template>
  <div>
    <el-card shadow="never">
      <el-tabs v-model="activeTab">
        <el-tab-pane label="待审批" name="pending" />
        <el-tab-pane label="部门已处理单据" name="done" />
        <el-tab-pane label="我审批的" name="mine" />
      </el-tabs>
      <MyApprovalActions v-if="activeTab === 'mine'" />
      <template v-else>
      <div class="approval-filters">
        <el-input v-model="searchNo" clearable placeholder="搜索单号" style="width:200px" />
        <el-date-picker v-model="dateRange" type="daterange" value-format="YYYY-MM-DD" start-placeholder="开始日期" end-placeholder="结束日期" />
        <span>按提交时间筛选</span>
      </div>

      <el-table :data="filteredList" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="assigned_approver_name" label="指定审批人" min-width="130" />
        <el-table-column prop="order_no" label="单号" width="180" />
        <el-table-column label="类型" width="100">
          <template #default="{ row }">
            <el-tag :type="row.order_type === 'purchase' ? 'warning' : 'success'" size="small">
              {{ row.order_type === 'purchase' ? '采购单' : '销售单' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="往来单位" min-width="130">
          <template #default="{ row }">{{ partnerName(row) }}</template>
        </el-table-column>
        <el-table-column label="申请人" min-width="110">
          <template #default="{ row }">
            {{ row.created_by_name || (row.created_by ? `用户#${row.created_by}` : '-') }}
          </template>
        </el-table-column>
        <el-table-column label="金额" width="130">
          <template #default="{ row }">¥{{ Number(row.total_amount || 0).toFixed(2) }}</template>
        </el-table-column>
        <el-table-column prop="created_at" label="提交时间" width="180" />
        <el-table-column label="状态" width="110">
          <template #default="{ row }">
            <el-tag :type="statusType(row.status)" size="small">{{ statusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" min-width="220" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openDetail(row)">详情</el-button>
            <template v-if="row.status === 0">
              <el-button type="success" size="small" @click="handleApprove(row, true)">批准</el-button>
              <el-button type="danger" size="small" @click="handleApprove(row, false)">驳回</el-button>
            </template>
          </template>
        </el-table-column>
      </el-table>
      </template>
    </el-card>

    <!-- 单据详情弹窗 -->
    <el-dialog v-model="detailVisible" title="单据详情" width="720px">
      <template v-if="detail">
        <el-descriptions :column="2" border size="small" style="margin-bottom: 16px">
          <el-descriptions-item label="单号">{{ detail.order_no }}</el-descriptions-item>
          <el-descriptions-item label="类型">
            {{ detail.order_type === 'purchase' ? '采购单' : '销售单' }}
          </el-descriptions-item>
          <el-descriptions-item label="往来单位">{{ partnerName(detail) }}</el-descriptions-item>
          <el-descriptions-item label="总额">
            <span style="color: #e6a23c; font-weight: 600">
              ¥{{ Number(detail.total_amount || 0).toFixed(2) }}
            </span>
          </el-descriptions-item>
          <el-descriptions-item label="状态">
            <el-tag :type="statusType(detail.status)" size="small">{{ statusText(detail.status) }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="提交时间">{{ detail.created_at || '-' }}</el-descriptions-item>
          <el-descriptions-item label="备注" :span="2">{{ detail.remark || '-' }}</el-descriptions-item>
          <el-descriptions-item label="审批人" :span="2">
            {{ approverName(detail) }}
          </el-descriptions-item>
        </el-descriptions>

        <el-divider content-position="left">商品明细</el-divider>
        <el-table :data="detailItems" v-loading="itemsLoading" border size="small">
          <el-table-column prop="product_name" label="商品" min-width="180" />
          <el-table-column prop="quantity" label="数量" width="100" />
          <el-table-column label="单价(¥)" width="120">
            <template #default="{ row }">¥{{ Number(row.price || 0).toFixed(2) }}</template>
          </el-table-column>
          <el-table-column label="小计(¥)" width="120">
            <template #default="{ row }">¥{{ Number(row.amount || row.price * row.quantity || 0).toFixed(2) }}</template>
          </el-table-column>
        </el-table>
      </template>
      <template #footer>
        <el-button @click="detailVisible = false">关闭</el-button>
        <template v-if="detail && detail.status === 0">
          <el-button type="success" @click="approveFromDetail(true)">批准</el-button>
          <el-button type="danger" @click="approveFromDetail(false)">驳回</el-button>
        </template>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import MyApprovalActions from '@/components/MyApprovalActions.vue'
import { ref, computed, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getPurchaseOrders, getPurchaseOrderItems, approvePurchaseOrder } from '@/api/purchase'
import { getSalesOrders, getSalesOrderItems, approveSalesOrder } from '@/api/sales'
import { getCustomers, getSuppliers } from '@/api/base'
import { getProducts } from '@/api/inventory'
import { useUserStore } from '@/stores/user'
import type { Product, Customer, Supplier } from '@/types'

interface ApprovalItem {
  id: number
  order_no: string
  order_type: 'purchase' | 'sales'
  total_amount?: string | number
  status?: number
  created_at?: string
  remark?: string
  supplier_id?: number
  customer_id?: number
  approved_by?: number | null
  [key: string]: unknown
}

const loading = ref(false)
const activeTab = ref('pending')
const searchNo = ref('')
const dateRange = ref<[string, string] | null>(null)
const list = ref<ApprovalItem[]>([])
const userStore = useUserStore()

// 各部门领导只审批本部门单据；admin 可审批全部；仓库部领导不参与采购/销售审批
const visibleTypes = computed<string[]>(() => {
  if (userStore.isAdmin) return ['purchase', 'sales']
  if (userStore.department === '采购部') return ['purchase']
  if (userStore.department === '销售部') return ['sales']
  return []
})

const detailVisible = ref(false)
const detail = ref<ApprovalItem | null>(null)
const detailItems = ref<any[]>([])
const itemsLoading = ref(false)

const products = ref<Product[]>([])
const customers = ref<Customer[]>([])
const suppliers = ref<Supplier[]>([])

const filteredList = computed(() => {
  const query = searchNo.value.trim().toLowerCase()
  return list.value.filter(x => {
    const state = activeTab.value === 'pending' ? x.status === 0 : x.status !== 0
    const date = String(x.created_at || '').slice(0, 10)
    return state && (!query || String(x.order_no || '').toLowerCase().includes(query)) && (!dateRange.value || (date >= dateRange.value[0] && date <= dateRange.value[1]))
  })
})

function statusText(s?: number) {
  return { 0: '待领导审批', 1: '待仓库处理', 2: '已驳回', 3: '已完成', 4: '待修改', 5: '待仓库领导批准' }[s ?? 0] || '未知'
}
function statusType(s?: number) {
  return { 0: 'warning', 1: 'warning', 2: 'danger', 3: 'success', 4: 'danger' }[s ?? 0] || 'info'
}

function partnerName(row: ApprovalItem) {
  if (row.order_type === 'purchase') {
    const s = suppliers.value.find((x) => x.id === row.supplier_id)
    return s ? s.name : `供应商#${row.supplier_id ?? '-'}`
  }
  const c = customers.value.find((x) => x.id === row.customer_id)
  return c ? c.name : `客户#${row.customer_id ?? '-'}`
}

function approverName(row: ApprovalItem) {
  if (typeof row.approver_name === 'string') return row.approver_name
  if (row.approved_by == null) return '-'
  return `用户#${row.approved_by}`
}

async function loadOptions() {
  try {
    const [p, c, s] = await Promise.all([
      getProducts({}), getCustomers({}), getSuppliers({}),
    ])
    products.value = Array.isArray(p.data) ? p.data : []
    customers.value = Array.isArray(c.data) ? c.data : []
    suppliers.value = Array.isArray(s.data) ? s.data : []
  } catch (e) {
    // 已提示
  }
}

async function loadData() {
  loading.value = true
  try {
    const [poRes, soRes] = await Promise.all([getPurchaseOrders({}), getSalesOrders({})])
    const po = (Array.isArray(poRes.data) ? poRes.data : []).map((x) => ({
      ...x,
      order_type: 'purchase' as const,
    }))
    const so = (Array.isArray(soRes.data) ? soRes.data : []).map((x) => ({
      ...x,
      order_type: 'sales' as const,
    }))
    const all = [...po, ...so]
    list.value = all.filter((x) =>
      visibleTypes.value.includes(x.order_type),
    )
  } catch (e) {
    // 已提示
  } finally {
    loading.value = false
  }
}

async function openDetail(row: ApprovalItem) {
  detail.value = row
  detailVisible.value = true
  itemsLoading.value = true
  detailItems.value = []
  try {
    const res =
      row.order_type === 'purchase'
        ? await getPurchaseOrderItems(row.id)
        : await getSalesOrderItems(row.id)
    const items = Array.isArray(res.data) ? res.data : []
    // 映射商品名
    detailItems.value = items.map((it: any) => ({
      ...it,
      product_name: products.value.find((p) => p.id === it.product_id)?.name || `商品#${it.product_id}`,
    }))
  } catch (e) {
    // 已提示
  } finally {
    itemsLoading.value = false
  }
}

async function handleApprove(row: ApprovalItem, approve: boolean) {
  await doApprove(row, approve)
}

async function approveFromDetail(approve: boolean) {
  if (!detail.value) return
  await doApprove(detail.value, approve)
  detailVisible.value = false
}

async function doApprove(row: ApprovalItem, approve: boolean) {
  try {
    if (row.order_type === 'purchase') {
      await approvePurchaseOrder({ order_id: row.id, approve })
    } else {
      await approveSalesOrder({ order_id: row.id, approve })
    }
    ElMessage.success(approve ? '已批准' : '已驳回')
    loadData()
  } catch (e) {
    // 已提示
  }
}

onMounted(() => {
  loadData()
  loadOptions()
})
</script>
<style scoped>
.approval-filters { display:flex; align-items:center; gap:12px; flex-wrap:wrap; margin:16px 0; }
.approval-filters span { color:#909399; font-size:12px; }
</style>
