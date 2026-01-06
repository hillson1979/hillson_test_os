// ACPI SDT 头 (所有表的通用头)
typedef struct {
    uint32_t signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

// RSDT 结构
typedef struct {
    acpi_sdt_header_t header;
    uint32_t entries[];         // 指向其他SDT的物理地址数组
} __attribute__((packed)) rsdt_t;

// XSDT 结构
typedef struct {
    acpi_sdt_header_t header;
    uint64_t entries[];         // 指向其他SDT的物理地址数组
} __attribute__((packed)) xsdt_t;


