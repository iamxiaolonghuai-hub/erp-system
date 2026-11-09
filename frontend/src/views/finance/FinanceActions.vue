<template>
  <el-card shadow="never">
    <el-alert title="经办人与复核人必须不同；复核时重新核对余额，避免重复退款、抵扣或超额收付款。列表最多显示最近1000条。" type="info" :closable="false" />
    <div class="toolbar">
      <el-select v-model="status" style="width:140px" @change="load"><el-option label="全部状态" value="" /><el-option label="待复核" value="0" /><el-option label="已通过" value="1" /><el-option label="已驳回" value="2" /></el-select>
      <el-select v-model="scope" style="width:140px"><el-option label="全部相关" value="all" /><el-option label="我发起的" value="mine" /><el-option label="待我复核" value="pending" /><el-option label="我批准的" value="approved" /></el-select>
      <el-input v-model="search" placeholder="申请号、业务单号、往来单位" clearable style="width:250px" />
      <el-date-picker v-model="dates" type="daterange" value-format="YYYY-MM-DD" start-placeholder="提交开始日期" end-placeholder="提交结束日期" />
      <el-button @click="load">刷新</el-button>
    </div>
    <el-table :data="filtered" v-loading="loading" border stripe>
      <el-table-column prop="action_no" label="申请编号" min-width="190" />
      <el-table-column label="操作" width="100"><template #default="{row}">{{financeKindText[row.kind as FinanceKind]}}</template></el-table-column>
      <el-table-column prop="partner_name" label="往来单位" min-width="130" />
      <el-table-column prop="biz_no" label="业务单号" min-width="180" />
      <el-table-column prop="target_biz_no" label="抵扣目标" min-width="180" />
      <el-table-column label="金额" width="110"><template #default="{row}">{{Number(row.amount).toFixed(2)}}</template></el-table-column>
      <el-table-column prop="created_by_name" label="经办人" width="110" /><el-table-column prop="assigned_approver_name" label="指定复核人" width="120" /><el-table-column prop="approved_by_name" label="实际复核人" width="120" />
      <el-table-column label="状态" width="100"><template #default="{row}">{{['待复核','已通过','已驳回'][row.status]}}{{row.reversed_by?'（已冲正）':''}}</template></el-table-column>
      <el-table-column prop="remark" label="申请原因" min-width="160" /><el-table-column prop="review_remark" label="复核意见" min-width="140" />
      <el-table-column prop="created_at" label="提交时间" width="170" /><el-table-column prop="approved_at" label="复核时间" width="170" />
      <el-table-column label="操作" fixed="right" width="200"><template #default="{row}">
        <template v-if="canReview(row)"><el-button size="small" type="success" :disabled="busy" @click="review(row,true)">通过</el-button><el-button size="small" type="danger" :disabled="busy" @click="review(row,false)">驳回</el-button></template>
        <el-button v-if="row.status===1 && row.kind==='offset' && !row.reversed_by" size="small" @click="form?.open({kind:'reverse',account_id:row.account_id,source_action_id:row.id,amount:Number(row.amount),reference:row.action_no})">申请冲正</el-button>
      </template></el-table-column>
    </el-table>
    <FinanceActionDialog ref="form" @submitted="load" />
  </el-card>
</template>
<script setup lang="ts">
import {computed,onMounted,ref} from 'vue'
import {ElMessage,ElMessageBox} from 'element-plus'
import {getFinanceActions,reviewFinanceAction,financeKindText,type FinanceAction,type FinanceKind} from '@/api/finance'
import {useUserStore} from '@/stores/user'
import FinanceActionDialog from '@/components/FinanceActionDialog.vue'
const user=useUserStore(),loading=ref(false),busy=ref(false),rows=ref<FinanceAction[]>([]),status=ref('0'),scope=ref('all'),search=ref(''),dates=ref<[string,string]>(),form=ref<InstanceType<typeof FinanceActionDialog>>()
const uid=computed(()=>Number(user.userInfo?.id))
function canReview(row:FinanceAction){return row.status===0 && row.created_by!==uid.value && (user.isAdmin || !row.assigned_approver_id || row.assigned_approver_id===uid.value)}
const filtered=computed(()=>rows.value.filter(row=>{
 if(scope.value==='mine' && row.created_by!==uid.value)return false
 if(scope.value==='pending' && !canReview(row))return false
 if(scope.value==='approved' && !(row.status===1 && row.approved_by===uid.value))return false
 const term=search.value.trim().toLowerCase();if(term && ![row.action_no,row.biz_no,row.target_biz_no,row.partner_name].some(v=>String(v||'').toLowerCase().includes(term)))return false
 const day=String(row.created_at||'').slice(0,10);return !dates.value || (day>=dates.value[0] && day<=dates.value[1])
}))
async function load(){loading.value=true;try{rows.value=(await getFinanceActions(status.value?{status:status.value}:{})).data||[]}catch{}finally{loading.value=false}}
async function review(row:FinanceAction,approve:boolean){
 try{const {value}=await ElMessageBox.prompt(approve?'确认复核后按该金额更新账目？':'请填写驳回原因',approve?'财务复核':'驳回申请',{inputType:'textarea',inputValidator:v=>approve || !!v?.trim() || '请填写原因'});busy.value=true;await reviewFinanceAction({action_id:row.id,approve,remark:value||''});ElMessage.success(approve?'已复核生效':'已驳回');await load()}catch{}finally{busy.value=false}
}
onMounted(load)
</script>
<style scoped>.toolbar{display:flex;gap:12px;flex-wrap:wrap;margin:18px 0;align-items:center}</style>
