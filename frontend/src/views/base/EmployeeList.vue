<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-input
          v-model="query.name"
          placeholder="按姓名/工号搜索"
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
          新增员工
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column prop="emp_no" label="工号" width="120" />
        <el-table-column prop="name" label="姓名" width="120" />
        <el-table-column label="关联账号" min-width="140"><template #default="{row}">{{row.user_id ? (accounts.find(u=>u.id===row.user_id)?.username || `账号#${row.user_id}`) : '未关联账号'}}</template></el-table-column>
        <el-table-column prop="department" label="部门" width="130" />
        <el-table-column prop="position" label="职位" width="130" />
        <el-table-column label="所属仓库" min-width="130">
          <template #default="{ row }">
            <el-tag v-if="warehouseName(row)" size="small" type="warning">
              {{ warehouseName(row) }}
            </el-tag>
            <span v-else style="color: #c0c4cc">-</span>
          </template>
        </el-table-column>
        <el-table-column prop="phone" label="电话" width="140" />
        <el-table-column label="状态" width="90">
          <template #default="{ row }">
            <el-tag :type="row.status === 1 ? 'success' : 'info'">
              {{ row.status === 1 ? '在职' : '离职' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="150" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openEdit(row)">编辑</el-button>
            <el-button size="small" type="danger" @click="handleDelete(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <el-dialog v-model="dialogVisible" :title="editingId ? '编辑员工' : '新增员工'" width="520px">
      <el-form ref="formRef" :model="form" :rules="formRules" label-width="90px">
        <el-form-item label="关联账号"><el-select v-model="form.user_id" clearable filterable @change="accountChanged" :disabled="!!editingId && !!boundUserId" placeholder="可选：选择登录账号" style="width:100%"><el-option v-for="u in accounts" :key="u.id" :label="`${u.real_name || u.username}（${u.username}）`" :value="u.id" /></el-select></el-form-item>
        <el-form-item label="工号" prop="emp_no">
          <el-input v-model="form.emp_no" placeholder="如 E0001" />
        </el-form-item>
        <el-form-item label="姓名" prop="name">
          <el-input v-model="form.name" placeholder="姓名" />
        </el-form-item>
        <el-form-item label="部门" prop="department">
          <el-select v-model="form.department_id" clearable filterable placeholder="请选择部门" style="width:100%" @change="departmentChanged"><el-option v-for="d in departments" :key="d.id" :label="d.name" :value="d.id" :disabled="d.status===0" /></el-select>
        </el-form-item>
        <el-form-item v-if="form.department === '仓库部'" label="所属仓库">
          <el-select v-model="form.warehouse_id" placeholder="选择所属仓库（仪表盘按此过滤）" clearable style="width: 100%">
            <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
          </el-select>
          <div style="color: #909399; font-size: 12px; line-height: 1.6; margin-top: 4px">
            分配后，该员工的仪表盘只展示此仓库的库存与本人相关的单据。
          </div>
        </el-form-item>
        <el-form-item label="职位" prop="position">
          <el-input v-model="form.position" placeholder="职位" />
        </el-form-item>
        <el-form-item label="电话" prop="phone">
          <el-input v-model="form.phone" placeholder="联系电话" />
        </el-form-item>
        <el-form-item label="邮箱" prop="email">
          <el-input v-model="form.email" placeholder="邮箱" />
        </el-form-item>
        <el-form-item label="状态" prop="status">
          <el-switch v-model="form.status" :active-value="1" :inactive-value="0" active-text="在职" inactive-text="离职" />
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
import { ElMessage, ElMessageBox, type FormInstance, type FormRules } from 'element-plus'
import { getEmployees, createEmployee, updateEmployee, deleteEmployee, getWarehouses } from '@/api/base'
import { getAllUsers, getDepartments } from '@/api/department'
import type { Employee, Warehouse, Department, DepartmentEmployee } from '@/types'

const loading = ref(false)
const saving = ref(false)
const list = ref<Employee[]>([])
const warehouses = ref<Warehouse[]>([])
const departments = ref<Department[]>([])
const accounts=ref<DepartmentEmployee[]>([])
const boundUserId=ref(0)
function accountChanged(value?:number){const account=accounts.value.find(u=>u.id===value);if(account){form.department_id=account.department_id||undefined;form.department=account.department;form.warehouse_id=Number(account.warehouse_id)||0;if(!form.name)form.name=account.real_name;if(!form.phone)form.phone=account.phone;if(!form.email)form.email=account.email}}
function departmentChanged(value?:number){form.department=departments.value.find(d=>d.id===value)?.name||'';if(form.department!=='仓库部')form.warehouse_id=0}
// userId -> { warehouse_id, warehouse_name }
const userWhMap = ref<Record<number, { warehouse_id?: number; warehouse_name?: string }>>({})
const query = reactive({ name: '' })
const dialogVisible = ref(false)
const editingId = ref<number | null>(null)
const formRef = ref<FormInstance>()
const form = reactive<Partial<Employee>>({
  emp_no: '', name: '', department: '', department_id: undefined, position: '', phone: '', email: '', status: 1,
  user_id: undefined, warehouse_id: 0,
})

const formRules: FormRules = {
  emp_no: [{ required: true, message: '请输入工号', trigger: 'blur' }],
  name: [{ required: true, message: '请输入姓名', trigger: 'blur' }],
}

function warehouseName(row: Employee) {
  if (!row.user_id) return ''
  return userWhMap.value[row.user_id]?.warehouse_name || ''
}

async function loadOptions() {
  try {
    const [whs, users, deps] = await Promise.all([getWarehouses({}), getAllUsers(),getDepartments()])
    departments.value=deps.data||[]
    accounts.value=users.data||[]
    warehouses.value = Array.isArray(whs.data) ? whs.data : []
    const map: Record<number, { warehouse_id?: number; warehouse_name?: string }> = {}
    for (const u of Array.isArray(users.data) ? users.data : []) {
      map[u.id] = {
        warehouse_id: (u.warehouse_id as number | undefined) ?? undefined,
        warehouse_name: (u.warehouse_name as string | undefined) || '',
      }
    }
    userWhMap.value = map
  } catch (e) {
    warehouses.value = []
    userWhMap.value = {}
  }
}

async function loadData() {
  loading.value = true
  try {
    const res = await getEmployees({ name: query.name || undefined })
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    loading.value = false
  }
}

function openCreate() {
  editingId.value = null
  boundUserId.value=0
  Object.assign(form, {
    emp_no: '', name: '', department: '', department_id: undefined, position: '', phone: '', email: '', status: 1,
    user_id: undefined, warehouse_id: 0,
  })
  dialogVisible.value = true
}

function openEdit(row: Employee) {
  editingId.value = row.id
  boundUserId.value=Number(row.user_id)||0
  Object.assign(form, {
    emp_no: row.emp_no, name: row.name, department: row.department, department_id: Number(row.department_id)||undefined, position: row.position,
    phone: row.phone, email: row.email, status: row.status ?? 1,
    user_id: row.user_id ?? undefined,
    warehouse_id: row.user_id ? (userWhMap.value[row.user_id]?.warehouse_id || 0) : 0,
  })
  dialogVisible.value = true
}

async function handleSave() {
  if (!formRef.value) return
  await formRef.value.validate(async (valid) => {
    if (!valid) return
    saving.value = true
    try {
      if (editingId.value) {
        await updateEmployee(editingId.value, form)
      } else {
        await createEmployee(form)
      }
      ElMessage.success('保存成功')
      dialogVisible.value = false
      loadData()
      loadOptions()
    } catch (e) {
    } finally {
      saving.value = false
    }
  })
}

async function handleDelete(row: Employee) {
  await ElMessageBox.confirm(`确定删除员工「${row.name}」吗？`, '提示', {
    confirmButtonText: '确定', cancelButtonText: '取消', type: 'warning',
  })
  try {
    await deleteEmployee(row.id)
    ElMessage.success('删除成功')
    loadData()
  } catch (e) {
  }
}

onMounted(() => {
  loadData()
  loadOptions()
})
</script>

<style scoped>
.toolbar { display: flex; gap: 12px; align-items: center; }
</style>
