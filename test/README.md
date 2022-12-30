## 测试原理

度量一个进程，当进程的代码段发生变化时，就可以触发动态度量报错机制。为了使进程代码段发生变化，此测试用例提供一个特殊的程序，在收到指定信号后，可以自行修改代码段。

## 测试流程

1. 执行指定程序

   ./test-kyrg &

2. 将进程添加到动态度量库中

   echo add \`pidof test-kyrg` 1 > /sys/kernel/security/kyrg/policy_process

3. 立即触发动态度量 （显示进程未杀死）

   echo 1 > /sys/kernel/security/kyrg/immediate

4. 发送指定信号，触发进程自行修改代码段

   kill -10 \`pidof test-kyrg`

5. 立即触发动态度量 

   echo 1 > /sys/kernel/security/kyrg/immediate

6. 查看结果

   审计信息中会显示进程度量失败的信息，进程会根据配置或将杀死。