<template>
  <section class="business-history">
    <h3>处理记录</h3>
    <el-table :data="events" size="small" v-loading="loading" empty-text="该单据暂无可追溯记录（旧记录不会推测补写）">
      <el-table-column prop="created_at" label="时间" min-width="180" />
      <el-table-column prop="operator_name" label="操作人" min-width="100" />
      <el-table-column label="动作" min-width="120"><template #default="{ row }">{{ labels[row.action] || row.action }}</template></el-table-column>
      <el-table-column prop="remark" label="说明" min-width="140" />
    </el-table>
  </section>
</template>
<script setup lang="ts">
import { ref,watch } from 'vue'
import { get } from '@/utils/request'
const props=defineProps<{table:string;id:number}>()
const events=ref<Record<string,any>[]>([]),loading=ref(false)
const labels:Record<string,string>={submit:'发起申请',approve:'批准',reject:'驳回',resubmit:'修改重提',prepare:'备货确认',ship:'放行出库',receive:'收货入库',warehouse_reject:'仓库打回',warehouse_complete:'退换货完成',review:'复核盘点差异',approval_handover:'移交审批待办',register:'登记收付款'}
let version=0
watch(()=>[props.table,props.id],async()=>{const current=++version;events.value=[];if(!props.id)return;loading.value=true;try{const result=await get<Record<string,any>[]>('/approval/history',{document_type:props.table,document_id:props.id});if(current===version)events.value=result.data}catch{}finally{if(current===version)loading.value=false}},{immediate:true})
</script>
<style scoped>
.business-history { margin:24px 0 8px; }.business-history h3 {font-size:14px;color:#475569;margin-bottom:12px;}
</style>
