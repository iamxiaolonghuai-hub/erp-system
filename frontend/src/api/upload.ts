import { useUserStore } from '@/stores/user'

// 上传图片，返回 URL
export async function uploadImage(file: File): Promise<string> {
  const userStore = useUserStore()
  const token = userStore.token
  const formData = new FormData()
  formData.append('file', file)

  const resp = await fetch('/api/upload/image', {
    method: 'POST',
    headers: { Authorization: `Bearer ${token}` },
    body: formData,
  })
  if (!resp.ok) {
    throw new Error('上传失败')
  }
  const body = await resp.json()
  if (body.code !== 0) {
    throw new Error(body.message || '上传失败')
  }
  return body.data.url as string
}
