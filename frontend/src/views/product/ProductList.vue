<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-input
          v-model="query.name"
          placeholder="按商品名称/编码搜索"
          clearable
          @clear="loadData"
          style="width: 260px"
          @keyup.enter="loadData"
        />
        <el-button type="primary" @click="loadData">
          <el-icon><Search /></el-icon>
          查询
        </el-button>
        <el-button type="success" @click="openCreate">
          <el-icon><Plus /></el-icon>
          新增商品
        </el-button>
        <el-button type="warning" @click="handleExport">
          <el-icon><Download /></el-icon>
          导出
        </el-button>
        <el-upload
          :show-file-list="false"
          accept=".csv"
          :before-upload="handleImport"
          style="display: inline-block"
        >
          <el-button type="info">
            <el-icon><Upload /></el-icon>
            导入
          </el-button>
        </el-upload>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column prop="sku" label="商品编码" width="140" />
        <el-table-column prop="name" label="商品名称" min-width="160" />
        <el-table-column label="供应商" min-width="160"><template #default="{ row }">{{ suppliers.find(s => s.id === supplierMap[row.id])?.name || '未设置' }}</template></el-table-column>
        <el-table-column prop="spec" label="规格" width="140" />
        <el-table-column prop="unit" label="单位" width="90" />
        <el-table-column prop="price" label="价格(¥)" width="110">
          <template #default="{ row }">
            {{ row.price != null ? Number(row.price).toFixed(2) : '-' }}
          </template>
        </el-table-column>
        <el-table-column label="序列号" width="90" align="center">
          <template #default="{ row }">
            <el-tag v-if="serialEnabled(row.id)" size="small" type="warning">已启用</el-tag>
            <span v-else style="color: #c0c4cc">-</span>
          </template>
        </el-table-column>
        <el-table-column label="状态" width="90">
          <template #default="{ row }">
            <el-tag :type="row.status === 1 ? 'success' : 'info'">
              {{ row.status === 1 ? '启用' : '停用' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="150" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openEdit(row)">编辑</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 新增/编辑弹窗 -->
    <el-dialog
      v-model="dialogVisible"
      :title="editingId ? '编辑商品' : '新增商品'"
      width="500px"
    >
      <el-form ref="formRef" :model="form" :rules="formRules" label-width="90px">
        <el-form-item label="商品编码" prop="sku">
          <el-input v-model="form.sku" placeholder="如 SKU0001" />
        </el-form-item>
        <el-form-item label="商品名称" prop="name">
          <el-input v-model="form.name" placeholder="商品名称" />
        </el-form-item>
        <el-form-item label="供应商" prop="supplier_id"><el-select v-model="form.supplier_id" filterable placeholder="请选择供应商" style="width:100%"><el-option v-for="s in suppliers" :key="s.id" :label="s.name" :value="s.id" :disabled="s.status !== 1" /></el-select></el-form-item>
        <el-form-item label="规格" prop="spec">
          <el-input v-model="form.spec" placeholder="如 0603 5%" />
        </el-form-item>
        <el-form-item label="单位" prop="unit">
          <el-input v-model="form.unit" placeholder="如 个/箱/件" />
        </el-form-item>
        <el-form-item label="价格" prop="price">
          <el-input-number v-model="form.price" :min="0" :precision="2" style="width: 100%" />
        </el-form-item>
        <el-form-item label="批次管理"><el-switch v-model="form.batch_enabled" :active-value="1" :inactive-value="0" @change="onBatchChange" /></el-form-item><el-form-item label="有效期管理"><el-switch v-model="form.expiry_enabled" :active-value="1" :inactive-value="0" :disabled="!form.batch_enabled" /><span>启用后入库须填写生产日期和有效期，过期批次禁止普通出库。</span></el-form-item><el-form-item label="序列号管理">
          <el-switch v-model="form.has_serial" :active-value="1" :inactive-value="0" />
          <div style="color: #909399; font-size: 12px; line-height: 1.6; margin-top: 4px; width: 100%">
            启用后，该商品的出入库/调拨需登记序列号（用于售后追踪），请在实际出入库时逐件填写序列号。
          </div>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="handleSave">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, type FormInstance, type FormRules } from 'element-plus'
import {
  getProducts, createProduct, updateProduct,
  getSerialInfo,
} from '@/api/inventory'
import { exportCsv, importCsv } from '@/api/importexport'
import { getSuppliers } from '@/api/base'
import type { Product, Supplier } from '@/types'

const suppliers = ref<Supplier[]>([])
const supplierMap = ref<Record<number, number>>({})
const loading = ref(false)
const saving = ref(false)
const list = ref<Product[]>([])
const serialInfoMap = ref<Record<number, number>>({})
const query = reactive({ name: '' })
const dialogVisible = ref(false)
const editingId = ref<number | null>(null)
const formRef = ref<FormInstance>()
const form = reactive<Partial<Product>>({ sku: '', name: '', unit: '', spec: '', price: 0, has_serial: 0, batch_enabled:0,expiry_enabled:0,supplier_id: undefined })

const formRules: FormRules = {
  supplier_id: [{ required: true, message: '请选择供应商', trigger: 'change' }],
  sku: [{ required: true, message: '请输入商品编码', trigger: 'blur' }],
  name: [{ required: true, message: '请输入商品名称', trigger: 'blur' }],
}

function serialEnabled(id: number) {
  return serialInfoMap.value[id] === 1
}

async function loadSerialInfo() {
  try {
    const res = await getSerialInfo()
    const map: Record<number, number> = {}
    for (const p of Array.isArray(res.data) ? res.data : []) {
      map[p.id] = p.has_serial
      supplierMap.value[p.id] = Number(p.supplier_id || 0)
    }
    serialInfoMap.value = map
  } catch (e) {
    serialInfoMap.value = {}
  }
}

async function loadData() {
  loading.value = true
  try {
    const res = await getProducts({ name: query.name || undefined })
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
    // 已由拦截器提示
  } finally {
    loading.value = false
  }
}

function onBatchChange(value:unknown){if(!value)form.expiry_enabled=0}
function openCreate() {
  editingId.value = null
  Object.assign(form, { sku: '', name: '', unit: '', spec: '', price: 0, has_serial: 0, batch_enabled:0,expiry_enabled:0,supplier_id: undefined })
  dialogVisible.value = true
}

function openEdit(row: Product) {
  editingId.value = row.id
  Object.assign(form, row)
  form.price = Number(row.price || 0)
  form.supplier_id = Number(row.supplier_id || supplierMap.value[row.id] || 0) || undefined
  form.has_serial = serialEnabled(row.id) ? 1 : 0
  dialogVisible.value = true
}

async function handleSave() {
  if (!formRef.value) return
  await formRef.value.validate(async (valid) => {
    if (!valid) return
    saving.value = true
    try {
      if (editingId.value) {
        await updateProduct(editingId.value, form)
      } else {
        await createProduct(form)
      }
      ElMessage.success('保存成功')
      dialogVisible.value = false
      loadData()
      loadSerialInfo()
    } catch (e) {
      // 已提示
    } finally {
      saving.value = false
    }
  })
}

async function handleExport() {
  try {
    await exportCsv('products')
    ElMessage.success('导出成功')
  } catch (e) {
    ElMessage.error('导出失败')
  }
}

async function handleImport(file: File) {
  const text = await file.text()
  try {
    const res = await importCsv('products', text)
    ElMessage.success(`导入完成：成功 ${res.data.success} 条，失败 ${res.data.failed} 条`)
    loadData()
  } catch (e) {
    ElMessage.error('导入失败')
  }
  return false // 阻止 el-upload 自动上传
}

onMounted(async () => {
  try { const res = await getSuppliers(); suppliers.value = res.data } catch {}
  loadData()
  loadSerialInfo()
})
</script>

<style scoped>
.toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
}
</style>
