# 负荷预测与公开数据回测

C++ 服务通过 JSON 调用预测模块，负责数据库读写。管理端“负荷预测”可运行任务，输入输出见[接口契约](../docs/接口契约.md)。以下命令从仓库根目录执行。

```sh
uv sync --project ml --frozen
uv run --project ml python -m ml.service --input /tmp/forecast-input.json --output /tmp/forecast-output.json
uv run --project ml pytest ml/tests -q
uv run --project ml ruff check ml
```

## 业务预测

订单电量按会话与整点小时的重叠时间分摊，小时平均负荷以 kW 表示。每桩具有至少 14 天历史和 48 个非零小时后训练随机森林，其余使用时段均值或当前状态估计。输出未来 24 小时负荷和空闲桩数，站级结果由桩级汇总，受额定功率和设备状态约束。

特征包含历史负荷、时段、星期、年周期与中国法定节假日。传入日天气时使用预测原点前一日的天气。高峰阈值采用历史小时负荷的 85 分位数，无历史时采用容量的 70%。结果记录历史截止时间、预测方法、天气来源和充电倍率。

## 公开数据回测

```sh
bash ml/download_datasets.sh jiaxing
uv run --project ml python -m ml.backtest \
  --data reference/datasets/jiaxing_2025/Dataset/Charging_Data.csv \
  --output ml/artifacts/jiaxing
```

下载脚本支持 `beijing`、`jiaxing` 和 `all`，原始文件存入 `reference/datasets`，按来源校验 MD5，不提交 Git。嘉兴交易 CSV 与 `Weather_Data.csv` 放在同一目录。

回测按时间前 80% 训练，后 20% 每 6 小时滚动测试。训练标签须在截止时间之前，标准化只使用训练数据。使用 MAE、RMSE、WAPE 与上周同期基线比较，并审计电量分摊前后的总量。

输出 `report.json`、`report.md` 和 `models.pkl` 至 `ml/artifacts/jiaxing`。已提交的[回测报告](reports/jiaxing/report.md)中，随机森林误差高于周周期基线。模型参数固定，业务站点使用各自历史训练。

数据来源：[嘉兴充电数据集](https://doi.org/10.6084/m9.figshare.28182251)。节假日使用 [chinese-calendar](https://github.com/liriansu-opus/chinese-calendar)。
