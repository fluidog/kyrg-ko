此测试用例针对动态防护的进程、模块、内核的完整性防护功能分别进行测试。

## 自动测试

执行（插入）动态防护模块后，在此目录下直接执行 `make test` 进行测试。如果后台审计日志中分别显示一个进程、一个模块和内核代码被篡改，即测试成功，防护功能正常。显示审计信息如下：

```bash
type=UNKNOWN[1467] msg=audit(12/30/2022 16:04:35.763:448) : process: 21336 has been modified
type=UNKNOWN[1467] msg=audit(12/30/2022 16:04:35.867:449) : module: kyrg has been modified
type=UNKNOWN[1467] msg=audit(12/30/2022 16:04:36.191:450) : kernel has been modified
```

## 测试原理

为了修改内核和模块的代码段，本测试用例使用 kprobe，当 kprobe 一个内核符号（函数）时，内核会替换符号处的指令。直接执行（插入）测试模块，即可修改指定模块或内核的代码段。

为了修改进程的代码段，进程自行调用 `mprotect` 系统调用即可将自己的代码段重新映射为可读写段，从而修改代码段。但为了控制修改代码段的时机，此测试程序只有当收到指定信号后，才会修改自己的代码段。

## 手动测试

### 1. 进程防护

1. 执行指定程序

   ```bash
   ./test-kyrg-process &
   ```

2. 添加进程防护对象

   ```bash
   echo "add `pidof -s test-kyrg-process` 1" > /sys/kernel/security/kyrg/policy_process
   ```

3. 立即触发动态防护 （显示进程未杀死）

   ```
   echo 1 > /sys/kernel/security/kyrg/immediate
   ```

4. 发送指定信号，触发进程自行修改代码段

   ```
   kill -10 `pidof test-kyrg-process`
   ```

5. 立即触发动态防护

   ```
   echo 1 > /sys/kernel/security/kyrg/immediate
   ```

6. 查看结果

   审计信息中会显示进程度量失败的信息，进程会根据配置或将杀死。显示审计信息如下：

   ```bash
   type=UNKNOWN[1467] msg=audit(12/30/2022 16:04:35.763:448) : process: 21336 has been modified
   ```

### 2. 模块（内核）防护

1. 添加模块防护对象

   ```bash
   echo "add kyrg" > /sys/kernel/security/kyrg/policy_module
   ```

2. 插入测试模块，对保护的模块代码进行修改。以下命令表示修改 `do_rg` 函数处的代码，`do_rg` 函数位于 kyrg 模块内。

   ```bash
   insmod test-kyrg.ko name=do_rg
   ```

3. 立即触发动态防护

   ```bash
   echo 1 > /sys/kernel/security/kyrg/immediate
   ```

4. 查看结果

   审计信息中会显示如下模块度量失败的信息：

   ```bash
   type=UNKNOWN[1467] msg=audit(12/30/2022 16:04:35.867:449) : module: kyrg has been modified
   ```

   

   