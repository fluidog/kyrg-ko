# Kylin runtime guard

此动态安全防护模块，用于防止运行中的内核或进程被篡改。其通过周期性的验证内核、模块、进程的代码段和只读数据段的完整性，来实现此保护功能。

此模块通过核外模块实现，可以应用于各种 linux 发行版，比如 Kylinos、Ubuntu 等，支持内核版本 >= 4.19。

## 快速使用

默认情况不对进程、内核或模块进行防护。按需添加防护对象。

1. 添加进程防护对象，格式：add \<pid> \<enforce>

   ```bash
   # 添加防护 pid 为 1 的进程，度量失败后，不杀死进程。
   echo "add 1 0" > /sys/kernel/security/kyrg/policy_process
   # 查看并验证相关度量信息
   cat /sys/kernel/security/kyrg/policy_process
   ```

2. 添加模块防护对象，格式：add \<mod>

   ```bash
   # 添加模块 kyrg
   echo "add kyrg" > /sys/kernel/security/kyrg/policy_module
   # 查看并验证
   cat /sys/kernel/security/kyrg/policy_module
   ```

3. 设置防护周期

   ```bash
   echo 10 > /sys/kernel/security/kyrg/period
   ```

4. 打开防护功能

   ```bash
   echo 1 > /sys/kernel/security/kyrg/status
   ```

## 功能性测试

为了快速验证动态防护相关功能正常，本模块提供相关测试工具。在源码根目录下执行 `make test`，会自动进行测试，相关测试原理及流程见： [动态防护功能测试](./test/README.md)。

## 性能测试



## 详细设计


详细设计文档见：[ 动态防护 ](https://fluidog.notion.site/1cfbfb77ca7d417695590386290594cd)
