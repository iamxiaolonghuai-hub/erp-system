<template>
  <div>
    <el-card shadow="never" class="form-card">
      <div class="form-title">出库登记</div>
      <el-divider />
      <el-alert
        title="出库会校验库存，库存不足时操作会被拒绝"
        type="warning"
        :closable="false"
        show-icon
        style="margin-bottom: 16px"
      />
      <el-form ref="formRef" :model="form" :rules="rules" label-width="100px" style="max-width: 520px">
        <el-form-item label="商品" prop="product_id">
          <el-select
            v-model="form.product_id"
            filterable
            placeholder="请选择商品"
            style="width: 100%"
          >
            <el-option
              v-for="p in products"
              :key="p.id"
              :label="`${p.name}（${p.sku}）`"
              :value="p.id"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="仓库" prop="warehouse_id">
          <el-select v-model="form.warehouse_id" placeholder="请选择仓库" style="width: 100%">
            <el-option
              v-for="w in warehouses"
              :key="w.id"
              :label="w.name"
              :value="w.id"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="出库数量" prop="qty">
          <el-input-number v-model="form.qty" :min="1" style="width: 100%" />
        </el-form-item>
        <el-form-item label="业务类型" prop="biz_type">
          <el-select v-model="form.biz_type" style="width: 100%">
            <el-option label="销售出库" value="sale_out" />
            <el-option label="采购退货" value="purchase_return" />
            <el-option label="其他出库" value="other_out" />
          </el-select>
        </el-form-item>
        <el-form-item label="业务单号" prop="biz_no">
          <el-input v-model="form.biz_no" placeholder="关联的销售单号（可选）" />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">确认出库</el-button>
          <el-button @click="resetForm">重置</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, type FormInstance, type FormRules } from 'element-plus'
import { stockOut, getProducts } from '@/api/inventory'
import { getWarehouses } from '@/api/base'
import type { Product, Warehouse } from '@/types'

const formRef = ref<FormInstance>()
const submitting = ref(false)
const products = ref<Product[]>([])
const warehouses = ref<Warehouse[]>([])
const form = reactive({
  product_id: undefined as number | undefined,
  warehouse_id: undefined as number | undefined,
  qty: 1,
  biz_type: 'sale_out',
  biz_no: '',
})

const rules: FormRules = {
  product_id: [{ required: true, message: '请选择商品', trigger: 'change' }],
  warehouse_id: [{ required: true, message: '请选择仓库', trigger: 'change' }],
  qty: [{ required: true, message: '请输入出库数量', trigger: 'blur' }],
}

async function loadOptions() {
  try {
    const [prodRes, whRes] = await Promise.all([getProducts({}), getWarehouses({})])
    products.value = Array.isArray(prodRes.data) ? prodRes.data : []
    warehouses.value = Array.isArray(whRes.data) ? whRes.data : []
  } catch (e) {
    // 已提示
  }
}

async function handleSubmit() {
  if (!formRef.value) return
  await formRef.value.validate(async (valid) => {
    if (!valid) return
    submitting.value = true
    try {
      const res = await stockOut({
        product_id: form.product_id as number,
        warehouse_id: form.warehouse_id as number,
        qty: form.qty,
        biz_type: form.biz_type,
        biz_no: form.biz_no,
      })
      ElMessage.success(`出库成功，当前库存 ${(res.data as { after_qty: number }).after_qty}`)
      resetForm()
    } catch (e) {
      // 已提示（库存不足等）
    } finally {
      submitting.value = false
    }
  })
}

function resetForm() {
  form.qty = 1
  form.biz_no = ''
}

onMounted(loadOptions)
</script>

<style scoped>
.form-card {
  max-width: 700px;
}
.form-title {
  font-size: 18px;
  font-weight: 600;
  color: #333;
}
</style>
