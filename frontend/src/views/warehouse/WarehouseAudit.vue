<template>
  <div>
    <el-alert
      :title="isLeaderView
        ? '待出库（销售）：仓管员已备货并填写物流，由你批准放行，批准后扣减库存并生成应收账'
        : '待出库（销售）：由你确认备货并填写物流信息，提交后待仓库领导批准'
      "
      type="info"
      :closable="false"
      show-icon
      style="margin-bottom: 16px"
    />
    <el-card shadow="never">
      <el-tabs v-model="activeTab">
        <el-tab-pane label="待入库（采购）" name="inbound" />
        <el-tab-pane :label="isLeaderView ? '待出库（销售·待领导批准）' : '待出库（销售·仓管员确认）'" name="outbound" />
        <el-tab-pane label="我处理的" name="history" />
      </el-tabs>
      <MyApprovalActions v-if="activeTab === 'history'" warehouse />
      <template v-else>
      <el-table :data="currentList" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="order_no" label="单号" width="190" />
        <el-table-column label="处理仓库" min-width="190"><template #default="{ row }"><span v-if="row.warehouse_id">{{ warehouseName(row.warehouse_id) }}</span><el-select v-else v-model="legacyWarehouses[row.id + ':' + (row._kind || activeTab)]" placeholder="旧单据：请选择仓库" filterable><el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" /></el-select></template></el-table-column>
        <el-table-column prop="created_by_name" label="发起人" min-width="120" />
        <el-table-column prop="approver_name" label="业务审批人" min-width="120" />
        <el-table-column v-if="activeTab === 'outbound'" prop="warehouse_confirmed_name" label="仓库确认人" min-width="120" />
        <el-table-column label="往来单位" min-width="140">
          <template #default="{ row }">{{ partnerName(row) }}</template>
        </el-table-column>
        <el-table-column label="金额" width="120">
          <template #default="{ row }">¥{{ Number(row.total_amount || 0).toFixed(2) }}</template>
        </el-table-column>
        <el-table-column label="状态" width="130">
          <template #default="{ row }">
            <el-tag :type="statusTag(row.status)" size="small">{{ statusText(row.status, row) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column v-if="activeTab === 'outbound'" label="物流信息" min-width="160">
          <template #default="{ row }">
            <template v-if="row.logistics_company || row.logistics_no">
              {{ row.logistics_company || '-' }} / {{ row.logistics_no || '-' }}
            </template>
            <span v-else style="color: #999">未填写</span>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="提交时间" width="170" />
        <el-table-column label="操作" min-width="200" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openDetail(row)">详情</el-button><BatchDelivery :id="row.id" :warehouse="warehouseFor(row)" :purchase="activeTab==='inbound'" :leader="isLeaderView" @changed="loadData" />
            <el-button
              v-if="false" type="success"
              size="small"
              @click="handleConfirm(row)"
            >{{ confirmBtnText() }}</el-button>
            <el-button type="danger" size="small" @click="openReject(row)">打回</el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-empty v-if="!loading && currentList.length === 0" description="暂无待处理单据" />
      </template>
    </el-card>

    <!-- 详情弹窗 -->
    <el-dialog v-model="detailVisible" :title="detailTitle" width="720px">
      <template v-if="detail">
        <el-descriptions :column="2" border size="small" style="margin-bottom: 16px">
          <el-descriptions-item label="单号">{{ detail.order_no }}</el-descriptions-item>
          <el-descriptions-item label="发起人">{{ detail.created_by_name || '—' }}</el-descriptions-item>
          <el-descriptions-item label="业务审批人">{{ detail.approver_name || '待审批' }}</el-descriptions-item>
          <el-descriptions-item v-if="activeTab === 'outbound'" label="仓库确认人">{{ detail.warehouse_confirmed_name || '待处理' }}</el-descriptions-item>
          <el-descriptions-item label="往来单位">{{ partnerName(detail) }}</el-descriptions-item>
          <el-descriptions-item label="总额">
            <span style="color: #e6a23c; font-weight: 600">¥{{ Number(detail.total_amount || 0).toFixed(2) }}</span>
          </el-descriptions-item>
          <el-descriptions-item label="状态">
            <el-tag :type="statusTag(detail.status)" size="small">{{ statusText(detail.status, detail) }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="备注" :span="2">{{ detail.remark || '-' }}</el-descriptions-item>
          <template v-if="activeTab === 'outbound'">
            <el-descriptions-item label="物流公司">{{ detail.logistics_company || '-' }}</el-descriptions-item>
            <el-descriptions-item label="物流单号">{{ detail.logistics_no || '-' }}</el-descriptions-item>
          </template>
          <el-descriptions-item label="附件图片" :span="2">
            <el-image v-if="detail.image_url" :src="detail.image_url" style="max-width: 200px" :preview-src-list="[detail.image_url]" fit="cover" />
            <span v-else>-</span>
          </el-descriptions-item>
        </el-descriptions>
        <BusinessHistory v-if="detail" :id="detail.id" :table="activeTab === 'inbound' || detail._kind === 'purchase' ? 'purchase_orders' : 'sales_orders'" />

        <el-divider content-position="left">商品明细</el-divider>
        <el-table :data="detailItems" v-loading="itemsLoading" border size="small">
          <el-table-column prop="product_name" label="商品" min-width="180" />
          <el-table-column prop="quantity" label="订单数量" width="100" /><el-table-column label="已收发" width="100"><template #default="{row}">{{row.received_qty ?? row.shipped_qty ?? 0}}</template></el-table-column><el-table-column prop="pending_qty" label="本批待放行" width="120" />
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
        <el-button v-if="false" type="success" @click="confirmFromDetail">{{ confirmBtnText() }}</el-button>
        <el-button type="danger" @click="rejectFromDetail">打回</el-button>
      </template>
    </el-dialog>

    <!-- 物流信息弹窗（仓管员确认销售出库） -->
    <el-dialog v-model="logisticsVisible" title="备货确认·填写物流信息" width="460px">
      <el-form :model="logisticsForm" label-width="90px">
        <el-form-item label="物流公司">
          <el-input v-model="logisticsForm.logistics_company" placeholder="如 顺丰/德邦" />
        </el-form-item>
        <el-form-item label="物流单号">
          <el-input v-model="logisticsForm.logistics_no" placeholder="物流单号" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="logisticsVisible = false">取消</el-button>
        <el-button type="primary" @click="confirmLogistics">确认出库</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import MyApprovalActions from '@/components/MyApprovalActions.vue'
import BatchDelivery from '@/components/BatchDelivery.vue'
import BusinessHistory from '@/components/BusinessHistory.vue'
import { ref, reactive, computed, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { getPurchaseOrders, getPurchaseOrderItems, warehouseInbound } from '@/api/purchase'
import { getSalesOrders, getSalesOrderItems, warehouseConfirm, warehouseOutbound } from '@/api/sales'
import { getSuppliers, getCustomers, getWarehouses } from '@/api/base'
import { getProducts } from '@/api/inventory'
import { useUserStore } from '@/stores/user'
import { getDepartmentDashboard } from '@/api/department'
import type { Product, Supplier, Customer, Warehouse } from '@/types'

const userStore = useUserStore()
// 领导视角：审批仓管员已确认的出库单；仓管员视角：确认出库填物流
const isLeaderView = computed(() => userStore.isLeader || userStore.isAdmin)

const loading = ref(false)
const activeTab = ref('inbound')
const purchaseList = ref<any[]>([])
const salesList = ref<any[]>([])

const detailVisible = ref(false)
const detail = ref<any | null>(null)
const detailItems = ref<any[]>([])
const itemsLoading = ref(false)

const products = ref<Product[]>([])
const suppliers = ref<Supplier[]>([])
const customers = ref<Customer[]>([])
const warehouses = ref<Warehouse[]>([])
const legacyWarehouses = ref<Record<string,number>>({})
function warehouseName(id: number) { return warehouses.value.find(w => w.id === Number(id))?.name || `仓库#${id}` }
function warehouseFor(row: any) { return Number(row.warehouse_id || legacyWarehouses.value[row.id + ':' + (row._kind || activeTab.value)] || 0) }

const currentList = computed(() => {
  return activeTab.value === 'inbound' ? purchaseList.value : salesList.value
})

const detailTitle = computed(() => {
  return activeTab.value === 'inbound' || detail.value?._kind === 'purchase' ? '采购单入库审核' : '销售单出库审核'
})

function statusText(status: number, row?: any) {
  const purchase = row?._kind === 'purchase' || activeTab.value === 'inbound'
  if (status === 0) return purchase ? '待采购审批' : '待销售审批'
  if (status === 1) return purchase ? '待入库' : '待出库'
  const map: Record<number, string> = {
    0: '待销售审批',
    1: '待出库',
    2: '已驳回',
    3: '已完成',
    4: '已打回待修改',
    5: '待领导批准',
  }
  return map[status] || `未知(${status})`
}

function statusTag(status: number) {
  const map: Record<number, string> = {
    0: 'info',
    1: 'warning',
    2: 'danger',
    3: 'success',
    4: 'danger',
    5: 'warning',
  }
  return (map[status] || 'info') as any
}

function confirmBtnText() {
  if (activeTab.value === 'inbound') return '确认入库'
  if (isLeaderView.value) return '批准出库'
  return '确认备货'
}

function partnerName(row: any) {
  if (activeTab.value === 'inbound' || row._kind === 'purchase') {
    const s = suppliers.value.find((x) => x.id === row.supplier_id)
    return s ? s.name : `供应商#${row.supplier_id ?? '-'}`
  }
  const c = customers.value.find((x) => x.id === row.customer_id)
  return c ? c.name : `客户#${row.customer_id ?? '-'}`
}

async function loadOptions() {
  try {
    const [p, s, c, w] = await Promise.all([getProducts({}), getSuppliers({}), getCustomers({}), getWarehouses({})])
    warehouses.value = Array.isArray(w.data) ? w.data.filter(x => x.status === 1) : []
    products.value = Array.isArray(p.data) ? p.data : []
    suppliers.value = Array.isArray(s.data) ? s.data : []
    customers.value = Array.isArray(c.data) ? c.data : []
  } catch (e) {
    // 已提示
  }
}

async function loadData() {
  loading.value = true
  try {
    const [poRes, soRes] = await Promise.all([getPurchaseOrders({}), getSalesOrders({})])
    // 采购入库：status=1（采购领导已批准，待仓库入库）
    const ownWarehouse = isLeaderView.value ? 0 : Number((await getDepartmentDashboard()).data.warehouse_id || 0)
    const visible = (x: any) => isLeaderView.value || !x.warehouse_id || Number(x.warehouse_id) === ownWarehouse
    purchaseList.value = (Array.isArray(poRes.data) ? poRes.data : []).filter((x: any) => x.status === 1 && visible(x))
    // 销售出库：仓管员看 status=1（待确认），领导看 status=5（待批准）
    salesList.value = (Array.isArray(soRes.data) ? soRes.data : []).filter(
      (x: any) => x.status === (isLeaderView.value ? 5 : 1) && visible(x)
    )
  } catch (e) {
    // 已提示
  } finally {
    loading.value = false
  }
}

async function openDetail(row: any) {
  detail.value = row
  detailVisible.value = true
  itemsLoading.value = true
  detailItems.value = []
  try {
    const res = activeTab.value === 'inbound' || row._kind === 'purchase'
      ? await getPurchaseOrderItems(row.id)
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

async function handleConfirm(row: any) {
  await doConfirm(row)
}

async function confirmFromDetail() {
  if (!detail.value) return
  await doConfirm(detail.value)
  detailVisible.value = false
}

async function doConfirm(row: any) {
  const warehouse_id = warehouseFor(row)
  if (!warehouse_id) { ElMessage.warning('请先选择该历史单据的实际处理仓库'); return }
  try {
    if (activeTab.value === 'inbound') {
      await warehouseInbound({ order_id: row.id, approve: true, warehouse_id })
      ElMessage.success('已确认入库')
      loadData()
    } else if (isLeaderView.value) {
      // 仓管领导：批准出库（扣库存+生成应收账）
      await warehouseOutbound({ order_id: row.id, approve: true, warehouse_id })
      ElMessage.success('已批准出库，库存已扣减')
      loadData()
    } else {
      // 仓管员：确认出库，填写物流信息
      openLogisticsDialog(row)
    }
  } catch (e) {
    // 已提示
  }
}

// 物流信息弹窗（仓管员确认销售出库）
const logisticsVisible = ref(false)
const logisticsRow = ref<any | null>(null)
const logisticsForm = reactive({
  logistics_company: '',
  logistics_no: '',
})

function openLogisticsDialog(row: any) {
  logisticsRow.value = row
  logisticsForm.logistics_company = ''
  logisticsForm.logistics_no = ''
  logisticsVisible.value = true
}

async function confirmLogistics() {
  if (!logisticsRow.value) return
  try {
    await warehouseConfirm({
      order_id: logisticsRow.value.id,
      warehouse_id: warehouseFor(logisticsRow.value),
      approve: true,
      logistics_company: logisticsForm.logistics_company,
      logistics_no: logisticsForm.logistics_no,
    })
    ElMessage.success('已确认备货，待仓库领导批准')
    logisticsVisible.value = false
    loadData()
  } catch (e) {
  }
}

function openReject(row: any) {
  ElMessageBox.prompt('请输入打回原因', '打回单据', {
    confirmButtonText: '确定打回',
    cancelButtonText: '取消',
    inputType: 'textarea',
    inputPlaceholder: '打回原因',
  })
    .then(({ value }) => doReject(row, value || ''))
    .catch(() => {})
}

async function rejectFromDetail() {
  if (!detail.value) return
  const { value } = await ElMessageBox.prompt('请输入打回原因', '打回单据', {
    confirmButtonText: '确定打回',
    cancelButtonText: '取消',
    inputType: 'textarea',
  }).catch(() => ({ value: null as string | null }))
  if (value === null) return
  await doReject(detail.value, value || '')
  detailVisible.value = false
}

async function doReject(row: any, remark: string) {
  const warehouse_id = warehouseFor(row)
  if (!warehouse_id) { ElMessage.warning('请先选择该历史单据的实际处理仓库'); return }
  try {
    if (activeTab.value === 'inbound') {
      await warehouseInbound({ order_id: row.id, approve: false, remark, warehouse_id })
    } else if (isLeaderView.value) {
      await warehouseOutbound({ order_id: row.id, approve: false, remark, warehouse_id })
    } else {
      await warehouseConfirm({ order_id: row.id, approve: false, remark, warehouse_id })
    }
    ElMessage.success('已打回')
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
.history-filters { display:flex; gap:12px; align-items:center; flex-wrap:wrap; margin:16px 0; }
.history-filters span { color:#909399; font-size:12px; }
</style>
