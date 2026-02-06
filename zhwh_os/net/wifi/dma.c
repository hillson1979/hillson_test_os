/**
 * @file dma.c
 * @brief Atheros WiFi DMA 管理实现
 */

#include "net/wifi/dma.h"
#include "net/wifi/atheros.h"
#include "net/wifi/reg.h"
#include "mm.h"
#include "string.h"
#include "printf.h"

/**
 * @brief 初始化 DMA 通道
 */
int atheros_dma_init(uint32_t mem_base, dma_channel_t *tx_ch, dma_channel_t *rx_ch) {
    printf("[atheros-dma] Initializing DMA...\n");

    // 初始化 TX 通道
    if (tx_ch) {
        tx_ch->num_desc = ATHEROS_NUM_TX_DESC;
        tx_ch->buf_size = ATHEROS_TX_BUF_SIZE;
        tx_ch->dir = DMA_DIR_TX;

        // 分配 TX 描述符（需要物理连续内存）
        uint32_t desc_size = tx_ch->num_desc * sizeof(dma_desc_t);
        tx_ch->desc_phys = pmm_alloc_pages((desc_size + 4095) / 4096);
        if (!tx_ch->desc_phys) {
            printf("[atheros-dma] Failed to allocate TX descriptors\n");
            return -1;
        }

        // 映射描述符到虚拟地址（使用 uncached 映射）
        uint32_t desc_virt = map_highmem_physical(tx_ch->desc_phys,
                                                   (desc_size + 4095) / 4096 * 4096, 0x10);
        if (!desc_virt) {
            printf("[atheros-dma] Failed to map TX descriptors\n");
            return -1;
        }
        tx_ch->desc = (dma_desc_t *)desc_virt;

        // 分配 TX 缓冲区（物理连续）
        uint32_t buf_total_size = tx_ch->num_desc * tx_ch->buf_size;
        uint32_t buf_phys = pmm_alloc_pages((buf_total_size + 4095) / 4096);
        if (!buf_phys) {
            printf("[atheros-dma] Failed to allocate TX buffers\n");
            return -1;
        }

        // 映射缓冲区到虚拟地址（使用 uncached 映射）
        uint32_t buf_virt = map_highmem_physical(buf_phys,
                                                   (buf_total_size + 4095) / 4096 * 4096, 0x10);
        if (!buf_virt) {
            printf("[atheros-dma] Failed to map TX buffers\n");
            return -1;
        }
        tx_ch->buffers = (uint8_t *)buf_virt;
        tx_ch->buf_phys = buf_phys;  // 保存物理地址

        // 初始化描述符
        memset(tx_ch->desc, 0, desc_size);

        // 内存屏障，确保 memset 完成后再设置描述符
        __asm__ volatile("mfence" ::: "memory");

        for (int i = 0; i < tx_ch->num_desc; i++) {
            tx_ch->desc[i].addr = buf_phys + i * tx_ch->buf_size;  // 物理地址
            tx_ch->desc[i].next = (i == tx_ch->num_desc - 1) ?
                tx_ch->desc_phys : tx_ch->desc_phys + (i + 1) * sizeof(dma_desc_t);
        }

        // 再次同步，确保描述符设置完成
        __asm__ volatile("mfence" ::: "memory");

        tx_ch->head = 0;
        tx_ch->tail = 0;

        // printf("[atheros-dma] TX channel: %d descriptors, %d byte buffers\n",
        //        tx_ch->num_desc, tx_ch->buf_size);
        // printf("[atheros-dma]   TX desc: phys=0x%x virt=0x%x\n", tx_ch->desc_phys, desc_virt);
        // printf("[atheros-dma]   TX buf:  phys=0x%x virt=0x%x\n", buf_phys, buf_virt);
    }

    // 初始化 RX 通道
    if (rx_ch) {
        rx_ch->num_desc = ATHEROS_NUM_RX_DESC;
        rx_ch->buf_size = ATHEROS_RX_BUF_SIZE;
        rx_ch->dir = DMA_DIR_RX;

        // 分配 RX 描述符
        uint32_t desc_size = rx_ch->num_desc * sizeof(dma_desc_t);
        rx_ch->desc_phys = pmm_alloc_pages((desc_size + 4095) / 4096);
        if (!rx_ch->desc_phys) {
            printf("[atheros-dma] Failed to allocate RX descriptors\n");
            return -1;
        }

        // 映射描述符到虚拟地址（使用 uncached 映射）
        uint32_t desc_virt = map_highmem_physical(rx_ch->desc_phys,
                                                   (desc_size + 4095) / 4096 * 4096, 0x10);
        if (!desc_virt) {
            printf("[atheros-dma] Failed to map RX descriptors\n");
            return -1;
        }
        rx_ch->desc = (dma_desc_t *)desc_virt;

        // 分配 RX 缓冲区
        uint32_t buf_total_size = rx_ch->num_desc * rx_ch->buf_size;
        uint32_t buf_phys = pmm_alloc_pages((buf_total_size + 4095) / 4096);
        if (!buf_phys) {
            printf("[atheros-dma] Failed to allocate RX buffers\n");
            return -1;
        }

        // 映射缓冲区到虚拟地址（使用 uncached 映射）
        uint32_t buf_virt = map_highmem_physical(buf_phys,
                                                   (buf_total_size + 4095) / 4096 * 4096, 0x10);
        if (!buf_virt) {
            printf("[atheros-dma] Failed to map RX buffers\n");
            return -1;
        }
        rx_ch->buffers = (uint8_t *)buf_virt;
        rx_ch->buf_phys = buf_phys;

        // 初始化描述符
        memset(rx_ch->desc, 0, desc_size);

        // 内存屏障
        __asm__ volatile("mfence" ::: "memory");

        for (int i = 0; i < rx_ch->num_desc; i++) {
            rx_ch->desc[i].addr = buf_phys + i * rx_ch->buf_size;
            rx_ch->desc[i].ctrl = 0x01;  // 使能
            rx_ch->desc[i].next = (i == rx_ch->num_desc - 1) ?
                rx_ch->desc_phys : rx_ch->desc_phys + (i + 1) * sizeof(dma_desc_t);
        }

        __asm__ volatile("mfence" ::: "memory");

        rx_ch->head = 0;
        rx_ch->tail = 0;

        // printf("[atheros-dma] RX channel: %d descriptors, %d byte buffers\n",
        //        rx_ch->num_desc, rx_ch->buf_size);
        // printf("[atheros-dma]   RX desc: phys=0x%x virt=0x%x\n", rx_ch->desc_phys, desc_virt);
        // printf("[atheros-dma]   RX buf:  phys=0x%x virt=0x%x\n", buf_phys, buf_virt);
    }

    // 配置 DMA 寄存器
    if (tx_ch) {
        atheros_reg_write(mem_base, ATHEROS_REG_TX_DESC_BASE, tx_ch->desc_phys);
    }
    if (rx_ch) {
        atheros_reg_write(mem_base, ATHEROS_REG_RX_DESC_BASE, rx_ch->desc_phys);
    }

    // 使能 DMA
    atheros_reg_set_bits(mem_base, ATHEROS_REG_DMA_CFG, 0x01);

    // printf("[atheros-dma] DMA initialized\n");
    return 0;
}

