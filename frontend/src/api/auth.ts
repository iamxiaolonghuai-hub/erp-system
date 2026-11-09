import { post, get } from '@/utils/request'
import type { LoginRequest, LoginData, UserInfo } from '@/types'

// 认证相关 API
export function login(data: LoginRequest) {
  return post<LoginData>('/auth/login', data)
}

export function getCurrentUser() {
  return get<UserInfo>('/auth/me')
}
