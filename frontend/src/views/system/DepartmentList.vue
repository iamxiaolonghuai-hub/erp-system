<template><el-card shadow="never">
  <el-alert title="部门是员工归属的统一来源。系统业务部门用于业务权限分工，可修改说明；新增部门用于组织管理，成员业务权限仍按系统已有规则执行。" type="info" :closable="false" />
  <div class="toolbar"><el-input v-model="search" placeholder="部门名称或编码" clearable style="width:260px" /><el-button @click="load">刷新</el-button><el-button type="primary" @click="open()">新增部门</el-button></div>
  <el-table :data="filtered" v-loading="loading" border stripe><el-table-column prop="name" label="部门名称" min-width="150" /><el-table-column prop="code" label="部门编码" min-width="130" /><el-table-column prop="description" label="说明" min-width="220" /><el-table-column label="状态" width="100"><template #default="{row}"><el-tag :type="row.status===0?'info':'success'">{{row.status===0?'停用':'启用'}}</el-tag></template></el-table-column><el-table-column label="操作" width="170"><template #default="{row}"><el-button size="small" @click="open(row)">编辑</el-button><el-button size="small" type="danger" :disabled="system(row.code)" @click="remove(row)">删除</el-button></template></el-table-column></el-table>
  <el-dialog v-model="visible" :title="id?'编辑部门':'新增部门'" width="520px"><el-form label-width="90px"><el-form-item label="名称"><el-input v-model="form.name" maxlength="64" :disabled="!!id&&system(originalCode)" /></el-form-item><el-form-item label="编码"><el-input v-model="form.code" maxlength="32" :disabled="!!id&&system(originalCode)" placeholder="英文字母开头，可含数字和下划线" /></el-form-item><el-form-item label="说明"><el-input v-model="form.description" type="textarea" maxlength="255" /></el-form-item><el-form-item label="启用"><el-switch v-model="form.status" :active-value="1" :inactive-value="0" :disabled="!!id&&system(originalCode)" /></el-form-item></el-form><template #footer><el-button @click="visible=false">取消</el-button><el-button type="primary" :loading="saving" @click="save">保存</el-button></template></el-dialog>
</el-card></template>
<script setup lang="ts">
import {computed,onMounted,reactive,ref} from 'vue'
import {ElMessage,ElMessageBox} from 'element-plus'
import {get,post,put,del} from '@/utils/request'
import type {Department} from '@/types'
const rows=ref<Department[]>([]),search=ref(''),loading=ref(false),saving=ref(false),visible=ref(false),id=ref(0),originalCode=ref('')
const form=reactive({name:'',code:'',description:'',status:1})
const system=(code:string)=>['sales','purchase','warehouse','finance'].includes(code)
const filtered=computed(()=>rows.value.filter(row=>`${row.name} ${row.code}`.toLowerCase().includes(search.value.trim().toLowerCase())))
async function load(){loading.value=true;try{const result=await get<Department[]>('/departments');rows.value=result.data||[]}finally{loading.value=false}}
function open(row?:Department){id.value=row?.id||0;originalCode.value=row?.code||'';Object.assign(form,{name:row?.name||'',code:row?.code||'',description:row?.description||'',status:Number(row?.status??1)});visible.value=true}
async function save(){form.name=form.name.trim();form.code=form.code.trim();if(!form.name||!form.code){ElMessage.warning('请填写名称和编码');return}saving.value=true;try{if(id.value)await put('/departments/'+id.value,{...form});else await post('/departments',{...form});visible.value=false;ElMessage.success('部门已保存');await load()}finally{saving.value=false}}
async function remove(row:Department){try{await ElMessageBox.confirm(`确认删除部门“${row.name}”？存在成员的部门无法删除。`,'删除部门');await del('/departments/'+row.id);ElMessage.success('部门已删除');await load()}catch{}}
onMounted(load)
</script>
<style scoped>.toolbar{display:flex;gap:12px;margin:16px 0}</style>
