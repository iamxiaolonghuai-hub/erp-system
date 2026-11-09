<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-button type="primary" @click="loadData">
          <el-icon><Refresh /></el-icon>
          刷新
        </el-button>
        <el-button type="success" @click="openCreate">
          <el-icon><Plus /></el-icon>
          新建盘点单
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column prop="check_no" label="盘点单号" min-width="180" />
        <el-table-column prop="counter_name" label="盘点人" min-width="120" />
        <el-table-column prop="created_by_name" label="盘点发起人" min-width="120" />
        <el-table-column prop="reviewer_name" label="复核人" min-width="120" />
        <el-table-column prop="warehouse_id" label="仓库ID" width="100" />
        <el-table-column label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="row.status === 1 ? 'success' : 'warning'">
              {{ row.status === 1 ? '已完成' : '待复核' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="remark" label="备注" min-width="140" />
        <el-table-column prop="created_at" label="创建时间" width="170" />
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openDetail(row)">明细</el-button>
            <el-button
              v-if="row.status !== 1 && (userStore.isAdmin || userStore.isLeader) && row.created_by !== userStore.userInfo?.id"
              size="small"
              type="success"
              @click="handleComplete(row)"
            >
              复核并调整
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 新建盘点单 -->
    <el-dialog v-model="dialogVisible" title="新建盘点单" width="820px">
      <el-form :model="form" label-width="90px">
        <el-form-item label="盘点人"><el-input :model-value="userStore.userInfo?.real_name || userStore.userInfo?.username || ''" disabled /><span style="font-size:12px;color:#909399">保存时自动记录当前登录人</span></el-form-item>
        <el-form-item label="盘点仓库" required>
          <el-select
            v-model="form.warehouse_id"
            filterable
            placeholder="选择仓库"
            style="width: 100%"
            @change="loadInventory"
          >
            <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="备注">
          <el-input v-model="form.remark" placeholder="备注（可选）" />
        </el-form-item>
      </el-form>

      <el-divider content-position="left">盘点明细（录入实盘数量）</el-divider>
      <el-table :data="form.items" border size="small" max-height="320">
        <el-table-column label="商品" min-width="200">
          <template #default="{ row }">
            <span>{{ row.product_name }}</span>
          </template>
        </el-table-column>
        <el-table-column label="账面数量" width="110">
          <template #default="{ row }">{{ row.book_qty }}</template>
        </el-table-column>
        <el-table-column label="实盘数量" width="160">
          <template #default="{ row }">
            <el-input-number v-model="row.actual_qty" :min="0" style="width: 100%" />
          </template>
        </el-table-column>
        <el-table-column label="差异" width="100">
          <template #default="{ row }">
            <span :style="{ color: row.actual_qty - row.book_qty !== 0 ? '#f56c6c' : '#67c23a' }">
              {{ row.actual_qty - row.book_qty }}
            </span>
          </template>
        </el-table-column>
      </el-table>

      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="handleSubmit">保存盘点单</el-button>
      </template>
    </el-dialog>

    <!-- 盘点明细查看 -->
    <el-dialog v-model="detailVisible" title="盘点明细" width="700px">
      <el-descriptions v-if="detailCheck" :column="2" border style="margin-bottom:16px"><el-descriptions-item label="盘点人">{{ detailCheck.counter_name || '未记录' }}</el-descriptions-item><el-descriptions-item label="发起人">{{ detailCheck.created_by_name || '未记录' }}</el-descriptions-item><el-descriptions-item label="复核人">{{ detailCheck.reviewer_name || '待复核' }}</el-descriptions-item><el-descriptions-item label="盘点单号">{{ detailCheck.check_no }}</el-descriptions-item></el-descriptions>
      <el-table :data="detailItems" v-loading="detailLoading" stripe border>
        <el-table-column prop="product_id" label="商品ID" width="100" />
        <el-table-column prop="book_qty" label="账面数量" width="110" />
        <el-table-column prop="actual_qty" label="实盘数量" width="110" />
        <el-table-column label="差异" width="100">
          <template #default="{ row }">
            <span :style="{ color: row.diff_qty !== 0 ? '#f56c6c' : '#67c23a' }">{{ row.diff_qty }}</span>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { newRequestId } from '@/utils/requestId'
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { getStockChecks, getStockCheckItems, createStockCheck, completeStockCheck } from '@/api/stockcheck'
import { getWarehouses } from '@/api/base'
import { getInventory } from '@/api/inventory'
import { getProducts } from '@/api/inventory'
import { useUserStore } from '@/stores/user'
import type { StockCheck, StockCheckItem, Warehouse, Inventory, Product } from '@/types'

const userStore = useUserStore()
const loading = ref(false)
const saving = ref(false)
const list = ref<StockCheck[]>([])
const warehouses = ref<Warehouse[]>([])
const dialogVisible = ref(false)
const detailVisible = ref(false)
const detailLoading = ref(false)
const detailItems = ref<StockCheckItem[]>([])
const detailCheck = ref<StockCheck | null>(null)

const form = reactive({
  request_id: newRequestId(),
  warehouse_id: undefined as number | undefined,
  remark: '',
  items: [] as { product_id: number; product_name: string; book_qty: number; actual_qty: number }[],
})

async function loadData() {
  loading.value = true
  try {
    const res = await getStockChecks({})
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    loading.value = false
  }
}

async function loadWarehouses() {
  try {
    const res = await getWarehouses({})
    warehouses.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  }
}

async function loadInventory() {
  if (!form.warehouse_id) return
  try {
    const [invRes, prodRes] = await Promise.all([getInventory({}), getProducts({})])
    const inventory = Array.isArray(invRes.data) ? invRes.data : []
    const products = Array.isArray(prodRes.data) ? prodRes.data : []
    const productMap = new Map<number, Product>(products.map((p: Product) => [p.id, p]))

    form.items = inventory
      .filter((i: Inventory) => i.warehouse_id === form.warehouse_id)
      .map((i: Inventory) => {
        const p = productMap.get(i.product_id)
        return {
          product_id: i.product_id,
          product_name: p ? `${p.name} (${p.sku})` : `商品${i.product_id}`,
          book_qty: i.quantity,
          actual_qty: i.quantity,
        }
      })
  } catch (e) {
  }
}

function openCreate() {
  form.request_id = newRequestId()
  form.warehouse_id = undefined
  form.remark = ''
  form.items = []
  dialogVisible.value = true
}

async function handleSubmit() {
  if (!form.warehouse_id) {
    ElMessage.warning('请选择盘点仓库')
    return
  }
  if (form.items.length === 0) {
    ElMessage.warning('该仓库暂无库存')
    return
  }
  saving.value = true
  try {
    await createStockCheck({
      warehouse_id: form.warehouse_id,
      request_id: form.request_id,
      remark: form.remark,
      items: form.items.map((it) => ({
        product_id: it.product_id,
        book_qty: it.book_qty,
        actual_qty: it.actual_qty,
      })),
    })
    ElMessage.success('盘点单创建成功')
    dialogVisible.value = false
    loadData()
  } catch (e) {
  } finally {
    saving.value = false
  }
}

async function openDetail(row: StockCheck) {
  detailCheck.value = row
  detailVisible.value = true
  detailLoading.value = true
  try {
    const res = await getStockCheckItems(row.id)
    detailItems.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    detailLoading.value = false
  }
}

async function handleComplete(row: StockCheck) {
  await ElMessageBox.confirm(
    `确定复核并调整单「${row.check_no}」吗？将按差异自动调整库存。`,
    '提示',
    { confirmButtonText: '确定', cancelButtonText: '取消', type: 'warning' },
  )
  try {
    await completeStockCheck(row.id)
    ElMessage.success('盘点完成，库存已调整')
    loadData()
  } catch (e) {
  }
}

onMounted(() => {
  loadData()
  loadWarehouses()
})
</script>

<style scoped>
.toolbar { display: flex; gap: 12px; }
</style>
