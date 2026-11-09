<template>
  <span class="partner-tools">
    <el-button @click="open">导入 CSV</el-button>
    <el-button :loading="exporting" @click="download">导出 CSV</el-button>
    <el-dialog v-model="visible" :title="`导入${label}`" width="720px" :close-on-click-modal="false" :before-close="close">
      <el-alert type="info" :closable="false" show-icon>
        按名称自动去重，忽略首尾空格和英文大小写。重复行跳过，不覆盖已有资料。支持 CSV，最多5000条、5MB。
      </el-alert>
      <div class="file-actions">
        <el-button :disabled="importing" @click="fileInput?.click()">选择 CSV 文件</el-button>
        <el-button link type="primary" @click="template">下载模板</el-button>
        <span>{{ filename || '尚未选择文件' }}</span>
        <input ref="fileInput" type="file" accept=".csv" hidden @change="selectFile" />
      </div>
      <p>名称必填；状态填1（启用）或0（停用），留空默认为启用。Excel 请另存为 CSV 后上传。</p>
      <template v-if="result">
        <el-alert :type="result.failed ? 'warning' : 'success'" :closable="false"
          :title="`新增 ${result.success} 条，重复跳过 ${result.skipped || 0} 条，失败 ${result.failed} 条`" />
        <el-table v-if="result.issues?.length" :data="result.issues" max-height="280" style="margin-top:12px">
          <el-table-column prop="row" label="文件行号" width="90" />
          <el-table-column prop="name" label="名称" min-width="150" />
          <el-table-column prop="message" label="处理说明" min-width="240" />
        </el-table>
        <p v-if="result.issues_truncated">仅显示前100条处理说明。</p>
      </template>
      <template #footer>
        <el-button :disabled="importing" @click="visible = false">关闭</el-button>
        <el-button type="primary" :loading="importing" :disabled="!csv || !!result" @click="submit">开始导入</el-button>
      </template>
    </el-dialog>
  </span>
</template>
<script setup lang="ts">
import { ref } from 'vue'
import { ElMessage } from 'element-plus'
import { exportCsv, importCsv, type ImportResult } from '@/api/importexport'
const props = defineProps<{ table: 'suppliers' | 'customers'; label: string }>()
const emit = defineEmits<{ imported: [] }>()
const visible = ref(false), importing = ref(false), exporting = ref(false)
const filename = ref(''), csv = ref(''), fileInput = ref<HTMLInputElement>()
const result = ref<ImportResult>()
function open() { visible.value = true; filename.value = ''; csv.value = ''; result.value = undefined }
function close(done: () => void) { if (!importing.value) done() }
async function selectFile(event: Event) {
  const input = event.target as HTMLInputElement, file = input.files?.[0]
  input.value = ''; csv.value = ''; filename.value = ''; result.value = undefined
  if (!file) return
  if (!/\.csv$/i.test(file.name) || file.size > 5 * 1024 * 1024) { ElMessage.error('请选择5MB以内的 CSV 文件'); return }
  try {
    const bytes = await file.arrayBuffer()
    let text: string
    try { text = new TextDecoder('utf-8', { fatal: true }).decode(bytes) }
    catch { text = new TextDecoder('gb18030', { fatal: true }).decode(bytes) }
    if (!text.trim()) throw new Error('文件内容为空')
    csv.value = text; filename.value = file.name
  } catch { ElMessage.error('文件无法读取，请另存为 UTF-8 CSV 后重试') }
}
async function submit() {
  importing.value = true
  try { result.value = (await importCsv(props.table, csv.value)).data; emit('imported') }
  catch { /* 请求拦截器显示错误 */ }
  finally { importing.value = false }
}
async function download() {
  exporting.value = true
  try { await exportCsv(props.table) } catch { ElMessage.error('导出失败，请重试') }
  finally { exporting.value = false }
}
function template() {
  const url = URL.createObjectURL(new Blob(['\uFEFF' + `${props.label}名称,联系人,电话,邮箱,地址,状态,备注\r\n`], { type: 'text/csv;charset=utf-8' }))
  const link = document.createElement('a'); link.href = url; link.download = `${props.label}导入模板.csv`; link.click(); URL.revokeObjectURL(url)
}
</script>
<style scoped>
.partner-tools { display: inline-flex; gap: 8px; }
.file-actions { display: flex; gap: 12px; align-items: center; margin-top: 20px; flex-wrap: wrap; }
p { color: #64748b; font-size: 13px; }
</style>
