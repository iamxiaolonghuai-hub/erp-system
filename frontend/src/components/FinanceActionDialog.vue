<template>
  <el-dialog v-model="visible" :title="`${financeKindText[form.kind]}申请`" width="570px" :close-on-click-modal="false" :show-close="!saving">
    <el-alert title="提交后不立即改变账目，须由另一位财务人员或管理员复核通过。此处登记实际发生的资金，不执行银行转账。" type="info" :closable="false" />
    <el-alert v-if="advice" :title="`原单：${advice.source_biz_no || '未关联'}；建议抵扣 ${advice.suggested_offset} 元，建议退款 ${advice.suggested_refund} 元。复核时重新校验。`" type="success" :closable="false" style="margin-top:12px" /><el-form label-width="110px" style="margin-top:20px">
      <el-form-item label="往来账目">
        <el-select v-model="form.account_id" filterable :disabled="fixedAccount" style="width:100%" @change="accountChanged">
          <el-option v-for="a in candidates" :key="a.id" :value="a.id" :label="`${a.partner_name} / ${a.biz_no} / 余额 ${Math.abs(remaining(a)).toFixed(2)}`" />
        </el-select>
      </el-form-item>
      <el-form-item v-if="form.kind === 'offset'" label="抵扣目标" required>
        <el-select v-model="form.target_account_id" filterable style="width:100%" @change="suggestAmount">
          <el-option v-for="a in targets" :key="a.id" :value="a.id" :label="`${a.biz_no} / 未结 ${Math.abs(remaining(a)).toFixed(2)}`" />
        </el-select>
      </el-form-item>
      <el-form-item v-if="reference" label="原记录">{{ reference }}</el-form-item>
      <el-form-item label="金额" required><el-input-number v-model="form.amount" :disabled="form.kind === 'reverse'" :min="0.01" :max="maximum" :precision="2" style="width:100%" /></el-form-item>
      <el-form-item v-if="form.kind !== 'offset'" label="资金方式"><el-input v-model="form.pay_method" maxlength="32" /></el-form-item>
      <el-form-item label="业务日期" required><el-date-picker v-model="form.pay_date" type="date" value-format="YYYY-MM-DD" style="width:100%" /></el-form-item>
      <el-form-item label="指定复核人" required><el-select v-model="form.assigned_approver_id" style="width:100%" placeholder="选择另一位财务人员或管理员"><el-option v-for="u in reviewers" :key="u.id" :value="u.id" :label="u.name" /></el-select></el-form-item>
      <el-form-item label="原因或备注" :required="form.kind !== 'payment'"><el-input v-model="form.remark" type="textarea" :rows="3" maxlength="255" show-word-limit /></el-form-item>
    </el-form>
    <template #footer><el-button :disabled="saving" @click="visible=false">取消</el-button><el-button type="primary" :loading="saving" @click="submit">提交复核</el-button></template>
  </el-dialog>
</template>
<script setup lang="ts">
import { newRequestId } from '@/utils/requestId'
import { get } from '@/utils/request'
import { computed, reactive, ref } from 'vue'
import { ElMessage } from 'element-plus'
import { getAccounts, getFinanceReviewers, submitFinanceAction, financeKindText, type FinanceKind } from '@/api/finance'
import type { Account } from '@/types'
const advice=ref<any>(null)
const emit=defineEmits<{submitted: []}>()
const visible=ref(false),saving=ref(false),fixedAccount=ref(false),reference=ref('')
const accounts=ref<Account[]>([]),reviewers=ref<{id:number;name:string}[]>([])
const form=reactive({kind:'payment' as FinanceKind,account_id:undefined as number|undefined,target_account_id:undefined as number|undefined,payment_id:undefined as number|undefined,source_action_id:undefined as number|undefined,amount:0,pay_method:'银行转账',pay_date:'',remark:'',assigned_approver_id:undefined as number|undefined,request_id:''})
const remaining=(a:Account)=>Number(a.amount||0)-Number(a.paid_amount||0)-Number(a.offset_amount||0)
const selected=computed(()=>accounts.value.find(a=>a.id===form.account_id))
const candidates=computed(()=>accounts.value.filter(a=>form.kind==='reverse'?a.id===form.account_id:form.kind==='payment'?remaining(a)>0:remaining(a)<0))
const targets=computed(()=>accounts.value.filter(a=>selected.value && a.partner_id===selected.value.partner_id && a.account_type===selected.value.account_type && remaining(a)>0))
const maximum=computed(()=>{if(form.kind==='reverse')return form.amount||0.01;let value=Math.abs(selected.value?remaining(selected.value):0);if(form.kind==='offset'){const target=targets.value.find(a=>a.id===form.target_account_id);value=target?Math.min(value,remaining(target)):0}return Math.max(0.01,value)})
function suggestAmount(){form.amount=maximum.value}
async function loadAdvice(){advice.value=null;if((form.kind==='refund'||form.kind==='offset')&&form.account_id){try{advice.value=(await get<any>('/finance/settlement-advice',{account_id:form.account_id})).data;if(form.kind==='offset'&&targets.value.some(a=>a.id===advice.value.source_account_id))form.target_account_id=advice.value.source_account_id}catch{}}}
async function accountChanged(){form.target_account_id=undefined;await loadAdvice();suggestAmount()}
async function open(options:{kind:FinanceKind;account_id?:number;payment_id?:number;source_action_id?:number;amount?:number;reference?:string}){
  try{
    const [a,u]=await Promise.all([getAccounts(),getFinanceReviewers()]);accounts.value=a.data||[];reviewers.value=u.data||[]
    Object.assign(form,{kind:options.kind,account_id:options.account_id,target_account_id:undefined,payment_id:options.payment_id,source_action_id:options.source_action_id,amount:options.amount||0,pay_method:'银行转账',pay_date:new Date().toLocaleDateString('sv-SE'),remark:'',assigned_approver_id:undefined,request_id:newRequestId()})
    fixedAccount.value=!!options.account_id;reference.value=options.reference||'';await loadAdvice();if(options.kind!=='reverse')suggestAmount();visible.value=true
  }catch{/*统一提示*/}
}
async function submit(){
  if(!form.account_id || !form.pay_date || !form.assigned_approver_id || form.amount<=0 || (form.kind==='offset' && !form.target_account_id) || (form.kind!=='payment' && !form.remark.trim())){ElMessage.warning('请完整填写账目、金额、复核人、日期和必要原因');return}
  saving.value=true;try{await submitFinanceAction({...form});ElMessage.success('已提交，等待另一人复核');visible.value=false;emit('submitted')}catch{/*统一提示*/}finally{saving.value=false}
}
defineExpose({open})
</script>
