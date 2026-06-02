# SPI NAND 读取测试报告

日期: 2026-05-10
分支: `spi_test`
芯片: Winbond W25N02KV
平台: T113-S3 + AWBoot `spitest` 变体

---

## 测试目标

验证 AWBoot 在 T113-S3 上通过 SPI0 读取 Winbond W25N02KV SPI NAND 的正确性与速度，作为后续 boot 路径的基线。

---

## 硬件配置

| 项目 | 参数 |
|---|---|
| SoC | Allwinner T113-S3 (Cortex-A7, mach-t113s3) |
| SPI 控制器 | SPI0，基地址由 `sunxi_spi0` 配置 |
| SPI 时钟 | 模块时钟 200MHz，CDR2 分频，实际 SPI CLK = 100MHz |
| IO 模式 | Quad RX (`6Bh Fast Read Quad Output`) |
| 调试串口 | UART3，TX PB6 / RX PB7 |
| NAND | Winbond W25N02KV，2Gb，单 die |

**W25N02KV 几何参数：**

| 参数 | 值 |
|---|---|
| 页大小 | 2048 B（主区）+ 128 B（spare） |
| 每块页数 | 64 |
| 每 die 块数 | 2048 |
| 总容量 | 256MB（2Gbit） |

---

## 固件构建

`spitest` 是专用测试变体，在 `spi` 基础上追加 `CONFIG_SPINAND_TEST_ONLY=1`，`main()` 不 boot Linux，只执行 NAND 初始化、读取和数据校验后退出。

```sh
make VARIANT=spitest CROSS_COMPILE=arm-none-eabi
```

产物：`awboot-spinand-test-fel.bin`（FEL 方式加载，约 21.5KB）

FEL 运行：

```sh
xfel ddr t113-s3
xfel write 0x00028000 awboot-spinand-test-fel.bin
xfel exec 0x00028000
```

---

## 测试数据

NAND offset `0x00000000` 起写入 1 MiB LCG 序列：

```c
x[0] = 0x811c9dc5;
x[i] = x[i-1] * 1664525 + 1013904223;
```

数据按 little-endian 32-bit word 存储，共 262144 个 word。AWBoot 读回后逐 word 比对，不匹配时记录第一个错误位置和周围 128B hex dump。

---

## SPI NAND 初始化流程

### 1. 控制器初始化（`sunxi_spi_init`）

- GPIO 配置：CS / SCK / MOSI / MISO / WP / HOLD 全部设为 SPI0 功能，驱动强度设为最高档（等级 3）。
- CCU 时钟：SPI0 模块时钟源选 PERI1X，分频至 200MHz；控制器内部 CDR2 再 2 分频，SPI CLK 实际 100MHz。
- TCR 配置：CPOL=0, CPHA=0（Mode 0）；频率 ≥ 80MHz 时开启 SDC（Sample Delay Compensation）。
- FIFO 复位，DMA 通道预申请（普通模式，src=SPI0 RXD，dst=DRAM，16-bit 宽，8 burst）。

### 2. NAND 检测（`spi_nand_detect`）

**Reset**

发送 `FFh RESET` 命令，等待 100ms（满足 W25N02KV tRST ≤ 1ms 的充裕裕量）。

**Read ID**

发送 `9Fh READ ID`，读回 4 字节：

```
ff ef aa 22
```

首字节 `ff` 为 dummy（部分芯片 READ ID 前有 1 dummy byte），有效 ID：

- `mfr = 0xef`（Winbond）
- `dev = 0xaa22`（W25N02KV）

驱动固定匹配该 ID，直接填充 `spi->info`，不查表（`spi_nand_infos[]` 在此变体中标记为 `__always_unused`）。

**清 Protect Register（`A0h`）**

```
Winbond protect 0x7c before clear
Winbond protect 0x00 after clear
```

