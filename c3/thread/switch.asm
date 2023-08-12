[bits 32]
section .text
global switch_to
switch_to:
  ;栈中此处是返回地址
  ;遵循ABI原则，保存好下面四个寄存器
  push esi
  push edi
  push ebx
  push ebp
  ;得到栈中的参数cur,cur是当前运行线程的struct task_struct,也就是PCB
  ;得到的是虚拟地址
  mov eax, [esp + 20]
  ;PCB中的第一个元素是self_kstack，保存的是每一个线程自己的栈顶指针
  ;将当前线程的栈顶指针保存到self_kstack中去
  mov [eax], esp
  ;以上是备份当前线程的情况，下面是恢复下一个线程的环境
  ;得到下一个参数next,next是即将运行的线程的struct task_struct,即PCB
  mov eax, [esp+24]
  ;从即将运行的线程的第一个元素self_kstack中拿到该线程的栈顶指针
  mov esp, [eax]
  ;将该线程保存的寄存器映像恢复，来准备执行该线程
  pop ebp
  pop ebx
  pop edi
  pop esi
  ret