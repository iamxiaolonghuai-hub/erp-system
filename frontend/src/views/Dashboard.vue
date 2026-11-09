<template>
  <div class="business-dashboard">
    <div class="dashboard-heading"><div><span class="dashboard-eyebrow">企业运营中心</span><h1>经营总览</h1><p>从商品到交付，连接企业的每一个业务环节。</p></div><span class="dashboard-date">{{ new Date().toLocaleDateString('zh-CN', { month: 'long', day: 'numeric', weekday: 'long' }) }}</span></div>
    <div class="summary-toolbar"><span>库存变更后自动更新，每15秒刷新</span><el-button :loading="loading" @click="loadStats">刷新统计</el-button></div>
    <el-alert v-if="failed" title="统计加载失败，请重试；当前不显示为0。" type="error" :closable="false" style="margin-bottom:16px" />
    <!-- 统计卡片（渐变风格） -->
    <el-row :gutter="20">
      <el-col :span="6" v-for="card in statCards" :key="card.label">
        <div class="grad-card" :class="{ clickable: card.label === '安全库存预警' }" :style="{ background: card.bg }" @click="card.label === '安全库存预警' && router.push('/inventory?low_stock=1')">
          <div class="grad-icon" :style="{ color: card.color }">
            <el-icon :size="26"><component :is="card.icon" /></el-icon>
          </div>
          <div class="grad-body">
            <div class="grad-label">{{ card.label }}</div>
            <div class="grad-value">{{ card.value }}</div>
          </div>
        </div>
      </el-col>
    </el-row>

    <!-- 欢迎横幅 -->
    <div class="welcome-banner">
      <div class="welcome-title">欢迎使用中小企业 ERP 管理系统</div>
      <div class="welcome-desc">
        当前登录：<b>{{ userStore.userInfo?.real_name || userStore.userInfo?.username }}</b>
      </div>
      <div class="welcome-badges">
        <span class="badge">采购与供应商</span>
        <span class="badge">销售与客户</span>
        <span class="badge">库存与交付</span>
        <span class="badge">财务与审批</span>
      </div>
      <p class="welcome-modules">
        功能模块：基础数据 · 采购 · 销售 · 库存（出入库/调拨/审批）· 财务管理 · 系统管理
      </p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'
import { useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { getInventoryStats, type InventoryStats } from '@/api/inventory'

const userStore = useUserStore()
const router=useRouter()
const stats = ref<InventoryStats | null>(null)
const loading=ref(false),failed=ref(false)
let timer:ReturnType<typeof setInterval> | undefined

const statCards = computed(() => [
  {
    label: '商品总数',
    value: stats.value ? Number(stats.value.product_count) : '—',
    icon: 'Goods',
    color: '#409eff',
    bg: 'linear-gradient(135deg, #409eff 0%, #66b1ff 60%, #8cc5ff 100%)',
  },
  {
    label: '库存记录',
    value: stats.value ? Number(stats.value.inventory_count) : '—',
    icon: 'Box',
    color: '#67c23a',
    bg: 'linear-gradient(135deg, #67c23a 0%, #85ce61 60%, #a8dd8a 100%)',
  },
  {
    label: '库存流水',
    value: stats.value ? Number(stats.value.record_count) : '—',
    icon: 'Tickets',
    color: '#e6a23c',
    bg: 'linear-gradient(135deg, #e6a23c 0%, #ebb563 60%, #f0c78a 100%)',
  },
  {
    label: '安全库存预警',
    value: stats.value ? Number(stats.value.low_stock) : '—',
    icon: 'Warning',
    color: '#f56c6c',
    bg: 'linear-gradient(135deg, #f56c6c 0%, #f78989 60%, #f9a8a8 100%)',
  },
])

async function loadStats() {
  if(loading.value)return
  loading.value=true
  try {
    stats.value=(await getInventoryStats()).data
    failed.value=false
  } catch (e) {
    failed.value=true;stats.value=null
  } finally {loading.value=false}
}
onMounted(()=>{loadStats();timer=setInterval(loadStats,15000);window.addEventListener('focus',loadStats)})
onBeforeUnmount(()=>{if(timer)clearInterval(timer);window.removeEventListener('focus',loadStats)})
</script>

<style scoped>
.summary-toolbar {display:flex;justify-content:space-between;align-items:center;gap:12px;color:#64748b;font-size:12px;margin-bottom:16px;}
.clickable {cursor:pointer;}
.grad-card {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 22px 20px;
  border-radius: 14px;
  color: #fff;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.08);
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}
.grad-card:hover {
  transform: translateY(-3px);
  box-shadow: 0 10px 24px rgba(0, 0, 0, 0.14);
}
.grad-icon {
  width: 52px;
  height: 52px;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.92);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}
.grad-body {
  display: flex;
  flex-direction: column;
}
.grad-label {
  font-size: 13px;
  opacity: 0.9;
}
.grad-value {
  font-size: 26px;
  font-weight: 700;
  line-height: 1.3;
}

.welcome-banner {
  margin-top: 20px;
  border-radius: 14px;
  padding: 28px 30px;
  color: #fff;
  background: linear-gradient(120deg, #1f2d3d 0%, #2b4a7a 55%, #409eff 100%);
  box-shadow: 0 8px 20px rgba(31, 45, 61, 0.18);
}
.welcome-title {
  font-size: 22px;
  font-weight: 700;
}
.welcome-desc {
  margin-top: 8px;
  font-size: 14px;
  opacity: 0.92;
}
.welcome-badges {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 18px;
}
.badge {
  background: rgba(255, 255, 255, 0.16);
  border: 1px solid rgba(255, 255, 255, 0.25);
  border-radius: 20px;
  padding: 4px 14px;
  font-size: 12.5px;
}
.welcome-modules {
  margin-top: 18px;
  font-size: 13px;
  opacity: 0.85;
}
</style>