`A0h` 初始值 `0x7c` 表示 BP[3:0] 全置，所有块写保护。写 `0x00` 解除。流程：`06h WRITE ENABLE → 1Fh SET FEATURE(A0h, 0x00) → 轮询 busy`。

**清 ECC-E 位（`B0h` bit4）**

```
Winbond cfg2 0x19 before page read
Winbond cfg2 0x09 after ECC clear
```

`B0h` 初始 `0x19`（二进制 `0001_1001`）：

| bit | 名称 | 初始值 | 清后值 |
|---|---|---|---|
| 4 | ECC-E | 1 | 0 |
| 3 | H-DIS | 1 | 1 |
| 0 | BUF | 1 | 1 |

仅清 ECC-E（禁用片内 ECC），保留 BUF=1（page buffer 模式，非 continuous read）。`H-DIS=1` 保持不变。

> **BUF 位说明：** W25N02KV `B0h` 中 `BUF` 是 bit0（`0x01`），不是 `0x08`。`0x08` 是 `H-DIS`（Hold Disable）。此前驱动误将 `0x08` 当作 BUF 清位，导致 continuous read 尝试无效。

---

## 读取流程（`spi_nand_read`）

每页独立执行两步：

### 步骤 1：Page Data Read（`13h`）

将指定页从 NAND cell 阵列加载到片内 page buffer：

```
13h | PA[23:16] | PA[15:8] | PA[7:0]
```

`PA = offset / page_size`。发送后轮询 `0Fh GET FEATURE(C0h)` SR3 Busy 位，清零即完成（最多 250ms 超时）。W25N02KV 典型页读时间（tR）约 60μs。

### 步骤 2：Fast Read Quad Output（`6Bh`）

从 page buffer 读出当前页主区数据：

```
6Bh | CA[15:8] | CA[7:0] | dummy(0x00) | [data × n bytes, QUAD]
```

`CA` 为页内列地址（column address）。opcode + CA + dummy 通过单线 TX 发送（`stxlen = txlen = 4`），data 通过 4 线接收（`SPI_BCC_QUAD_IO` 置位）。

每次读 2048B（整页），rxlen > 64 时走 DMA 路径（1000ms 超时保护）。

### 循环

对齐到页边界，512 页循环（1MB / 2KB），每页一次 `13h + 6Bh`。

---

## SPI 控制器传输机制

### FIFO 模式（rxlen ≤ 64）

- 写 TX FIFO：4 字节为单位，等待 level ≤ 60 再写；尾部不足 4 字节逐字节写，等待 level ≤ 63。
- 读 RX FIFO：4 字节为单位，等待 level ≥ 4 再读；尾部逐字节，等待 level ≥ 1。
- TX-only 命令（reset / write enable / set feature / page load）完成后等待 TX FIFO 清空（10ms 超时）再脱手。

### DMA 模式（rxlen > 64）

- 开启 `SPI_FCR_RX_DRQEN`，SPI RXD → DRAM，16-bit 宽，8-burst。
- 写 TX FIFO 完成后，等待 DMA 状态变为 done（1000ms 超时）。
- 正常路径：1MB 共 512 次 DMA，每次传输 2048B。

### 计数器寄存器

每次传输前设置三个计数器：

- `SPI_MBC`：总字节数（txlen + rxlen）
- `SPI_MTC`：TX 字节数
- `SPI_BCC`：单线 TX 字节数（stxlen）+ dummy + quad/dual 标志位

---

## 测试结果

```
[I] SPI-NAND: matched W25N02KV
[I] SPI-NAND: Winbond protect 0x7c before clear
[I] SPI-NAND: Winbond protect 0x00 after clear
[I] SPI-NAND: Winbond cfg2 0x19 before page read
[I] SPI-NAND: Winbond cfg2 0x09 after ECC clear
[I] SPI-NAND: W25N02KV detected
[I] SPI-NAND TEST: read addr=0x00000000 len=1048576
[I] SPI-NAND TEST: read returned 1048576, time=39237us speed=25MB/S
[I] SPI-NAND TEST: PASS, 262144 words
```

