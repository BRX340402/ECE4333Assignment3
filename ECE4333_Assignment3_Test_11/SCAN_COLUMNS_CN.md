# 扫描 CSV 字段

CSV 为 UTF-8 BOM，所有单元格正确加引号。每行是网页收到的一次状态快照或记录事件；各数据源并非同时采样。所有 JSON 字段递归展开为带点的列名，新增接口字段也会进入表格；没有的数据留空。

| 列 | 含义/单位 |
|---|---|
| scan_id / host_iso / elapsed_s | 本次扫描ID、电脑接收时刻UTC、本次记录开始后的秒数（包括启动和结束等待） |
| event / detail | sample、connection_gap、start_confirmed、start_unconfirmed、finish_confirmed 及事件说明 |
| segment | UNO运行编号改变或计时回退时递增，避免把重启前后连接成一条轨迹 |
| uno_fresh / uno_new | N221接收年龄小于1500ms；本行的UT或RN是否区别于上次记录。分析独立UNO样本应同时选1 |
| sensors_fresh / sensors_new | N210接收年龄小于2500ms；N210的UT是否变化。分析独立N210样本应同时选1 |
| esp_ms / age_ms / sensors_age_ms | ESP32时间、N221/N210在ESP32缓存的接收年龄，毫秒 |
| uno.UT / sensors.UT | 各UNO消息生成时刻，毫秒；不是精确的每个传感器采样时刻 |
| uno.L / uno.C / uno.R | 左/中/右循迹ADC，0～1023 |
| sensors.U | 超声波距离，cm；负值表示无有效距离 |
| sensors.V | 电池电压，mV |
| uno.GY / uno.YA | 陀螺仪可用标志；累计Z轴航向角，整数度 |
| sensors.GR10 | 偏置修正后的Z轴角速度×10，除以10得到deg/s；GY=0时不能视为有效测量 |
| uno.KX / uno.KY / uno.KH | 轮指令运动学预测X/Y(cm)及航向(deg) |
| uno.X / uno.Y / uno.H | 陀螺仪辅助估计X/Y(cm)及航向(deg)；陀螺仪不可用时退化为轮模型 |
| uno.PL / uno.PR | 左右电机有符号PWM指令，非实测轮速 |
| uno.M / uno.TS / uno.RN | 运行模式、交通状态、UNO运行编号 |
| uno.FC / uno.FS / uno.ES / uno.ER | 故障码、故障来源状态、急停标志及原因 |
| uno.SE / uno.SP | 是否搜线、搜线阶段 |
| uno.GN / uno.ZR / uno.PD | 陀螺仪导航开关、手动归零修订号、队友PD开关 |
| uno.FR / uno.UV / uno.GC / uno.ID | 剩余RAM字节、固件协议版本、绿灯计数、识别请求编号 |
| sensors.S / sensors.Q | 云台角度(deg)、旧视觉数据序号 |
| sensors.L/C/R/A/D/M | N210时刻的循迹/左右PWM/模式，与N221未必同一时刻 |
| sensors.B/G/T/AI/VF/P/VD/LED | 原接口原因、保护状态、视觉锁存、AI结果、新鲜标志、置信度、距离、LED模式；仅按原来源解释 |
| objects.red.* / objects.green.* / objects.stop.* | 对应目标有效性、是否看到、像素数、宽度、距离(cm)、框坐标和推理时间等所有现有字段 |
| vision_health.* | 颜色帧序号/年龄/处理耗时/区域数等，按返回字段记录；相同帧不是一次新的独立识别 |
| calibration.* | 识别尺寸/图像尺寸/参考距离/有效标志等标定参数 |
| settings.* | 当前巡航速度、减速、阈值、搜线角度、应用确认等 |
| framesize / detect_enabled / color_enabled | 摄像头framesize枚举（非像素宽度）、STOP检测开关、颜色检测开关 |

TS：0 OFF；2 TRACK；3 APPROACH；4 VERIFY STOP；5 WAIT FOR GREEN；6 STOP HOLD；7 CROSS STRAIGHT；8 FAULT。

分析先排除缺口和过期快照，再按UNO源时间戳分别对齐。相机也应按帧序号/年龄判定新鲜度，不应把每一网页快照算成一次新识别。finish_confirmed 仅代表操作者确认已跑完并收到了停车反馈，不是自动覆盖率证明。

Test10补充：uno.IC是零偏校准尝试的修订号；uno.IV为方向验证位（0未验证、1左通过、3左右通过）。GN默认1，自主启动要求GY=1且IV=3。settings.RA/SA现在分别是右/左相对丢线航向的恢复边界，30～110度，不能沿用旧版“先右扫RA再左扫SA”的解释。SE=1时SP=0为局部修正，1～6为有限范围脉冲寻找阶段。YA保留连续角度，网页仅对显示值归一化。N221目标发送间隔600ms；无法记录每个控制周期。

## Test11 设备日志补充（当前版本）
设备 CSV 的列名与网页 live table 不同：设备源数据统一放在 data 下。

| 列 | 含义 |
|---|---|
| esp_ms | ESP32 收到/生成本条记录的时刻，毫秒 |
| source | uno / command / objects / event / settings / calibration 等 |
| data.N | UNO 消息类型或发出的命令号 |
| data.UT | UNO 自开机计时，毫秒；不要与 esp_ms 直接当同一时钟 |
| data.RM | N210：0 原逻辑，1 无交通控制采集，2 固定路线+交通 |
| data.RI | N210：下一个弯道索引 0～4，对应网页 1～5 |
| data.RL | N210：按弯道顺序确认的估计圈数，不是位置实测证明 |
| data.RT | N210：预计出口的连续航向，度；内部地图参考整度 |
| data.RC | N210：路线配置修订号，0～255 回绕 |
| data.YA | N221：连续陀螺仪航向，度 |
| data.GR10 | N210：陀螺仪角速度乘 10 |
| data.PL / data.PR | N221：输出左右 PWM |
| data.X/Y/H | N221：陀螺仪融合的估计位姿 |
| data.KX/KY/KH | N221：轮指令模型估计位姿 |
| data.red.* / data.green.* / data.stop.* | source=objects 的相机对象快照 |
| data.type | source=event：start_request、run_observed、stopped |
| data.turns | 启动时五个路线转角数组 |
| device_log.dropped | 导出元数据：因容量不足丢弃的记录数，非零即不完整 |

N210/N221 的其他传感器字段沿用此前定义；Test11 移除了始终为0的PD占位字段。不同来源的数据不是同步采样，空格不等于0。暂停、红灯等待、断线及复位时应根据时序和状态字段分析，而非只连坐标点。
