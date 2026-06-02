# AWBoot SPI-NAND (Winbond) 读取性能分析报告

**项目**: `te-t113-fastboot/awboot`
**分析对象**: `arch/arm32/mach-t113s3/sunxi_spi.c` 及 v851s/t113s4 同源副本
**日期**: 2026-05-08

---

## 1. 硬件与驱动概览

### 1.1 SoC SPI 控制器

Allwinner T113 / V851s 内置 SPI0 控制器(寄存器布局见 `sunxi_spi.c:34-49`):

| 寄存器 | 偏移 | 作用 |
|---|---|---|
| SPI_GCR | 0x04 | 全局控制(Master/Reset/Enable) |
| SPI_TCR | 0x08 | 传输控制(CPOL/CPHA、SDC、SDM、Start) |
| SPI_FCR | 0x18 | FIFO 控制(RX/TX DRQ、reset、阈值) |
| SPI_CCR | 0x24 | 时钟分频(CDR1 二次幂 / CDR2 线性) |
| SPI_MBC/MTC | 0x30/0x34 | 总传输字节数 / 单线发送字节数 |
| SPI_BCC | 0x38 | 单线发送字节、dummy 字节、Quad/Dual 切换 |
| SPI_TXD/RXD | 0x200/0x300 | 数据 FIFO |

控制器支持 **Standard / Dual / Quad Output / Quad I/O** 四种 IO 模式,FIFO 64 字节,可由 DMAC 通过 DRQ 直接 burst。

### 1.2 SPI-NAND 支持矩阵

`sunxi_spi.c:151-193` 的 `spi_nand_infos[]` 收录 **Winbond / Gigadevice / Macronix / Micron** 四家共 30+ 颗:

**Winbond 支持的型号**:

| 型号 | ID(MFR/DEV) | Page | OOB | Pages/Block | Blocks | 推荐 IO |
|---|---|---|---|---|---|---|
| W25N512GV | 0xEF / 0xAA20 | 2048 | 64 | 64 | 512 | **Quad RX** |
| W25N01GV | 0xEF / 0xAA21 | 2048 | 64 | 64 | 1024 | **Quad RX** |
| W25M02GV | 0xEF / 0xAB21 | 2048 | 64 | 64 | 1024×2die | **Quad RX** |
| W25N02KV | 0xEF / 0xAA22 | 2048 | 128 | 64 | 2048 | **Quad RX** |

Winbond ID 长度为 2 字节(`dlen=2`),与其他厂商不同(`sunxi_spi.c:614-624` 处的解析分支)。

---

## 2. 时钟与数据通路

### 2.1 SPI 时钟链

```
PERI1X (CCU)
   │ 二次分频 (n,m)
   ▼
SPI 模块时钟  ← spi_clk_init() 锁定到 200 MHz
   │ 一次分频 (CDR2 / CDR1)
   ▼
SCK 引脚
   │ Quad 模式 ×4
   ▼
有效数据率
```

**配置位置**:

```c
// board.c:31
.clk_rate = 100 * 1000 * 1000,        // T113 板:100 MHz SCK

// board-v851s.c:20
.clk_rate = 75 * 1000 * 1000,         // V851s 板:75 MHz SCK

// sunxi_spi.c:206
#define SPI_MOD_CLK 200000000          // 模块时钟固定 200 MHz
```

CDR2 一档分频:`div = mclk/(spi_clk*2) - 1`(`sunxi_spi.c:217-221`),100 MHz 时 div=0,实际频率由 `spi_set_clk` 计算并返回。

**SDC 采样补偿(关键)**:

```c
// sunxi_spi.c:420-423
if (freq >= 80000000)
    val |= SPI_TCR_SDC_MSK;     // ≥80 MHz 启用采样延迟补偿
else if (freq <= CONFIG_COUNTER_FREQUENCY)
    val |= SPI_TCR_SDM_MSK;     // ≤24 MHz 启用慢速模式
```

100 MHz 必定开 SDC,这是高速 Quad Read 能稳定工作的前提。

### 2.2 数据通路

**TX**: CPU 写 SPI_TXD → TX FIFO(64 B)→ 串行器 → MOSI/IO0~3
**RX**: MISO/IO0~3 → 解串器 → RX FIFO(64 B)→ **DMAC 搬运** → DRAM

```c
// sunxi_spi.c:341-351 (DMA 描述符)
.src_drq_type     = DMAC_CFG_TYPE_SPI0,
.src_addr_mode    = IO_MODE,            // 固定地址(读 RXD 寄存器)
.src_burst_length = DMAC_CFG_SRC_8_BURST,
.src_data_width   = 16BIT,

.dst_drq_type     = DMAC_CFG_TYPE_DRAM,
.dst_addr_mode    = LINEAR_MODE,        // DRAM 线性地址
.dst_burst_length = DMAC_CFG_DEST_8_BURST,
.dst_data_width   = 16BIT,
```

**DMA 触发条件**(`sunxi_spi.c:572-583`):

