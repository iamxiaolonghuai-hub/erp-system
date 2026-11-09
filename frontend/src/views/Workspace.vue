<template>
  <div class="business-dashboard">
    <div class="dashboard-heading"><div><span class="dashboard-eyebrow">企业运营中心</span><h1>我的工作台</h1><p>掌握部门进度，让每一笔业务都有条不紊。</p></div><span class="dashboard-date">{{ new Date().toLocaleDateString('zh-CN', { month: 'long', day: 'numeric', weekday: 'long' }) }}</span></div>
    <!-- 欢迎卡片 -->
    <el-card shadow="never" class="welcome-card">
      <div class="welcome-title">
        欢迎，{{ userStore.userInfo?.real_name || userStore.userInfo?.username }}
      </div>
      <div class="welcome-desc">
        当前角色：<el-tag size="small" :type="roleTagType">{{ roleText }}</el-tag>
        <el-tag v-if="userStore.department" size="small" type="info" style="margin-left: 8px">
          {{ userStore.department }}
        </el-tag>
        <el-tag v-if="isWarehouseStaff && warehouseName" size="small" type="warning" style="margin-left: 8px">
          所属仓库：{{ warehouseName }}
        </el-tag>
      </div>
      <el-divider />
      <div class="welcome-text">
        <p v-if="userStore.isAdmin">您拥有全部权限，可前往「完整后台」进行系统管理。</p>
        <p v-else-if="userStore.isLeader">您是{{ userStore.department }}领导，可管理本部门员工并查看本部门仪表盘。</p>
        <p v-else-if="isWarehouseStaff">
          {{ warehouseName ? `您负责「${warehouseName}」的出入库与库存管理，以下统计均按本仓库过滤。` : '尚未分配所属仓库，请联系管理员在「员工管理」中分配。' }}
        </p>
        <p v-else>您是{{ userStore.department }}员工，可进行本部门相关日常操作。</p>
      </div>
      <el-button v-if="userStore.isAdmin" type="primary" plain @click="goAdmin">
        前往完整后台
      </el-button>
    </el-card>

    <!-- 部门仪表盘（领导/员工可见） -->
    <el-card shadow="never" v-loading="dashboardLoading" style="margin-top: 16px">
      <template #header>
        <span style="font-weight: 600">{{ isWarehouseStaff ? '我的仓库工作台' : '部门仪表盘' }}</span>
      </template>
      <el-row :gutter="16">
        <el-col :span="isWarehouseStaff ? 6 : 4" v-for="stat in dashboardStats" :key="stat.label">
          <div class="stat-card" :style="statStyle(stat.color)">
            <div class="stat-value">{{ stat.value }}</div>
            <div class="stat-label">{{ stat.label }}</div>
            <div v-if="stat.sub" class="stat-sub">{{ stat.sub }}</div>
          </div>
        </el-col>
      </el-row>
      <el-empty v-if="!dashboardLoading && dashboardStats.length === 0" description="暂无数据" />
    </el-card>

    <!-- 业务趋势折线图（今日 + 日/周/月/年） -->
    <el-card shadow="never" style="margin-top: 16px">
      <template #header>
        <div class="chart-header">
          <span style="font-weight: 600">
            业务趋势
            <el-tag v-if="todayText" size="small" type="success" style="margin-left: 8px">
              {{ todayText }}
            </el-tag>
          </span>
          <el-radio-group v-model="period" size="small" @change="loadTrend">
            <el-radio-button value="day">日</el-radio-button>
            <el-radio-button value="week">周</el-radio-button>
            <el-radio-button value="month">月</el-radio-button>
            <el-radio-button value="year">年</el-radio-button>
          </el-radio-group>
        </div>
      </template>
      <div ref="chartRef" v-loading="trendLoading" style="width: 100%; height: 320px"></div>
    </el-card>

    <!-- 售后趋势折线图（销售员工：售后单数/售后金额） -->
    <el-card v-if="isSaleStaff" shadow="never" style="margin-top: 16px">
      <template #header>
        <div class="chart-header">
          <span style="font-weight: 600">
            售后趋势
            <el-tag v-if="todayAfterText" size="small" type="warning" style="margin-left: 8px">
              {{ todayAfterText }}
            </el-tag>
          </span>
          <el-radio-group v-model="period" size="small" @change="loadTrend">
            <el-radio-button value="day">日</el-radio-button>
            <el-radio-button value="week">周</el-radio-button>
            <el-radio-button value="month">月</el-radio-button>
            <el-radio-button value="year">年</el-radio-button>
          </el-radio-group>
        </div>
      </template>
      <div ref="chartAfterRef" v-loading="trendLoading" style="width: 100%; height: 280px"></div>
    </el-card>

    <!-- 仓库部员工：我的出入库申请分布 -->
    <el-card v-if="isWarehouseStaff" shadow="never" style="margin-top: 16px">
      <template #header>
        <span style="font-weight: 600">我的出入库申请</span>
      </template>
      <el-row :gutter="16">
        <el-col :span="8">
          <div class="mini-stat">
            <div class="mini-value" style="color: #409eff">{{ myOpsIn }}</div>
            <div class="mini-label">入库申请</div>
          </div>
        </el-col>
        <el-col :span="8">
          <div class="mini-stat">
            <div class="mini-value" style="color: #e6a23c">{{ myOpsOut }}</div>
            <div class="mini-label">出库申请</div>
          </div>
        </el-col>
        <el-col :span="8">
          <div class="mini-stat">
            <div class="mini-value" style="color: #67c23a">{{ myOpsTransfer }}</div>
            <div class="mini-label">调拨申请</div>
          </div>
        </el-col>
      </el-row>
      <el-table v-if="myOpsRecent.length > 0" :data="myOpsRecent" stripe border size="small" style="margin-top: 12px">
        <el-table-column prop="operation_no" label="单号" min-width="170" />
        <el-table-column label="类型" width="80" align="center">
          <template #default="{ row }">
            <el-tag :type="opsTypeTag(row.type)" size="small">{{ opsTypeText(row.type) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="product_name" label="商品" min-width="120" />
        <el-table-column prop="qty" label="数量" width="70" align="center" />
        <el-table-column label="仓库" min-width="140">
          <template #default="{ row }">
            {{ row.type === 'transfer' ? `${row.from_warehouse} → ${row.to_warehouse}` : (row.to_warehouse || row.from_warehouse) }}
          </template>
        </el-table-column>
        <el-table-column label="状态" width="90" align="center">
          <template #default="{ row }">
            <el-tag :type="opsStatusTag(row.status)" size="small">{{ opsStatusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="时间" width="160" />
      </el-table>
      <el-empty v-else description="暂无出入库申请" :image-size="60" />
    </el-card>

    <!-- 销售部员工：最近我的销售单 -->
    <el-card v-if="isSaleStaff && recentSales.length > 0" shadow="never" style="margin-top: 16px">
      <template #header>
        <span style="font-weight: 600">最近我的销售单</span>
      </template>
      <el-table :data="recentSales" stripe border size="small">
        <el-table-column prop="order_no" label="单号" min-width="180" />
        <el-table-column prop="customer_name" label="客户" min-width="120" />
        <el-table-column label="金额(¥)" width="130" align="right">
          <template #default="{ row }">{{ Number(row.total_amount).toFixed(2) }}</template>
        </el-table-column>
        <el-table-column label="状态" width="110" align="center">
          <template #default="{ row }">
            <el-tag :type="statusTag(row.status)" size="small">{{ statusText(row.status) }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="created_at" label="创建时间" width="170" />
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, onMounted, onBeforeUnmount, nextTick } from 'vue'
import { useRouter } from 'vue-router'
import * as echarts from 'echarts'
import { useUserStore } from '@/stores/user'
import { getDepartmentDashboard, getDepartmentTrend } from '@/api/department'

const router = useRouter()
const userStore = useUserStore()

const isWarehouseStaff = computed(
  () => userStore.department === '仓库部' && !userStore.isAdmin && !userStore.isLeader,
)
const isSaleStaff = computed(
  () => userStore.department === '销售部' && !userStore.isAdmin && !userStore.isLeader,
)

const roleText = computed(() => {
  if (userStore.isAdmin) return '管理员'
  if (userStore.isLeader) return '领导'
  return '员工'
})
const roleTagType = computed(() => {
  if (userStore.isAdmin) return 'danger'
  if (userStore.isLeader) return 'warning'
  return 'success'
})

const dashboardLoading = ref(false)
let dashboardTimer: ReturnType<typeof setInterval> | undefined
const warehouseName = ref('')
const myOpsIn = ref(0)
const myOpsOut = ref(0)
const myOpsTransfer = ref(0)
const myOpsRecent = ref<Record<string, unknown>[]>([])
const recentSales = ref<Record<string, unknown>[]>([])
const dashboardStats = ref<{ label: string; value: string | number; sub?: string; color?: string }[]>([])

// 趋势折线图
const trendLoading = ref(false)
const period = ref('day')
const todayText = ref('')
const todayAfterText = ref('')
const chartRef = ref<HTMLElement | null>(null)
const chartAfterRef = ref<HTMLElement | null>(null)
let chart: echarts.ECharts | null = null
let afterChart: echarts.ECharts | null = null

const COLOR = ['#409eff', '#67c23a', '#e6a23c', '#f56c6c']

function statStyle(color?: string) {
  const c = color || '#409eff'
  return { background: `linear-gradient(135deg, ${c}, ${c}cc)`, color: '#fff', border: 'none' }
}

function opsTypeText(t: string) {
  return { in: '入库', out: '出库', transfer: '调拨' }[t] || t
}
function opsTypeTag(t: string) {
  return { in: 'success', out: 'warning', transfer: 'primary' }[t] || 'info'
}
function opsStatusText(s: number | string) {
  return { 0: '待审批', 1: '已批准', 2: '已驳回' }[Number(s)] ?? '未知'
}
function opsStatusTag(s: number | string) {
  return { 0: 'warning', 1: 'success', 2: 'danger' }[Number(s)] ?? 'info'
}
function statusText(s: number | string) {
  const map: Record<number, string> = {
    0: '待销售审批', 1: '待出库', 2: '已驳回', 3: '已完成', 4: '打回待修改', 5: '待领导批准',
  }
  return map[Number(s)] ?? '未知'
}
function statusTag(s: number | string) {
  const map: Record<number, string> = {
    0: 'warning', 1: 'primary', 2: 'danger', 3: 'success', 4: 'info', 5: 'warning',
  }
  return map[Number(s)] ?? 'info'
}

async function loadDashboard() {
  if (dashboardLoading.value) return
  dashboardLoading.value = true
  try {
    const res = await getDepartmentDashboard()
    const d = (res.data || {}) as Record<string, unknown>
    const stats: { label: string; value: string | number; sub?: string; color?: string }[] = []
    const dept = userStore.department

    if (isWarehouseStaff.value) {
      warehouseName.value = (d.warehouse_name as string) || ''
      const hasWh = Boolean(d.has_warehouse)
      if (hasWh) {
        stats.push({ label: '本仓库库存品种', value: Number(d.inventory_count || 0), color: COLOR[0], sub: warehouseName.value })
        stats.push({ label: '本仓库库存总量', value: Number(d.total_qty || 0), color: COLOR[1], sub: '件' })
        stats.push({ label: '安全库存预警', value: Number(d.low_stock || 0), color: '#f56c6c', sub: '本仓库达到或低于安全库存' })
        stats.push({ label: '待确认出库单', value: Number(d.pending_confirm || 0), color: COLOR[2], sub: '销售单待出库' })
        stats.push({ label: '我的出入库申请', value: Number(d.my_operations || 0), color: COLOR[3], sub: `待审批 ${Number(d.my_pending_ops || 0)} 笔` })
      }
      myOpsIn.value = Number(d.my_ops_in || 0)
      myOpsOut.value = Number(d.my_ops_out || 0)
      myOpsTransfer.value = Number(d.my_ops_transfer || 0)
      myOpsRecent.value = Array.isArray(d.my_ops_recent) ? d.my_ops_recent : []
    } else if (isSaleStaff.value) {
      // 顶部：今日 / 本周 / 当年售出金额（本人，时间维度非总金额）
      stats.push({ label: '今日售出(¥)', value: Number(d.today_amount || 0).toFixed(2), color: COLOR[0], sub: '本人今日' })
      stats.push({ label: '本周售出(¥)', value: Number(d.week_amount || 0).toFixed(2), color: COLOR[1], sub: '本人本周' })
      stats.push({ label: '当年售出(¥)', value: Number(d.year_amount || 0).toFixed(2), color: COLOR[2], sub: '本人当年' })
      stats.push({ label: '待领导审批', value: Number(d.pending_count || 0), color: COLOR[3], sub: '我的待审批单' })
      recentSales.value = Array.isArray(d.recent_sales) ? d.recent_sales : []
    } else {
      if (d.employee_count !== undefined) {
        stats.push({ label: '部门员工数', value: Number(d.employee_count) })
      }
      if (dept === '销售部' || dept === '采购部') {
        stats.push({ label: '订单数', value: Number(d.order_count || 0) })
        stats.push({ label: '总金额(¥)', value: Number(d.total_amount || 0).toFixed(2) })
      } else if (dept === '仓库部') {
        stats.push({ label: '库存品种数', value: Number(d.inventory_count || 0) })
        stats.push({ label: '库存总量', value: Number(d.total_qty || 0) })
        stats.push({ label: '安全库存预警', value: Number(d.low_stock || 0) })
        stats.push({ label: '待确认出库单', value: Number(d.pending_confirm || 0) })
      } else if (dept === '财务部') {
        stats.push({ label: '应收(¥)', value: Number(d.receivable || 0).toFixed(2) })
        stats.push({ label: '应付(¥)', value: Number(d.payable || 0).toFixed(2) })
      }
    }
    dashboardStats.value = stats
  } catch (e) {
    // 已提示
  } finally {
    dashboardLoading.value = false
  }
}

function goAdmin() {
  router.push('/dashboard')
}

// 序列单位：按名称判断（金额→元、单数→单、件数→件），其余用接口 unit
function seriesUnit(name: string, unit: string) {
  if (name.includes('金额')) return '元'
  if (name.includes('单数')) return '单'
  if (name.includes('件数')) return '件'
  return unit
}

// ===== 业务趋势折线图 =====
async function loadTrend() {
  trendLoading.value = true
  try {
    const res = await getDepartmentTrend(undefined, period.value)
    const d = res.data || { labels: [], series: [], values: [], unit: '' }
    await nextTick()
    const labels = d.labels || []
    const unit = d.unit || (isWarehouseStaff.value ? '数量(件)' : '金额(¥)')
    const series = (d.series && d.series.length > 0 ? d.series : [{ name: '业务', values: d.values || [] }]) as { name: string; values: number[] }[]

    // 售后序列独立成第二张图（销售部返回售后序列）
    const afterSeries = series.filter((s) => s.name.includes('售后'))
    const bizSeries = series.filter((s) => !s.name.includes('售后'))

    renderTrendTo(chartRef.value, labels, bizSeries, unit, chart)
    if (afterSeries.length > 0) {
      renderTrendTo(chartAfterRef.value, labels, afterSeries, unit, afterChart)
    }

    // 今日数据：day 周期取最后一天（今天）各序列合计
    if (period.value === 'day' && labels.length > 0) {
      const idx = labels.length - 1
      todayText.value = bizSeries.length > 0
        ? `今日 ${bizSeries.map((s) => `${s.name} ${Number(s.values?.[idx] || 0).toFixed(0)}${seriesUnit(s.name, unit)}`).join(' / ')}`
        : ''
      todayAfterText.value = afterSeries.length > 0
        ? `今日 ${afterSeries.map((s) => `${s.name} ${Number(s.values?.[idx] || 0).toFixed(0)}${seriesUnit(s.name, unit)}`).join(' / ')}`
        : ''
    } else {
      todayText.value = ''
      todayAfterText.value = ''
    }
  } catch (e) {
    // 已提示
  } finally {
    trendLoading.value = false
  }
}

function renderTrendTo(
  el: HTMLElement | null,
  labels: string[],
  series: { name: string; values: number[] }[],
  unit: string,
  inst: echarts.ECharts | null,
) {
  if (!el) return
  const target = inst ?? echarts.init(el)
  if (inst == null) {
    if (el === chartRef.value) chart = target
    else if (el === chartAfterRef.value) afterChart = target
  }
  const colors = ['#409eff', '#67c23a', '#e6a23c', '#f56c6c', '#909399']
  target.setOption({
    tooltip: {
      trigger: 'axis',
      formatter: (params: any) => {
        const arr = Array.isArray(params) ? params : [params]
        let html = `${arr[0].axisValue}<br/>`
        arr.forEach((p: any) => {
          html += `${p.marker}${p.seriesName}：${Number(p.value).toFixed(2)} ${seriesUnit(p.seriesName, unit)}<br/>`
        })
        return html
      },
    },
    legend: series.length > 1 ? { top: 0, data: series.map((s) => s.name) } : undefined,
    grid: { left: 60, right: 30, top: series.length > 1 ? 40 : 30, bottom: 40 },
    xAxis: {
      type: 'category',
      data: labels,
      boundaryGap: false,
      axisLabel: { rotate: labels.length > 20 ? 45 : 0 },
    },
    yAxis: {
      type: 'value',
      name: unit,
      splitLine: { lineStyle: { type: 'dashed' } },
    },
    series: series.map((s, i) => ({
      name: s.name,
      type: 'line',
      smooth: true,
      data: s.values,
      symbol: 'circle',
      symbolSize: 6,
      lineStyle: { width: 3, color: colors[i % colors.length] },
      itemStyle: { color: colors[i % colors.length] },
      areaStyle: i === 0
        ? { color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: 'rgba(64,158,255,0.3)' },
            { offset: 1, color: 'rgba(64,158,255,0.02)' },
          ]) }
        : undefined,
    })),
  })
}

function handleResize() {
  chart?.resize()
  afterChart?.resize()
}

onMounted(() => {
  dashboardTimer = setInterval(loadDashboard, 15000)
  window.addEventListener('focus', loadDashboard)
  loadDashboard()
  loadTrend()
  window.addEventListener('resize', handleResize)
})

onBeforeUnmount(() => {
  if (dashboardTimer) clearInterval(dashboardTimer)
  window.removeEventListener('focus', loadDashboard)
  window.removeEventListener('resize', handleResize)
  chart?.dispose()
  afterChart?.dispose()
  chart = null
  afterChart = null
})
</script>

<style scoped>
.welcome-card {
  margin-bottom: 16px;
}
.welcome-title {
  font-size: 20px;
  font-weight: 600;
  color: #333;
}
.welcome-desc {
  margin-top: 8px;
  color: #666;
}
.welcome-text {
  color: #666;
  line-height: 1.8;
}
.stat-card {
  text-align: center;
  padding: 20px 0;
  border-radius: 10px;
  color: #fff;
  transition: transform 0.2s;
}
.stat-card:hover {
  transform: translateY(-3px);
}
.stat-value {
  font-size: 28px;
  font-weight: 700;
}
.stat-label {
  margin-top: 8px;
  font-size: 13px;
  opacity: 0.92;
}
.stat-sub {
  margin-top: 6px;
  font-size: 12px;
  opacity: 0.85;
}
.mini-stat {
  text-align: center;
  padding: 18px 0;
  background: #f7f8fa;
  border-radius: 10px;
  border: 1px solid #ebeef5;
}
.mini-value {
  font-size: 26px;
  font-weight: 700;
}
.mini-label {
  margin-top: 6px;
  font-size: 13px;
  color: #606266;
}
.chart-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
</style>
