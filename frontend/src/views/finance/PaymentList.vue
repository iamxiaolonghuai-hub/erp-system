<template>
  <el-card shadow="never">
    <el-alert title="只显示已经复核生效的现金收付。冲正保留原记录并增加反向记录；贷项抵扣请在财务复核页面查看。" type="info" :closable="false" />
    <div class="toolbar"><el-button type="primary" @click="form?.open({kind:'payment'})">申请收付款</el-button><el-button @click="load">刷新</el-button><el-input v-model="search" placeholder="单号、单位或经办人" style="width:260px" clearable /><el-date-picker v-model="dates" type="daterange" value-format="YYYY-MM-DD" start-placeholder="业务开始日期" end-placeholder="业务结束日期" /></div>
    <el-table :data="filtered" v-loading="loading" border stripe>
      <el-table-column prop="payment_no" label="收付单号" min-width="180" /><el-table-column prop="biz_no" label="业务单号" min-width="180" /><el-table-column prop="partner_name" label="往来单位" min-width="130" />
      <el-table-column label="资金方向" width="110"><template #default="{row}">{{row.payment_type==='receive'?'收入':'支出'}}{{Number(row.amount)<0?'冲正':''}}</template></el-table-column>
      <el-table-column label="金额" width="110"><template #default="{row}">{{Number(row.amount).toFixed(2)}}</template></el-table-column>
      <el-table-column prop="pay_method" label="方式" width="100" /><el-table-column prop="pay_date" label="日期" width="110" /><el-table-column prop="created_by_name" label="经办人" width="110" /><el-table-column prop="approved_by_name" label="复核人" width="110" /><el-table-column prop="remark" label="备注" min-width="150" />
      <el-table-column label="操作" fixed="right" width="120"><template #default="{row}"><el-button v-if="!row.reversal_of && !row.reversed_by && Number(row.amount)>0" size="small" @click="form?.open({kind:'reverse',account_id:row.account_id,payment_id:row.id,amount:Number(row.amount),reference:row.payment_no})">申请冲正</el-button><span v-else>{{row.reversed_by?'已冲正':'冲正记录'}}</span></template></el-table-column>
    </el-table>
    <FinanceActionDialog ref="form" @submitted="load" />
  </el-card>
</template>
<script setup lang="ts">
import {computed,onMounted,ref} from 'vue'
import {getPayments} from '@/api/finance'
import type {Payment} from '@/types'
import FinanceActionDialog from '@/components/FinanceActionDialog.vue'
const rows=ref<Payment[]>([]),loading=ref(false),search=ref(''),dates=ref<[string,string]>(),form=ref<InstanceType<typeof FinanceActionDialog>>()
const filtered=computed(()=>rows.value.filter(row=>{const term=search.value.trim().toLowerCase();const matches=!term||[row.payment_no,row.biz_no,row.partner_name,row.created_by_name].some(v=>String(v||'').toLowerCase().includes(term));const day=String(row.pay_date||'').slice(0,10);return matches&&(!dates.value||(day>=dates.value[0]&&day<=dates.value[1]))}))
async function load(){loading.value=true;try{rows.value=(await getPayments()).data||[]}catch{}finally{loading.value=false}}
onMounted(load)
</script>
<style scoped>.toolbar{display:flex;gap:12px;flex-wrap:wrap;align-items:center;margin:18px 0}</style>
