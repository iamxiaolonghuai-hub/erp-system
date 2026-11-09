<template>
  <el-card shadow="never">
    <el-alert title="应收应付与退货贷项分开展示，金额均为正数。退货贷项可申请退款或抵扣，不代表已经发生现金退款。" type="info" :closable="false" />
    <div class="toolbar"><el-radio-group v-model="filter"><el-radio-button value="all">全部</el-radio-button><el-radio-button value="receivable">应收</el-radio-button><el-radio-button value="payable">应付</el-radio-button><el-radio-button value="credit">退货贷项</el-radio-button></el-radio-group><el-input v-model="search" placeholder="往来单位或业务单号" clearable style="width:240px" /><el-checkbox v-model="unsettled">仅未结清</el-checkbox><el-button @click="loadData">刷新</el-button><el-button type="primary" @click="goActions">财务复核与历史</el-button></div>
    <el-table :data="filtered" v-loading="loading" stripe border>
      <el-table-column prop="biz_no" label="业务单号" min-width="180" /><el-table-column prop="partner_name" label="往来单位" min-width="150" />
      <el-table-column label="类型" min-width="150"><template #default="{row}">{{Number(row.amount)<0?(row.account_type==='receivable'?'客户退货贷项':'供应商退货贷项'):(row.account_type==='receivable'?'客户应收款':'供应商应付款')}}</template></el-table-column>
      <el-table-column label="业务" width="110"><template #default="{row}">{{bizText(row.biz_type)}}</template></el-table-column>
      <el-table-column label="账目金额" width="120"><template #default="{row}">{{fmt(row.amount)}}</template></el-table-column>
      <el-table-column label="现金已结算" width="120"><template #default="{row}">{{fmt(row.paid_amount)}}</template></el-table-column>
      <el-table-column label="抵扣已结算" width="120"><template #default="{row}">{{fmt(row.offset_amount)}}</template></el-table-column>
      <el-table-column label="未结余额" width="120"><template #default="{row}"><span :style="{color:balance(row)!==0?'#e6a23c':'#67c23a'}">{{Math.abs(balance(row)).toFixed(2)}}</span></template></el-table-column>
      <el-table-column label="状态" width="90"><template #default="{row}"><el-tag :type="row.status===1?'success':'warning'">{{row.status===1?'已结清':'未结清'}}</el-tag></template></el-table-column>
      <el-table-column label="操作" width="250" fixed="right"><template #default="{row}">
        <el-button v-if="Number(row.amount)>0" size="small" type="primary" :disabled="row.status===1" @click="form?.open({kind:'payment',account_id:row.id})">{{row.account_type==='receivable'?'申请收款':'申请付款'}}</el-button>
        <template v-else><el-button size="small" :disabled="row.status===1" @click="form?.open({kind:'refund',account_id:row.id})">{{row.account_type==='receivable'?'退给客户':'供应商退款'}}</el-button><el-button size="small" :disabled="row.status===1" @click="form?.open({kind:'offset',account_id:row.id})">抵扣</el-button></template>
        <el-button size="small" @click="openDetail(row)">详情</el-button>
      </template></el-table-column>
    </el-table>
    <FinanceActionDialog ref="form" @submitted="loadData" />
    <el-dialog v-model="detailVisible" title="账目结算记录" width="1000px">
      <p>业务单号：{{detail?.biz_no}}　现金与抵扣分开记录；冲正保留原记录。</p>
      <h4>现金收付及冲正</h4>
      <el-table :data="payments" border max-height="250"><el-table-column prop="payment_no" label="单号" min-width="180" /><el-table-column label="资金方向" width="100"><template #default="{row}">{{row.payment_type==='receive'?'收入':'支出'}}{{Number(row.amount)<0?'冲正':''}}</template></el-table-column><el-table-column prop="amount" label="金额" width="100" /><el-table-column prop="created_by_name" label="经办人" width="110" /><el-table-column prop="approved_by_name" label="复核人" width="110" /><el-table-column prop="pay_date" label="日期" width="110" /><el-table-column prop="remark" label="备注" min-width="150" /></el-table>
      <h4>申请及抵扣记录</h4>
      <el-table :data="actions" border max-height="250"><el-table-column prop="action_no" label="申请号" min-width="180" /><el-table-column label="操作" width="100"><template #default="{row}">{{financeKindText[row.kind as FinanceKind]}}</template></el-table-column><el-table-column prop="amount" label="金额" width="100" /><el-table-column label="状态" width="100"><template #default="{row}">{{['待复核','已通过','已驳回'][row.status]}}</template></el-table-column><el-table-column prop="created_by_name" label="经办人" width="110" /><el-table-column prop="approved_by_name" label="复核人" width="110" /><el-table-column prop="remark" label="原因" min-width="150" /></el-table>
    </el-dialog>
  </el-card>
</template>
<script setup lang="ts">
import {computed,onMounted,ref} from 'vue'
import {useRouter} from 'vue-router'
import {getAccounts,getPayments,getFinanceActions,financeKindText,type FinanceAction,type FinanceKind} from '@/api/finance'
import type {Account,Payment} from '@/types'
import {useUserStore} from '@/stores/user'
import FinanceActionDialog from '@/components/FinanceActionDialog.vue'
const router=useRouter(),user=useUserStore(),loading=ref(false),list=ref<Account[]>([]),filter=ref('all'),search=ref(''),unsettled=ref(false),form=ref<InstanceType<typeof FinanceActionDialog>>()
const detailVisible=ref(false),detail=ref<Account>(),payments=ref<Payment[]>([]),actions=ref<FinanceAction[]>([])
const fmt=(value:unknown)=>Math.abs(Number(value||0)).toFixed(2)
const balance=(row:Account)=>Math.round((Number(row.amount||0)-Number(row.paid_amount||0)-Number(row.offset_amount||0))*100)/100
const bizText=(value:string)=>({purchase:'采购',sale:'销售',purchase_return:'采购退货',sales_return:'销售退货'}[value]||value)
const filtered=computed(()=>list.value.filter(row=>(filter.value==='all'||(filter.value==='credit'?Number(row.amount)<0:Number(row.amount)>=0&&row.account_type===filter.value))&&(!unsettled.value||row.status!==1)&&(!search.value||`${row.partner_name} ${row.biz_no}`.toLowerCase().includes(search.value.trim().toLowerCase()))))
async function loadData(){loading.value=true;try{list.value=(await getAccounts()).data||[]}catch{}finally{loading.value=false}}
function goActions(){router.push(user.isAdmin?'/finance/actions-admin':'/finance/actions')}
async function openDetail(row:Account){detail.value=row;payments.value=[];actions.value=[];detailVisible.value=true;try{const [p,a]=await Promise.all([getPayments({filter:JSON.stringify({account_id:row.id})}),getFinanceActions({account_id:row.id})]);payments.value=p.data||[];actions.value=a.data||[]}catch{}}
onMounted(loadData)
</script>
<style scoped>.toolbar{display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin:18px 0}</style>
