<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-input
          v-model="query.username"
          placeholder="按用户名/姓名搜索"
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
          新增用户
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column prop="username" label="用户名" width="140" />
        <el-table-column prop="real_name" label="姓名" width="120" />
        <el-table-column prop="email" label="邮箱" min-width="160" />
        <el-table-column prop="phone" label="电话" width="140" />
        <el-table-column label="状态" width="90">
          <template #default="{ row }">
            <el-tag :type="row.status === 1 ? 'success' : 'info'">
              {{ row.status === 1 ? '启用' : '禁用' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="220" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openEdit(row)">编辑</el-button>
            <el-button size="small" type="warning" @click="openReset(row)">重置密码</el-button>
            <el-button size="small" type="danger" @click="handleDelete(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 新增/编辑用户 -->
    <el-dialog v-model="dialogVisible" :title="editingId ? '编辑用户' : '新增用户'" width="480px">
      <el-form ref="formRef" :model="form" :rules="formRules" label-width="90px">
        <el-form-item v-if="!editingId" label="用户名" prop="username">
          <el-input v-model="form.username" placeholder="登录用户名" />
        </el-form-item>
        <el-form-item v-if="!editingId" label="密码" prop="password">
          <el-input v-model="form.password" type="password" placeholder="初始密码" show-password />
        </el-form-item>
        <el-form-item label="姓名" prop="real_name">
          <el-input v-model="form.real_name" placeholder="真实姓名" />
        </el-form-item>
        <el-form-item label="邮箱">
          <el-input v-model="form.email" placeholder="邮箱" />
        </el-form-item>
        <el-form-item label="电话">
          <el-input v-model="form.phone" placeholder="电话" />
        </el-form-item>
        <el-form-item v-if="!editingId" label="角色">
          <el-select v-model="form.role_id" placeholder="选择角色" style="width: 100%">
            <el-option v-for="r in roles" :key="r.id" :label="r.name" :value="r.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="状态">
          <el-switch v-model="form.status" :active-value="1" :inactive-value="0" active-text="启用" inactive-text="禁用" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="handleSave">保存</el-button>
      </template>
    </el-dialog>

    <!-- 重置密码 -->
    <el-dialog v-model="resetVisible" title="重置密码" width="400px">
      <el-form label-width="80px">
        <el-form-item label="新密码">
          <el-input v-model="resetPasswordForm.password" type="password" show-password placeholder="新密码" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="resetVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="handleReset">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox, type FormInstance, type FormRules } from 'element-plus'
import { getUsers, createUser, updateUser, deleteUser, resetPassword, getRoles } from '@/api/system'
import type { UserInfo, Role } from '@/types'

const loading = ref(false)
const saving = ref(false)
const list = ref<UserInfo[]>([])
const roles = ref<Role[]>([])
const query = reactive({ username: '' })
const dialogVisible = ref(false)
const resetVisible = ref(false)
const editingId = ref<number | null>(null)
const resettingId = ref<number | null>(null)
const formRef = ref<FormInstance>()
const form = reactive({
  username: '',
  password: '',
  real_name: '',
  email: '',
  phone: '',
  role_id: undefined as number | undefined,
  status: 1,
})
const resetPasswordForm = reactive({ password: '' })

const formRules: FormRules = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  password: [{ required: true, message: '请输入密码', trigger: 'blur' }],
  real_name: [{ required: true, message: '请输入姓名', trigger: 'blur' }],
}

async function loadData() {
  loading.value = true
  try {
    const res = await getUsers({ username: query.username || undefined })
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    loading.value = false
  }
}

async function loadRoles() {
  try {
    const res = await getRoles({})
    roles.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  }
}

function openCreate() {
  editingId.value = null
  Object.assign(form, { username: '', password: '', real_name: '', email: '', phone: '', role_id: undefined, status: 1 })
  dialogVisible.value = true
}

function openEdit(row: UserInfo) {
  editingId.value = row.id
  Object.assign(form, {
    username: row.username, password: '', real_name: row.real_name,
    email: row.email, phone: row.phone, status: row.status ?? 1,
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
        await updateUser(editingId.value, {
          real_name: form.real_name, email: form.email, phone: form.phone, status: form.status,
        })
      } else {
        await createUser({
          username: form.username, password: form.password, real_name: form.real_name,
          email: form.email, phone: form.phone, role_id: form.role_id,
        })
      }
      ElMessage.success('保存成功')
      dialogVisible.value = false
      loadData()
    } catch (e) {
    } finally {
      saving.value = false
    }
  })
}

function openReset(row: UserInfo) {
  resettingId.value = row.id
  resetPasswordForm.password = ''
  resetVisible.value = true
}

async function handleReset() {
  if (!resettingId.value || !resetPasswordForm.password) {
    ElMessage.warning('请输入新密码')
    return
  }
  saving.value = true
  try {
    await resetPassword(resettingId.value, resetPasswordForm.password)
    ElMessage.success('密码重置成功')
    resetVisible.value = false
  } catch (e) {
  } finally {
    saving.value = false
  }
}

async function handleDelete(row: UserInfo) {
  await ElMessageBox.confirm(`确定删除用户「${row.username}」吗？`, '提示', {
    confirmButtonText: '确定', cancelButtonText: '取消', type: 'warning',
  })
  try {
    await deleteUser(row.id)
    ElMessage.success('删除成功')
    loadData()
  } catch (e) {
  }
}

onMounted(() => {
  loadData()
  loadRoles()
})
</script>

<style scoped>
.toolbar { display: flex; gap: 12px; }
</style>
