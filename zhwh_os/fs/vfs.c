/**
 * @file vfs.c
 * @brief VFS（虚拟文件系统）层实现
 *
 * 作用：提供统一的文件操作接口，屏蔽不同文件系统的差异
 * 参考：Linux VFS 层
 */

#include "fs.h"
#include "mm.h"
#include "printf.h"
#include "string.h"
#include "syscall.h"  // for copy_from_user

// 外部声明
extern int copy_from_user(char *dst, const char *src, uint32_t n);

// ================================
// 全局变量
// ================================
static super_block_t *root_sb = NULL;  // 根文件系统超级块

/**
 * @brief 设置根文件系统超级块
 */
void vfs_set_root_sb(super_block_t *sb) {
    root_sb = sb;
    printf("[vfs] Root super block set: sb=0x%x, root_ino=%d\n",
           (uint32_t)sb, sb ? sb->s_root->i_ino : 0);
}

// ================================
// Inode 管理
// ================================

/**
 * @brief 获取 inode（根据编号）
 */
inode_t *iget(super_block_t *sb, uint32_t ino) {
    if (!sb || !sb->s_inodes) {
        return NULL;
    }

    // 遍历 inode 链表
    struct llist_header *pos;
    llist_for_each(pos, sb->s_inodes) {
        inode_t *inode = (inode_t*)((char*)pos - __builtin_offsetof(inode_t, i_list));
        if (inode->i_ino == ino) {
            // 增加引用计数
            inode->i_nlink++;
            return inode;
        }
    }

    return NULL;  // 未找到
}

/**
 * @brief 释放 inode
 */
void iput(inode_t *inode) {
    if (!inode) {
        return;
    }

    inode->i_nlink--;
    if (inode->i_nlink <= 0) {
        // 调用文件系统的 free_inode 方法
        if (inode->i_sb && inode->i_sb->s_root == inode) {
            printf("[vfs] iput: cannot free root inode\n");
            return;
        }

        // ⚠️ 简化版：直接调用 ramfs_free_inode
        // 后续可以添加回调函数
        extern void ramfs_free_inode(inode_t *);
        ramfs_free_inode(inode);
    }
}

// ================================
// Dentry 管理
// ================================

/**
 * @brief 在目录中查找 dentry
 */
dentry_t *d_lookup(inode_t *dir, const char *name) {
    if (!dir || !dir->i_op || !dir->i_op->lookup) {
        return NULL;
    }

    dentry_t *result;
    if (dir->i_op->lookup(dir, name, &result) == 0) {
        return result;
    }

    return NULL;
}

/**
 * @brief 分配并初始化 dentry
 */
dentry_t *d_alloc(inode_t *inode, const char *name) {
    dentry_t *dentry;

    dentry = (dentry_t *)kmalloc(sizeof(dentry_t));
    if (!dentry) {
        return NULL;
    }

    memset(dentry, 0, sizeof(dentry_t));
    dentry->d_inode = inode;
    dentry->d_parent = NULL;
    dentry->d_name_len = strlen(name);
    dentry->d_name = (char *)kmalloc(dentry->d_name_len + 1);
    if (!dentry->d_name) {
        kfree(dentry);
        return NULL;
    }
    strcpy(dentry->d_name, name);

    if (inode) {
        dentry->d_sb = inode->i_sb;
    }

    llist_init_head(&dentry->d_hash);
    llist_init_head(&dentry->d_list);
    llist_init_head(&dentry->d_lru);

    return dentry;
}

/**
 * @brief 将 dentry 与 inode 关联
 */
void d_instantiate(dentry_t *dentry, inode_t *inode) {
    dentry->d_inode = inode;
}

/**
 * @brief 添加 dentry 到哈希表和父目录
 */
void d_add(dentry_t *dentry, inode_t *inode) {
    d_instantiate(dentry, inode);

    if (dentry->d_parent && dentry->d_parent->d_inode) {
        inode_t *dir = dentry->d_parent->d_inode;
        if (dir->i_children) {
            llist_append(dir->i_children, &dentry->d_list);
        }
    }
}

// ================================
// 路径解析
// ================================

/**
 * @brief 路径查找（返回 inode）
 */
inode_t *path_lookup(const char *path) {
    if (!path || !root_sb) {
        return NULL;
    }

    printf("[vfs] path_lookup: '%s'\n", path);

    inode_t *current = root_sb->s_root;
    char path_copy[256];
    char *token;

    // 复制路径（避免修改原字符串）
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    // 绝对路径从根目录开始
    if (path[0] == '/') {
        current = root_sb->s_root;
        token = strtok(path_copy + 1, "/");
    } else {
        token = strtok(path_copy, "/");
    }

    // 逐级查找
    while (token != NULL) {
        printf("[vfs] Looking up '%s' in dir_ino=%d\n", token, current->i_ino);

        if (!S_ISDIR(current->i_mode)) {
            printf("[vfs] Not a directory\n");
            return NULL;
        }

        if (!current->i_op || !current->i_op->lookup) {
            printf("[vfs] No lookup operation\n");
            return NULL;
        }

        dentry_t *dentry;
        if (current->i_op->lookup(current, token, &dentry) != 0) {
            printf("[vfs] Path component not found: %s\n", token);
            return NULL;
        }

        current = dentry->d_inode;
        token = strtok(NULL, "/");
    }

    printf("[vfs] path_lookup: found inode=%d\n", current->i_ino);
    return current;
}

