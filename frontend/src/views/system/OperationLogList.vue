<template>
  <div>
    <el-card shadow="never">
      <div class="toolbar">
        <el-button type="primary" @click="loadData">
          <el-icon><Refresh /></el-icon>
          刷新
        </el-button>
      </div>

      <el-table :data="list" v-loading="loading" stripe border style="margin-top: 16px">
        <el-table-column prop="id" label="ID" width="70" />
        <el-table-column prop="username" label="操作人" width="130" />
        <el-table-column prop="method" label="方法" width="90" />
        <el-table-column prop="path" label="请求路径" min-width="200" />
        <el-table-column prop="ip" label="来源IP" width="140" />
        <el-table-column prop="status_code" label="状态码" width="90" />
        <el-table-column prop="cost_ms" label="耗时(ms)" width="100" />
        <el-table-column prop="created_at" label="时间" width="170" />
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { getOperationLogs } from '@/api/system'
import type { OperationLog } from '@/types'

const loading = ref(false)
const list = ref<OperationLog[]>([])

async function loadData() {
  loading.value = true
  try {
    const res = await getOperationLogs({})
    list.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    loading.value = false
  }
}

onMounted(loadData)
</script>

<style scoped>
.toolbar { display: flex; gap: 12px; }
</style>
