# SPI NAND W25N02KV 阶段性测试记录

日期: 2026-05-10

## 目标

本阶段目标是只测试 AWBoot 当前 SPI NAND 读取程序的能力: 在目标板上从 Winbond W25N02KV 的 offset `0x00000000` 读取 1 MiB 数据, 验证读出的内容是否正确, 并观察实际读取速度。

测试数据已经预先写入 NAND, 数据本身视为可信。若校验失败, 只分析 SPI NAND 读取路径。

目标板 UART 使用 UART3, TX `PB6`, RX `PB7`。

## 测试数据

NAND offset `0x00000000` 开始写入 1 MiB LCG 数据:

```c
x[0] = 0x811c9dc5;
x[i] = x[i - 1] * 1664525 + 1013904223;
```

数据按 little-endian 32-bit word 存储, 共 `262144` 个 word。

AWBoot `spitest` 变体读取 `CONFIG_SPINAND_TEST_ADDR` 和 `CONFIG_SPINAND_TEST_LEN`, 当前配置为:

```c
#define CONFIG_SPINAND_TEST_ADDR 0x00000000U
#define CONFIG_SPINAND_TEST_LEN  MB(1)
#define CONFIG_SPINAND_TEST_SEED 0x811c9dc5U
```

校验失败时会打印:

- mismatch 总数
- 第一个 mismatch 的 word index / offset / expected / got
- 第一个错误点附近 128 bytes dump

## 构建和运行

构建:

```sh
make VARIANT=spitest build LOG_LEVEL=40 CROSS_COMPILE=arm-none-eabi
make VARIANT=spitest mkboot CROSS_COMPILE=arm-none-eabi
```

产物:

```text
awboot-spinand-test-fel.bin
```

FEL 运行方式:

```sh
xfel ddr t113-s3
xfel write 0x00028000 awboot-spinand-test-fel.bin
xfel exec 0x00028000
```

## 当前阶段结论

当前阶段保留的稳定版本是 page read 方式:

1. 检测 W25N02KV。
2. 清 Winbond `A0h` protect register。
3. 清 Winbond `B0h` 的 `ECC-E` 位。
4. 保持 `BUF=1`, 不启用 continuous read。
5. 每页执行 `13h Page Data Read`。
6. 等待 busy 清零。
7. 使用 `6Bh Fast Read Quad Output` 通过 DMA 读取当前页主区数据。

该版本已经在目标板上校验通过:

```text
SPI-NAND TEST: read returned 1048576, time=38869us speed=25MB/S
SPI-NAND TEST: PASS, 262144 words
```

速度约 `25 MB/S`, 正确性稳定。该版本作为阶段性基线。

## 已确认的问题和修复

### FIFO / PIO 基础问题

早期驱动存在基础收发问题:

- `SPI_FCR_RX_LEVEL_MSK` 写成了错误表达式。
- `spi_query_txfifo()` 固定返回 0, 不能反映 TX FIFO 真实状态。
- PIO 4-byte 批量读写循环逻辑错误。
- TX-only 命令没有可靠等待 TX 完成, 会影响 reset / write enable / set feature 等命令。

这些问题已修复, 否则读 ID / 写 feature / page load 可能卡住或数据偏移。

### W25N02KV ID

目标 NAND 读 ID 为:

```text
ff ef aa 22
```

第一个 `ff` 是 dummy/shift 字节, 有效 ID 为:

```text
mfr = 0xef
dev = 0xaa22
```

驱动中固定匹配为 `W25N02KV`, 参数:

```text
page_size       = 2048
spare_size      = 128
pages_per_block = 64
blocks_per_die  = 2048
mode            = SPI_IO_QUAD_RX
```

### Winbond feature 配置

启动时观察到:

```text
Winbond protect 0x7c before clear
Winbond protect 0x00 after clear
Winbond cfg2 0x19 before page read
Winbond cfg2 0x09 after ECC clear
```

说明:

- `A0h` protect register 初始为 `0x7c`, 需要清零。
- `B0h` 初始 `0x19`。
- 清 `ECC-E` 后为 `0x09`。
- 当前 page read 基线不再清 `BUF`, 避免进入 continuous 路径。

注意: 本轮调试确认 W25N02KV 的 `B0h` 中 `BUF` 是 bit0 (`0x01`), `0x08` 是 `H-DIS`, 不能把 `0x08` 当作 BUF 位。

## Continuous Read 尝试记录

本轮尝试过 W25N02KV continuous read, 但未得到正确且更快的结果, 因此当前阶段放弃 continuous。

### 1. 直接启用 BUF=0 后一次性读 1 MiB

配置:

```text
B0h: 0x19 -> 0x09 -> 0x08
continuous read enabled
```

一次性 `6Bh` 读 1 MiB 时:

```text
time ~= 21028us
speed ~= 47MB/S
first mismatch offset=0x00000800
0x00000800: ff ff ff ff ...
```

第一页 `0x00000000..0x000007ff` 正确, 从 `0x800` 开始错误。说明高速 raw stream 没有得到第二页主区数据。