// ================================
// 文件操作（VFS 层）
// ================================

/**
 * @brief 打开文件
 */
file_t *filp_open(const char *filename, int flags) {
    printf("[vfs] filp_open: filename ptr=0x%x\n", (uint32_t)filename);
    printf("[vfs] filp_open: filename[0]=0x%x ('%c'), filename[1]=0x%x\n",
           (unsigned char)filename[0], filename[0] ? filename[0] : '?',
           (unsigned char)filename[1]);
    printf("[vfs] filp_open: '%s', flags=%d\n", filename, flags);
    printf("[vfs] filp_open: root_sb=0x%x\n", (uint32_t)root_sb);

    // 查找文件
    inode_t *inode = path_lookup(filename);
    if (!inode) {
        // 文件不存在，检查是否需要创建
        if (flags & O_CREAT) {
            printf("[vfs] filp_open: file not found, O_CREAT not implemented yet\n");
            return NULL;
        } else {
            printf("[vfs] filp_open: file not found\n");
            return NULL;
        }
    }

    // 分配 file 结构
    file_t *file = (file_t *)kmalloc(sizeof(file_t));
    if (!file) {
        printf("[vfs] filp_open: failed to allocate file\n");
        return NULL;
    }

    memset(file, 0, sizeof(file_t));
    file->f_inode = inode;
    file->f_flags = flags;
    file->f_pos = 0;
    file->f_op = inode->i_fop;

    // 调用文件系统的 open 方法
    if (file->f_op && file->f_op->open) {
        if (file->f_op->open(inode, file) != 0) {
            printf("[vfs] filp_open: open failed\n");
            kfree(file);
            return NULL;
        }
    }

    printf("[vfs] filp_open: success, inode=%d\n", inode->i_ino);
    return file;
}

/**
 * @brief 关闭文件
 */
int filp_close(file_t *file) {
    if (!file) {
        return -1;
    }

    printf("[vfs] filp_close: inode=%d\n", file->f_inode->i_ino);

    // 调用文件系统的 close 方法
    if (file->f_op && file->f_op->close) {
        file->f_op->close(file);
    }

    // 释放 inode
    iput(file->f_inode);

    // 释放 file 结构
    kfree(file);

    return 0;
}

/**
 * @brief 读取文件
 */
int filp_read(file_t *file, char *buffer, uint32_t size) {
    if (!file || !buffer) {
        return -1;
    }

    printf("[vfs] filp_read: inode=%d, size=%u\n", file->f_inode->i_ino, size);

    // 检查权限
    if ((file->f_flags & O_RDWR) == 0 && (file->f_flags & O_RDONLY) == 0) {
        printf("[vfs] filp_read: file not opened for reading\n");
        return -1;
    }

    // 调用文件系统的 read 方法
    if (!file->f_op || !file->f_op->read) {
        printf("[vfs] filp_read: no read operation\n");
        return -1;
    }

    return file->f_op->read(file, buffer, size);
}

/**
 * @brief 写入文件
 */
int filp_write(file_t *file, const char *buffer, uint32_t size) {
    if (!file || !buffer) {
        return -1;
    }

    printf("[vfs] filp_write: inode=%d, size=%u\n", file->f_inode->i_ino, size);

    // 检查权限
    if ((file->f_flags & O_RDWR) == 0 && (file->f_flags & O_WRONLY) == 0) {
        printf("[vfs] filp_write: file not opened for writing\n");
        return -1;
    }

    // 调用文件系统的 write 方法
    if (!file->f_op || !file->f_op->write) {
        printf("[vfs] filp_write: no write operation\n");
        return -1;
    }

    return file->f_op->write(file, buffer, size);
}

/**
 * @brief 定位文件位置
 */
int filp_lseek(file_t *file, int64_t offset, int whence) {
    if (!file) {
        return -1;
    }

    printf("[vfs] filp_lseek: inode=%d, offset=%lld, whence=%d\n",
           file->f_inode->i_ino, offset, whence);

    // 调用文件系统的 lseek 方法
    if (!file->f_op || !file->f_op->lseek) {
        printf("[vfs] filp_lseek: no lseek operation\n");
        return -1;
    }

    return file->f_op->lseek(file, offset, whence);
}

// ================================
// VFS 初始化
// ================================

/**
 * @brief 设置根文件系统
 */
void vfs_set_root(super_block_t *sb) {
    root_sb = sb;
    printf("[vfs] Root file system set\n");
}
