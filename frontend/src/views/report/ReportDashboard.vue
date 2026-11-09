<template>
  <div>
    <el-alert title="金额按实际已过账批次统计，未收发订单不计入。退货单独列示；毛利以出库成本快照核算；历史成本或流水缺失时显示待补全，不按零成本计算。" type="info" :closable="false" />
    <el-row :gutter="16">
      <el-col :span="6" v-for="card in statCards" :key="card.label">
        <el-card shadow="hover">
          <div class="stat">
            <div class="stat-label">{{ card.label }}</div>
            <div class="stat-value" :style="{ color: card.color }">{{ card.value }}</div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 采购/销售汇总 -->
    <el-row :gutter="16" style="margin-top: 16px">
      <el-col :span="12">
        <el-card shadow="never">
          <template #header>采购汇总（按供应商）</template>
          <el-table :data="purchaseSummary" stripe border size="small">
            <el-table-column prop="supplier_id" label="供应商ID" width="110" />
            <el-table-column label="采购金额(¥)" min-width="140">
              <template #default="{ row }">{{ Number(row.total_amount).toFixed(2) }}</template>
            </el-table-column>
            <el-table-column prop="return_amount" label="退货金额" width="120" /><el-table-column prop="net_amount" label="净额" width="120" /><el-table-column prop="order_count" label="单数" width="90" />
          </el-table>
        </el-card>
      </el-col>
      <el-col :span="12">
        <el-card shadow="never">
          <template #header>销售汇总（按客户）</template>
          <el-table :data="salesSummary" stripe border size="small">
            <el-table-column prop="customer_id" label="客户ID" width="110" />
            <el-table-column label="销售金额(¥)" min-width="140">
              <template #default="{ row }">{{ Number(row.total_amount).toFixed(2) }}</template>
            </el-table-column>
            <el-table-column prop="return_amount" label="退货金额" width="120" /><el-table-column prop="net_amount" label="净额" width="120" /><el-table-column prop="order_count" label="单数" width="90" />
          </el-table>
        </el-card>
      </el-col>
    </el-row>

    <!-- 库存概况 -->
    <el-card shadow="never" style="margin-top: 16px">
      <template #header>库存概况（按仓库）</template>
      <el-table :data="inventorySummary" stripe border size="small">
        <el-table-column prop="warehouse_id" label="仓库ID" width="110" />
        <el-table-column prop="product_count" label="商品种类数" width="130" />
        <el-table-column prop="reserved_qty" label="预占数量" width="120" /><el-table-column prop="available_qty" label="可用数量" width="120" /><el-table-column prop="total_quantity" label="库存总量" min-width="140" />
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { getDashboard, getPurchaseSummary, getSalesSummary, getInventorySummary } from '@/api/report'

const dashboard = ref({
  product_count: 0, inventory_total: 0, low_stock: 0,
  purchase_total: 0, sales_total: 0, gross_profit: null as number | null, receivable: 0, payable: 0,
})
const purchaseSummary = ref<any[]>([])
const salesSummary = ref<any[]>([])
const inventorySummary = ref<any[]>([])

const statCards = computed(() => [
  { label: '商品总数', value: dashboard.value.product_count, color: '#409eff' },
  { label: '库存总量', value: dashboard.value.inventory_total, color: '#67c23a' },
  { label: '低库存预警', value: dashboard.value.low_stock, color: '#f56c6c' },
  { label: '毛利(¥)', value: dashboard.value.gross_profit==null?'历史成本待补全':Number(dashboard.value.gross_profit).toFixed(2), color: '#e6a23c' },
  { label: '采购总额(¥)', value: Number(dashboard.value.purchase_total).toFixed(2), color: '#909399' },
  { label: '销售总额(¥)', value: Number(dashboard.value.sales_total).toFixed(2), color: '#409eff' },
  { label: '应收(¥)', value: Number(dashboard.value.receivable).toFixed(2), color: '#f56c6c' },
  { label: '应付(¥)', value: Number(dashboard.value.payable).toFixed(2), color: '#e6a23c' },
])

async function loadAll() {
  try {
    const [d, p, s, i] = await Promise.all([
      getDashboard(), getPurchaseSummary(), getSalesSummary(), getInventorySummary(),
    ])
    dashboard.value = d.data
    purchaseSummary.value = Array.isArray(p.data) ? p.data : []
    salesSummary.value = Array.isArray(s.data) ? s.data : []
    inventorySummary.value = Array.isArray(i.data) ? i.data : []
  } catch (e) {
  }
}

onMounted(loadAll)
</script>

<style scoped>
.stat {
  text-align: center;
}
.stat-label {
  font-size: 13px;
  color: #999;
  margin-bottom: 8px;
}
.stat-value {
  font-size: 22px;
  font-weight: 600;
}
</style>