/**
 * @brief 清理 DMA 通道
 */
void atheros_dma_cleanup(dma_channel_t *channel) {
    if (!channel) return;

    printf("[atheros-dma] Cleaning up %s channel\n",
           channel->dir == DMA_DIR_TX ? "TX" : "RX");

    // 释放描述符
    if (channel->desc_phys) {
        pmm_free_pages(channel->desc_phys,
                      (channel->num_desc * sizeof(dma_desc_t) + 4095) / 4096);
    }

    // 释放缓冲区
    if (channel->buf_phys) {
        pmm_free_pages(channel->buf_phys,
                      (channel->num_desc * channel->buf_size + 4095) / 4096);
    }

    memset(channel, 0, sizeof(dma_channel_t));
}

/**
 * @brief 发送数据包
 */
int atheros_dma_tx_send(dma_channel_t *ch, const uint8_t *data, uint32_t len) {
    if (!ch || ch->dir != DMA_DIR_TX || !data || len == 0) {
        return -1;
    }

    // 检查描述符是否可用
    uint16_t next_head = (ch->head + 1) % ch->num_desc;
    if (next_head == ch->tail) {
        printf("[atheros-dma] TX descriptor ring full\n");
        return -1;
    }

    // 复制数据到缓冲区（使用虚拟地址）
    uint8_t *buf_virt = ch->buffers + ch->head * ch->buf_size;
    memcpy(buf_virt, data, len);

    // 设置描述符（使用物理地址）
    ch->desc[ch->head].addr = ch->buf_phys + ch->head * ch->buf_size;
    ch->desc[ch->head].len = len;
    ch->desc[ch->head].ctrl = 0x01;  // 使能发送

    // 移动头指针
    ch->head = next_head;

    return len;
}

