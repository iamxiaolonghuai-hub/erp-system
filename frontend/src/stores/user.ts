import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { UserInfo } from '@/types'
import * as authApi from '@/api/auth'

export const useUserStore = defineStore(
  'user',
  () => {
    const token = ref<string>(localStorage.getItem('erp_token') || '')
    const userInfo = ref<UserInfo | null>(
      JSON.parse(localStorage.getItem('erp_user') || 'null'),
    )

    // 角色判断：admin=管理员，leader=领导，staff=员工
    const roles = computed(() => userInfo.value?.roles || [])
    const isAdmin = computed(() => roles.value.includes('admin'))
    const isLeader = computed(() => roles.value.includes('leader'))
    const isStaff = computed(() => roles.value.includes('staff'))
    const department = computed(() => userInfo.value?.department || '')

    // 登录
    async function login(username: string, password: string) {
      const res = await authApi.login({ username, password })
      token.value = res.data.token
      userInfo.value = res.data.user
      localStorage.setItem('erp_token', token.value)
      localStorage.setItem('erp_user', JSON.stringify(userInfo.value))
      return res.data
    }

    let refreshPromise:Promise<void>|null=null
    function refreshUserInfo():Promise<void> {
      if(refreshPromise)return refreshPromise
      const currentToken=token.value
      refreshPromise=authApi.getCurrentUser().then(res=>{if(token.value===currentToken){userInfo.value=res.data;localStorage.setItem('erp_user',JSON.stringify(res.data))}}).finally(()=>{refreshPromise=null})
      return refreshPromise
    }

    // 退出登录
    function logout() {
      token.value = ''
      userInfo.value = null
      localStorage.removeItem('erp_token')
      localStorage.removeItem('erp_user')
    }

    return { token, userInfo, roles, isAdmin, isLeader, isStaff, department, login, logout, refreshUserInfo }
  },
)