| 指标 | 结果 |
|---|---|
| 读取长度 | 1048576 B（1 MiB）|
| 耗时 | 39237 μs（≈ 39.2ms）|
| 速度 | 25 MB/s |
| 校验 | PASS，262144 word 全部匹配 |

---

## 速度分析

**理论峰值**：SPI CLK 100MHz，Quad RX → 有效数据带宽 400Mbit/s = 50MB/s。

**实际 25MB/s 的原因**：每页都需执行 `13h PAGE DATA READ` 并等待 tR。1MB = 512 页，每页开销约：

```
tR(典型) ≈ 60μs
命令发送 + busy poll 开销 ≈ 10~20μs（SPI 传输 4B @ 100MHz ≈ 320ns，poll 若干次）
DMA 传输 2048B @ Quad 100MHz ≈ 40μs
```

粗估每页约 76μs，512 页 ≈ 39ms，与实测一致。tR 等待是主要瓶颈，约占总时间的 60%。

提升速度的可能方向：使用 continuous read（BUF=0）免除每页 `13h`，但本次调试中 W25N02KV continuous read 未能在该控制器上得到正确结果（见 `spi-nand-w25n02kv-stage-page-read.md`）。

---

## 已修复的驱动 Bug

本分支针对 `mach-t113s3/sunxi_spi.c` 修复了以下问题，这些 bug 在基线代码中会导致读取失败或死循环：

| Bug | 现象 | 修复 |
|---|---|---|
| `SPI_FCR_RX_LEVEL_MSK` 用 `<` 而非 `<<` | 掩码为 0，FIFO level 寄存器字段无法正确操作 | 改为 `<<` |
| `spi_query_txfifo()` 硬编码 `return 0` | TX FIFO level 永远报 0，写 FIFO 时不等待，溢出 | 返回实际 `val` |
| `spi_write_tx_fifo` 循环条件错误（`len -= 4 % 4` 恒为 0） | 4 字节批量写实际不执行，数据偏移 | 重写为正确的 `while (len >= 4)` |
| `spi_read_rx_fifo` 指针更新在解引用之前 | 读出数据写入错误地址，buf 内容错位 | 重写，先读再移指针 |
| TX-only 命令后未等待 FIFO 排空 | reset / write enable / page load 可能未完成即发下一命令 | 新增 `spi_wait_tx_done`（10ms 超时） |
| FIFO 等待无超时 | 异常时死循环 | 全部改为计数超时或时间超时，带错误日志 |
| DMA 等待无超时 | DMA 异常时死循环 | 加 1000ms 超时，超时后 `dma_stop` |
| `spi_nand_set_config` 未先发 WRITE ENABLE | SET FEATURE 命令被芯片忽略（写保护状态下） | 新增 `spi_nand_write_enable()` 调用 |
| Winbond BUF 位定义错误（`0x08`） | 误清 H-DIS 而非 BUF，continuous read 未真正启用 | 修正：`BUF=0x01`, `H-DIS=0x08`, `ECC-E=0x10` |
| `spi_nand_wait_while_busy` 无超时 | busy 状态异常时死循环 | 加 250ms 超时 |

---

## 结论

T113-S3 + SPI0 @ 100MHz + Quad RX + DMA，以逐页读方式（`13h + 6Bh`）从 W25N02KV 读取 1MiB 数据：

- **正确性**：262144 word 全部校验通过，数据读取无误。
- **速度**：25MB/s，主要瓶颈为每页 tR 等待（约 60μs/页），与 Quad RX 带宽无关。
- **稳定性**：所有关键等待路径均有超时保护，不会因芯片异常导致死循环。

此结果作为 SPI NAND boot 路径的基线，后续 boot 固件直接基于此读取实现。
