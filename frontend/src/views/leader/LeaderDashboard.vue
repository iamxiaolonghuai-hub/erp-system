<template>
  <div class="business-dashboard">
    <div class="dashboard-heading"><div><span class="dashboard-eyebrow">企业运营中心</span><h1>部门经营概览</h1><p>聚焦业务趋势、团队表现与待处理事项。</p></div><span class="dashboard-date">{{ new Date().toLocaleDateString('zh-CN', { month: 'long', day: 'numeric', weekday: 'long' }) }}</span></div>
    <!-- 概览统计卡片 -->
    <el-row :gutter="20" v-loading="dashboardLoading">
      <el-col :span="dept === '销售部' ? 4 : 6" v-for="stat in dashboardStats" :key="stat.label">
        <div class="grad-card" :style="{ background: cardBg(dashboardStats.indexOf(stat)) }">
          <div class="grad-value">{{ stat.value }}</div>
          <div class="grad-label">{{ stat.label }}</div>
        </div>
      </el-col>
    </el-row>

    <!-- 业务趋势折线图（销售：售卖金额/单数/件数） -->
    <el-card shadow="never" style="margin-top: 16px">
      <template #header>
        <div class="chart-header">
          <span style="font-weight: 600">
            {{ trendTitle }}
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
      <div ref="chartRef" v-loading="trendLoading" style="width: 100%; height: 340px"></div>
    </el-card>

    <!-- 售后趋势折线图（销售部：售后单数/售后金额） -->
    <el-card v-if="dept === '销售部'" shadow="never" style="margin-top: 16px">
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
      <div ref="chartAfterRef" v-loading="trendLoading" style="width: 100%; height: 300px"></div>
    </el-card>

    <!-- 销售部领导：员工销售数据（部门全量按员工分组） -->
    <el-card v-if="dept === '销售部'" shadow="never" style="margin-top: 16px">
      <template #header>
        <span style="font-weight: 600">员工销售数据</span>
      </template>
      <el-table :data="employeeSales" stripe border size="default">
        <el-table-column prop="real_name" label="销售员" min-width="120" />
        <el-table-column prop="order_count" label="订单数" width="100" align="center" />
        <el-table-column label="销售额(¥)" width="150" align="right">
          <template #default="{ row }">
            <span style="font-weight: 600; color: #409eff">{{ Number(row.total_amount).toFixed(2) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="待审批" width="100" align="center">
          <template #default="{ row }">
            <el-tag v-if="Number(row.pending_count) > 0" size="small" type="warning">
              {{ row.pending_count }}
            </el-tag>
            <span v-else style="color: #c0c4cc">0</span>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- 销售部领导：最近部门销售单 -->
    <el-card v-if="dept === '销售部' && recentSales.length > 0" shadow="never" style="margin-top: 16px">
      <template #header>
        <span style="font-weight: 600">最近销售单</span>
      </template>
      <el-table :data="recentSales" stripe border size="default">
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
import { ref, computed, onMounted, onBeforeUnmount, nextTick } from 'vue'
import * as echarts from 'echarts'
import { getDepartmentDashboard, getDepartmentTrend } from '@/api/department'
import { useUserStore } from '@/stores/user'

const userStore = useUserStore()

const dashboardLoading = ref(false)
let dashboardTimer: ReturnType<typeof setInterval> | undefined
const trendLoading = ref(false)
const dashboardStats = ref<{ label: string; value: string | number }[]>([])
const employeeSales = ref<Record<string, unknown>[]>([])
const recentSales = ref<Record<string, unknown>[]>([])
const todayText = ref('')
const todayAfterText = ref('')

const period = ref('day')
const chartRef = ref<HTMLElement | null>(null)
const chartAfterRef = ref<HTMLElement | null>(null)
let chart: echarts.ECharts | null = null
let afterChart: echarts.ECharts | null = null

const dept = computed(() => userStore.department)

const trendTitle = computed(() => {
  const base = `${dept.value || '本部门'}`
  const p = { day: '近30天', week: '近12周', month: '近12月', year: '近5年' }[period.value] || ''
  if (dept.value === '销售部') return `${base} · 销售趋势（${p}）`
  const metric = metricText.value
  return `${base} · ${metric}趋势（${p}）`
})

const metricText = computed(() => {
  if (dept.value === '销售部') return '售卖金额'
  if (dept.value === '采购部') return '采购金额'
  if (dept.value === '仓库部') return '出入库数量'
  if (dept.value === '财务部') return '往来账金额'
  return '业务'
})

// 部门主题渐变
const deptPalette = computed(() => {
  const map: Record<string, string[]> = {
    '销售部': ['linear-gradient(135deg, #409eff 0%, #66b1ff 100%)', '#3a9bff'],
    '采购部': ['linear-gradient(135deg, #67c23a 0%, #85ce61 100%)', '#5fb336'],
    '仓库部': ['linear-gradient(135deg, #e6a23c 0%, #efb45c 100%)', '#d8942f'],
    '财务部': ['linear-gradient(135deg, #9b59b6 0%, #bb8ac9 100%)', '#8e44ad'],
  }
  return map[dept.value || ''] || ['linear-gradient(135deg, #409eff 0%, #66b1ff 100%)', '#3a9bff']
})

const paletteAll = [
  'linear-gradient(135deg, #409eff 0%, #66b1ff 100%)',
  'linear-gradient(135deg, #67c23a 0%, #85ce61 100%)',
  'linear-gradient(135deg, #e6a23c 0%, #efb45c 100%)',
  'linear-gradient(135deg, #909399 0%, #b1b3b8 100%)',
]

function cardBg(i: number) {
  // 第一张卡用部门主题色，其余用统一色板循环
  if (i === 0) return deptPalette.value[0]
  return paletteAll[i % paletteAll.length]
}

async function loadDashboard() {
  if (dashboardLoading.value) return
  dashboardLoading.value = true
  try {
    const res = await getDepartmentDashboard()
    const d = (res.data || {}) as Record<string, unknown>
    const stats: { label: string; value: string | number }[] = []
    if (d.employee_count !== undefined && dept.value !== '销售部' && dept.value !== '采购部') {
      stats.push({ label: '部门员工数', value: Number(d.employee_count) })
    }
    if (dept.value === '销售部' || dept.value === '采购部') {
      if (dept.value === '销售部') {
        // 顶部：今日 / 本周 / 当年售出金额（时间维度，非总金额）
        stats.push({ label: '今日售出(¥)', value: Number(d.today_amount || 0).toFixed(2) })
        stats.push({ label: '本周售出(¥)', value: Number(d.week_amount || 0).toFixed(2) })
        stats.push({ label: '当年售出(¥)', value: Number(d.year_amount || 0).toFixed(2) })
        if (d.pending_count !== undefined) {
          stats.push({ label: '待审批单', value: Number(d.pending_count) })
        }
        stats.push({ label: '部门员工数', value: Number(d.employee_count) })
      } else {
        stats.push({ label: '订单数', value: Number(d.order_count || 0) })
        stats.push({ label: '总金额(¥)', value: Number(d.total_amount || 0).toFixed(2) })
        if (d.pending_count !== undefined) {
          stats.push({ label: '待审批单', value: Number(d.pending_count) })
        }
        stats.push({ label: '部门员工数', value: Number(d.employee_count) })
      }
    } else if (dept.value === '仓库部') {
      stats.push({ label: '库存品种数', value: Number(d.inventory_count || 0) })
      stats.push({ label: '库存总量', value: Number(d.total_qty || 0) })
      stats.push({ label: '安全库存预警', value: Number(d.low_stock || 0) })
      if (d.pending_confirm !== undefined) {
        stats.push({ label: '待确认出库', value: Number(d.pending_confirm) })
      }
      if (d.ops_pending_approve !== undefined) {
        stats.push({ label: '待审批操作单', value: Number(d.ops_pending_approve) })
      }
    } else if (dept.value === '财务部') {
      stats.push({ label: '应收(¥)', value: Number(d.receivable || 0).toFixed(2) })
      stats.push({ label: '应付(¥)', value: Number(d.payable || 0).toFixed(2) })
    }
    dashboardStats.value = stats
    employeeSales.value = Array.isArray(d.employee_sales) ? d.employee_sales : []
    recentSales.value = Array.isArray(d.recent_sales) ? d.recent_sales : []
  } catch (e) {
    // 已提示
  } finally {
    dashboardLoading.value = false
  }
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

async function loadTrend() {
  trendLoading.value = true
  try {
    const res = await getDepartmentTrend(undefined, period.value)
    const d = res.data || { labels: [], values: [], unit: '' }
    await nextTick()
    const labels = d.labels || []
    const unit = d.unit || '金额(¥)'
    const series = (d.series && d.series.length > 0
      ? d.series
      : [{ name: metricText.value, values: d.values || [] }]) as { name: string; values: number[] }[]

    // 售后序列独立成第二张图（仅销售部返回售后序列）
    const afterSeries = series.filter((s) => s.name.includes('售后'))
    const bizSeries = series.filter((s) => !s.name.includes('售后'))

    renderMultiTo(chartRef.value, labels, bizSeries, unit, chart)
    if (afterSeries.length > 0) {
      renderMultiTo(chartAfterRef.value, labels, afterSeries, unit, afterChart)
    }

    // 今日数据：day 周期取最后一天（今天）各序列合计
    if (period.value === 'day' && labels.length > 0) {
      const idx = labels.length - 1
      todayText.value = bizSeries.length > 0
        ? `今日 ${bizSeries.map((s: any) => `${s.name} ${Number(s.values?.[idx] || 0).toFixed(0)}${seriesUnit(s.name, unit)}`).join(' / ')}`
        : ''
      todayAfterText.value = afterSeries.length > 0
        ? `今日 ${afterSeries.map((s: any) => `${s.name} ${Number(s.values?.[idx] || 0).toFixed(0)}${seriesUnit(s.name, unit)}`).join(' / ')}`
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

// 序列单位：按名称判断（金额→元、单数→单、件数→件），其余用接口 unit
function seriesUnit(name: string, unit: string) {
  if (name.includes('金额')) return '元'
  if (name.includes('单数')) return '单'
  if (name.includes('件数')) return '件'
  return unit
}

function renderMultiTo(
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
      areaStyle: i === 0 ? {
        color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
          { offset: 0, color: 'rgba(64,158,255,0.3)' },
          { offset: 1, color: 'rgba(64,158,255,0.02)' },
        ]),
      } : undefined,
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
.grad-card {
  text-align: center;
  padding: 24px 12px;
  border-radius: 14px;
  color: #fff;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.08);
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}
.grad-card:hover {
  transform: translateY(-3px);
  box-shadow: 0 10px 24px rgba(0, 0, 0, 0.14);
}
.grad-value {
  font-size: 28px;
  font-weight: 700;
}
.grad-label {
  margin-top: 6px;
  font-size: 13px;
  opacity: 0.92;
}
.chart-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}
</style>
