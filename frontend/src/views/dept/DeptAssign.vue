<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-select
          v-model="filterDept"
          placeholder="按部门筛选"
          clearable
          style="width: 200px"
          @change="loadData"
        >
          <el-option v-for="d in departments" :key="d.id" :label="d.name" :value="d.id" :disabled="d.status===0" />
        </el-select>
        <el-button type="primary" @click="loadData">
          <el-icon><Search /></el-icon>
          查询
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column prop="username" label="账号" width="140" />
        <el-table-column prop="real_name" label="姓名" width="120" />
        <el-table-column prop="department" label="所属部门" width="140">
          <template #default="{ row }">
            <el-tag size="small" :type="row.department ? '' : 'info'">
              {{ row.department || '未分配' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="角色" width="110">
          <template #default="{ row }">
            <el-tag :type="row.role_code === 'leader' ? 'warning' : row.role_code === 'admin' ? 'danger' : 'success'" size="small">
              {{ roleText(row.role_code) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="warehouse_name" label="所属仓库" min-width="130" />
        <el-table-column label="操作" width="240" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" @click="openAssignment(row, false)">调度</el-button>
            <el-button link type="primary" @click="openAssignment(row, true)">移交待办</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
    <el-dialog v-model="visible" :title="handoverOnly ? '移交审批待办' : '员工调度'" width="560px"
      :close-on-click-modal="false" :before-close="closeDialog">
      <div v-loading="previewing">
        <p>{{ selected?.real_name || selected?.username }} · 当前部门 {{ selected?.department || '未分配' }}</p>
        <el-form label-width="110px">
          <template v-if="!handoverOnly">
            <el-form-item label="目标部门">
              <el-select v-model="targetDept" @change="selectDepartment">
                <el-option v-for="d in departments" :key="d.id" :label="d.name" :value="d.id" :disabled="d.status === 0" />
              </el-select>
            </el-form-item>
            <el-form-item v-if="targetWarehouseDept" label="所属仓库" required>
              <el-select v-model="targetWarehouse" filterable placeholder="选择启用仓库">
                <el-option v-for="w in warehouses" :key="w.id" :label="w.name" :value="w.id" />
              </el-select>
            </el-form-item>
          </template>
          <template v-if="!previewing && (handoverOnly || needsHandover)">
            <el-alert v-if="groups.length" type="warning" :closable="false" show-icon
              title="请为每组待办选择接收人。已处理记录保留原审批人。" />
            <el-alert v-else type="info" :closable="false" title="没有待移交的指定审批" />
            <el-form-item v-for="group in groups" :key="group.department" :label="`${group.department}（${group.count}）`">
              <el-select v-model="receivers[group.department]" filterable placeholder="选择待办接收人">
                <el-option v-for="candidate in group.candidates" :key="candidate.id" :label="candidate.name" :value="candidate.id" />
              </el-select>
              <el-text v-if="!group.candidates.length" type="danger">请先安排符合资格的接收人</el-text>
            </el-form-item>
          </template>
        </el-form>
      </div>
      <template #footer>
        <el-button :disabled="saving" @click="visible = false">取消</el-button>
        <el-button type="primary" :loading="saving" :disabled="!canSave" @click="saveAssignment">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getDepartments, assignEmployee, getAllUsers, getHandoverPreview } from '@/api/department'
import type { HandoverGroup } from '@/api/department'
import { getWarehouses } from '@/api/base'
import type { Department, DepartmentEmployee, Warehouse } from '@/types'

const loading = ref(false)
const list = ref<DepartmentEmployee[]>([])
const departments = ref<Department[]>([])
const filterDept = ref<number>()
const visible = ref(false)
const saving = ref(false)
const previewing = ref(false)
const previewReady = ref(false)
const selected = ref<DepartmentEmployee>()
const targetDept = ref<number>()
const targetWarehouse = ref<number>()
const warehouses = ref<Warehouse[]>([])
const handoverOnly = ref(false)
const groups = ref<HandoverGroup[]>([])
const receivers = ref<Record<string, number>>({})
const targetWarehouseDept = computed(() => departments.value.find(d => d.id === targetDept.value)?.name === '仓库部')
const needsHandover = computed(() => targetDept.value !== Number(selected.value?.department_id || 0))
const canSave = computed(() => previewReady.value && !saving.value &&
  (handoverOnly.value ? groups.value.length > 0 : !!targetDept.value && (!targetWarehouseDept.value || !!targetWarehouse.value)) &&
  (!(handoverOnly.value || needsHandover.value) || groups.value.every(g => receivers.value[g.department])))
let previewRequest = 0

function roleText(code?: string) {
  if (code === 'leader') return '领导'
  if (code === 'admin') return '管理员'
  return '员工'
}

async function loadDepartments() {
  try {
    const res = await getDepartments()
    departments.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
    // 已提示
  }
}

async function loadData() {
  loading.value = true
  try {
    const res = await getAllUsers()
    let users = Array.isArray(res.data) ? res.data : []
    if (filterDept.value) {
      users = users.filter((u) => Number(u.department_id) === filterDept.value)
    }
    list.value = users
  } catch (e) {
    // 已提示
  } finally {
    loading.value = false
  }
}

function selectDepartment() {
  targetWarehouse.value = targetWarehouseDept.value && selected.value?.department === '仓库部'
    ? Number(selected.value.warehouse_id) || undefined : undefined
}

function closeDialog(done: () => void) {
  if (!saving.value) done()
}

async function openAssignment(row: DepartmentEmployee, onlyHandover: boolean) {
  const request = ++previewRequest
  selected.value = row
  handoverOnly.value = onlyHandover
  targetDept.value = Number(row.department_id) || undefined
  selectDepartment()
  receivers.value = {}
  groups.value = []
  visible.value = true
  previewing.value = true
  previewReady.value = false
  try {
    const [preview, warehouseList] = await Promise.all([
      getHandoverPreview(row.id), onlyHandover ? null : getWarehouses({}),
    ])
    if (request !== previewRequest) return
    groups.value = preview.data
    if (warehouseList) warehouses.value = warehouseList.data.filter(w => w.status === 1)
    previewReady.value = true
  } catch {
    // 请求层已提示。
  } finally {
    if (request === previewRequest) previewing.value = false
  }
}

async function saveAssignment() {
  if (!selected.value || !canSave.value) return
  saving.value = true
  try {
    const res = await assignEmployee(selected.value.id, {
      department_id: handoverOnly.value ? undefined : targetDept.value,
      warehouse_id: !handoverOnly.value && targetWarehouseDept.value ? targetWarehouse.value : undefined,
      handover_only: handoverOnly.value,
      handover: handoverOnly.value || needsHandover.value ? receivers.value : {},
    })
    ElMessage.success(res.data.transferred ? `已移交 ${res.data.transferred} 条待办` : '员工调度已保存')
    visible.value = false
    await loadData()
  } catch {
    // 保留选择，以便更正后重试。
  } finally {
    saving.value = false
  }
}

onMounted(() => {
  loadDepartments()
  loadData()
})
</script>

<style scoped>
.toolbar {
  display: flex;
  gap: 12px;
}
</style>
