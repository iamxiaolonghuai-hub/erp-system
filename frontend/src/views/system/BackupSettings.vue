<template><div>
  <el-alert title="管理员填写另一台服务器的备份目标。私钥留在宿主机，不在此上传；目标服务器需预先核实SSH主机指纹。备份会短暂停止ERP后端写入，请安排业务空闲时间。" type="info" :closable="false" />
  <el-alert v-if="loaded&&!online" title="未检测到最近15分钟的宿主机任务心跳。保存配置或任务排队不等于已经备份，请先按部署说明安装定时任务。" type="warning" :closable="false" style="margin-top:12px" />
  <el-alert v-if="failed" :title="`最近任务失败：${failed.details?.message||'请查看服务器日志'}`" type="error" :closable="false" style="margin-top:12px" />
  <el-card shadow="never" style="margin-top:16px"><template #header>异机备份配置</template><el-form label-width="140px" style="max-width:760px" v-loading="loading">
    <el-form-item label="启用定时备份"><el-switch v-model="form.enabled" /></el-form-item>
    <el-form-item label="目标服务器"><el-input v-model="form.host" placeholder="IPv4或主机名，例如 backup.example.com" /></el-form-item>
    <el-form-item label="SSH端口"><el-input-number v-model="form.port" :min="1" :max="65535" /></el-form-item>
    <el-form-item label="登录用户名"><el-input v-model="form.username" placeholder="例如 ubuntu" /></el-form-item>
    <el-form-item label="远端保存目录"><el-input v-model="form.remote_dir" placeholder="/srv/erp-backups" /></el-form-item>
    <el-form-item label="宿主机私钥路径"><el-input v-model="form.ssh_key_path" placeholder="/opt/erp/keys/backup_ed25519" /></el-form-item>
    <el-form-item label="本地保存目录"><el-input v-model="form.local_dir" placeholder="/opt/erp/backups" /></el-form-item>
    <el-form-item label="每日时间 北京时间"><el-time-select v-model="form.time" start="00:00" step="00:15" end="23:45" /></el-form-item>
    <el-form-item label="本地保留份数"><el-input-number v-model="form.keep_count" :min="1" :max="365" /><span style="margin-left:12px">仅远端校验成功后清理旧本地备份，远端不自动删除。</span></el-form-item>
    <el-form-item><el-button type="primary" :loading="saving" @click="save">保存配置</el-button><el-button :loading="requesting" :disabled="!form.enabled||dirty" @click="request">提交立即备份</el-button><el-button @click="load()">刷新状态</el-button></el-form-item>
  </el-form><p>失败通知：系统页面告警和服务器日志。保存配置会取消原先尚未执行的排队任务，已开始的任务按其执行配置完成。计划每天尝试一次，失败后由管理员检查并手动重试。</p><p>任务心跳：{{online?'近期在线':'未检测到近期心跳'}}　最近记录：{{lastSeen||'尚未连接'}}。执行时间表格采用数据库记录时间。</p></el-card>
  <el-card shadow="never" style="margin-top:16px"><template #header>最近30次任务</template><el-table :data="jobs" border stripe><el-table-column prop="id" label="编号" width="80" /><el-table-column label="状态" width="140"><template #default="{row}"><el-tag :type="row.status==='success'?'success':row.status==='failed'?'danger':'warning'">{{states[row.status]||row.status}}</el-tag></template></el-table-column><el-table-column label="发起人" width="130"><template #default="{row}">{{row.requested_by_name||(row.requested_by?'用户#'+row.requested_by:'定时任务')}}</template></el-table-column><el-table-column prop="created_at" label="登记时间" width="170" /><el-table-column prop="completed_at" label="完成时间" width="170" /><el-table-column label="目标服务器" min-width="150"><template #default="{row}">{{row.details?.target_host||'待执行'}}</template></el-table-column><el-table-column label="结果" min-width="260"><template #default="{row}">{{row.details?.message||(row.status==='queued'?'等待宿主机任务领取':'执行中，可查看服务器日志')}}</template></el-table-column><el-table-column label="归档文件" min-width="230"><template #default="{row}">{{row.details?.archive||'—'}}</template></el-table-column></el-table></el-card>
</div></template>
<script setup lang="ts">
import {computed,onMounted,onUnmounted,reactive,ref} from 'vue'
import {ElMessage,ElMessageBox} from 'element-plus'
import {get,post} from '@/utils/request'
import {newRequestId} from '@/utils/requestId'
const form=reactive({enabled:false,host:'',port:22,username:'',remote_dir:'/srv/erp-backups',ssh_key_path:'/opt/erp/keys/backup_ed25519',local_dir:'/opt/erp/backups',time:'03:00',keep_count:14})
const loaded=ref(false),loading=ref(false),saving=ref(false),requesting=ref(false),online=ref(false),lastSeen=ref(''),jobs=ref<any[]>([]),saved=ref(''),requestKey=ref(newRequestId())
const dirty=computed(()=>JSON.stringify(form)!==saved.value),states:Record<string,string>={queued:'等待宿主机执行',running:'执行中',success:'已完成远端校验',failed:'失败或已取消'}
const failed=computed(()=>jobs.value[0]?.status==='failed'?jobs.value[0]:null)
async function load(settings=true){if(settings)loading.value=true;try{const r=await get<any>('/operations/backup');if(settings){Object.assign(form,r.data.settings);saved.value=JSON.stringify(form)}jobs.value=r.data.jobs||[];online.value=!!r.data.agent_online;lastSeen.value=r.data.agent_last_seen||'';loaded.value=true}catch{}finally{loading.value=false}}
async function save(){try{await ElMessageBox.confirm(form.enabled?`确认将数据库和附件定时备份到 ${form.username}@${form.host}:${form.remote_dir}？`:'确认保存并禁用自动备份？正在执行的任务不会强制中断。','保存备份策略');saving.value=true;await post('/operations/backup',{...form});ElMessage.success('已保存，实际执行状态以任务记录为准');await load()}catch{}finally{saving.value=false}}
async function request(){try{await ElMessageBox.confirm(`备份将短暂停止ERP后端写入，将数据库和附件传输到 ${form.username}@${form.host}:${form.remote_dir}。确认排队执行？`,'提交立即备份');requesting.value=true;await post('/operations/backup/request',{request_id:requestKey.value});requestKey.value=newRequestId();ElMessage.success('已排队，等待宿主机任务执行');await load(false)}catch{}finally{requesting.value=false}}
let timer:ReturnType<typeof setInterval>|undefined
onMounted(()=>{load();timer=setInterval(()=>load(false),15000)})
onUnmounted(()=>{if(timer)clearInterval(timer)})
</script>
