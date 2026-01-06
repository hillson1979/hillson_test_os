void parse_specific_ebda_data(uint32_t ebda_addr) ;

uint32_t get_ebda_physical_address();

uint64_t find_ebda_in_mmap(uint32_t mmap_addr, uint32_t mmap_length);

void find_ebda_in_acpi(struct multiboot *mbi);

void parse_acpi_for_ebda(struct acpi_rsdp *rsdp);
