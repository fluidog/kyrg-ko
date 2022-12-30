1. 添加进程
   ```batchfile
   echo "add 1 0" > /sys/kernel/security/kyrg/policy_process   # 添加
   cat /sys/kernel/security/kyrg/policy_process  # 验证
   ```
2. 添加模块
   ```batchfile
   echo "add kyrg" > /sys/kernel/security/kyrg/policy_module
   cat /sys/kernel/security/kyrg/policy_module
   ```
3. 修改周期
   ```batchfile
   echo 10 > /sys/kernel/security/kyrg/period
   cat /sys/kernel/security/kyrg/period
   ```
4. 开关功能
   ```batchfile
   echo 0 > /sys/kernel/security/kyrg/status # 关闭
   cat /sys/kernel/security/kyrg/status # 验证
   ```