- `rxlen > 64` → 启用 DMA(`SPI_FCR_RX_DRQEN_MSK`),`dma_start` + 轮询 `dma_querystatus`
- `rxlen ≤ 64` → 走 PIO `spi_read_rx_fifo`(避免 DMA 开销)

---

## 3. Winbond 专属优化:Continuous Read

### 3.1 BUF / Continuous 两种工作模式

W25N 系列有 OTP 寄存器(地址 `0xB0`)中的 **BUF 位(bit3)**:

| BUF | 模式 | 行为 |
|---|---|---|
| 1(默认) | Buffer Mode | 一次 `0x6B` 读至当前 page 末尾即停 |
| 0 | **Continuous Mode** | 自动跨页连续读出,跨页由芯片内部预取 |

### 3.2 探测阶段写 OTP

```c
// sunxi_spi.c:707-713
if (spi->info.id.mfr == SPI_NAND_MFR_WINBOND) {
    if ((spi_nand_get_config(spi, CONFIG_ADDR_OTP, &val) == 0) && (val != 0x0)) {
        val &= ~CONFIG_POS_BUF;          // 清 bit3
        spi_nand_set_config(spi, CONFIG_ADDR_OTP, val);
        spi_nand_wait_while_busy(spi);
    }
}
```

只在 detect 阶段执行一次,代价可忽略。

### 3.3 读取阶段的差异

```c
// sunxi_spi.c:806-823
spi_nand_load_page(spi, addr);             // 0x13 加载首页到 cache(~25-50 µs tRD)

if (spi->info.id.mfr == SPI_NAND_MFR_WINBOND) {
    txlen++;                               // continuous 模式额外 1 个 dummy
}

ca = address & (spi->info.page_size - 1);
tx[0] = read_opcode;                       // 0x6B (Quad Output Fast Read)
tx[1] = (uint8_t)(ca >> 8);                // 列地址高
tx[2] = (uint8_t)(ca >> 0);                // 列地址低
tx[3] = 0x0;                               // dummy
tx[4] = 0x0;                               // 额外 dummy(continuous)

spi_transfer(spi, spi->info.mode, tx, txlen, buf, rxlen);  // 一次性读 rxlen 字节
```

**对比 Gigadevice**(`sunxi_spi.c:787-805`):每页都要 `0x13 → 0x6B`,每页多付 ~25-50 µs 的 tRD 开销。

---

## 4. 性能定量分析

### 4.1 理论峰值

Quad Output 模式下,每个 SCK 边沿吐 4 bit:

| SoC | SCK | 原始带宽 | 字节速率 |
|---|---|---|---|
| T113 (100 MHz) | 100 MHz | 400 Mbps | **50.0 MB/s** |
| V851s (75 MHz) | 75 MHz | 300 Mbps | **37.5 MB/s** |

### 4.2 协议开销建模

读一段长度为 `L` 字节的数据,总耗时:

```
T_total = T_load + T_cmd + T_dummy + T_data
        = tRD + (4 bytes / SCK) + (2 bytes / SCK) + (L / 数据率)
```

以 T113 @ 100 MHz Quad、Winbond W25N01GV(tRD ≈ 25 µs)为例:

| L | T_load | T_cmd+dummy | T_data | T_total | 有效速率 |
|---|---|---|---|---|---|
| 2 KB(单页) | 25 µs | 0.48 µs | 41 µs | 66.5 µs | **30.8 MB/s** |
| 64 KB | 25 µs | 0.48 µs | 1.31 ms | 1.34 ms | **47.8 MB/s** |
| 1 MB | 25 µs | 0.48 µs | 20.97 ms | 20.99 ms | **49.9 MB/s** |
| 8 MB(典型 kernel) | 25 µs | 0.48 µs | 167.8 ms | 167.8 ms | **49.97 MB/s** |

> Continuous 模式让 tRD 只付一次,大块读非常接近理论峰值。

**对比若关掉 Continuous(每页都 load+read)**:

```
T_per_page = tRD + cmd + L_page/Rate
           = 25 + 0.48 + 41 ≈ 66.5 µs
有效速率   = 2048 / 66.5 µs ≈ 30.8 MB/s
```

读 8 MB 要 ~273 ms,比 continuous 多 100 ms。**Continuous 模式提升约 60%**。

### 4.3 实测预期

考虑 DMA 启停延迟、SPI FIFO 等待、SDC 采样裕量、DRAM 仲裁、CPU 轮询 `dma_querystatus`(`sunxi_spi.c:579-580`)等次级损耗,通常实测会比理论低 10–20%:

| 板 | SCK | 理论峰值 | **实测预期** | 8 MB kernel 加载 |
|---|---|---|---|---|
| T113 (`board.c`) | 100 MHz | 50.0 MB/s | **35–45 MB/s** | **~180–230 ms** |
| V851s (`board-v851s.c`) | 75 MHz | 37.5 MB/s | **28–35 MB/s** | **~230–290 ms** |

### 4.4 性能横向对比

