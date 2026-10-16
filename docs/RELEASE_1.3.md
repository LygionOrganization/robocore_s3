# RoboCore S3 1.3

总线舵机按 STS/SMS、HLS、SCS 分组，每组独立展示 ID、位置控制和反馈组件，支持手机布局。

IMU：启动软复位与复位完成检查；禁用残留 FIFO 模式；配置回读；检查数据就绪和 24 位样本计数；丢弃跨更新读取的数据；没有新数据时限时返回 imu_data_timeout。T=740 增加 sample_count。

依据：QST QMI8658A Rev A 第 5.3、5.5、5.9、7.4 节。
https://files.waveshare.com/upload/5/5f/QMI8658A_Datasheet_Rev_A.pdf

主机回归覆盖姿态对应数值变化、计数停滞、回绕、未就绪、跨帧、I²C 失败、配置失败和复位失败。网页分组及双语/移动布局测试通过。

未在用户板子上验证，不能认定已经确认读数恒定的实机根因。请烧录后连续执行 T=740 并观察 sample_count 与 accel_g。
