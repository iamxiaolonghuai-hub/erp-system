<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <span style="font-weight: 600; font-size: 16px">
          本部门员工（{{ userStore.department }}）
        </span>
        <el-button type="primary" @click="loadData">
          <el-icon><Refresh /></el-icon>
          刷新
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column prop="username" label="账号" width="140" />
        <el-table-column prop="real_name" label="姓名" width="120" />
        <el-table-column prop="department" label="部门" width="120" />
        <el-table-column label="角色" width="110">
          <template #default="{ row }">
            <el-tag :type="row.role_code === 'leader' ? 'warning' : 'success'" size="small">
              {{ roleText(row.role_code) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="email" label="邮箱" min-width="160" />
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
            <el-button
              v-if="row.role_code === 'staff' && canEdit(row)"
              :type="row.status === 1 ? 'danger' : 'success'"
              size="small"
              @click="toggleStatus(row)"
            >
              {{ row.status === 1 ? '禁用' : '启用' }}
            </el-button>
            <span v-else style="color: #c0c4cc">—</span>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { getDepartmentEmployees, updateUserStatus } from '@/api/department'
import { useUserStore } from '@/stores/user'
import type { DepartmentEmployee } from '@/types'

const userStore = useUserStore()
const loading = ref(false)
const list = ref<DepartmentEmployee[]>([])

function roleText(code?: string) {
  if (code === 'leader') return '领导'
  if (code === 'admin') return '管理员'
  return '员工'
}

// 是否可编辑该行（领导不能改自己）
function canEdit(row: DepartmentEmployee) {
  return row.id !== userStore.userInfo?.id
}

async function toggleStatus(row: DepartmentEmployee) {
  const next = row.status === 1 ? 0 : 1
  const action = next === 0 ? '禁用' : '启用'
  await ElMessageBox.confirm(
    `确定要${action}员工「${row.real_name || row.username}」吗？`,
    '提示',
    { confirmButtonText: '确定', cancelButtonText: '取消', type: 'warning' },
  )
  try {
    await updateUserStatus(row.id, next)
    ElMessage.success(`${action}成功`)
    loadData()
  } catch (e) {
    // 已提示
  }
}

async function loadData() {
  loading.value = true
  try {
    const res = await getDepartmentEmployees()
    list.value = Array.isArray(res.data) ? res.data : []
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
  justify-content: space-between;
  align-items: center;
}
</style>
