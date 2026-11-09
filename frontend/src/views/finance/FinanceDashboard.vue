<template>
  <div class="business-dashboard">
    <div class="dashboard-heading"><div><span class="dashboard-eyebrow">企业运营中心</span><h1>财务工作台</h1><p>追踪资金往来，清晰掌握应收与应付。</p></div><span class="dashboard-date">{{ new Date().toLocaleDateString('zh-CN', { month: 'long', day: 'numeric', weekday: 'long' }) }}</span></div>
    <!-- 汇总卡片 -->
    <el-row :gutter="16">
      <el-col :span="6">
        <el-card shadow="hover" class="stat-card gradient-red">
          <div class="stat-icon"><el-icon><Sell /></el-icon></div>
          <div class="stat-label">客户未结应收</div>
          <div class="stat-value">¥{{ fmt(summary.receivable) }}</div>
          <div class="stat-sub">正向账单，不与退货贷项混算</div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover" class="stat-card gradient-orange">
          <div class="stat-icon"><el-icon><ShoppingCart /></el-icon></div>
          <div class="stat-label">供应商未结应付</div>
          <div class="stat-value">¥{{ fmt(summary.payable) }}</div>
          <div class="stat-sub">正向账单，不与退货贷项混算</div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover" class="stat-card gradient-green">
          <div class="stat-icon"><el-icon><Money /></el-icon></div>
          <div class="stat-label">客户退货未处理贷项</div>
          <div class="stat-value">¥{{ fmt(summary.received) }}</div>
          <div class="stat-sub">可退款或抵扣，尚未处理</div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card shadow="hover" class="stat-card gradient-blue">
          <div class="stat-icon"><el-icon><Coin /></el-icon></div>
          <div class="stat-label">供应商退货未处理贷项</div>
          <div class="stat-value">¥{{ fmt(summary.paid) }}</div>
          <div class="stat-sub">可收回或抵扣，尚未处理</div>
        </el-card>
      </el-col>
    </el-row>

    <el-card shadow="never" style="margin-top: 16px">
      <template #header>
        <div class="card-header">
          <span>最近往来账</span>
          <el-button size="small" type="primary" @click="$router.push('/finance/accounts')">查看全部</el-button>
        </div>
      </template>
      <el-table :data="recentAccounts" v-loading="loading" stripe border>
        <el-table-column label="类型" min-width="160">
          <template #default="{ row }">
            <el-tag :type="row.account_type === 'receivable' ? 'danger' : 'warning'">
              {{ Number(row.amount)<0 ? (row.account_type==='receivable'?'客户退货贷项':'供应商退货贷项') : (row.account_type==='receivable'?'客户应收':'供应商应付') }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="biz_no" label="业务单号" min-width="180" />
        <el-table-column prop="partner_name" label="往来单位" min-width="150" />
        <el-table-column label="账目金额(¥)" width="120">
          <template #default="{ row }">{{ fmt(row.amount) }}</template>
        </el-table-column>
        <el-table-column label="应收侧现金已结算" width="110">
          <template #default="{ row }">
            <span v-if="row.account_type === 'receivable'" style="color: #f56c6c">{{ fmt(row.paid_amount) }}</span>
            <span v-else style="color: #c0c4cc">-</span>
          </template>
        </el-table-column>
        <el-table-column label="应付侧现金已结算" width="110">
          <template #default="{ row }">
            <span v-if="row.account_type === 'payable'" style="color: #e6a23c">{{ fmt(row.paid_amount) }}</span>
            <span v-else style="color: #c0c4cc">-</span>
          </template>
        </el-table-column>
        <el-table-column label="未结清(¥)" width="120">
          <template #default="{ row }">
            <span :style="{ color: balance(row) > 0 ? '#f56c6c' : '#67c23a' }">{{ Math.abs(balance(row)).toFixed(2) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="状态" width="90">
          <template #default="{ row }">
            <el-tag :type="row.status === 1 ? 'success' : 'warning'">{{ row.status === 1 ? '已结清' : '未结清' }}</el-tag>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { getAccounts } from '@/api/finance'
import type { Account } from '@/types'

const loading = ref(false)
const accounts = ref<Account[]>([])

const summary = computed(() => {
  let receivable=0,payable=0,received=0,paid=0
  for(const a of accounts.value){
    const value=Math.round((Number(a.amount||0)-Number(a.paid_amount||0)-Number(a.offset_amount||0))*100)/100
    if(Number(a.amount)>=0){if(a.account_type==='receivable')receivable+=Math.max(0,value);else payable+=Math.max(0,value)}
    else {if(a.account_type==='receivable')received+=Math.max(0,-value);else paid+=Math.max(0,-value)}
  }
  return {receivable,payable,received,paid}
})

const recentAccounts = computed(() => accounts.value.slice(0, 10))

function fmt(v?: string | number) {
  const n = Number(v || 0)
  return Math.abs(n).toFixed(2)
}

function balance(row: Account) {
  return Number(row.amount || 0) - Number(row.paid_amount || 0) - Number(row.offset_amount || 0)
}

async function loadData() {
  loading.value = true
  try {
    const res = await getAccounts({})
    accounts.value = Array.isArray(res.data) ? res.data : []
  } catch (e) {
  } finally {
    loading.value = false
  }
}

onMounted(loadData)
</script>

<style scoped>
.stat-card {
  text-align: center;
  color: #fff;
  border: none;
  border-radius: 10px;
  transition: transform 0.2s;
}
.stat-card:hover {
  transform: translateY(-3px);
}
.gradient-red { background: linear-gradient(135deg, #f5576c, #f093fb); }
.gradient-orange { background: linear-gradient(135deg, #f6d365, #fda085); }
.gradient-green { background: linear-gradient(135deg, #43e97b, #38f9d7); }
.gradient-blue { background: linear-gradient(135deg, #4facfe, #00f2fe); }
.stat-icon {
  font-size: 26px;
  opacity: 0.9;
  margin-bottom: 6px;
}
.stat-label { font-size: 13px; opacity: 0.92; margin-bottom: 6px; }
.stat-value { font-size: 26px; font-weight: 700; }
.stat-sub { font-size: 12px; opacity: 0.85; margin-top: 6px; }
.card-header { display: flex; align-items: center; justify-content: space-between; }
</style>
