# Kernel NULL Pointer Dereference Error

## Notes

- **Line 1**: Summarizes what happened. It indicates that a kernel NULL pointer dereference occurred.
- **Line 18**: Shows where the fault occurred in the code. The `pc` (program counter) points to the function `faulty_write` with an offset, helping identify the location of the issue.

## Error Message

```plaintext
1  Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
2  Mem abort info:
3    ESR = 0x96000045
4    EC = 0x25: DABT (current EL), IL = 32 bits
5    SET = 0, FnV = 0
6    EA = 0, S1PTW = 0
7    FSC = 0x05: level 1 translation fault
8  Data abort info:
9    ISV = 0, ISS = 0x00000045
10   CM = 0, WnR = 1
11   user pgtable: 4k pages, 39-bit VAs, pgdp=0000000042080000
12   [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
13   Internal error: Oops: 96000045 [#1] SMP
14   Modules linked in: faulty(O) hello(O) scull(O)
15   CPU: 0 PID: 126 Comm: sh Tainted: G           O      5.15.18 #1
16   Hardware name: linux,dummy-virt (DT)
17   pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
18   pc : faulty_write+0x14/0x20 [faulty]
19   lr : vfs_write+0xa8/0x2b0
20   sp : ffffffc008d23d80
21   x29: ffffffc008d23d80 x28: ffffff80020d8cc0 x27: 0000000000000000
22   x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
23   x23: 0000000040001000 x22: 000000000000000c x21: 0000005559e821a0
24   x20: 0000005559e821a0 x19: ffffff8002069800 x18: 0000000000000000
25   x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
26   x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
27   x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
28   x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
29   x5 : 0000000000000001 x4 : ffffffc0006fc000 x3 : ffffffc008d23df0
30   x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
31   Call trace:
32   faulty_write+0x14/0x20 [faulty]
33   ksys_write+0x68/0x100
34   __arm64_sys_write+0x20/0x30
35   invoke_syscall+0x54/0x130
36   el0_svc_common.constprop.0+0x44/0xf0
37   do_el0_svc+0x40/0xa0
38   el0_svc+0x20/0x60
39   el0t_64_sync_handler+0xe8/0xf0
40   el0t_64_sync+0x1a0/0x1a4
41   Code: d2800001 d2800000 d503233f d50323bf (b900003f)
42   ---[ end trace 9d15569e1bb3c522 ]---
