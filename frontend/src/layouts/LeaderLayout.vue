<template>
  <el-container class="layout">
    <el-aside :width="isCollapse ? '64px' : '210px'" class="aside">
      <div class="logo">
        <span v-if="!isCollapse">领导工作台</span>
        <span v-else>领导</span>
      </div>
      <el-menu
        :default-active="activeMenu"
        :collapse="isCollapse"
        router
        background-color="#001529"
        text-color="#a6adb4"
        active-text-color="#ffffff"
        class="menu"
      >
        <el-menu-item index="/leader/home">
          <el-icon><HomeFilled /></el-icon>
          <template #title>部门仪表盘</template>
        </el-menu-item>
        <!-- 待审批单据（按部门） -->
        <el-menu-item v-if="deptHasApproval" index="/leader/approval">
          <el-icon><Checked /></el-icon>
          <template #title>待审批单据</template>
        </el-menu-item>
        <!-- 申报审批（采购部/销售部） -->
        <el-menu-item
          v-if="userStore.department === '销售部' || userStore.department === '采购部'"
          index="/leader/application"
        >
          <el-icon><DocumentChecked /></el-icon>
          <template #title>申报审批</template>
        </el-menu-item>
        <!-- 本部门员工管理 -->
        <el-menu-item index="/leader/employees">
          <el-icon><User /></el-icon>
          <template #title>本部门员工</template>
        </el-menu-item>
        <!-- 本部门单据查看 -->
        <el-menu-item v-for="item in deptDocs" :key="item.path" :index="item.path">
          <el-icon><component :is="item.icon" /></el-icon>
          <template #title>{{ item.title }}</template>
        </el-menu-item>
      </el-menu>
    </el-aside>

    <el-container>
      <el-header class="header">
        <div class="header-left">
          <el-icon class="collapse-btn" @click="isCollapse = !isCollapse">
            <Fold v-if="!isCollapse" />
            <Expand v-else />
          </el-icon>
          <span class="page-title">{{ currentTitle }}</span>
        </div>
        <div class="header-right">
          <el-tag size="small" type="warning" style="margin-right: 12px">领导</el-tag>
          <el-tag v-if="userStore.department" size="small" type="info" style="margin-right: 12px">
            {{ userStore.department }}
          </el-tag>
          <el-dropdown @command="handleCommand">
            <span class="user-info">
              <el-icon><UserFilled /></el-icon>
              {{ userStore.userInfo?.real_name || userStore.userInfo?.username || '用户' }}
              <el-icon><ArrowDown /></el-icon>
            </span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="logout">退出登录</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </el-header>

      <el-main class="main">
        <router-view />
      </el-main>
    </el-container>
  </el-container>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessageBox } from 'element-plus'
import { useUserStore } from '@/stores/user'

const route = useRoute()
const router = useRouter()
const userStore = useUserStore()

const isCollapse = ref(window.innerWidth < 900)
const activeMenu = computed(() => route.path)
const currentTitle = computed(() => String(route.meta.title || ''))

// 部门是否有审批需求（销售/采购需要审批单据）
const deptHasApproval = computed(() => {
  return userStore.department === '销售部' || userStore.department === '采购部'
})

// 本部门单据查看入口
const deptDocs = computed(() => {
  const dept = userStore.department
  if (dept === '销售部') {
    return [
      { path: '/leader/sales', title: '销售单', icon: 'Sell' },
      { path: '/leader/sales-return', title: '销售退货', icon: 'RefreshLeft' },
    ]
  }
  if (dept === '采购部') {
    return [
      { path: '/leader/purchase', title: '采购单', icon: 'ShoppingCart' },
      { path: '/leader/purchase-return', title: '采购退货', icon: 'RefreshLeft' },
    ]
  }
  if (dept === '仓库部') {
    return [
      { path: '/leader/warehouse-returns/purchase', title: '采购退货待出库', icon: 'RefreshLeft' },
      { path: '/leader/warehouse-returns/sales', title: '销售退换货待处理', icon: 'RefreshLeft' },
      { path: '/leader/warehouse-audit', title: '仓库审核', icon: 'Checked' },
      { path: '/leader/inventory/operation', title: '库存调整审批', icon: 'Switch' },
      { path: '/leader/inventory/check', title: '盘点差异复核', icon: 'Finished' },
      { path: '/leader/base/warehouses', title: '仓库管理', icon: 'Location' },
      { path: '/leader/inventory', title: '库存查询', icon: 'Box' },
      { path: '/leader/inventory/serials', title: '序列号台账', icon: 'Box' },
      { path: '/leader/inventory/lots', title: '商品批次库存', icon: 'Box' },
    ]
  }
  if (dept === '财务部') {
    return []
  }
  return []
})

async function handleCommand(command: string) {
  if (command === 'logout') {
    await ElMessageBox.confirm('确定要退出登录吗？', '提示', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    })
    userStore.logout()
    router.push('/login')
  }
}
</script>

<style scoped>
.layout {
  height: 100vh;
}
.aside {
  background-color: #001529;
  transition: width 0.3s;
  overflow: hidden;
}
.logo {
  height: 60px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #fff;
  font-size: 16px;
  font-weight: 600;
  background-color: #002140;
}
.menu {
  border-right: none;
}
.header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background-color: #fff;
  border-bottom: 1px solid #e6e6e6;
  padding: 0 20px;
}
.header-left {
  display: flex;
  align-items: center;
  gap: 12px;
}
.collapse-btn {
  font-size: 20px;
  cursor: pointer;
}
.page-title {
  font-size: 16px;
  font-weight: 500;
}
.header-right {
  display: flex;
  align-items: center;
}
.user-info {
  display: flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  font-size: 14px;
  color: #333;
}
.main {
  background-color: #f0f2f5;
  padding: 20px;
}
</style>
