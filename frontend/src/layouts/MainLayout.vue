<template>
  <el-container class="layout">
    <!-- 侧边栏 -->
    <el-aside :width="isCollapse ? '64px' : '220px'" class="aside">
      <div class="logo">
        <span v-if="!isCollapse">ERP 管理系统</span>
        <span v-else>ERP</span>
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
        <el-menu-item index="/dashboard">
          <el-icon><HomeFilled /></el-icon>
          <template #title>首页</template>
        </el-menu-item>
        <el-menu-item index="/products">
          <el-icon><Goods /></el-icon>
          <template #title>商品管理</template>
        </el-menu-item>
        <el-sub-menu index="base-group">
          <template #title>
            <el-icon><OfficeBuilding /></el-icon>
            <span>基础数据</span>
          </template>
          <el-menu-item index="/base/suppliers">
            <el-icon><OfficeBuilding /></el-icon>
            <template #title>供应商管理</template>
          </el-menu-item>
          <el-menu-item index="/base/customers">
            <el-icon><User /></el-icon>
            <template #title>客户管理</template>
          </el-menu-item>
          <el-menu-item index="/base/employees">
            <el-icon><UserFilled /></el-icon>
            <template #title>员工管理</template>
          </el-menu-item>
          <el-menu-item index="/base/warehouses">
            <el-icon><Location /></el-icon>
            <template #title>仓库管理</template>
          </el-menu-item>
          <el-menu-item index="/base/categories">
            <el-icon><Menu /></el-icon>
            <template #title>分类管理</template>
          </el-menu-item>
        </el-sub-menu>
        <el-sub-menu index="inventory-group">
          <template #title>
            <el-icon><Box /></el-icon>
            <span>库存管理</span>
          </template>
          <el-menu-item index="/inventory">
            <el-icon><List /></el-icon>
            <template #title>库存查询</template>
          </el-menu-item>
          <el-menu-item index="/inventory/operation">
            <el-icon><Switch /></el-icon>
            <template #title>库存调整(含审批)</template>
          </el-menu-item>
          <el-menu-item index="/inventory/check">
            <el-icon><Finished /></el-icon>
            <template #title>库存盘点</template>
          </el-menu-item>
        </el-sub-menu>
        <el-menu-item index="/finance/accounts">
          <el-icon><Money /></el-icon>
          <template #title>财务管理</template>
        </el-menu-item>
        <el-menu-item index="/finance/actions-admin"><el-icon><Checked /></el-icon><template #title>财务复核</template></el-menu-item>
        <el-menu-item index="/finance/payments-admin"><el-icon><Money /></el-icon><template #title>收付款与冲正</template></el-menu-item>
        <el-menu-item index="/warehouse-returns/purchase"><el-icon><Checked /></el-icon><template #title>采购退货待出库</template></el-menu-item>
        <el-menu-item index="/warehouse-returns/sales"><el-icon><Checked /></el-icon><template #title>销售退换货待处理</template></el-menu-item>
        <el-menu-item index="/inventory/fulfillment"><el-icon><Checked /></el-icon><template #title>分批收发审核</template></el-menu-item>
        <el-menu-item index="/inventory/lots"><el-icon><Box /></el-icon><template #title>商品批次库存</template></el-menu-item><el-menu-item index="/inventory/cost"><el-icon><Money /></el-icon><template #title>库存成本</template></el-menu-item><el-sub-menu index="system-group">
          
          <template #title>
            <el-icon><Setting /></el-icon>
            <span>系统管理</span>
          </template>
          <el-menu-item index="/system/users">
            <el-icon><User /></el-icon>
            <template #title>用户管理</template>
          </el-menu-item>
          <el-menu-item index="/system/backup"><el-icon><Setting /></el-icon><template #title>异机备份设置</template></el-menu-item><el-menu-item index="/system/logs">
            <el-icon><Document /></el-icon>
            <template #title>操作日志</template>
          </el-menu-item>
          <el-menu-item index="/system/departments"><el-icon><Setting /></el-icon><template #title>部门管理</template></el-menu-item>
          <el-menu-item index="/system/dept-assign">
            <el-icon><Switch /></el-icon>
            <template #title>员工调度</template>
          </el-menu-item>
        </el-sub-menu>
      </el-menu>
    </el-aside>

    <el-container>
      <!-- 顶栏 -->
      <el-header class="header">
        <div class="header-left">
          <el-icon class="collapse-btn" @click="isCollapse = !isCollapse">
            <Fold v-if="!isCollapse" />
            <Expand v-else />
          </el-icon>
          <span class="page-title">{{ currentTitle }}</span>
        </div>
        <div class="header-right">
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

      <!-- 主内容 -->
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
  font-size: 18px;
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