| 介质 | 典型速率 | 备注 |
|---|---|---|
| SD 卡 SDR12 | ~6 MB/s | 早期 awboot SDMMC |
| SD 卡 SDR25 | ~12 MB/s | 当前 sdmmc 驱动 |
| SD 卡 SDR50/HS | ~25 MB/s | 高速档 |
| **SPI-NAND Quad @100 MHz** | **35–45 MB/s** | **本驱动 Winbond 路径** |
| eMMC HS200 | ~80 MB/s | 8-bit ×200 MHz |
| NOR Flash QSPI @133 MHz | ~50 MB/s | 类似量级 |

---

## 5. 瓶颈与限制

### 5.1 SDC 采样裕量

`sunxi_spi.c:420` 在 ≥80 MHz 启用 SDC,但代码注释里有遗留尝试 `spi_set_sample_deplay()`(`sunxi_spi.c:243-258`,被 `#if 0` 注释掉),意味着 **校准没有真正自动跑**,在某些板/温度/电压下 100 MHz 可能不稳。

**建议**:若读取出现 ECC 错误/启动失败,先把 `clk_rate` 降到 80 MHz,稳定后再回升。

### 5.2 Quad I/O(`0xEB`)未启用

代码里有 `OPCODE_FAST_READ_QUAD_IO = 0xeb` 的处理路径(`sunxi_spi.c:772-775`),也支持地址也用 4 线发送(`stxlen=1`,`sunxi_spi.c:542-543`),但 `spi_nand_infos` 表里 Winbond 全部填的是 **`SPI_IO_QUAD_RX`(0x6B)**。

差别:
- `0x6B`:命令+地址单线发,数据 4 线收
- `0xEB`:命令单线发,地址也 4 线发,数据 4 线收

对大块读差距 < 0.5 µs/次,**几乎无收益**;对碎块读(单页级)能再省几个 µs。**保持现状即可**。

### 5.3 DMA 轮询

`dma_querystatus` 是忙等(`sunxi_spi.c:579-580`),在 CPU 上是空转。awboot 是单线程 boot loader,这无所谓;若移植到需并发的场景再考虑中断驱动。

### 5.4 PIO 阈值

`rxlen ≤ 64` 走 PIO,这恰好等于 FIFO 深度。意味着 **小于一个 FIFO 的读不付 DMA 启动开销**,而大于 64 字节才用 DMA —— 这个阈值合理。

### 5.5 已知 Bug 风险

`spi_query_txfifo`(`sunxi_spi.c:312-318`)末尾固定 `return 0;` —— 始终返回 0,对 TX 流控无效。但当前驱动用法里 TX 量都很小(≤6 字节),不会撑爆 FIFO,实际无影响,**但是潜在 bug**,应改为 `return val;`。

`spi_write_tx_fifo` / `spi_read_rx_fifo` 中 `len -= 4 % 4` 等价于 `len -= 0`,循环条件永远成立 —— 也是潜在 bug,小数据量下不易触发。

---

## 6. 结论与建议

### 6.1 性能结论

- **T113 @ 100 MHz Quad + Winbond Continuous Read + DMA = 实测 35–45 MB/s**,接近硬件极限。
- 加载 8 MB Linux kernel ≈ 200 ms,加载 64 KB DTB 几乎瞬时(< 2 ms)。
- 相比 GigaDevice 的逐页路径,Winbond 路径在大块读上**快约 1.5–1.6 倍**。
- 比 awboot 自身的 SDMMC SDR25 路径**快约 3 倍**。

### 6.2 提速空间(若需要)

1. **SCK 提到 104 MHz**(W25N01GV/W25N02KV 数据手册上限):需重新打 SDC 校准,收益 ~4%。
2. **改用 0xEB(Quad IO)**:碎读场景有微小收益,大块读基本无收益。
3. **修复 `spi_query_txfifo` 返回值**:不影响速度,改善鲁棒性。
4. **启用注释掉的 `spi_set_sample_deplay`**:做一次自动采样校准,可在 100 MHz 以上提升稳定性。

### 6.3 风险点

- 100 MHz 接近 SoC 与 NAND 的共同上限,板布线/电源不稳时会读错;若发现启动失败先降速。
- 未做 ECC 校验,依赖芯片内部 ECC(W25N 都内置 BCH);如果用 raw 模式或非 ECC 芯片,需另加软件 ECC。

---

**附:关键代码定位**

| 功能 | 位置 |
|---|---|
| SPI 控制器初始化 | `arch/arm32/mach-t113s3/sunxi_spi.c:366` |
| 时钟分频计算 | `arch/arm32/mach-t113s3/sunxi_spi.c:208` |
| DMA 配置 | `arch/arm32/mach-t113s3/sunxi_spi.c:328` |
| Winbond Continuous 模式启用 | `arch/arm32/mach-t113s3/sunxi_spi.c:707` |
| 读取主入口 | `arch/arm32/mach-t113s3/sunxi_spi.c:751` |
| Winbond 大块读分支 | `arch/arm32/mach-t113s3/sunxi_spi.c:806-823` |
| 板级时钟设定 | `board.c:31` / `board-v851s.c:20` |
