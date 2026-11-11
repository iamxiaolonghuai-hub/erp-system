<template>
  <el-card shadow="never">
    <div class="toolbar">
      <el-input v-model="serial" placeholder="输入完整序列号" clearable @keyup.enter="load" />
      <el-button type="primary" :loading="loading" @click="load">查询</el-button>
      <el-button v-if="canRegister" @click="open">登记期初</el-button>
    </div>
    <el-table v-loading="loading" :data="history" stripe>
      <el-table-column prop="serial_no" label="序列号" min-width="130" />
      <el-table-column prop="product_name" label="商品" min-width="140" />
      <el-table-column prop="lot_no" label="批次" min-width="100" />
      <el-table-column prop="warehouse_name" label="操作仓库" min-width="120" />
      <el-table-column label="动作" min-width="115">
        <template #default="{row}">{{ actions[row.action] || row.action }}</template>
      </el-table-column>
      <el-table-column label="当前状态" min-width="100">
        <template #default="{row}">{{ states[row.unit_state] || '—' }}</template>
      </el-table-column>
      <el-table-column prop="biz_no" label="业务单号" min-width="160" />
      <el-table-column prop="created_at" label="操作时间" min-width="155" />
    </el-table>
    <el-dialog v-model="visible" title="登记期初序列号" width="540px">
      <el-form label-width="100px">
        <el-form-item label="商品">
          <el-select v-model="form.product_id" filterable>
            <el-option v-for="p in products" :key="p.id" :value="p.id" :label="p.name" />
          </el-select>
        </el-form-item>
        <el-form-item label="仓库">
          <el-select v-model="form.warehouse_id" filterable>
            <el-option v-for="w in warehouses" :key="w.id" :value="w.id" :label="w.name" />
          </el-select>
        </el-form-item>
        <el-form-item v-if="isBatched" label="期初批次号">
          <el-input v-model="form.lot_no" placeholder="已登记的期初批次" />
        </el-form-item>
        <el-form-item label="序列号">
          <el-input v-model="form.serial_nos" type="textarea" placeholder="用英文逗号分隔，最多 500 件" />
        </el-form-item>
        <el-form-item label="登记依据"><el-input v-model="form.reason" maxlength="255" /></el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="visible=false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="save">登记</el-button>
      </template>
    </el-dialog>
  </el-card>
</template>

<script setup lang="ts">
import { computed, reactive, ref } from 'vue'
import { ElMessage } from 'element-plus'
import { getWarehouses } from '@/api/base'
import { getProducts } from '@/api/inventory'
import type { Product, Warehouse } from '@/types'
import { useUserStore } from '@/stores/user'
import { newRequestId } from '@/utils/requestId'
import { get, post } from '@/utils/request'

const user = useUserStore()
const canRegister = computed(() => user.isLeader || user.isAdmin)
const serial = ref('')
const loading = ref(false)
const saving = ref(false)
const visible = ref(false)
const history = ref<Record<string, unknown>[]>([])
const products = ref<Product[]>([])
const warehouses = ref<Warehouse[]>([])
const form = reactive({
  product_id: undefined as number | undefined, warehouse_id: undefined as number | undefined,
  lot_no: '', serial_nos: '', reason: '', request_id: '',
})
const isBatched = computed(() => Number(products.value.find(p => p.id === form.product_id)?.batch_enabled) === 1)
const states: Record<string, string> = { in_stock: '在库', sold: '已出库', in_transit: '调拨在途', removed: '已移出' }
const actions: Record<string, string> = {
  opening: '期初登记', purchase_in: '采购入库', sale_out: '销售出库', purchase_return: '采购退货',
  sales_return: '销售退回', sales_exchange_out: '销售换出', transfer_in: '调拨签收', transfer_out: '调拨发运',
  stock_in: '调整入库', stock_out: '调整出库', scrap_out: '报损出库',
}

async function load() {
  if (!serial.value.trim()) return
  loading.value = true
  history.value = []
  try {
    const result = await get<Record<string, unknown>[]>('/stock/serials', { serial_no: serial.value.trim() })
    history.value = result.data
  } catch {
    // Request errors are shown by the shared client.
  } finally {
    loading.value = false
  }
}
async function open() {
  try {
    const [p, w] = await Promise.all([getProducts({}), getWarehouses({})])
    products.value = p.data.filter(item => Number(item.has_serial) === 1)
    warehouses.value = w.data.filter(item => Number(item.status) === 1)
    Object.assign(form, { product_id: undefined, warehouse_id: undefined, lot_no: '', serial_nos: '', reason: '', request_id: newRequestId() })
    visible.value = true
  } catch {
    // Request errors are shown by the shared client.
  }
}
async function save() {
  if (!form.product_id || !form.warehouse_id || !form.serial_nos.trim() || !form.reason.trim()) {
    ElMessage.warning('请填写商品、仓库、序列号与登记依据')
    return
  }
  saving.value = true
  try {
    const result = await post('/stock/serials/opening', { ...form, quantity: form.serial_nos.split(',').length })
    ElMessage.success(result.message)
    visible.value = false
    serial.value = form.serial_nos.split(',')[0].trim()
    await load()
  } catch {
    // Request errors are shown by the shared client.
  } finally {
    saving.value = false
  }
}
</script>

<style scoped>
.toolbar { display: flex; gap: 12px; margin-bottom: 16px; }
.toolbar .el-input { max-width: 260px; }
</style>