### 2. 认为 raw stream 包含 OOB, 按 `2048 + 128` 压缩

W25N02KV 页大小是 `2048 + 128`, 因此尝试读取 raw stream 后跳过每页 128B spare。

现象:

```text
continuous raw=1113984 pages=512 oob-skip=65408
first mismatch offset=0x00000800
got=0xeeeeeeee 或 0xffffffff
```

说明第一页后并未自然出现下一页主区数据, 即使按 OOB 跳过也不正确。

### 3. 尝试 32 dummy clocks

曾尝试把 `6Bh` continuous read 的 dummy 从 24 clocks 调整为 32 clocks。

作为 TX byte 发送时, 首页整体丢 4 字节:

```text
first mismatch word=0
expected=0x811c9dc5
got=0x99fc7460
```

这说明该控制器路径下, 这些额外字节不是正确的 dummy 表达方式。

使用 SPI 控制器 BCC dummy counter 生成 32 clocks 后, 现象仍为首页丢 4 字节, 不成立。

### 4. 尝试保持 CS 并分段读取

尝试用 GPIO 强制 /CS 保持低:

```text
读 2048B 主区
读/跳 128B spare
等待 25us / 35us / 80us
继续读下一页
```

结果仍在 `0x800` 失败:

```text
0x00000800: ee ee ff ff ff ff ...
```

说明只靠 GPIO 保持 CS 不能让芯片/控制器保持在有效 continuous read 状态内, 或页边界后的时钟序列并不是这样使用。

### 5. 不发送 13h 的 sequential read

尝试跳过 `13h Page Data Read`, 直接 `6Bh` 读 raw stream。

结果首页也错误:

```text
first mismatch word=0
got=0x3bc41dc5
```

说明当前测试路径仍需要先执行 `13h` 装载起始页。

### 6. 大窗口 raw scan

尝试在第一页后搜索第二页第一个 LCG word, 搜索窗口从 4KB 扩到 64KB。

结果:

```text
raw page stride not found
```

说明在当前 `13h + 6Bh` raw continuous stream 中, 第二页主区数据没有在合理窗口内自然出现。

## 当前实现细节

当前稳定实现位于:

```text
arch/arm32/mach-t113s3/sunxi_spi.c
```

### 初始化和识别

`spi_nand_detect()`:

1. `OPCODE_RESET (ffh)` reset NAND。
2. `OPCODE_READ_ID (9fh)` 读 4 bytes ID。
3. 若 raw ID 为 `ff ef aa 22`, 按 W25N02KV 填充 `spi->info`。
4. 对 Winbond:
   - 读 `A0h` protect register。
   - 如果非 0, 写 0 并读回确认。
   - 调用 `spi_nand_winbond_config_page_read()` 清 ECC-E。

`spi_nand_winbond_config_page_read()`:

```c
spi_nand_get_config(spi, CONFIG_ADDR_OTP, &cfg);
if ((cfg & CONFIG_POS_ECC) != 0U) {
    spi_nand_set_config(spi, CONFIG_ADDR_OTP, cfg & ~CONFIG_POS_ECC);
    spi_nand_wait_while_busy(spi);
    spi_nand_get_config(spi, CONFIG_ADDR_OTP, &cfg);
}
```

该函数不清 `BUF`, 因此不启用 continuous read。

### 每页读取

`spi_nand_load_page()`:

```c
pa = offset / spi->info.page_size;
tx[0] = OPCODE_READ_PAGE;  // 13h
tx[1] = pa >> 16;
tx[2] = pa >> 8;
tx[3] = pa;
spi_transfer(spi, SPI_IO_SINGLE, tx, 4, 0, 0);
spi_nand_wait_while_busy(spi);
```

`spi_nand_read()` page loop:

```c
while (cnt > 0) {
    ca = address & (spi->info.page_size - 1);
    n = min(cnt, spi->info.page_size - ca);

    spi_nand_load_page(spi, address);

    tx[0] = OPCODE_FAST_READ_QUAD_O; // 6Bh
    tx[1] = ca >> 8;
    tx[2] = ca;
    tx[3] = 0x0;                     // dummy

    spi_transfer(spi, SPI_IO_QUAD_RX, tx, 4, buf, n);

    address += n;
    buf += n;
    len += n;
    cnt -= n;
}
```

### SPI 传输

当前 W25N02KV 使用 `SPI_IO_QUAD_RX`:

- opcode / column / dummy 通过单线 TX 阶段发送。
- data 通过四线 RX 阶段接收。
- `rxlen > 64` 时使用 DMA。
- 1 MiB 测试实际为每页 2048B DMA 接收。

## 阶段性基线

保留当前 page read 实现作为阶段性版本:

```text
W25N02KV + T113 SPI0 @ 100MHz + Quad RX + DMA
读取 1 MiB LCG 数据
正确性: PASS
速度: 约 25 MB/S
```

continuous read 暂时不作为可用路径。后续若继续研究, 应优先使用逻辑分析仪确认 /CS、SCLK、IO0-IO3 在页边界处的真实波形, 而不是继续只靠软件猜测时序。
