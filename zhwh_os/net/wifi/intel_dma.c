/**
 * @file intel_dma.c
 * @brief Intel WiFi DMA 实现
 *
 * 基于 Linux iwlwifi 驱动的 DMA 机制
 */

#include "net/wifi/intel_dma.h"
#include "net/wifi/intel.h"
#include "net/wifi/reg.h"
#include "mm.h"
#include "string.h"
#include "printf.h"

/**
 * @brief 初始化 TX 队列
 */
int intel_tx_queue_init(uint32_t mem_base, intel_tx_queue_t *q,
                         intel_tx_queue_type_t type, uint16_t size) {
    printf("[intel-dma] Initializing TX queue (type=%d, size=%d)\n", type, size);

    memset(q, 0, sizeof(intel_tx_queue_t));

    q->type = type;
    q->queue_size = size;
    q->write_ptr = 0;
    q->read_ptr = 0;

    // 分配 TFD 数组（需要物理连续内存）
    uint32_t tfd_size = size * sizeof(intel_tfd_t);
    q->tfd_base_phys = pmm_alloc_pages((tfd_size + 4095) / 4096);
    if (!q->tfd_base_phys) {
        printf("[intel-dma] Failed to allocate TFD array\n");
        return -1;
    }

    // 映射 TFD 到虚拟地址（使用 uncached 映射）
    uint32_t tfd_virt = map_highmem_physical(q->tfd_base_phys,
                                               (tfd_size + 4095) / 4096 * 4096, 0x10);
    if (!tfd_virt) {
        printf("[intel-dma] Failed to map TFD array\n");
        return -1;
    }
    q->tfd_base = (intel_tfd_t *)tfd_virt;

    // 分配 TB 缓冲区（每个 TFD 对应一个 TB）
    uint32_t tb_total_size = size * IWL_RX_BUF_SIZE;  // 使用 4KB TB
    q->tb_buffers_phys = pmm_alloc_pages((tb_total_size + 4095) / 4096);
    if (!q->tb_buffers_phys) {
        printf("[intel-dma] Failed to allocate TB buffers\n");
        return -1;
    }

    // 映射 TB 到虚拟地址（使用 uncached 映射）
    uint32_t tb_virt = map_highmem_physical(q->tb_buffers_phys,
                                             (tb_total_size + 4095) / 4096 * 4096, 0x10);
    if (!tb_virt) {
        printf("[intel-dma] Failed to map TB buffers\n");
        return -1;
    }
    q->tb_buffers = (uint8_t *)tb_virt;

    // 清零 TFD 数组
    memset(q->tfd_base, 0, tfd_size);

    // 内存屏障
    __asm__ volatile("mfence" ::: "memory");

    // 设置队列基地址寄存器
    uint32_t queue_reg = FH_MEM_CBBC_QUEUE0 + type * 4;
    atheros_reg_write(mem_base, queue_reg, q->tfd_base_phys);

    printf("[intel-dma] TX queue initialized:\n");
    printf("[intel-dma]   TFD: phys=0x%x virt=0x%x\n", q->tfd_base_phys, tfd_virt);
    printf("[intel-dma]   TB:  phys=0x%x virt=0x%x\n", q->tb_buffers_phys, tb_virt);

    return 0;
}

/**
 * @brief 停止 TX 队列
 */
void intel_tx_queue_stop(uint32_t mem_base, intel_tx_queue_t *q) {
    // TODO: 实现队列停止逻辑
}

/**
 * @brief 发送数据包（Intel 特定）
 */
