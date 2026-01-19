/**
 * @file ramfs.c
 * @brief RAMFS 实现 - 基于内存的简单文件系统
 *
 * 设计原则：
 *   1. 所有数据和元数据都存储在内存中
 *   2. 不持久化到磁盘，重启后数据丢失
 *   3. 适合初期开发和测试
 *   4. 简单、快速、易于调试
 *
 * 参考：Linux ramfs, tmpfs
 */

#include "fs.h"
#include "mm.h"
#include "printf.h"
#include "string.h"
#include "param.h"

// ================================
// 辅助函数
// ================================

/**
 * @brief 简单的 realloc 实现
 */
static void *krealloc(void *ptr, uint32_t new_size) {
    if (!ptr) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    // 分配新内存
    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) {
        return NULL;
    }

    // 拷贝旧数据（假设旧大小不超过新大小）
    // ⚠️ 简化版：不追踪旧大小，只是拷贝最多新大小的数据
    memcpy(new_ptr, ptr, new_size);

    // 释放旧内存
    kfree(ptr);

    return new_ptr;
}

// ================================
// 全局变量
// ================================
static uint32_t ramfs_inode_counter = 1;  // inode 编号计数器
static struct super_block *ramfs_sb = NULL;  // ramfs 超级块

// ================================
// Inode 管理
// ================================

/**
 * @brief 分配并初始化一个 inode
 */
inode_t *ramfs_alloc_inode(struct super_block *sb, int mode) {
    inode_t *inode;

    // 分配 inode 结构
    inode = (inode_t *)kmalloc(sizeof(inode_t));
    if (!inode) {
        printf("[ramfs] Failed to allocate inode\n");
        return NULL;
    }

    // 初始化字段
    memset(inode, 0, sizeof(inode_t));
    inode->i_ino = ramfs_inode_counter++;
    inode->i_mode = mode;
    inode->i_size = 0;
    inode->i_nlink = 1;  // 默认链接数为 1
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_atime = 0;
    inode->i_mtime = 0;
    inode->i_ctime = 0;
    inode->i_sb = sb;
    inode->i_parent = NULL;

    // 根据文件类型初始化数据
    if (S_ISREG(mode)) {
        // 普通文件：分配数据缓冲区（初始大小 256 字节）
        inode->i_data = kmalloc(256);
        if (!inode->i_data) {
            printf("[ramfs] Failed to allocate data buffer\n");
            kfree(inode);
            return NULL;
        }
        memset(inode->i_data, 0, 256);
        inode->i_size = 0;
    } else if (S_ISDIR(mode)) {
        // 目录：初始化子目录项列表
        inode->i_children = (struct llist_header *)kmalloc(sizeof(struct llist_header));
        if (!inode->i_children) {
            printf("[ramfs] Failed to allocate children list\n");
            kfree(inode);
            return NULL;
        }
        llist_init_head(inode->i_children);
    }

    // 添加到超级块的 inode 链表
    if (sb->s_inodes) {
        llist_append(sb->s_inodes, &inode->i_list);
    }

    printf("[ramfs] Allocated inode: ino=%d, mode=0x%x\n", inode->i_ino, inode->i_mode);
    return inode;
}

/**
 * @brief 释放一个 inode
 */
void ramfs_free_inode(inode_t *inode) {
    if (!inode) {
        return;
    }

    printf("[ramfs] Freeing inode: ino=%d, nlink=%d\n", inode->i_ino, inode->i_nlink);

    // 释放数据
    if (S_ISREG(inode->i_mode)) {
        if (inode->i_data) {
            kfree(inode->i_data);
        }
    } else if (S_ISDIR(inode->i_mode)) {
        // 递归释放子目录项
        if (inode->i_children) {
            struct llist_header *pos, *next;
            llist_for_each_safe(pos, next, inode->i_children) {
                dentry_t *dentry = (dentry_t*)((char*)pos - __builtin_offsetof(dentry_t, d_list));
                if (dentry->d_inode) {
                    ramfs_free_inode(dentry->d_inode);
                }
                kfree(dentry->d_name);
                kfree(dentry);
            }
            kfree(inode->i_children);
        }
    }

    // 从链表中移除
    llist_del(&inode->i_list);

    // 释放 inode 结构
    kfree(inode);
}

// ================================
// Inode 操作实现
// ================================

/**
 * @brief 在目录中查找文件
 */
