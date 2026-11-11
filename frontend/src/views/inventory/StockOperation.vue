<template>
  <div>
    <!-- 发起操作单 -->
    <el-card shadow="never" style="margin-bottom: 16px">
      <template #header>
        <span style="font-weight: 600">发起库存调整</span>
      </template>
      <el-tabs v-model="opType">
        <el-tab-pane label="入库" name="in" />
        <el-tab-pane label="出库" name="out" />
        <el-tab-pane label="调拨" name="transfer" />
      </el-tabs>

      <el-form :model="form" label-width="100px" style="max-width: 640px">
        <el-form-item label="商品" required>
          <el-select v-model="form.product_id" filterable placeholder="选择商品" style="width: 100%">
            <el-option v-for="p in products" :key="p.id" :label="`${p.name}（${p.sku}）`" :value="p.id" />
          </el-select>
        </el-form-item>

        <!-- 序列号（商品启用序列号管理时必填） -->
        <el-form-item v-if="selectedProduct?.has_serial" label="序列号" required>
          <el-input
            v-model="form.serial_nos"
            type="textarea"
            :rows="2"
            placeholder="多个序列号用英文逗号分隔，如 SN001,SN002,SN003"
          />
          <div style="color: #909399; font-size: 12px; line-height: 1.6; margin-top: 4px; width: 100%">
            该商品已启用序列号管理，数量 {{ form.qty }} 需与序列号个数一致。
          </div>
        </el-form-item>

        <!-- 入库：目标仓库 -->
        <el-form-item v-if="opType === 'in'" label="入库仓库" required>
          <el-select v-model="form.to_warehouse_id" filterable placeholder="选择入库仓库" style="width: 100%">
            <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
          </el-select>
        </el-form-item>

        <!-- 出库：来源仓库 -->
        <el-form-item v-if="opType === 'out'" label="出库仓库" required>
          <el-select v-model="form.from_warehouse_id" filterable placeholder="选择出库仓库" style="width: 100%">
            <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
          </el-select>
        </el-form-item>

        <!-- 调拨：来源 + 目标 -->
        <template v-if="opType === 'transfer'">
          <el-form-item label="调出仓库" required>
            <el-select v-model="form.from_warehouse_id" filterable placeholder="选择调出仓库" style="width: 100%">
              <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
            </el-select>
          </el-form-item>
          <el-form-item label="调入仓库" required>
            <el-select v-model="form.to_warehouse_id" filterable placeholder="选择调入仓库" style="width: 100%">
              <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
            </el-select>
          </el-form-item>
        </template>

        <el-form-item label="数量" required>
          <el-input-number v-model="form.qty" :min="1" style="width: 100%" />
        </el-form-item>

        <template v-if="opType==='in' && selectedProduct?.batch_enabled"><el-form-item label="批次号"><el-input v-model="form.lot_no" /></el-form-item><el-form-item label="生产日期"><el-date-picker v-model="form.manufactured_on" type="date" value-format="YYYY-MM-DD" /></el-form-item><el-form-item label="有效期"><el-date-picker v-model="form.expires_on" type="date" value-format="YYYY-MM-DD" /></el-form-item></template><el-form-item v-if="opType==='in'" label="单位成本"><el-input v-model="form.unit_cost" placeholder="可核实的成本，最多6位小数；缺失则标记成本未知" /></el-form-item><el-form-item label="业务类型">
          <el-select v-model="form.biz_type" style="width: 100%">
            <el-option v-if="opType === 'in'" label="采购入库" value="purchase_in" />
            <el-option v-if="opType === 'in'" label="退货入库" value="return_in" />
            <el-option v-if="opType === 'in'" label="其他入库" value="other_in" />
            <el-option v-if="opType === 'out'" label="销售出库" value="sale_out" />
            <el-option v-if="opType === 'out'" label="采购退货" value="purchase_return" />
            <el-option v-if="opType === 'out'" label="报损（含过期商品）" value="scrap" /><el-option v-if="opType === 'out'" label="其他出库" value="other_out" />
            <el-option v-if="opType === 'transfer'" label="仓库调拨" value="transfer" />
          </el-select>
        </el-form-item>

        <!-- 物流信息（出库时） -->
        <template v-if="opType === 'out'">
          <el-form-item label="物流公司">
            <el-input v-model="form.logistics_company" placeholder="如 顺丰/德邦（可选）" />
          </el-form-item>
          <el-form-item label="物流单号">
            <el-input v-model="form.logistics_no" placeholder="物流单号（可选）" />
          </el-form-item>
        </template>

        <el-form-item label="备注">
          <el-input v-model="form.remark" placeholder="备注（可选）" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">提交申请</el-button>
          <el-button @click="resetForm">重置</el-button>
        </el-form-item>
      <ApproverSelect v-model="form.assigned_approver_id" />
      </el-form>
    </el-card>

    <!-- 操作单列表 -->
    <el-card shadow="never">
      <template #header>
        <div style="display: flex; align-items: center; justify-content: space-between">
          <span style="font-weight: 600">库存调整单</span>
          <el-button size="small" type="primary" @click="loadList">刷新</el-button>
        </div>
      </template>

      <el-tabs v-model="listTab" @tab-change="loadList">
        <el-tab-pane label="待审批" name="0" />
        <el-tab-pane label="已批准" name="1" />
        <el-tab-pane label="已驳回" name="2" />
        <el-tab-pane label="调拨在途" name="3" />
        <el-tab-pane label="全部" name="all" />
      </el-tabs>
      <div class="record-filters">
        <el-checkbox v-if="canApprove" v-model="onlyMine">我审批的</el-checkbox>
        <el-input v-model="searchNo" clearable placeholder="搜索出入库单号" style="width:210px" />
        <el-date-picker v-model="dateRange" type="daterange" value-format="YYYY-MM-DD" start-placeholder="开始日期" end-placeholder="结束日期" />
        <span>按提交时间筛选</span>
      </div>

      <el-table :data="filteredList" v-loading="loading" stripe border>
        <el-table-column prop="assigned_approver_name" label="指定审批人" min-width="130" />
        <el-table-column prop="operation_no" label="单号" width="190" />
        <el-table-column label="类型" width="80">
          <template #default="{ row }">
            <el-tag :type="typeTag(row.type)" size="small">{{ typeText(row.type) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="product_name" label="商品" min-width="150" />
        <el-table-column label="数量" width="80">
          <template #default="{ row }">{{ row.qty }}</template>
        </el-table-column>
        <el-table-column label="仓库" min-width="160">
          <template #default="{ row }">
            <template v-if="row.type === 'transfer'">
              {{ row.from_warehouse_name }} → {{ row.to_warehouse_name }}
            </template>
            <template v-else>
              {{ row.from_warehouse_name || row.to_warehouse_name || '-' }}
            </template>
          </template>
        </el-table-column>
        <el-table-column label="物流" min-width="140">
          <template #default="{ row }">
            <span v-if="row.logistics_company || row.logistics_no">
              {{ row.logistics_company }} {{ row.logistics_no }}
            </span>
            <span v-else>-</span>
          </template>
        </el-table-column>
        <el-table-column label="序列号" min-width="150">
          <template #default="{ row }">
            <span v-if="row.serial_nos">{{ row.serial_nos }}</span>
            <span v-else>-</span>
          </template>
        </el-table-column>
        <el-table-column label="状态" width="90">
          <template #default="{ row }">
            <el-tag :type="statusTag(row.status)" size="small">{{ row.type === 'transfer' && row.status === 1 ? (row.transfer_received_at ? '已签收' : '历史已过账') : statusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="提交时间" width="170" />
        <el-table-column label="发起人" width="100">
          <template #default="{ row }">{{ row.applicant_name || row.applicant_id || '-' }}</template>
        </el-table-column>
        <el-table-column label="审批人" width="170">
          <template #default="{ row }">
            <template v-if="canApprove && row.status === 0">-</template>
            <template v-else>
              <div>{{ row.approver_name || '-' }}</div>
              <div v-if="row.approved_at" style="color: #909399; font-size: 12px">{{ row.approved_at }}</div>
            </template>
          </template>
        </el-table-column>
        <el-table-column label="签收" min-width="145">
          <template #default="{ row }">
            <div v-if="row.transfer_received_at">{{ row.transfer_received_by || '—' }}<br />{{ row.transfer_received_at }}</div>
            <span v-else>—</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="160" fixed="right">
          <template #default="{ row }">
            <template v-if="row.status === 0">
              <el-button size="small" type="success" @click="doApprove(row, true)">批准</el-button>
              <el-button size="small" type="danger" @click="doApprove(row, false)">驳回</el-button>
            </template>
            <el-button v-if="canReceive(row)" size="small" type="primary" :loading="receiving === row.id" @click="receive(row.id)">
              确认签收
            </el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-empty v-if="!loading && filteredList.length === 0" description="暂无操作单" />
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { newRequestId } from '@/utils/requestId'
import ApproverSelect from '@/components/ApproverSelect.vue'
import { ref, reactive, computed, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  getProducts, createStockOperation, approveStockOperation, getStockOperations,
  getSerialInfo,
} from '@/api/inventory'
import { post } from '@/utils/request'
import { getWarehouses } from '@/api/base'
import { useUserStore } from '@/stores/user'
import type { Product, Warehouse } from '@/types'

const userStore = useUserStore()
const canApprove = computed(() => userStore.isLeader || userStore.isAdmin)

const opType = ref('in')
const listTab = ref('0')
const receiving = ref<number>()
function canReceive(row: any) {
  const user = userStore.userInfo
  return row.type === 'transfer' && row.status === 3 && Number(row.approved_by) !== Number(user?.id)
    && (canApprove.value || Number(row.to_warehouse_id) === Number(user?.warehouse_id))
}
async function receive(id: number) {
  try {
    await ElMessageBox.confirm('确认已收到本单全部货物？签收后计入目标仓库存。', '调拨签收')
    receiving.value = id
    const result = await post('/stock/transfer/receive', { operation_id: id, request_id: newRequestId() })
    ElMessage.success(result.message)
    await loadList()
  } catch {
    // 取消或请求失败由交互组件提示。
  } finally {
    receiving.value = undefined
  }
}
const submitting = ref(false)
const loading = ref(false)
const products = ref<Product[]>([])
const warehouses = ref<Warehouse[]>([])
const serialInfoMap = ref<Record<number, number>>({})
const list = ref<any[]>([])
const onlyMine = ref(false)
const searchNo = ref('')
const dateRange = ref<[string,string] | null>(null)
const filteredList = computed(() => list.value.filter(row => {
  const date=String(row.created_at || '').slice(0,10)
  return (!onlyMine.value || Number(row.approved_by) === Number(userStore.userInfo?.id)) && (!searchNo.value.trim() || String(row.operation_no || '').toLowerCase().includes(searchNo.value.trim().toLowerCase())) && (!dateRange.value || (date >= dateRange.value[0] && date <= dateRange.value[1]))
}))

const form = reactive({
  request_id: newRequestId(),
  assigned_approver_id: undefined as number | undefined,
  product_id: undefined as number | undefined,
  from_warehouse_id: undefined as number | undefined,
  to_warehouse_id: undefined as number | undefined,
  qty: 1,
  unit_cost:'',lot_no:'',manufactured_on:'',expires_on:'',biz_type: '',
  logistics_company: '',
  logistics_no: '',
  serial_nos: '',
  remark: '',
})

// 选中商品（融合序列号开关信息）
const selectedProduct = computed(() => {
  const p = products.value.find((x) => x.id === form.product_id)
  if (!p) return undefined
  return { ...p, has_serial: serialInfoMap.value[p.id] || 0 }
})

function typeText(t: string) {
  return { in: '入库', out: '出库', transfer: '调拨' }[t] || t
}
function typeTag(t: string) {
  return { in: 'success', out: 'warning', transfer: 'info' }[t] || 'info'
}
function statusText(s: number) {
  return { 0: '待审批', 1: '已批准', 2: '已驳回', 3: '在途' }[s] || '未知'
}
function statusTag(s: number) {
  return { 0: 'warning', 1: 'success', 2: 'danger' }[s] || 'info'
}

async function loadOptions() {
  try {
    const [p, w, s] = await Promise.all([
      getProducts({}),
      getWarehouses({}),
      getSerialInfo(),
    ])
    products.value = Array.isArray(p.data) ? p.data : []
    warehouses.value = Array.isArray(w.data) ? w.data : []
    const map: Record<number, number> = {}
    for (const x of Array.isArray(s.data) ? s.data : []) {
      map[x.id] = x.has_serial
    }
    serialInfoMap.value = map
  } catch (e) {
  }
}

async function loadList() {
  loading.value = true
  try {
    const params: { type?: string; status?: number } = {}
    if (listTab.value !== 'all') params.status = Number(listTab.value)
    const res = await getStockOperations(params)
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    loading.value = false
  }
}

function resetForm() {
  form.request_id = newRequestId()
  form.product_id = undefined
  form.from_warehouse_id = undefined
  form.to_warehouse_id = undefined
  form.qty = 1
  form.unit_cost='';form.lot_no='';form.manufactured_on='';form.expires_on='';form.biz_type = ''
  form.logistics_company = ''
  form.logistics_no = ''
  form.serial_nos = ''
  form.remark = ''
}

async function handleSubmit() {
  if (!form.product_id) {
    ElMessage.warning('请选择商品')
    return
  }
  // 序列号必填校验
  if (selectedProduct.value?.has_serial) {
    const sns = (form.serial_nos || '').split(',').map((s) => s.trim()).filter(Boolean)
    if (sns.length === 0) {
      ElMessage.warning('该商品启用了序列号管理，请填写序列号')
      return
    }
    if (sns.length !== form.qty) {
      ElMessage.warning(`序列号个数（${sns.length}）与数量（${form.qty}）不一致`)
      return
    }
    form.serial_nos = sns.join(',')
  }
  if (opType.value === 'in' && !form.to_warehouse_id) {
    ElMessage.warning('请选择入库仓库')
    return
  }
  if (opType.value === 'out' && !form.from_warehouse_id) {
    ElMessage.warning('请选择出库仓库')
    return
  }
  if (opType.value === 'transfer') {
    if (!form.from_warehouse_id || !form.to_warehouse_id) {
      ElMessage.warning('请选择调出和调入仓库')
      return
    }
    if (form.from_warehouse_id === form.to_warehouse_id) {
      ElMessage.warning('调出和调入仓库不能相同')
      return
    }
  }
  submitting.value = true
  try {
    await createStockOperation({
      type: opType.value,
      product_id: form.product_id,
      from_warehouse_id: form.from_warehouse_id,
      to_warehouse_id: form.to_warehouse_id,
      qty: form.qty,
      unit_cost:form.unit_cost,lot_no:form.lot_no,manufactured_on:form.manufactured_on,expires_on:form.expires_on,biz_type: form.biz_type,
      logistics_company: form.logistics_company,
      logistics_no: form.logistics_no,
      serial_nos: form.serial_nos,
      request_id: form.request_id,
      remark: form.remark,
      assigned_approver_id: form.assigned_approver_id,
    })
    ElMessage.success('操作单已提交，等待领导审批')
    resetForm()
    listTab.value = '0'
    loadList()
  } catch (e) {
  } finally {
    submitting.value = false
  }
}

async function doApprove(row: any, approve: boolean) {
  let remark = ''
  if (!approve) {
    try {
      const { value } = await ElMessageBox.prompt('请输入驳回原因', '驳回操作单', {
        confirmButtonText: '确认驳回',
        cancelButtonText: '取消',
        inputType: 'textarea',
        inputPlaceholder: '驳回原因（必填）',
        inputValidator: (v: string) => (v && v.trim() ? true : '请填写驳回原因'),
      })
      remark = value || ''
    } catch {
      return
    }
  }
  try {
    await approveStockOperation({ operation_id: row.id, approve, remark })
    ElMessage.success(approve && row.type === 'transfer' ? '已发运，等待目标仓签收' : approve ? '已批准，库存已更新' : '已驳回')
    loadList()
  } catch (e) {
  }
}

onMounted(() => {
  loadOptions()
  loadList()
})
</script>

<style scoped>
.record-filters { display:flex; align-items:center; flex-wrap:wrap; gap:12px; margin:16px 0; }
.record-filters span { color:#909399; font-size:12px; }
</style>