int intel_tx_send(uint32_t mem_base, intel_tx_queue_t *q,
                   const uint8_t *data, uint32_t len) {
    if (!q || !data || len == 0 || len > IWL_RX_BUF_SIZE) {
        return -1;
    }

    // 检查队列是否已满
    uint16_t next_write = (q->write_ptr + 1) % q->queue_size;
    if (next_write == q->read_ptr) {
        printf("[intel-dma] TX queue full\n");
        return -1;
    }

    // 获取当前 TFD 和 TB
    intel_tfd_t *tfd = &q->tfd_base[q->write_ptr];
    uint8_t *tb_buf = q->tb_buffers + q->write_ptr * IWL_RX_BUF_SIZE;

    // 复制数据到 TB
    memcpy(tb_buf, data, len);

    // 设置 TFD
    tfd->tb1_addr = q->tb_buffers_phys + q->write_ptr * IWL_RX_BUF_SIZE;
    tfd->tb1_len = len;
    tfd->tb1_flags = 0;
    tfd->num_tbs = 1;  // 只使用 1 个 TB

    tfd->tb2_addr = 0;
    tfd->tb2_len = 0;
    tfd->tb2_flags = 0;
    tfd->reserved = 0;

    // 内存屏障
    __asm__ volatile("mfence" ::: "memory");

    // 移动写指针
    q->write_ptr = next_write;

    // 通知硬件（写写指针寄存器）
    uint32_t db_reg = FH_MEM_TFDQ_DB0 + q->type * 4;
    atheros_reg_write(mem_base, db_reg, q->write_ptr);

    printf("[intel-dma] TX sent: %d bytes (write_ptr=%d)\n", len, q->write_ptr);

    return len;
}

/**
 * @brief 检查 TX 完成
 */
int intel_tx_complete(uint32_t mem_base, intel_tx_queue_t *q) {
    if (!q) {
        return -1;
    }

    // 从硬件读读指针
    uint32_t read_reg;
    switch (q->type) {
        case IWL_TX_QUEUE_CMD:
            read_reg = FH_MEM_TFDQ_DB0;
            break;
        case IWL_TX_QUEUE_DATA:
            read_reg = FH_MEM_TFDQ_DB1;
            break;
        default:
            read_reg = FH_MEM_TFDQ_DB0 + q->type * 4;
            break;
    }

    uint32_t hw_read_ptr = atheros_reg_read(mem_base, read_reg);

    if (hw_read_ptr != q->read_ptr) {
        // 有完成的包
        q->read_ptr = hw_read_ptr;
        return 1;
    }

    return 0;
}

/**
 * @brief 初始化 RX 队列
 */
int intel_rx_queue_init(uint32_t mem_base, intel_rx_queue_t *q, uint16_t size) {
    printf("[intel-dma] Initializing RX queue (size=%d)\n", size);

    memset(q, 0, sizeof(intel_rx_queue_t));

    q->num_rbs = size;
    q->write_ptr = 0;
    q->read_ptr = 0;

    // 分配 RBD 数组
    uint32_t rbd_size = size * sizeof(intel_rbd_t);
    q->rbd_base_phys = pmm_alloc_pages((rbd_size + 4095) / 4096);
    if (!q->rbd_base_phys) {
        printf("[intel-dma] Failed to allocate RBD array\n");
        return -1;
    }

    // 映射 RBD 到虚拟地址（使用 uncached 映射）
    uint32_t rbd_virt = map_highmem_physical(q->rbd_base_phys,
                                               (rbd_size + 4095) / 4096 * 4096, 0x10);
    if (!rbd_virt) {
        printf("[intel-dma] Failed to map RBD array\n");
        return -1;
    }
    q->rbd_base = (intel_rbd_t *)rbd_virt;

    // 分配 RX 缓冲区
    uint32_t buf_total_size = size * IWL_RX_BUF_SIZE;
    q->buffers_phys = pmm_alloc_pages((buf_total_size + 4095) / 4096);
    if (!q->buffers_phys) {
        printf("[intel-dma] Failed to allocate RX buffers\n");
        return -1;
    }

    // 映射 RX 缓冲区到虚拟地址（使用 uncached 映射）
    uint32_t buf_virt = map_highmem_physical(q->buffers_phys,
                                              (buf_total_size + 4095) / 4096 * 4096, 0x10);
    if (!buf_virt) {
        printf("[intel-dma] Failed to map RX buffers\n");
        return -1;
    }
    q->buffers = (uint8_t *)buf_virt;

    // 初始化 RBD
    memset(q->rbd_base, 0, rbd_size);

    // 内存屏障
    __asm__ volatile("mfence" ::: "memory");

    for (int i = 0; i < size; i++) {
        q->rbd_base[i].addr = q->buffers_phys + i * IWL_RX_BUF_SIZE;
        q->rbd_base[i].len = IWL_RX_BUF_SIZE;
        q->rbd_base[i].reserved = 0;
    }

    // 内存屏障
    __asm__ volatile("mfence" ::: "memory");

    // 设置 RX 缓冲区基地址
    atheros_reg_write(mem_base, CSR_FBHB_BASE0, q->rbd_base_phys);
    atheros_reg_write(mem_base, CSR_FBHB_BASE1, 0);  // 高 32 位（对于 32 位系统）

    // 设置 RX 缓冲区大小
    atheros_reg_write(mem_base, CSR_FBHB_SIZE0, size);
    atheros_reg_write(mem_base, CSR_FBHB_SIZE1, (IWL_RX_BUF_SIZE >> 8) & 0xFFF);

    printf("[intel-dma] RX queue initialized:\n");
    printf("[intel-dma]   RBD: phys=0x%x virt=0x%x\n", q->rbd_base_phys, rbd_virt);
    printf("[intel-dma]   BUF: phys=0x%x virt=0x%x\n", q->buffers_phys, buf_virt);

    return 0;
}