int ramfs_lookup(inode_t *dir, const char *name, dentry_t **result) {
    if (!dir || !S_ISDIR(dir->i_mode)) {
        printf("[ramfs] lookup: not a directory\n");
        return -1;
    }

    printf("[ramfs] lookup: dir_ino=%d, name='%s'\n", dir->i_ino, name);

    // 遍历子目录项
    struct llist_header *pos;
    llist_for_each(pos, dir->i_children) {
        dentry_t *dentry = (dentry_t*)((char*)pos - __builtin_offsetof(dentry_t, d_list));
        if (strcmp(dentry->d_name, name) == 0) {
            *result = dentry;
            printf("[ramfs] lookup: found inode=%d\n", dentry->d_inode->i_ino);
            return 0;
        }
    }

    printf("[ramfs] lookup: not found\n");
    return -1;  // 未找到
}

/**
 * @brief 在目录中创建文件
 */
int ramfs_create(inode_t *dir, const char *name, int mode, dentry_t **result) {
    if (!dir || !S_ISDIR(dir->i_mode)) {
        printf("[ramfs] create: parent is not a directory\n");
        return -1;
    }

    printf("[ramfs] create: dir_ino=%d, name='%s', mode=0x%x\n", dir->i_ino, name, mode);

    // 检查文件是否已存在
    dentry_t *existing;
    if (ramfs_lookup(dir, name, &existing) == 0) {
        printf("[ramfs] create: file already exists\n");
        return -1;
    }

    // 分配新的 inode
    inode_t *inode = ramfs_alloc_inode(dir->i_sb, mode);
    if (!inode) {
        printf("[ramfs] create: failed to allocate inode\n");
        return -1;
    }

    // 设置父目录
    inode->i_parent = dir;

    // 分配 dentry
    dentry_t *dentry = (dentry_t *)kmalloc(sizeof(dentry_t));
    if (!dentry) {
        printf("[ramfs] create: failed to allocate dentry\n");
        ramfs_free_inode(inode);
        return -1;
    }

    // 初始化 dentry
    dentry->d_inode = inode;
    dentry->d_parent = NULL;  // 将由调用者设置
    dentry->d_name_len = strlen(name);
    dentry->d_name = (char *)kmalloc(dentry->d_name_len + 1);
    if (!dentry->d_name) {
        printf("[ramfs] create: failed to allocate name\n");
        kfree(dentry);
        ramfs_free_inode(inode);
        return -1;
    }
    strcpy(dentry->d_name, name);
    dentry->d_sb = dir->i_sb;
    dentry->d_flags = 0;
    llist_init_head(&dentry->d_hash);
    llist_init_head(&dentry->d_list);
    llist_init_head(&dentry->d_lru);

    // 添加到父目录的子列表
    llist_append(dir->i_children, &dentry->d_list);

    // 增加父目录的链接数
    dir->i_nlink++;

    *result = dentry;
    printf("[ramfs] create: success, inode=%d\n", inode->i_ino);
    return 0;
}

/**
 * @brief 创建目录
 */
int ramfs_mkdir(inode_t *dir, const char *name, int mode) {
    printf("[ramfs] mkdir: dir_ino=%d, name='%s'\n", dir->i_ino, name);

    dentry_t *dentry;
    int ret = ramfs_create(dir, name, mode | S_IFDIR, &dentry);
    if (ret != 0) {
        printf("[ramfs] mkdir: failed\n");
        return -1;
    }

    // 在新目录中创建 "." 和 ".." 条目
    // ⚠️ 简化版：暂时不创建，后续可以添加
    printf("[ramfs] mkdir: success, inode=%d\n", dentry->d_inode->i_ino);
    return 0;
}

// ================================
// File 操作实现
// ================================

/**
 * @brief 打开文件
 */
int ramfs_open(inode_t *inode, file_t *file) {
    printf("[ramfs] open: inode=%d\n", inode->i_ino);
    file->f_pos = 0;  // 重置读写位置
    return 0;
}

/**
 * @brief 关闭文件
 */
int ramfs_close(file_t *file) {
    printf("[ramfs] close: inode=%d\n", file->f_inode->i_ino);
    return 0;
}

/**
 * @brief 读取文件
 */
int ramfs_read(file_t *file, char *buffer, uint32_t size) {
    inode_t *inode = file->f_inode;

    printf("[ramfs] read: inode=%d, size=%u, pos=%llu\n",
           inode->i_ino, size, file->f_pos);

    // 检查是否是目录
    if (S_ISDIR(inode->i_mode)) {
        printf("[ramfs] read: cannot read directory\n");
        return -1;
    }

    // 检查文件是否有数据
    if (!inode->i_data) {
        printf("[ramfs] read: file is empty\n");
        return 0;
    }

    // 计算实际可读大小
    uint64_t remaining = inode->i_size - file->f_pos;
    uint32_t to_read = (size < remaining) ? size : remaining;

    if (to_read == 0) {
        printf("[ramfs] read: EOF\n");
        return 0;  // EOF
    }

    // 拷贝数据
    memcpy(buffer, (char *)inode->i_data + file->f_pos, to_read);
    file->f_pos += to_read;

    printf("[ramfs] read: read %u bytes\n", to_read);
    return to_read;
}

