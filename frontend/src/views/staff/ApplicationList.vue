<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-tabs v-model="activeType" style="flex: 1" @tab-change="loadData">
          <el-tab-pane v-if="isPurchaseDept" label="供应商申报" name="supplier" />
          <el-tab-pane v-if="isPurchaseDept" label="商品申报" name="product" />
          <el-tab-pane v-if="!isPurchaseDept" label="客户申报" name="customer" />
        </el-tabs>
        <el-button type="success" @click="openCreate">
          <el-icon><Plus /></el-icon>
          新建申报
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="assigned_approver_name" label="指定审批人" min-width="130" />
        <el-table-column prop="created_by_name" label="发起人" min-width="110" />
        <el-table-column prop="approver_name" label="审批人" min-width="110" />
        <el-table-column prop="application_no" label="申报单号" width="190" />
        <el-table-column label="类型" width="100">
          <template #default="{ row }">
            <el-tag :type="typeTagType(row.type)" size="small">{{ typeText(row.type) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="title" label="申报名称" min-width="160" />
        <el-table-column label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="statusType(row.status)" size="small">{{ statusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="remark" label="备注/驳回原因" min-width="140" />
        <el-table-column prop="created_at" label="申报时间" width="180" />
        <el-table-column label="操作" width="90" fixed="right">
          <template #default="{ row }">
            <el-button v-if="row.status === 2" size="small" type="warning" @click="openEdit(row)">修改</el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-empty v-if="!loading && list.length === 0" description="暂无申报记录" />
    </el-card>

    <!-- 新建/编辑申报弹窗 -->
    <el-dialog v-model="dialogVisible" :title="dialogTitle" width="560px">
      <el-form ref="formRef" :model="form" :rules="formRules" label-width="90px">
        <el-form-item v-if="isPurchaseDept" label="申报类型">
          <el-radio-group v-model="form.type" :disabled="editingId != null">
            <el-radio-button value="supplier">供应商</el-radio-button>
            <el-radio-button value="product">商品</el-radio-button>
          </el-radio-group>
        </el-form-item>

        <!-- 供应商/客户字段 -->
        <template v-if="form.type !== 'product'">
          <el-form-item :label="form.type === 'supplier' ? '供应商名' : '客户名'" prop="title">
            <el-input v-model="form.title" placeholder="名称" />
          </el-form-item>
          <el-form-item label="联系人">
            <el-input v-model="form.contact" placeholder="联系人" />
          </el-form-item>
          <el-form-item label="电话">
            <el-input v-model="form.phone" placeholder="联系电话" />
          </el-form-item>
          <el-form-item label="邮箱">
            <el-input v-model="form.email" placeholder="邮箱" />
          </el-form-item>
          <el-form-item label="地址">
            <el-input v-model="form.address" placeholder="地址" />
          </el-form-item>
        </template>

        <!-- 商品字段 -->
        <template v-else>
          <el-form-item label="商品名" prop="title">
            <el-input v-model="form.title" placeholder="商品名称" />
          </el-form-item>
          <el-form-item label="供应商" prop="product.supplier_id">
            <el-select v-model="form.product.supplier_id" filterable placeholder="选择供应商" style="width: 100%">
              <el-option v-for="s in suppliers" :key="s.id" :label="s.name" :value="s.id" />
            </el-select>
          </el-form-item>
          <el-form-item label="SKU">
            <el-input v-model="form.product.sku" placeholder="商品编码，如 SKU0006" />
          </el-form-item>
          <el-form-item label="单位">
            <el-input v-model="form.product.unit" placeholder="如 个/件/箱" />
          </el-form-item>
          <el-form-item label="规格">
            <el-input v-model="form.product.spec" placeholder="规格型号" />
          </el-form-item>
          <el-form-item label="分类">
            <el-select v-model="form.product.category_id" filterable placeholder="选择分类" style="width: 100%">
              <el-option v-for="c in categories" :key="c.id" :label="c.name" :value="c.id" />
            </el-select>
          </el-form-item>
          <el-form-item label="销售价(¥)">
            <el-input-number v-model="form.product.price" :min="0" :precision="2" style="width: 100%" />
          </el-form-item>
          <el-form-item label="成本价(¥)">
            <el-input-number v-model="form.product.cost_price" :min="0" :precision="2" style="width: 100%" />
          </el-form-item>
          <el-form-item label="描述">
            <el-input v-model="form.product.description" type="textarea" :rows="2" placeholder="商品描述" />
          </el-form-item>
        </template>

        <el-form-item label="备注">
          <el-input v-model="form.remark" placeholder="申报说明（可选）" />
        </el-form-item>
      <ApproverSelect v-if="!editingId" v-model="form.assigned_approver_id" />
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="handleSubmit">
          {{ editingId ? '重新提交' : '提交申报' }}
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { newRequestId } from '@/utils/requestId'
import ApproverSelect from '@/components/ApproverSelect.vue'
import { ref, reactive, computed, onMounted } from 'vue'
import { ElMessage, type FormInstance, type FormRules } from 'element-plus'
import { getApplications, createApplication, resubmitApplication } from '@/api/application'
import { getCategories } from '@/api/base'
import { getSuppliers } from '@/api/base'
import { useUserStore } from '@/stores/user'
import type { Category, Supplier } from '@/types'

const userStore = useUserStore()

const loading = ref(false)
const saving = ref(false)
const list = ref<any[]>([])
const categories = ref<Category[]>([])
const suppliers = ref<Supplier[]>([])
const dialogVisible = ref(false)
const formRef = ref<FormInstance>()
const editingId = ref<number | null>(null)

// 采购部 → supplier/product；销售部 → customer
const isPurchaseDept = computed(() => userStore.department === '采购部')
const activeType = ref('supplier')

const form = reactive({
  request_id: newRequestId(),
  assigned_approver_id: undefined as number | undefined,
  type: 'supplier' as 'supplier' | 'customer' | 'product',
  title: '',
  remark: '',
  contact: '',
  phone: '',
  email: '',
  address: '',
  product: {
    sku: '', unit: '件', spec: '', category_id: undefined as number | undefined,
    supplier_id: undefined as number | undefined,
    price: 0, cost_price: 0, description: '',
  },
})

const dialogTitle = computed(() => {
  const typeName = { supplier: '供应商', customer: '客户', product: '商品' }[form.type]
  return editingId.value ? `修改${typeName}申报` : `申报${typeName}`
})

const formRules: FormRules = {
  title: [{ required: true, message: '请填写名称', trigger: 'blur' }],
  'product.supplier_id': [{ required: true, message: '请选择供应商', trigger: 'change' }],
}

function typeText(t?: string) {
  return { supplier: '供应商', customer: '客户', product: '商品' }[t || ''] || t || '-'
}
function typeTagType(t?: string) {
  return { supplier: 'warning', customer: 'success', product: 'primary' }[t || ''] || 'info'
}

function statusText(s?: number) {
  return { 0: '待审批', 1: '已批准', 2: '已驳回' }[s ?? 0] || '未知'
}
function statusType(s?: number) {
  return { 0: 'warning', 1: 'success', 2: 'danger' }[s ?? 0] || 'info'
}

async function loadData() {
  loading.value = true
  try {
    const res = await getApplications({ type: activeType.value })
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
    // 已提示
  } finally {
    loading.value = false
  }
}

async function loadOptions() {
  try {
    const c = await getCategories({})
    categories.value = Array.isArray(c.data) ? c.data : []
  } catch (e) {
    // 已提示
  }
  try {
    const s = await getSuppliers()
    suppliers.value = Array.isArray(s.data) ? s.data : []
  } catch (e) {
    // 已提示
  }
}

function resetForm() {
  form.request_id = newRequestId()
  form.type = activeType.value as any
  form.title = ''
  form.remark = ''
  form.contact = ''
  form.phone = ''
  form.email = ''
  form.address = ''
  form.product = {
    sku: '', unit: '件', spec: '', category_id: undefined,
    supplier_id: undefined,
    price: 0, cost_price: 0, description: '',
  }
}

function openCreate() {
  form.request_id = newRequestId()
  editingId.value = null
  resetForm()
  dialogVisible.value = true
}

function openEdit(row: any) {
  editingId.value = row.id
  resetForm()
  form.type = row.type
  form.title = row.title
  try {
    const p = JSON.parse(row.payload || '{}')
    if (row.type === 'product') {
      form.product = {
        sku: p.sku || '', unit: p.unit || '件', spec: p.spec || '',
        category_id: p.category_id, supplier_id: p.supplier_id,
        price: Number(p.price || 0),
        cost_price: Number(p.cost_price || 0), description: p.description || '',
      }
    } else {
      form.contact = p.contact || ''
      form.phone = p.phone || ''
      form.email = p.email || ''
      form.address = p.address || ''
    }
  } catch (e) {
    // 忽略
  }
  dialogVisible.value = true
}

function buildPayload() {
  if (form.type === 'product') {
    return {
      ...form.product,
      price: String(form.product.price),
      cost_price: String(form.product.cost_price),
    }
  }
  return { contact: form.contact, phone: form.phone, email: form.email, address: form.address }
}

async function handleSubmit() {
  if (!formRef.value) return
  await formRef.value.validate(async (valid) => {
    if (!valid) return
    saving.value = true
    try {
      if (editingId.value) {
        await resubmitApplication({
          application_id: editingId.value,
          title: form.title,
          payload: buildPayload(),
        })
        ElMessage.success('已重新提交，等待审批')
      } else {
        await createApplication({
          type: form.type,
          title: form.title,
          payload: buildPayload(),
          request_id: form.request_id,
      remark: form.remark,
      assigned_approver_id: form.assigned_approver_id,
        })
        ElMessage.success('申报已提交，等待审批')
      }
      dialogVisible.value = false
      loadData()
    } catch (e) {
      // 已提示
    } finally {
      saving.value = false
    }
  })
}

onMounted(() => {
  activeType.value = isPurchaseDept.value ? 'supplier' : 'customer'
  loadData()
  loadOptions()
})
</script>

<style scoped>
.toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
</style>