/**
 * @brief 接收数据包
 */
int atheros_dma_rx_recv(dma_channel_t *ch, uint8_t *data, uint32_t *len) {
    if (!ch || ch->dir != DMA_DIR_RX || !data || !len) {
        return -1;
    }

    // 检查是否有接收到的数据
    if (ch->tail == ch->head) {
        return -1;  // 没有数据
    }

    // 检查描述符状态
    if (!(ch->desc[ch->tail].status & ATHEROS_RXDESC_DONE)) {
        return -1;  // 数据未就绪
    }

    // 获取数据长度
    uint32_t pkt_len = ch->desc[ch->tail].len;
    if (pkt_len > ch->buf_size) {
        pkt_len = ch->buf_size;
    }

    // 复制数据（使用虚拟地址）
    uint8_t *buf_virt = ch->buffers + ch->tail * ch->buf_size;
    memcpy(data, buf_virt, pkt_len);
    *len = pkt_len;

    // 清除状态并移动尾指针
    ch->desc[ch->tail].status = 0;
    ch->tail = (ch->tail + 1) % ch->num_desc;

    return pkt_len;
}

/**
 * @brief 检查发送完成
 */
int atheros_dma_tx_complete(dma_channel_t *ch) {
    if (!ch || ch->dir != DMA_DIR_TX) {
        return -1;
    }

    if (ch->tail == ch->head) {
        return 0;  // 空闲
    }

    // 检查描述符是否完成
    if (ch->desc[ch->tail].status & ATHEROS_TXDESC_DONE) {
        ch->tail = (ch->tail + 1) % ch->num_desc;
        return 1;
    }

    return 0;
}

/**
 * @brief 检查接收可用
 */
int atheros_dma_rx_avail(dma_channel_t *ch) {
    if (!ch || ch->dir != DMA_DIR_RX) {
        return -1;
    }

    if (ch->head == ch->tail) {
        return 0;  // 没有数据
    }

    // 检查描述符是否就绪
    if (ch->desc[ch->tail].status & ATHEROS_RXDESC_DONE) {
        return 1;
    }

    return 0;
}

/**
 * @brief 获取 DMA 统计
 */
void atheros_dma_get_stats(const dma_channel_t *ch, dma_stats_t *stats) {
    if (!ch || !stats) return;

    memset(stats, 0, sizeof(dma_stats_t));

    // TODO: 收集真实统计信息
    if (ch->dir == DMA_DIR_TX) {
        stats->tx_packets = ch->head;
    } else {
        stats->rx_packets = ch->head;
    }
}

/**
 * @brief 重置 DMA 统计
 */
void atheros_dma_reset_stats(dma_channel_t *ch) {
    if (!ch) return;

    // 重置计数器
    // TODO: 实现统计重置
}