/**
 * @brief 写入文件
 */
int ramfs_write(file_t *file, const char *buffer, uint32_t size) {
    inode_t *inode = file->f_inode;

    printf("[ramfs] write: inode=%d, size=%u, pos=%llu\n",
           inode->i_ino, size, file->f_pos);

    // 检查是否是目录
    if (S_ISDIR(inode->i_mode)) {
        printf("[ramfs] write: cannot write directory\n");
        return -1;
    }

    // 计算新的文件大小
    uint64_t new_size = file->f_pos + size;
    if (new_size > inode->i_size) {
        // 需要扩展数据缓冲区
        void *new_data = krealloc(inode->i_data, new_size);
        if (!new_data) {
            printf("[ramfs] write: failed to expand buffer\n");
            return -1;
        }
        inode->i_data = new_data;
        inode->i_size = new_size;
    }

    // 拷贝数据
    memcpy((char *)inode->i_data + file->f_pos, buffer, size);
    file->f_pos += size;

    // 更新修改时间
    inode->i_mtime = 0;  // ⚠️ 暂时设为 0，后续可以添加真实时间

    printf("[ramfs] write: wrote %u bytes, new_size=%u\n", size, inode->i_size);
    return size;
}

/**
 * @brief 定位文件位置
 */
int ramfs_lseek(file_t *file, int64_t offset, int whence) {
    uint64_t new_pos;

    switch (whence) {
        case 0:  // SEEK_SET
            new_pos = offset;
            break;
        case 1:  // SEEK_CUR
            new_pos = file->f_pos + offset;
            break;
        case 2:  // SEEK_END
            new_pos = file->f_inode->i_size + offset;
            break;
        default:
            printf("[ramfs] lseek: invalid whence=%d\n", whence);
            return -1;
    }

    // 检查是否超出文件范围
    if (new_pos > file->f_inode->i_size) {
        printf("[ramfs] lseek: offset beyond file size\n");
        return -1;
    }

    file->f_pos = new_pos;
    printf("[ramfs] lseek: new_pos=%llu\n", new_pos);
    return new_pos;
}

// ================================
// 操作函数表
// ================================

static inode_operations_t ramfs_inode_ops = {
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .mkdir = ramfs_mkdir,
    .rmdir = NULL,
    .unlink = NULL,
    .rename = NULL,
};

static file_operations_t ramfs_file_ops = {
    .open = ramfs_open,
    .close = ramfs_close,
    .read = ramfs_read,
    .write = ramfs_write,
    .lseek = ramfs_lseek,
    .ioctl = NULL,
};

// ================================
// 挂载 ramfs
// ================================

/**
 * @brief 挂载 ramfs 文件系统
 */
super_block_t *ramfs_mount(void) {
    printf("[ramfs] Mounting ramfs...\n");

    // 分配超级块
    super_block_t *sb = (super_block_t *)kmalloc(sizeof(super_block_t));
    if (!sb) {
        printf("[ramfs] Failed to allocate super block\n");
        return NULL;
    }

    memset(sb, 0, sizeof(super_block_t));
    sb->s_magic = 0x12345678;  // ramfs 魔数
    sb->s_flags = 0;

    // 初始化链表
    sb->s_inodes = (struct llist_header *)kmalloc(sizeof(struct llist_header));
    sb->s_dentries = (struct llist_header *)kmalloc(sizeof(struct llist_header));
    if (!sb->s_inodes || !sb->s_dentries) {
        printf("[ramfs] Failed to allocate lists\n");
        if (sb->s_inodes) kfree(sb->s_inodes);
        if (sb->s_dentries) kfree(sb->s_dentries);
        kfree(sb);
        return NULL;
    }
    llist_init_head(sb->s_inodes);
    llist_init_head(sb->s_dentries);

    // 创建根目录
    inode_t *root = ramfs_alloc_inode(sb, S_IFDIR | S_IRWXU);
    if (!root) {
        printf("[ramfs] Failed to create root inode\n");
        kfree(sb->s_inodes);
        kfree(sb->s_dentries);
        kfree(sb);
        return NULL;
    }

    root->i_op = &ramfs_inode_ops;
    root->i_fop = &ramfs_file_ops;
    root->i_parent = root;  // 根目录的父目录是自己

    sb->s_root = root;
    ramfs_sb = sb;

    printf("[ramfs] Mounted successfully, root_ino=%d\n", root->i_ino);
    return sb;
}

