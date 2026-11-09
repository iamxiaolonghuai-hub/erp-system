<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-input
          v-model="query.keyword"
          placeholder="按商品/仓库搜索"
          clearable
          style="width: 260px"
          @keyup.enter="loadData"
        />
        <el-button type="primary" @click="loadData">
          <el-icon><Search /></el-icon>
          查询
        </el-button>
        <el-checkbox v-model="onlyLow">仅看安全库存预警（{{ warningCount }}）</el-checkbox>
      </div>

      <el-table :data="filteredList" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column label="商品名称" min-width="160">
          <template #default="{ row }">
            {{ productName(row.product_id) }}
          </template>
        </el-table-column>
        <el-table-column label="仓库" width="120">
          <template #default="{ row }">
            {{ warehouseName(row.warehouse_id) }}
          </template>
        </el-table-column>
        <el-table-column prop="reserved_qty" label="已预占" width="100" /><el-table-column prop="available_qty" label="可用库存" width="110" /><el-table-column prop="quantity" label="当前库存" width="120">
          <template #default="{ row }">
            <el-tag :type="row.quantity <= row.min_quantity ? 'danger' : 'success'">
              {{ row.quantity }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="min_quantity" label="安全库存" width="110" />
        <el-table-column label="库存状态" min-width="120">
          <template #default="{ row }">
            <span v-if="row.quantity <= row.min_quantity" style="color: #f56c6c">
              已达到或低于安全库存，请补货
            </span>
            <span v-else style="color: #67c23a">正常</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="110" fixed="right">
          <template #default="{ row }">
            <el-button v-if="canSetSafety" size="small" type="primary" plain @click="openSetSafety(row)">设置安全库存</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 设置安全库存弹窗 -->
    <el-dialog v-model="safetyVisible" title="设置安全库存" width="420px">
      <el-form :model="safetyForm" label-width="110px">
        <el-form-item label="商品">
          <span>{{ productName(safetyRow?.product_id ?? 0) }}</span>
        </el-form-item>
        <el-form-item label="仓库">
          <span>{{ warehouseName(safetyRow?.warehouse_id ?? 0) }}</span>
        </el-form-item>
        <el-form-item label="当前库存">
          <span>{{ safetyRow?.quantity }}</span>
        </el-form-item>
        <el-form-item label="安全库存" required>
          <el-input-number v-model="safetyForm.min_quantity" :min="0" style="width: 100%" />
        </el-form-item>
        <div style="color: #909399; font-size: 12px; padding-left: 110px">
          库存小于或等于该值时，列表和仪表盘均计入预警
        </div>
      </el-form>
      <template #footer>
        <el-button @click="safetyVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="saveSafety">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { ElMessage } from 'element-plus'
import { getInventory, getProducts, updateInventoryMinQuantity } from '@/api/inventory'
import { getWarehouses } from '@/api/base'
import type { Inventory, Product, Warehouse } from '@/types'

const loading = ref(false)
const list = ref<Inventory[]>([])
const products = ref<Product[]>([])
const warehouses = ref<Warehouse[]>([])
const query = reactive({ keyword: '' })
const route=useRoute(),user=useUserStore()
const onlyLow=ref(route.query.low_stock==='1')
const canSetSafety=computed(()=>user.isAdmin || (user.isLeader && user.department==='仓库部'))
const warningCount=computed(()=>list.value.filter(row=>Number(row.quantity)<=Number(row.min_quantity)).length)

// 设置安全库存
const safetyVisible = ref(false)
const saving = ref(false)
const safetyRow = ref<Inventory | null>(null)
const safetyForm = reactive({ min_quantity: 0 })

function openSetSafety(row: Inventory) {
  safetyRow.value = row
  safetyForm.min_quantity = Number(row.min_quantity || 0)
  safetyVisible.value = true
}

async function saveSafety() {
  if (!safetyRow.value) return
  saving.value = true
  try {
    await updateInventoryMinQuantity(safetyRow.value.id, Number(safetyForm.min_quantity) || 0)
    ElMessage.success('安全库存已更新')
    safetyVisible.value = false
    loadData()
  } catch (e) {
    // 已提示
  } finally {
    saving.value = false
  }
}

const filteredList = computed(() => {
  const kw = query.keyword.trim().toLowerCase()
  const rows=onlyLow.value ? list.value.filter(row=>Number(row.quantity)<=Number(row.min_quantity)) : list.value
  if (!kw) return rows
  return rows.filter((row) => {
    const pname = productName(row.product_id).toLowerCase()
    const wname = warehouseName(row.warehouse_id).toLowerCase()
    return pname.includes(kw) || wname.includes(kw)
  })
})

function productName(pid: number): string {
  const p = products.value.find((x) => x.id === pid)
  return p ? p.name : `商品#${pid}`
}

function warehouseName(wid: number): string {
  const w = warehouses.value.find((x) => x.id === wid)
  return w ? w.name : `仓库#${wid}`
}

async function loadData() {
  loading.value = true
  try {
    const [invRes, prodRes, whRes] = await Promise.all([
      getInventory({}),
      getProducts({}),
      getWarehouses({}),
    ])
    list.value = Array.isArray(invRes.data) ? invRes.data : []
    products.value = Array.isArray(prodRes.data) ? prodRes.data : []
    warehouses.value = Array.isArray(whRes.data) ? whRes.data : []
  } catch (e) {
    // 已提示
  } finally {
    loading.value = false
  }
}

onMounted(loadData)
</script>

<style scoped>
.toolbar {
  display: flex;
  gap: 12px;
}
</style>
