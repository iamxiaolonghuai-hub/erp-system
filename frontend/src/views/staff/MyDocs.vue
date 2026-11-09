<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-tabs v-model="activeTab" style="flex: 1">
          <el-tab-pane v-for="t in tabs" :key="t.name" :label="t.label" :name="t.name" />
        </el-tabs>
        <el-button type="warning" plain @click="handleExport">
          <el-icon><Download /></el-icon>
          导出
        </el-button>
      </div>

      <el-table :data="currentList" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="order_no" label="单号" width="180" />
        <el-table-column v-if="activeTab === 'after_sales'" label="类型" width="80" align="center">
          <template #default="{ row }">
            <el-tag :type="row.return_type === 'exchange' ? 'warning' : 'info'" size="small">
              {{ row.return_type === 'exchange' ? '换货' : '退货' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="往来单位" min-width="140">
          <template #default="{ row }">{{ partnerName(row) }}</template>
        </el-table-column>
        <el-table-column label="金额" width="140">
          <template #default="{ row }">¥{{ Number(row.total_amount || 0).toFixed(2) }}</template>
        </el-table-column>
        <el-table-column label="状态" width="120">
          <template #default="{ row }">
            <el-tag :type="statusType(row.status)" size="small">{{ statusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="remark" label="备注" min-width="160" />
        <el-table-column prop="created_at" label="提交时间" width="180" />
        <el-table-column label="操作" width="90" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openDetail(row)">详情</el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-empty v-if="!loading && currentList.length === 0" description="暂无我经手的单据" />
    </el-card>

    <!-- 详情弹窗 -->
    <el-dialog v-model="detailVisible" title="单据详情" width="720px">
      <template v-if="detail">
        <el-descriptions :column="2" border size="small" style="margin-bottom: 16px">
          <el-descriptions-item label="单号">{{ detail.order_no || detail.return_no }}</el-descriptions-item>
          <el-descriptions-item label="类型">
            {{ typeText(detail.order_type) }}
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
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getPurchaseOrders, getPurchaseOrderItems } from '@/api/purchase'
import { getSalesOrders, getSalesOrderItems } from '@/api/sales'
import { getSalesReturns, getSalesReturnItems } from '@/api/return'
import { getCustomers, getSuppliers } from '@/api/base'
import { getProducts } from '@/api/inventory'
import { exportCsv } from '@/api/importexport'
import { useUserStore } from '@/stores/user'
import type { Product, Customer, Supplier } from '@/types'

const userStore = useUserStore()

const loading = ref(false)
const activeTab = ref('sales')
const purchaseList = ref<any[]>([])
const salesList = ref<any[]>([])
const aftersalesList = ref<any[]>([])

const detailVisible = ref(false)
const detail = ref<any | null>(null)
const detailItems = ref<any[]>([])
const itemsLoading = ref(false)

const products = ref<Product[]>([])
const customers = ref<Customer[]>([])
const suppliers = ref<Supplier[]>([])

// 按部门显示单据类型：销售部只看销售单+售后单（无采购单），采购部只看采购单（不看销售单），仓库部看采购单+销售单+售后单（经手确认），其他部门保留采购单+销售单
const tabs = computed(() => {
  if (userStore.department === '销售部') {
    return [
      { name: 'sales', label: '销售单' },
      { name: 'after_sales', label: '售后单' },
    ]
  }
  if (userStore.department === '采购部') {
    return [
      { name: 'purchase', label: '采购单' },
    ]
  }
  if (userStore.department === '仓库部') {
    return [
      { name: 'purchase', label: '采购单' },
      { name: 'sales', label: '销售单' },
      { name: 'after_sales', label: '售后单' },
    ]
  }
  return [
    { name: 'purchase', label: '采购单' },
    { name: 'sales', label: '销售单' },
  ]
})

const currentList = computed(() => {
  if (activeTab.value === 'after_sales') return aftersalesList.value
  return activeTab.value === 'purchase' ? purchaseList.value : salesList.value
})

function typeText(t: string | undefined) {
  if (t === 'purchase') return '采购单'
  if (t === 'after_sales') return '售后单'
  return '销售单'
}

function statusText(s: number) {
  return { 0: '待领导审批', 1: '待仓库处理', 2: '已驳回', 3: '已完成', 4: '待修改' }[s] || '未知'
}
function statusType(s: number) {
  return { 0: 'warning', 1: 'warning', 2: 'danger', 3: 'success', 4: 'danger' }[s] || 'info'
}

function partnerName(row: any) {
  if (row.order_type === 'after_sales') {
    return row.customer_name || `客户#${row.customer_id ?? '-'}`
  }
  if (row.order_type === 'purchase') {
    const s = suppliers.value.find((x) => x.id === row.supplier_id)
    return s ? s.name : `供应商#${row.supplier_id ?? '-'}`
  }
  const c = customers.value.find((x) => x.id === row.customer_id)
  return c ? c.name : `客户#${row.customer_id ?? '-'}`
}

async function handleExport() {
  const table =
    activeTab.value === 'after_sales'
      ? 'sales_returns'
      : activeTab.value === 'purchase'
        ? 'purchase_orders'
        : 'sales_orders'
  try {
    await exportCsv(table)
    ElMessage.success('导出成功')
  } catch (e) {
    ElMessage.error('导出失败')
  }
}

async function openDetail(row: any) {
  detail.value = row
  detailVisible.value = true
  itemsLoading.value = true
  detailItems.value = []
  try {
    const res =
      row.order_type === 'purchase'
        ? await getPurchaseOrderItems(row.id)
        : row.order_type === 'after_sales'
          ? await getSalesReturnItems(row.id)
          : await getSalesOrderItems(row.id)
    const items = Array.isArray(res.data) ? res.data : []
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
  const myId = userStore.userInfo?.id
  const dept = userStore.department
  // 后端按经手人过滤（创建/审批/仓库确认任一环节），仓库员工也能看到自己确认过的单据
  const opFilter = myId ? { operator_id: myId } : {}
  const isSales = dept === '销售部'
  const isWarehouse = dept === '仓库部'
  try {
    const tasks: Promise<any>[] = [getSalesOrders(opFilter)]
    if (!isSales) tasks.push(getPurchaseOrders(opFilter))
    if (isSales || isWarehouse) tasks.push(getSalesReturns(opFilter))
    const results = await Promise.all(tasks)
    const so = (Array.isArray(results[0].data) ? results[0].data : [])
      .map((x: any) => ({ ...x, order_type: 'sales' }))
    salesList.value = so
    if (!isSales) {
      const po = (Array.isArray(results[1].data) ? results[1].data : [])
        .map((x: any) => ({ ...x, order_type: 'purchase' }))
      purchaseList.value = po
    }
    if (isSales || isWarehouse) {
      const arIdx = isSales ? 1 : 2
      const ar = (Array.isArray(results[arIdx].data) ? results[arIdx].data : [])
        .map((x: any) => ({
          ...x,
          order_type: 'after_sales',
          order_no: x.return_no || '',
        }))
      aftersalesList.value = ar
    }
  } catch (e) {
    // 已提示
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  // 采购部只有采购单 tab，默认选中它
  if (userStore.department === '采购部') activeTab.value = 'purchase'
  loadData()
  loadOptions()
})
</script>

<style scoped>
.toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
}
.toolbar :deep(.el-tabs__header) {
  margin-bottom: 0;
}
.toolbar .el-button {
  margin-left: auto;
  flex-shrink: 0;
}
</style>
