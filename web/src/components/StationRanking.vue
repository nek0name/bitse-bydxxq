<script setup lang="ts">
import { useChart } from '../composables/useChart'
import type { StationRankingItem } from '../types/dashboard'
import { graphic, firstTooltipItem } from '../lib/echarts'

const props = defineProps<{ data: StationRankingItem[] }>()

const chartElement = useChart(() => {
  const rows = props.data.slice(0, 6)
  return {
    grid: { left: 82, right: 24, top: 10, bottom: 20 },
    tooltip: {
      trigger: 'axis',
      renderMode: 'richText',
      axisPointer: { type: 'shadow' },
      formatter: (params) => {
        const point = firstTooltipItem(params)
        const item = point && rows[point.dataIndex]
        return item ? `${item.stationName}\n${item.energyKwh.toFixed(1)} kWh` : ''
      },
    },
    xAxis: {
      type: 'value',
      axisLabel: { color: '#AAA8B8', fontSize: 10 },
      splitLine: { lineStyle: { color: 'rgba(122,153,190,.12)' } },
    },
    yAxis: {
      type: 'category',
      inverse: true,
      data: rows.map((row) => row.stationName),
      axisLabel: { color: '#AAA8B8', fontSize: 11, width: 72, overflow: 'truncate' },
      axisLine: { show: false },
      axisTick: { show: false },
    },
    series: [
      {
        type: 'bar',
        barWidth: 12,
        data: rows.map((row) => ({
          value: row.energyKwh,
          itemStyle: {
            color: new graphic.LinearGradient(1, 0, 0, 0, [
              { offset: 0, color: '#C8C3EB' },
              { offset: 1, color: '#857AD6' },
            ]),
          },
        })),
        label: {
          show: true,
          position: 'right',
          color: '#F0EFF5',
          fontSize: 10,
          formatter: ({ dataIndex }) => rows[dataIndex].energyKwh.toFixed(1),
        },
        itemStyle: { borderRadius: [0, 6, 6, 0] },
      },
    ],
  }
})
</script>

<template><div ref="chartElement" class="chart" /></template>