/**
 * @brief 接收数据包
 */
int intel_rx_recv(uint32_t mem_base, intel_rx_queue_t *q,
                   uint8_t *data, uint32_t *len) {
    if (!q || !data || !len) {
        return -1;
    }

    // 从硬件读取写指针
    uint32_t hw_write_ptr = atheros_reg_read(mem_base, FH_MEM_RSCSR1_CHNL0);

    // 更新软件写指针
    q->write_ptr = hw_write_ptr & 0xFF;  // 取低 8 位

    // 检查是否有数据
    if (q->read_ptr == q->write_ptr) {
        return -1;  // 没有数据
    }

    // 获取当前 RX 缓冲区
    uint8_t *rx_buf = q->buffers + q->read_ptr * IWL_RX_BUF_SIZE;

    // 🔥 调试：打印接收到的数据（前 64 字节）
    printf("[intel-dma] RX: read_ptr=%d write_ptr=%d, dumping first 64 bytes:\n", q->read_ptr, q->write_ptr);
    for (int i = 0; i < 64 && i < IWL_RX_BUF_SIZE; i++) {
        printf("%02X ", rx_buf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    // Intel RX 格式：
    // - 字节 0-3: 包长度（小端序）
    // - 字节 4: 保留
    // - 字节 5-6: CMD_ID（对于固件响应）
    // - 字节 7+: 数据

    // 读取包长度（前 4 字节，小端序）
    uint32_t pkt_len = *((uint32_t *)rx_buf);

    // 限制拷贝长度
    uint32_t copy_len = pkt_len;
    if (copy_len > *len) {
        copy_len = *len;
    }
    if (copy_len > IWL_RX_BUF_SIZE - 4) {
        copy_len = IWL_RX_BUF_SIZE - 4;
    }

    // 跳过长度字段，复制数据部分
    memcpy(data, rx_buf + 4, copy_len);
    *len = copy_len;

    printf("[intel-dma] RX: pkt_len=%d, copied=%d bytes\n", pkt_len, copy_len);

    // 移动读指针
    q->read_ptr = (q->read_ptr + 1) % q->num_rbs;

    // 重新填充缓冲区
    intel_rx_replenish(mem_base, q);

    return copy_len;
}

/**
 * @brief 重新填充 RX 缓冲区
 */
void intel_rx_replenish(uint32_t mem_base, intel_rx_queue_t *q) {
    // TODO: 实现缓冲区重新填充逻辑
}
