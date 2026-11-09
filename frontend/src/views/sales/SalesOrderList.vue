<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-button type="primary" @click="loadData">
          <el-icon><Refresh /></el-icon>
          刷新
        </el-button>
        <el-button v-if="canCreate" type="success" @click="openCreate">
          <el-icon><Plus /></el-icon>
          新建销售单
        </el-button>
        <el-button v-if="userStore.isAdmin" type="warning" plain @click="handleExport">
          <el-icon><Download /></el-icon>
          导出销售单
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="assigned_approver_name" label="指定审批人" min-width="130" />
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column prop="order_no" label="销售单号" min-width="180" />
        <el-table-column prop="created_by_name" label="发起人" min-width="110" />
        <el-table-column prop="approver_name" label="业务审批人" min-width="110" />
        <el-table-column prop="warehouse_confirmed_name" label="仓库确认人" min-width="110" />
        <el-table-column prop="warehouse_approver_name" label="仓库批准人" min-width="110" />
        <el-table-column label="客户" min-width="140">
          <template #default="{ row }">{{ customerName(row.customer_id) }}</template>
        </el-table-column>
        <el-table-column label="总额(¥)" width="130">
          <template #default="{ row }">
            {{ row.total_amount != null ? Number(row.total_amount).toFixed(2) : '-' }}
          </template>
        </el-table-column>
        <el-table-column label="状态" width="110">
          <template #default="{ row }">
            <el-tag :type="statusType(row.status)">{{ statusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="remark" label="备注" min-width="140" />
        <el-table-column prop="created_at" label="创建时间" width="170" />
        <el-table-column label="操作" width="210" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openDetail(row)">详情</el-button><OrderLifecycle :id="row.id" type="sales" :state="row.status ?? 0" :creator="Number(row.created_by)" @changed="loadData" />
            <el-button v-if="canResubmit(row)" size="small" type="warning" @click="openEdit(row)">修改重提</el-button>
            <el-tooltip v-else-if="row.status === 2 || row.status === 4" content="已超过一周，不可重新提交">
              <el-button size="small" type="warning" disabled>修改重提</el-button>
            </el-tooltip>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 销售单详情弹窗 -->
    <el-dialog v-model="detailVisible" title="销售单详情" width="720px">
      <template v-if="detail">
        <el-descriptions :column="2" border size="small" style="margin-bottom: 16px">
          <el-descriptions-item label="单号">{{ detail.order_no }}</el-descriptions-item>
          <el-descriptions-item label="客户">{{ customerName(detail.customer_id) }}</el-descriptions-item>
          <el-descriptions-item label="总额">
            <span style="color: #e6a23c; font-weight: 600">
              ¥{{ Number(detail.total_amount || 0).toFixed(2) }}
            </span>
          </el-descriptions-item>
          <el-descriptions-item label="状态">
            <el-tag :type="statusType(detail.status)" size="small">{{ statusText(detail.status) }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="备注" :span="2">{{ detail.remark || '-' }}</el-descriptions-item>
          <el-descriptions-item label="物流公司">{{ detail.logistics_company || '-' }}</el-descriptions-item>
          <el-descriptions-item label="物流单号">{{ detail.logistics_no || '-' }}</el-descriptions-item>
          <el-descriptions-item label="创建时间" :span="2">{{ detail.created_at || '-' }}</el-descriptions-item>
        </el-descriptions>
        <BusinessHistory v-if="detail" :id="detail.id" table="sales_orders" />

        <el-divider content-position="left">商品明细</el-divider>
        <el-table :data="detailItems" v-loading="itemsLoading" border size="small">
          <el-table-column prop="product_name" label="商品" min-width="180" />
          <el-table-column prop="quantity" label="订单数量" width="100" /><el-table-column prop="shipped_qty" label="已发数量" width="100" /><el-table-column prop="pending_qty" label="本批待放行" width="120" />
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

    <!-- 新建销售单弹窗 -->
    <el-dialog v-model="dialogVisible" :title="editingId ? '修改销售明细并重新审批' : '新建销售单（审批后出库）'" width="760px">
      <el-alert v-if="editingId" title="重新提交仅修改商品明细，客户、仓库和备注沿用原单据。" type="info" :closable="false" style="margin-bottom:16px" />
      <el-form :model="form" label-width="90px">
        <el-form-item label="客户" required>
          <el-select v-model="form.customer_id" :disabled="!!editingId" filterable placeholder="选择客户" style="width: 100%">
            <el-option v-for="c in customers" :key="c.id" :label="c.name" :value="c.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="出库仓库" required>
          <el-select v-model="form.warehouse_id" :disabled="!!editingId" filterable placeholder="选择出库仓库" style="width: 100%">
            <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="备注">
          <el-input v-model="form.remark" :disabled="!!editingId" placeholder="备注（可选）" />
        </el-form-item>
      <ApproverSelect v-if="!editingId" v-model="form.assigned_approver_id" />
      </el-form>

      <el-divider content-position="left">商品明细</el-divider>
      <el-table :data="form.items" border size="small">
        <el-table-column label="商品" min-width="200">
          <template #default="{ row }">
            <el-select v-model="row.product_id" filterable placeholder="选择商品" style="width: 100%">
              <el-option v-for="p in products" :key="p.id" :label="`${p.name} (${p.sku})`" :value="p.id" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column label="数量" width="140">
          <template #default="{ row }">
            <el-input-number v-model="row.quantity" :min="1" style="width: 100%" />
          </template>
        </el-table-column>
        <el-table-column label="单价(¥)" width="150">
          <template #default="{ row }">
            <el-input-number v-model="row.price" :min="0" :precision="2" style="width: 100%" />
          </template>
        </el-table-column>
        <el-table-column label="小计(¥)" width="120">
          <template #default="{ row }">
            {{ (row.quantity * row.price).toFixed(2) }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="80">
          <template #default="{ $index }">
            <el-button size="small" type="danger" @click="removeItem($index)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
      <div class="add-row">
        <el-button size="small" type="primary" plain @click="addItem">
          <el-icon><Plus /></el-icon>
          添加明细
        </el-button>
        <div class="total">合计：¥ {{ totalAmount.toFixed(2) }}</div>
      </div>

      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="handleSubmit">提交销售单</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { newRequestId } from '@/utils/requestId'
import OrderLifecycle from '@/components/OrderLifecycle.vue'
import BusinessHistory from '@/components/BusinessHistory.vue'
import ApproverSelect from '@/components/ApproverSelect.vue'
import { ref, reactive, computed, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getSalesOrders, getSalesOrderItems, createSalesOrder, resubmitSalesOrder } from '@/api/sales'
import { getCustomers, getWarehouses } from '@/api/base'
import { getProducts, getInventory } from '@/api/inventory'
import { exportCsv } from '@/api/importexport'
import { useUserStore } from '@/stores/user'
import type { SalesOrder, Customer, Warehouse, Product, Inventory } from '@/types'

const loading = ref(false)
const saving = ref(false)
const list = ref<SalesOrder[]>([])
const userStore = useUserStore()
// 新建销售单：销售员工发起、admin 可代建；领导只审批不新建
const canCreate = computed(() => userStore.isAdmin || (!userStore.isLeader && userStore.department === '销售部'))
const customers = ref<Customer[]>([])
const warehouses = ref<Warehouse[]>([])
const products = ref<Product[]>([])
const inventory = ref<Inventory[]>([])
const dialogVisible = ref(false)
const editingId = ref<number | null>(null)

const detailVisible = ref(false)
const detail = ref<SalesOrder | null>(null)
const detailItems = ref<any[]>([])
const itemsLoading = ref(false)

const form = reactive({
  request_id: newRequestId(),
  assigned_approver_id: undefined as number | undefined,
  customer_id: undefined as number | undefined,
  warehouse_id: undefined as number | undefined,
  remark: '',
  items: [] as { product_id?: number; quantity: number; price: number }[],
})

const totalAmount = computed(() =>
  form.items.reduce((sum, it) => sum + (it.quantity * it.price), 0),
)

function customerName(id?: number) {
  const c = customers.value.find((x) => x.id === id)
  return c ? c.name : `客户#${id ?? '-'}`
}

function statusText(s?: number) {
  return { 8: '剩余已关闭', 6: '已撤回', 7: '已作废', 0: '待销售审批', 1: '待仓库确认', 2: '已驳回', 3: '已完成', 4: '待修改', 5: '待仓库领导批准' }[s ?? 0] || '未知'
}
function statusType(s?: number) {
  return { 0: 'warning', 1: 'warning', 2: 'danger', 3: 'success', 4: 'danger', 5: 'warning' }[s ?? 0] || 'info'
}

// 被驳回(2)/打回(4)后一周内可修改并重新提交，超期不可
function canResubmit(row: { status?: number; updated_at?: string }) {
  if (row.status !== 2 && row.status !== 4 && row.status !== 6) return false
  if (!row.updated_at) return true
  const t = new Date(row.updated_at.replace(' ', 'T')).getTime()
  if (Number.isNaN(t)) return true
  return (Date.now() - t) / 86400000 <= 7
}

async function handleExport() {
  try {
    await exportCsv('sales_orders')
    ElMessage.success('导出成功')
  } catch (e) {
    ElMessage.error('导出失败')
  }
}

async function openDetail(row: SalesOrder) {
  detail.value = row
  detailVisible.value = true
  itemsLoading.value = true
  detailItems.value = []
  try {
    const res = await getSalesOrderItems(row.id)
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

async function loadData() {
  loading.value = true
  try {
    const res = await getSalesOrders({})
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    loading.value = false
  }
}

async function loadOptions() {
  try {
    const [c, w, p, inv] = await Promise.all([
      getCustomers({}), getWarehouses({}), getProducts({}), getInventory({}),
    ])
    customers.value = Array.isArray(c.data) ? c.data : []
    warehouses.value = Array.isArray(w.data) ? w.data : []
    products.value = Array.isArray(p.data) ? p.data : []
    inventory.value = Array.isArray(inv.data) ? inv.data : []
  } catch (e) {
  }
}

function stockOf(productId: number, warehouseId: number | undefined) {
  if (!warehouseId) return 0
  const rec = inventory.value.find(
    (i) => i.product_id === productId && i.warehouse_id === warehouseId,
  )
  return rec ? rec.quantity : 0
}

function openCreate() {
  form.request_id = newRequestId()
  editingId.value = null
  form.customer_id = undefined
  form.warehouse_id = undefined
  form.remark = ''
  form.items = [{ product_id: undefined, quantity: 1, price: 0 }]
  dialogVisible.value = true
}

async function openEdit(row: SalesOrder) {
  editingId.value = row.id
  form.customer_id = row.customer_id
  form.warehouse_id = Number(row.warehouse_id) || undefined
  form.remark = row.remark || ''
  form.items = []
  try {
    const res = await getSalesOrderItems(row.id)
    const items = Array.isArray(res.data) ? res.data : []
    form.items = items.map((it: any) => ({
      product_id: it.product_id,
      quantity: it.quantity,
      price: Number(it.price || 0),
    }))
    if (form.items.length === 0) {
      form.items = [{ product_id: undefined, quantity: 1, price: 0 }]
    }
  } catch (e) {
    form.items = [{ product_id: undefined, quantity: 1, price: 0 }]
  }
  dialogVisible.value = true
}

function addItem() {
  form.items.push({ product_id: undefined, quantity: 1, price: 0 })
}

function removeItem(index: number) {
  if (form.items.length <= 1) {
    ElMessage.warning('至少保留一条明细')
    return
  }
  form.items.splice(index, 1)
}

async function handleSubmit() {
  const items = form.items.filter((it) => it.product_id)
  if (items.length === 0) {
    ElMessage.warning('请至少添加一条商品明细')
    return
  }
  saving.value = true
  try {
    if (editingId.value) {
      await resubmitSalesOrder({
        order_id: editingId.value,
        items: items as { product_id: number; quantity: number; price: number }[],
      })
      ElMessage.success('已重新提交，等待审批')
    } else {
      if (!form.customer_id || !form.warehouse_id) {
        ElMessage.warning('请选择客户和出库仓库')
        saving.value = false
        return
      }
      // 前端预校验库存
      for (const it of items) {
        const stock = stockOf(it.product_id!, form.warehouse_id)
        if (it.quantity > stock) {
          ElMessage.error(`商品库存不足（当前 ${stock}，需 ${it.quantity}）`)
          saving.value = false
          return
        }
      }
      await createSalesOrder({
        customer_id: form.customer_id,
        warehouse_id: form.warehouse_id,
        request_id: form.request_id,
      remark: form.remark,
      assigned_approver_id: form.assigned_approver_id,
        items: items as { product_id: number; quantity: number; price: number }[],
      })
      ElMessage.success('销售单已提交，等待审批')
    }
    dialogVisible.value = false
    loadData()
  } catch (e) {
  } finally {
    saving.value = false
  }
}

onMounted(() => {
  loadData()
  loadOptions()
})
</script>

<style scoped>
.toolbar { display: flex; align-items: center; gap: 12px; }
.add-row {
  margin-top: 12px;
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.total {
  font-size: 15px;
  font-weight: 600;
  color: #e6a23c;
}
</style>