/**
 * @brief 文件系统初始化
 */
void fs_init(void) {
    printf("[fs] Initializing file system...\n");

    super_block_t *sb = ramfs_mount();
    if (!sb) {
        printf("[fs] Failed to mount ramfs\n");
        return;
    }

    // ⚠️ 关键修复：设置 VFS 层的 root_sb
    extern void vfs_set_root_sb(super_block_t *);
    vfs_set_root_sb(sb);

    printf("[fs] File system initialized\n");

    // 获取根目录 inode
    inode_t *root = sb->s_root;

    printf("[fs] Creating test files...\n");

    // 创建 /test.txt
    dentry_t *test_dentry;
    if (ramfs_create(root, "test.txt", 0644 | S_IFREG, &test_dentry) == 0) {
        printf("[fs] Created /test.txt\n");

        // 写入一些测试内容
        const char *test_content = "Hello from ramfs!\nThis is a test file.\n";
        if (test_dentry->d_inode->i_data) {
            strncpy((char*)test_dentry->d_inode->i_data, test_content, strlen(test_content));
            test_dentry->d_inode->i_size = strlen(test_content);
            printf("[fs] Written %d bytes to /test.txt\n", test_dentry->d_inode->i_size);
        }
    } else {
        printf("[fs] Failed to create /test.txt (may already exist)\n");
    }

    // 创建 /fstest.txt
    dentry_t *fstest_dentry;
    if (ramfs_create(root, "fstest.txt", 0644 | S_IFREG, &fstest_dentry) == 0) {
        printf("[fs] Created /fstest.txt\n");

        // 写入测试内容
        const char *fstest_content = "File System Test File\n=====================\n"
                                    "This file was created during kernel initialization.\n"
                                    "You can read it using the read() system call.\n";
        if (fstest_dentry->d_inode->i_data) {
            strncpy((char*)fstest_dentry->d_inode->i_data, fstest_content, strlen(fstest_content));
            fstest_dentry->d_inode->i_size = strlen(fstest_content);
            printf("[fs] Written %d bytes to /fstest.txt\n", fstest_dentry->d_inode->i_size);
        }
    } else {
        printf("[fs] Failed to create /fstest.txt (may already exist)\n");
    }

    printf("[fs] Test files ready\n");

    // ========== 内核级文件读取测试 ==========
    printf("\n[fs] === Kernel-level File Read Test ===\n");

    // 测试读取 /test.txt
    printf("[fs] Attempting to read /test.txt...\n");
    file_t test_file;
    memset(&test_file, 0, sizeof(test_file));
    test_file.f_inode = test_dentry->d_inode;
    test_file.f_op = test_dentry->d_inode->i_fop;
    test_file.f_pos = 0;
    test_file.f_flags = O_RDONLY;

    char read_buf[256];
    int bytes_read = ramfs_read(&test_file, read_buf, sizeof(read_buf) - 1);
    printf("[fs] ramfs_read returned: %d bytes\n", bytes_read);

    if (bytes_read > 0) {
        read_buf[bytes_read] = '\0';
        printf("[fs] Content of /test.txt:\n");
        printf("[fs] >>>%s<<<\n", read_buf);
    } else {
        printf("[fs] Failed to read /test.txt\n");
    }

    // 测试读取 /fstest.txt
    printf("\n[fs] Attempting to read /fstest.txt...\n");
    file_t fstest_file;
    memset(&fstest_file, 0, sizeof(fstest_file));
    fstest_file.f_inode = fstest_dentry->d_inode;
    fstest_file.f_op = fstest_dentry->d_inode->i_fop;
    fstest_file.f_pos = 0;
    fstest_file.f_flags = O_RDONLY;

    bytes_read = ramfs_read(&fstest_file, read_buf, sizeof(read_buf) - 1);
    printf("[fs] ramfs_read returned: %d bytes\n", bytes_read);

    if (bytes_read > 0) {
        read_buf[bytes_read] = '\0';
        printf("[fs] Content of /fstest.txt:\n");
        printf("[fs] >>>%s<<<\n", read_buf);
    } else {
        printf("[fs] Failed to read /fstest.txt\n");
    }

    printf("[fs] === Kernel Test Complete ===\n\n");
}
