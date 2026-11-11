<template>
  <el-container class="layout">
    <el-aside :width="isCollapse ? '64px' : '200px'" class="aside">
      <div class="logo">
        <span v-if="!isCollapse">财务工作台</span>
        <span v-else>财务</span>
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
        <el-menu-item index="/finance/home">
          <el-icon><HomeFilled /></el-icon>
          <template #title>财务首页</template>
        </el-menu-item>
        <el-menu-item index="/finance/accounts">
          <el-icon><Money /></el-icon>
          <template #title>往来账</template>
        </el-menu-item>
        <el-menu-item index="/finance/payments">
          <el-icon><Wallet /></el-icon>
          <template #title>收付款记录</template>
        </el-menu-item>
        <el-menu-item index="/finance/cost"><el-icon><Money /></el-icon><template #title>库存成本</template></el-menu-item><el-menu-item index="/finance/actions"><el-icon><Checked /></el-icon><template #title>财务复核</template></el-menu-item>
        <el-menu-item index="/finance/period"><el-icon><Money /></el-icon><template #title>期间对账</template></el-menu-item>
        <el-menu-item index="/finance/my-actions"><el-icon><Checked /></el-icon><template #title>我复核的</template></el-menu-item>
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
          <el-tag size="small" type="info" style="margin-right: 12px">财务部</el-tag>
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
.layout { height: 100vh; }
.aside { background-color: #001529; transition: width 0.3s; overflow: hidden; }
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
.menu { border-right: none; }
.header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background-color: #fff;
  border-bottom: 1px solid #e6e6e6;
  padding: 0 20px;
}
.header-left { display: flex; align-items: center; gap: 12px; }
.collapse-btn { font-size: 20px; cursor: pointer; }
.page-title { font-size: 16px; font-weight: 500; }
.header-right { display: flex; align-items: center; }
.user-info {
  display: flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  font-size: 14px;
  color: #333;
}
.main { background-color: #f0f2f5; padding: 20px; }
</style>
