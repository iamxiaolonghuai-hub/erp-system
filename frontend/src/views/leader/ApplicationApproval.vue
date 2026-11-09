<template>
  <div>
    <el-card shadow="never">
      <el-tabs v-model="activeTab">
        <el-tab-pane label="待审批" name="pending" />
        <el-tab-pane label="已处理" name="done" />
      </el-tabs>

      <el-table :data="filteredList" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="assigned_approver_name" label="指定审批人" min-width="130" />
        <el-table-column prop="application_no" label="申报单号" width="190" />
        <el-table-column label="类型" width="100">
          <template #default="{ row }">
            <el-tag :type="typeTagType(row.type)" size="small">{{ typeText(row.type) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="title" label="申报名称" min-width="160" />
        <el-table-column prop="applicant_name" label="申请人" width="110" />
        <el-table-column label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="statusType(row.status)" size="small">{{ statusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="申报时间" width="180" />
        <el-table-column label="操作" min-width="200" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click="openDetail(row)">详情</el-button>
            <template v-if="row.status === 0">
              <el-button type="success" size="small" @click="handleApprove(row, true)">批准</el-button>
              <el-button type="danger" size="small" @click="openReject(row)">驳回</el-button>
            </template>
          </template>
        </el-table-column>
      </el-table>
      <el-empty v-if="!loading && filteredList.length === 0" description="暂无申报单" />
    </el-card>

    <!-- 详情弹窗 -->
    <el-dialog v-model="detailVisible" title="申报详情" width="600px">
      <template v-if="detail">
        <el-descriptions :column="2" border size="small" style="margin-bottom: 16px">
          <el-descriptions-item label="申报单号">{{ detail.application_no }}</el-descriptions-item>
          <el-descriptions-item label="类型">
            {{ typeText(detail.type) }}申报
          </el-descriptions-item>
          <el-descriptions-item label="申报名称">{{ detail.title }}</el-descriptions-item>
          <el-descriptions-item label="申请人">{{ detail.applicant_name || '-' }}</el-descriptions-item>
          <el-descriptions-item label="状态">
            <el-tag :type="statusType(detail.status)" size="small">{{ statusText(detail.status) }}</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="申报时间">{{ detail.created_at || '-' }}</el-descriptions-item>
          <el-descriptions-item label="审批人">{{ detail.approver_name || '-' }}</el-descriptions-item>
          <el-descriptions-item label="备注" :span="2">{{ detail.remark || '-' }}</el-descriptions-item>
        </el-descriptions>
        <BusinessHistory v-if="detail" :id="detail.id" table="applications" />

        <el-divider content-position="left">申报内容</el-divider>
        <el-descriptions v-if="payloadObj" :column="2" border size="small">
          <el-descriptions-item v-for="(v, k) in payloadObj" :key="k" :label="fieldLabel(k)">
            {{ v }}
          </el-descriptions-item>
        </el-descriptions>
        <el-empty v-else description="无详细内容" />
      </template>
      <template #footer>
        <el-button @click="detailVisible = false">关闭</el-button>
        <template v-if="detail && detail.status === 0">
          <el-button type="success" @click="approveFromDetail(true)">批准</el-button>
          <el-button type="danger" @click="approveFromDetail(false)">驳回</el-button>
        </template>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import BusinessHistory from '@/components/BusinessHistory.vue'
import { ref, computed, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { getApplications, approveApplication } from '@/api/application'
import { useUserStore } from '@/stores/user'
import type { ApplicationItem } from '@/api/application'

const userStore = useUserStore()

const loading = ref(false)
const activeTab = ref('pending')
const list = ref<ApplicationItem[]>([])

const detailVisible = ref(false)
const detail = ref<ApplicationItem | null>(null)

// 采购部领导审批供应商+商品申报，销售部领导审批客户申报
const isPurchaseDept = computed(() => userStore.department === '采购部')
const myTypes = computed(() => (isPurchaseDept.value ? ['supplier', 'product'] : ['customer']))

const filteredList = computed(() => {
  const base = activeTab.value === 'pending'
    ? list.value.filter((x) => (x.status ?? 0) === 0)
    : list.value.filter((x) => (x.status ?? 0) !== 0)
  return base.filter((x) => myTypes.value.includes(x.type))
})

const payloadObj = computed(() => {
  if (!detail.value?.payload) return null
  try {
    return JSON.parse(detail.value.payload) as Record<string, unknown>
  } catch {
    return null
  }
})

function statusText(s?: number) {
  return { 0: '待审批', 1: '已批准', 2: '已驳回' }[s ?? 0] || '未知'
}
function statusType(s?: number) {
  return { 0: 'warning', 1: 'success', 2: 'danger' }[s ?? 0] || 'info'
}

function typeText(t?: string) {
  return { supplier: '供应商', customer: '客户', product: '商品' }[t || ''] || t || '-'
}
function typeTagType(t?: string) {
  return { supplier: 'warning', customer: 'success', product: 'primary' }[t || ''] || 'info'
}

function fieldLabel(key: string) {
  const map: Record<string, string> = {
    contact: '联系人', phone: '电话', email: '邮箱', address: '地址',
    sku: 'SKU', unit: '单位', spec: '规格', category_id: '分类',
    price: '销售价(¥)', cost_price: '成本价(¥)', description: '描述', image_url: '图片',
  }
  return map[key] || key
}

async function loadData() {
  loading.value = true
  try {
    const res = await getApplications({})
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
    // 已提示
  } finally {
    loading.value = false
  }
}

function openDetail(row: ApplicationItem) {
  detail.value = row
  detailVisible.value = true
}

function openReject(row: ApplicationItem) {
  ElMessageBox.prompt('请输入驳回原因', '驳回申报', {
    confirmButtonText: '确定驳回',
    cancelButtonText: '取消',
    inputType: 'textarea',
    inputPlaceholder: '驳回原因',
  })
    .then(({ value }) => {
      doApprove(row, false, value || '')
    })
    .catch(() => {})
}

async function handleApprove(row: ApplicationItem, approve: boolean) {
  await doApprove(row, approve, '')
}

async function approveFromDetail(approve: boolean) {
  if (!detail.value) return
  await doApprove(detail.value, approve, '')
  detailVisible.value = false
}

async function doApprove(row: ApplicationItem, approve: boolean, remark: string) {
  try {
    await approveApplication({ application_id: row.id, approve, remark })
    ElMessage.success(approve ? '已批准' : '已驳回')
    loadData()
  } catch (e) {
    // 已提示
  }
}

onMounted(loadData)
</script>
