import axios, { type AxiosInstance, type AxiosRequestConfig } from 'axios'
import { ElMessage } from 'element-plus'
import type { ApiResponse } from '@/types'
import { useUserStore } from '@/stores/user'
import router from '@/router'

// 创建 axios 实例
const service: AxiosInstance = axios.create({
  baseURL: '/api',
  timeout: 15000,
})

// 请求拦截器：自动附加 JWT
service.interceptors.request.use(
  (config) => {
    const userStore = useUserStore()
    if (userStore.token) {
      config.headers.Authorization = `Bearer ${userStore.token}`
    }
    return config
  },
  (error) => Promise.reject(error),
)

// 响应拦截器：统一处理错误码
service.interceptors.response.use(
  (response) => {
    const body = response.data as ApiResponse
    // Restful 接口直接返回裸数组，统一包装成 { code: 0, data: [...] }
    if (Array.isArray(body)) {
      return { code: 0, message: 'success', data: body } as unknown as typeof response
    }
    // 业务错误码非 0 视为失败
    if (body.code !== undefined && body.code !== 0) {
      ElMessage.error(body.message || '请求失败')
      // 401 未认证，跳转登录
      if (body.code === 40100) {
        const userStore = useUserStore()
        userStore.logout()
        router.push('/login')
      }
      return Promise.reject(new Error(body.message || 'Error'))
    }
    // 正常返回 body（即 { code, message, data }），前端通过 res.data 取值
    return body as unknown as typeof response
  },
  (error) => {
    if (error.response?.status === 401) {
      ElMessage.error('登录已过期，请重新登录')
      const userStore = useUserStore()
      userStore.logout()
      router.push('/login')
    } else {
      const body = error.response?.data
      ElMessage.error(error.code === 'ECONNABORTED' ? `请求超时：${error.config?.method?.toUpperCase() || ''} ${error.config?.url || ''}，请稍后重试` : (body?.message || body?.error || error.message || '网络错误'))
    }
    return Promise.reject(error)
  },
)

// 通用请求方法
export function request<T = unknown>(config: AxiosRequestConfig): Promise<ApiResponse<T>> {
  return service.request(config) as unknown as Promise<ApiResponse<T>>
}

export function get<T = unknown>(url: string, params?: object): Promise<ApiResponse<T>> {
  return request<T>({ method: 'get', url, params })
}

export function post<T = unknown>(url: string, data?: object): Promise<ApiResponse<T>> {
  return request<T>({ method: 'post', url, data })
}

export function put<T = unknown>(url: string, data?: object): Promise<ApiResponse<T>> {
  return request<T>({ method: 'put', url, data })
}

export function del<T = unknown>(url: string, params?: object): Promise<ApiResponse<T>> {
  return request<T>({ method: 'delete', url, params })
}

export default service
