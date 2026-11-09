<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-button type="primary" @click="loadData">
          <el-icon><Refresh /></el-icon>
          刷新
        </el-button>
        <el-button v-if="canCreate" type="danger" @click="openCreate">
          <el-icon><Back /></el-icon>
          发起{{ typeLabel }}{{ isPurchase ? '退货' : '售后' }}
        </el-button>
        <el-button v-if="userStore.isAdmin && !warehouseQueue" type="warning" plain @click="handleExport">
          <el-icon><Download /></el-icon>
          导出{{ isPurchase ? '退货' : '售后' }}表
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column v-if="warehouseQueue" prop="order_no" label="原单号" min-width="160" />
        <el-table-column prop="assigned_approver_name" label="指定审批人" min-width="130" />
        <el-table-column prop="approver_name" label="业务审批人" min-width="120" />
        <el-table-column prop="warehouse_confirmed_name" label="仓库确认人" min-width="120" />
        <el-table-column prop="return_no" label="单号" min-width="190" />
        <el-table-column v-if="!isPurchase" label="类型" width="80" align="center">
          <template #default="{ row }">
            <el-tag :type="row.return_type === 'exchange' ? 'warning' : 'info'" size="small">
              {{ row.return_type === 'exchange' ? '换货' : '退货' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column :label="partnerLabel" min-width="130">
          <template #default="{ row }">{{ partnerName(row) }}</template>
        </el-table-column>
        <el-table-column label="申请人" min-width="110">
          <template #default="{ row }">
            {{ row.created_by_name || (row.created_by ? `用户#${row.created_by}` : '-') }}
          </template>
        </el-table-column>
        <el-table-column :label="(isPurchase ? '退货' : '售后') + '金额(¥)'" width="130">
          <template #default="{ row }">
            {{ Number(row.total_amount || 0).toFixed(2) }}
          </template>
        </el-table-column>
        <el-table-column label="状态" width="110">
          <template #default="{ row }">
            <el-tag :type="statusType(row.status)">{{ statusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="remark" label="备注" min-width="110" />
        <el-table-column label="物流" min-width="150">
          <template #default="{ row }">
            <template v-if="row.logistics_company || row.logistics_no">
              {{ row.logistics_company }} {{ row.logistics_no }}
            </template>
            <span v-else style="color: #909399">-</span>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="创建时间" width="170" />
        <el-table-column label="操作" min-width="220" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openDetail(row)">详情</el-button>
            <el-button v-if="canApproveRow(row)" size="small" type="success" @click="approve(row, true)">批准</el-button>
            <el-button v-if="canApproveRow(row)" size="small" type="danger" @click="approve(row, false)">驳回</el-button>
            <el-button v-if="canWarehouseRow(row)" size="small" type="success" @click="warehouseConfirm(row)">
              {{ row.return_type === 'exchange' ? '确认换货' : '确认退货' }}
            </el-button>
            <el-button v-if="canWarehouseRow(row)" size="small" type="danger" @click="warehouseReject(row)">打回</el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-empty v-if="!loading && list.length === 0" description="暂无单据" />
    </el-card>

    <!-- 详情弹窗 -->
    <el-dialog v-model="detailVisible" :title="detailTitle" width="760px">
      <template v-if="detail">
        <el-descriptions :column="2" border size="small" style="margin-bottom: 16px">
          <el-descriptions-item label="单号">{{ detail.return_no }}</el-descriptions-item>
          <el-descriptions-item :label="partnerLabel">{{ partnerName(detail) }}</el-descriptions-item>
          <el-descriptions-item :label="(isPurchase ? '退货' : '售后') + '金额'">
            <span style="color: #f56c6c; font-weight: 600">¥{{ Number(detail.total_amount || 0).toFixed(2) }}</span>
          </el-descriptions-item>
          <el-descriptions-item label="状态">
            <el-tag :type="statusType(detail.status)" size="small">{{ statusText(detail.status) }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="备注" :span="2">{{ detail.remark || '-' }}</el-descriptions-item>
          <el-descriptions-item label="物流">
            {{ detail.logistics_company || '-' }}{{ detail.logistics_no ? ' ' + detail.logistics_no : '' }}
          </el-descriptions-item>
        </el-descriptions>

        <el-divider content-position="left">商品明细</el-divider>
        <el-table :data="detailItems" v-loading="itemsLoading" border size="small">
          <el-table-column v-if="!isPurchase" label="方向" width="90" align="center">
            <template #default="{ row }">
              <el-tag :type="row.direction === 'replace' ? 'warning' : 'success'" size="small">
                {{ row.direction === 'replace' ? '换出' : '退入' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="product_name" label="商品" min-width="180" />
          <el-table-column prop="quantity" label="数量" width="100" />
          <el-table-column label="单价(¥)" width="120">
            <template #default="{ row }">¥{{ Number(row.price || 0).toFixed(2) }}</template>
          </el-table-column>
          <el-table-column label="小计(¥)" width="120">
            <template #default="{ row }">¥{{ Number(row.amount || 0).toFixed(2) }}</template>
          </el-table-column>
        </el-table>
      </template>
      <template #footer>
        <el-button @click="detailVisible = false">关闭</el-button>
      </template>
    </el-dialog>

    <!-- 发起售后/退货弹窗 -->
    <el-dialog v-model="dialogVisible" :title="'发起' + typeLabel + (isPurchase ? '退货' : (form.returnType === 'exchange' ? '换货' : '退货'))" width="820px">
      <el-form :model="form" label-width="100px">
        <el-form-item v-if="!isPurchase" label="售后类型">
          <el-radio-group v-model="form.returnType">
            <el-radio-button value="return">退货</el-radio-button>
            <el-radio-button value="exchange">换货</el-radio-button>
          </el-radio-group>
        </el-form-item>
        <el-form-item :label="'原' + typeLabel + '单'" required>
          <el-select v-model="form.order_id" filterable placeholder="选择已完成的单据" style="width: 100%" @change="onOrderChange">
            <el-option v-for="o in completedOrders" :key="o.id" :label="`${o.order_no}（¥${Number(o.total_amount || 0).toFixed(2)}）`" :value="o.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="仓库" required>
          <el-select v-model="form.warehouse_id" filterable placeholder="选择仓库" style="width: 100%">
            <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="备注">
          <el-input v-model="form.remark" :placeholder="(isPurchase ? '退货' : '售后') + '原因（可选）'" />
        </el-form-item>
        <el-form-item :label="isPurchase ? '退货物流' : '退回物流'" required>
          <el-input v-model="form.logistics_company" placeholder="物流公司，如顺丰" style="width: 45%" />
          <el-input v-model="form.logistics_no" placeholder="物流单号" style="width: 45%; margin-left: 8px" />
        </el-form-item>
      <ApproverSelect v-model="form.assigned_approver_id" />
      </el-form>

      <el-divider content-position="left">退入商品明细（客户退回）</el-divider>
      <el-table :data="form.items" border size="small">
        <el-table-column label="商品" min-width="200">
          <template #default="{ row }">
            <el-select v-model="row.product_id" filterable placeholder="选择商品" style="width: 100%" @change="onItemProductChange(row)">
              <el-option v-for="p in products" :key="p.id" :label="`${p.name} (${p.sku})`" :value="p.id" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column label="数量" width="140">
          <template #default="{ row }">
            <el-input-number v-model="row.quantity" :min="1" style="width: 100%" />
          </template>
        </el-table-column>
        <el-table-column label="单价(¥)" width="140">
          <template #default="{ row }">
            <el-input-number v-model="row.price" :min="0" :precision="2" style="width: 100%" />
          </template>
        </el-table-column>
        <el-table-column label="小计(¥)" width="110">
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
        <div class="total">退入合计：¥ {{ returnTotal.toFixed(2) }}</div>
      </div>

      <template v-if="!isPurchase && form.returnType === 'exchange'">
        <el-divider content-position="left">换出商品明细（发给客户）</el-divider>
        <el-table :data="form.exchangeItems" border size="small">
          <el-table-column label="商品" min-width="200">
            <template #default="{ row }">
              <el-select v-model="row.product_id" filterable placeholder="选择商品" style="width: 100%" @change="onItemProductChange(row)">
                <el-option v-for="p in products" :key="p.id" :label="`${p.name} (${p.sku})`" :value="p.id" />
              </el-select>
            </template>
          </el-table-column>
          <el-table-column label="数量" width="140">
            <template #default="{ row }">
              <el-input-number v-model="row.quantity" :min="1" style="width: 100%" />
            </template>
          </el-table-column>
          <el-table-column label="单价(¥)" width="140">
            <template #default="{ row }">
              <el-input-number v-model="row.price" :min="0" :precision="2" style="width: 100%" />
            </template>
          </el-table-column>
          <el-table-column label="小计(¥)" width="110">
            <template #default="{ row }">
              {{ (row.quantity * row.price).toFixed(2) }}
            </template>
          </el-table-column>
          <el-table-column label="操作" width="80">
            <template #default="{ $index }">
              <el-button size="small" type="danger" @click="removeExchangeItem($index)">删除</el-button>
            </template>
          </el-table-column>
        </el-table>
        <div class="add-row">
          <el-button size="small" type="primary" plain @click="addExchangeItem">
            <el-icon><Plus /></el-icon>
            添加换出商品
          </el-button>
          <div class="total" style="color: #e6a23c">换出合计：¥ {{ exchangeTotal.toFixed(2) }}</div>
        </div>
      </template>

      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="danger" :loading="saving" @click="handleSubmit">
          提交{{ !isPurchase && form.returnType === 'exchange' ? '换货' : '退货' }}
        </el-button>
      </template>
    </el-dialog>

    <!-- 仓库确认弹窗（填写物流信息） -->
    <el-dialog v-model="whVisible" :title="whTitle" width="480px">
      <el-form :model="whForm" label-width="100px">
        <el-form-item label="物流公司" required>
          <el-input v-model="whForm.logistics_company" placeholder="如顺丰/圆通" />
        </el-form-item>
        <el-form-item label="物流单号" required>
          <el-input v-model="whForm.logistics_no" placeholder="运单号" />
        </el-form-item>
        <el-form-item label="备注">
          <el-input v-model="whForm.remark" type="textarea" placeholder="可选" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="whVisible = false">取消</el-button>
        <el-button type="success" :loading="saving" @click="whSubmit">确认完成</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { newRequestId } from '@/utils/requestId'
import ApproverSelect from '@/components/ApproverSelect.vue'
import { ref, reactive, computed, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  getPurchaseReturns, getSalesReturns, getPurchaseReturnItems, getSalesReturnItems,
  createPurchaseReturn, createSalesReturn, approvePurchaseReturn, approveSalesReturn,
  warehousePurchaseReturn, warehouseSalesReturn,
} from '@/api/return'
import { getPurchaseOrders, getPurchaseOrderItems } from '@/api/purchase'
import { getSalesOrders, getSalesOrderItems } from '@/api/sales'
import { getWarehouses } from '@/api/base'
import { getProducts } from '@/api/inventory'
import { exportCsv } from '@/api/importexport'
import { useUserStore } from '@/stores/user'
import type { Warehouse, Product } from '@/types'

const route = useRoute()
const userStore = useUserStore()

// returnType: 'purchase' 采购退货 / 'sales' 销售退货
const returnType = computed(() => String(route.meta.returnType || 'purchase'))
const isPurchase = computed(() => returnType.value === 'purchase')
const typeLabel = computed(() => (isPurchase.value ? '采购' : '销售'))
const partnerLabel = computed(() => (isPurchase.value ? '供应商' : '客户'))

// 部门领导负责业务审批，仓库人员负责实物确认。
const warehouseQueue = computed(() => route.meta.warehouseQueue === true)
const isWarehouse = computed(() => userStore.department === '仓库部')
// 采购退货→采购部领导审批；销售退换货→销售部领导审批；admin 均可
const canApprove = computed(() =>
  userStore.isAdmin ||
  (userStore.isLeader &&
    (isPurchase.value ? userStore.department === '采购部'
                      : userStore.department === '销售部')),
)
function canApproveRow(row: any) {
  const uid = Number(userStore.userInfo?.id)
  return !warehouseQueue.value && canApprove.value && row.status === 0 &&
    Number(row.created_by) !== uid &&
    (userStore.isAdmin || !row.assigned_approver_id || Number(row.assigned_approver_id) === uid)
}
function canWarehouseRow(row: any) {
  return row.status === 1 && (userStore.isAdmin || (isWarehouse.value &&
    (userStore.isLeader || Number(row.warehouse_id) === Number(userStore.userInfo?.warehouse_id))))
}
// 发起售后/退货：本部门员工发起（领导只审批不发起），admin 可代发起
const canCreate = computed(() => !warehouseQueue.value && (
  userStore.isAdmin ||
  (!userStore.isLeader &&
    (isPurchase.value ? userStore.department === '采购部'
                      : userStore.department === '销售部'))),
)

async function handleExport() {
  try {
    await exportCsv(isPurchase.value ? 'purchase_returns' : 'sales_returns')
    ElMessage.success('导出成功')
  } catch (e) {
    ElMessage.error('导出失败')
  }
}

const loading = ref(false)
const saving = ref(false)
const list = ref<any[]>([])
const warehouses = ref<Warehouse[]>([])
const products = ref<Product[]>([])
const completedOrders = ref<any[]>([])

const dialogVisible = ref(false)
const detailVisible = ref(false)
const detail = ref<any | null>(null)
const detailItems = ref<any[]>([])
const itemsLoading = ref(false)

const form = reactive({
  request_id: newRequestId(),
  assigned_approver_id: undefined as number | undefined,
  order_id: undefined as number | undefined,
  warehouse_id: undefined as number | undefined,
  remark: '',
  logistics_company: '',
  logistics_no: '',
  returnType: 'return',
  items: [] as { product_id?: number; quantity: number; price: number }[],
  exchangeItems: [] as { product_id?: number; quantity: number; price: number }[],
})

// 仓库确认弹窗（填物流）
const whVisible = ref(false)
const whRow = ref<any | null>(null)
const whForm = reactive({ logistics_company: '', logistics_no: '', remark: '' })
const whTitle = computed(() => {
  if (!whRow.value) return ''
  const isExchange = whRow.value.return_type === 'exchange'
  return isPurchase.value ? '确认采购退货（填写物流）' : (isExchange ? '确认换货（填写换出物流）' : '确认退货（填写物流）')
})

const returnTotal = computed(() =>
  form.items.reduce((sum, it) => sum + (it.quantity * it.price), 0),
)
const exchangeTotal = computed(() =>
  form.exchangeItems.reduce((sum, it) => sum + (it.quantity * it.price), 0),
)

const detailTitle = computed(() => {
  if (isPurchase.value) return '采购退货详情'
  return detail.value?.return_type === 'exchange' ? '销售换货详情' : '销售退货详情'
})

function statusText(s?: number) {
  return { 0: '待审批', 1: '待仓库审核', 2: '已驳回', 3: '已完成', 4: '已打回' }[s ?? 0] || '未知'
}
function statusType(s?: number) {
  return { 0: 'warning', 1: 'warning', 2: 'danger', 3: 'success', 4: 'danger' }[s ?? 0] || 'info'
}

function partnerName(row: any) {
  return isPurchase.value ? (row.supplier_name || `供应商#${row.supplier_id}`) : (row.customer_name || `客户#${row.customer_id}`)
}

let listRequest = 0
async function loadData() {
  const request = ++listRequest
  loading.value = true
  try {
    const params = warehouseQueue.value ? { warehouse_queue: 1 } : undefined
    const res = isPurchase.value ? await getPurchaseReturns(params) : await getSalesReturns(params)
    if (request === listRequest) list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    if (request === listRequest) loading.value = false
  }
}

async function loadOptions() {
  try {
    const [w, p] = await Promise.all([getWarehouses({}), getProducts({})])
    warehouses.value = Array.isArray(w.data) ? w.data : []
    products.value = Array.isArray(p.data) ? p.data : []
  } catch (e) {
  }
}

// 加载已完成(status=3)的订单，供发起退货选择
async function loadCompletedOrders() {
  try {
    const res = isPurchase.value ? await getPurchaseOrders({}) : await getSalesOrders({})
    completedOrders.value = (Array.isArray(res.data) ? res.data : []).filter((x: any) => (x.status === 3 || x.status === 8))
  } catch (e) {
  }
}

async function openDetail(row: any) {
  detail.value = row
  detailVisible.value = true
  itemsLoading.value = true
  detailItems.value = []
  try {
    const res = isPurchase.value
      ? await getPurchaseReturnItems(row.id)
      : await getSalesReturnItems(row.id)
    detailItems.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    itemsLoading.value = false
  }
}

function openCreate() {
  form.request_id = newRequestId()
  form.order_id = undefined
  form.warehouse_id = undefined
  form.remark = ''
  form.logistics_company = ''
  form.logistics_no = ''
  form.returnType = 'return'
  form.items = [{ product_id: undefined, quantity: 1, price: 0 }]
  form.exchangeItems = [{ product_id: undefined, quantity: 1, price: 0 }]
  dialogVisible.value = true
  loadCompletedOrders()
}

// 选择原单后，自动填充退入商品明细
async function onOrderChange(orderId: number) {
  try {
    const res = isPurchase.value
      ? await getPurchaseOrderItems(orderId)
      : await getSalesOrderItems(orderId)
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
  }
}

function onItemProductChange(row: any) {
  const p = products.value.find((x) => x.id === row.product_id)
  if (p && p.price != null) row.price = Number(p.price)
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

function addExchangeItem() {
  form.exchangeItems.push({ product_id: undefined, quantity: 1, price: 0 })
}

function removeExchangeItem(index: number) {
  if (form.exchangeItems.length <= 1) {
    ElMessage.warning('至少保留一条换出商品')
    return
  }
  form.exchangeItems.splice(index, 1)
}

async function handleSubmit() {
  if (!form.order_id || !form.warehouse_id) {
    ElMessage.warning('请选择原单据和仓库')
    return
  }
  if (!form.logistics_company.trim() || !form.logistics_no.trim()) {
    ElMessage.warning('请填写物流公司和物流单号')
    return
  }
  const isExchange = !isPurchase.value && form.returnType === 'exchange'
  const items = form.items.filter((it) => it.product_id)
  if (items.length === 0) {
    ElMessage.warning('请至少添加一条退入商品明细')
    return
  }
  let payload: { product_id: number; quantity: number; price: number; direction: string }[] =
    items.map((it) => ({ product_id: it.product_id!, quantity: it.quantity, price: it.price, direction: 'return' }))
  if (isExchange) {
    const exItems = form.exchangeItems.filter((it) => it.product_id)
    if (exItems.length === 0) {
      ElMessage.warning('换货必须同时添加换出商品明细')
      return
    }
    payload = [
      ...payload,
      ...exItems.map((it) => ({ product_id: it.product_id!, quantity: it.quantity, price: it.price, direction: 'replace' })),
    ]
  }
  saving.value = true
  try {
    const data = {
      order_id: form.order_id,
      warehouse_id: form.warehouse_id,
      request_id: form.request_id,
      remark: form.remark,
      assigned_approver_id: form.assigned_approver_id,
      logistics_company: form.logistics_company.trim(),
      logistics_no: form.logistics_no.trim(),
      items: payload,
    }
    if (isPurchase.value) {
      await createPurchaseReturn(data)
    } else {
      await createSalesReturn({ ...data, return_type: form.returnType })
    }
    ElMessage.success(isExchange ? '换货单已提交，等待审批' : '退货单已提交，等待审批')
    dialogVisible.value = false
    loadData()
  } catch (e) {
  } finally {
    saving.value = false
  }
}

async function approve(row: any, approveFlag: boolean) {
  try {
    const data = { return_id: row.id, approve: approveFlag }
    if (isPurchase.value) {
      await approvePurchaseReturn(data)
    } else {
      await approveSalesReturn(data)
    }
    ElMessage.success(approveFlag ? '已批准' : '已驳回')
    loadData()
  } catch (e) {
  }
}

function warehouseConfirm(row: any) {
  whRow.value = row
  whForm.logistics_company = ''
  whForm.logistics_no = ''
  whForm.remark = ''
  whVisible.value = true
}

async function whSubmit() {
  if (!whRow.value) return
  if (!whForm.logistics_company.trim() || !whForm.logistics_no.trim()) {
    ElMessage.warning('请填写物流公司和物流单号')
    return
  }
  saving.value = true
  try {
    const data = {
      return_id: whRow.value.id,
      approve: true,
      remark: whForm.remark,
      logistics_company: whForm.logistics_company.trim(),
      logistics_no: whForm.logistics_no.trim(),
    }
    if (isPurchase.value) {
      await warehousePurchaseReturn(data)
    } else {
      await warehouseSalesReturn(data)
    }
    ElMessage.success('确认完成')
    whVisible.value = false
    loadData()
  } catch (e) {
  } finally {
    saving.value = false
  }
}

function warehouseReject(row: any) {
  ElMessageBox.prompt('请输入打回原因', '打回退货单', {
    confirmButtonText: '确定打回',
    cancelButtonText: '取消',
    inputType: 'textarea',
  }).then(async ({ value }) => {
    try {
      const data = { return_id: row.id, approve: false, remark: value || '' }
      if (isPurchase.value) {
        await warehousePurchaseReturn(data)
      } else {
        await warehouseSalesReturn(data)
      }
      ElMessage.success('已打回')
      loadData()
    } catch (e) {
    }
  }).catch(() => {})
}

watch([returnType, warehouseQueue], () => {
  detailVisible.value = false
  dialogVisible.value = false
  whVisible.value = false
  list.value = []
  loadData()
})
onMounted(() => {
  loadData()
  loadOptions()
})
</script>

<style scoped>
.toolbar { display: flex; gap: 12px; }
.add-row {
  margin-top: 12px;
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.total {
  font-size: 15px;
  font-weight: 600;
  color: #f56c6c;
}
</style>
