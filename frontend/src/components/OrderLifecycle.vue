<template><el-button v-if="state===0 && creator===Number(user.userInfo?.id)" size="small" @click="change('withdraw')">撤回</el-button><el-button v-if="(user.isAdmin || user.isLeader) && [0,1,2,4,5,6].includes(state)" size="small" type="danger" @click="change('void')">作废</el-button><el-button v-if="(user.isAdmin || user.isLeader) && [1,5].includes(state)" size="small" type="warning" @click="change('close')">关闭剩余</el-button></template>
<script setup lang="ts">
import { newRequestId } from '@/utils/requestId'
import {ElMessage,ElMessageBox} from 'element-plus'
import {post} from '@/utils/request'
import {useUserStore} from '@/stores/user'
const props=defineProps<{id:number;type:string;state:number;creator?:number}>(),emit=defineEmits<{changed:[]}>(),user=useUserStore()
async function change(action:string){try{const {value}=await ElMessageBox.prompt('请填写原因，操作后保留单据与轨迹',action==='void'?'作废单据':action==='close'?'关闭剩余数量':'撤回单据',{inputValidator:v=>!!v?.trim()||'请填写原因'});await post('/orders/lifecycle',{order_id:props.id,type:props.type,action,reason:value,request_id:newRequestId()});ElMessage.success('操作成功');emit('changed')}catch{}}
</script>
