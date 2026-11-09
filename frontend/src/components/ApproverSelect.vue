<template><el-form-item label="审批领导"><el-select :model-value="modelValue" @update:model-value="$emit('update:modelValue', $event)" clearable filterable placeholder="不指定：进入本部门审批队列" style="width:100%" :loading="loading"><el-option v-for="person in people" :key="person.id" :label="person.name" :value="person.id" /></el-select><div style="font-size:12px;color:#64748b;margin-top:6px">{{ error || '指定后由所选领导审批；管理员可统一处理。' }}</div></el-form-item></template>
<script setup lang="ts">
import { ref,onMounted } from 'vue'
import { get } from '@/utils/request'
defineProps<{ modelValue?: number }>()
defineEmits<{ 'update:modelValue': [value: number | undefined] }>()
const people=ref<{id:number;name:string}[]>([]),loading=ref(false),error=ref('')
onMounted(async()=>{loading.value=true;try{people.value=(await get<{id:number;name:string}[]>('/approval/leaders')).data;if(!people.value.length)error.value='本部门暂无可选择的领导，将进入部门审批队列。'}catch{error.value='审批人加载失败，请刷新后重试。'}finally{loading.value=false}})
</script>
