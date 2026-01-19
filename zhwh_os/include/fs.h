#ifndef FS_H
#define FS_H

#include "types.h"
#include "llist.h"

// ================================
// 文件类型定义
// ================================
#define S_IFMT   0170000  // 文件类型掩码
#define S_IFDIR  0040000  // 目录
#define S_IFREG  0100000  // 常规文件
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)

// 文件权限（简化版）
#define S_IRWXU  00700  // 用户读写执行
#define S_IRUSR  00400  // 用户读
#define S_IWUSR  00200  // 用户写
#define S_IXUSR  00100  // 用户执行

// ================================
// 前向声明
// ================================
struct inode;
struct dentry;
struct file;
struct super_block;
struct file_operations;
struct inode_operations;

// ================================
// Inode（索引节点）- 文件系统核心结构
// ================================
// 作用：表示文件系统中的对象（文件、目录等）
// 设计原则：
//   1. inode 包含文件的元数据（大小、权限、时间戳等）
//   2. inode 不包含文件名（文件名在 dentry 中）
//   3. 多个 dentry 可以指向同一个 inode（硬链接）
//   4. ramfs 中，数据直接存储在 inode 中
typedef struct inode {
    uint32_t i_ino;              // inode 编号
    uint32_t i_mode;             // 文件类型和权限
    uint32_t i_size;             // 文件大小（字节）
    uint32_t i_nlink;            // 硬链接数
    uint32_t i_uid;              // 所有者用户 ID
    uint32_t i_gid;              // 所有者组 ID
    uint64_t i_atime;            // 最后访问时间
    uint64_t i_mtime;            // 最后修改时间
    uint64_t i_ctime;            // 最后状态改变时间

    // ⚠️ ramfs 特有：数据直接存储在 inode 中
    void *i_data;                // 文件数据（普通文件）
    struct llist_header *i_children;  // 子目录项列表（目录）

    struct inode *i_parent;      // 父目录（用于 ".."）
    struct super_block *i_sb;    // 所属的超级块

    // 操作函数表
    struct inode_operations *i_op;
    struct file_operations *i_fop;

    // 私有数据（可用于扩展）
    void *i_private;

    struct llist_header i_hash;   // hash 链表（用于快速查找）
    struct llist_header i_list;   // inode 链表
} inode_t;

// ================================
// Dentry（目录项）- 目录缓存结构
// ================================
// 作用：连接文件名和 inode
// 设计原则：
//   1. dentry 是目录缓存，用于加速路径查找
//   2. dentry 树反映了文件系统的目录结构
//   3. 未使用的 dentry 会被缓存（LRU），提高性能
typedef struct dentry {
    struct inode *d_inode;       // 指向的 inode
    struct dentry *d_parent;     // 父目录
    struct llist_header *d_children;  // 子目录项列表

    char *d_name;                // 文件名（必须以 NULL 结尾）
    uint32_t d_name_len;         // 文件名长度

    struct super_block *d_sb;    // 所属的超级块
    uint32_t d_flags;            // 标志位

    struct llist_header d_hash;   // hash 链表（用于快速查找）
    struct llist_header d_list;   // dentry 链表
    struct llist_header d_lru;    // LRU 链表（用于缓存管理）
} dentry_t;

// ================================
// File（打开的文件）- 进程视角的文件
// ================================
// 作用：表示进程打开的文件
// 设计原则：
//   1. file 对应于进程的文件描述符
//   2. 多个 file 可以指向同一个 inode（多次打开同一文件）
//   3. file 包含读写位置（f_pos），每个进程独立
typedef struct file {
    struct inode *f_inode;       // 指向的 inode
    struct file_operations *f_op;  // 操作函数表

    uint32_t f_flags;            // 打开标志（O_RDONLY, O_WRONLY 等）
    uint64_t f_pos;              // 当前读写位置

    void *f_private;             // 私有数据（可用于扩展）
} file_t;

// ================================
// Super Block（超级块）- 文件系统描述
// ================================
// 作用：描述整个文件系统的信息
typedef struct super_block {
    uint32_t s_magic;            // 文件系统魔数
    uint32_t s_flags;            // 标志位

    struct inode *s_root;        // 根目录 inode
    struct llist_header *s_inodes;  // 所有 inode 链表
    struct llist_header *s_dentries; // 所有 dentry 链表

    void *s_fs_info;             // 文件系统私有信息
} super_block_t;

// ================================
// 文件操作函数表（VFS 接口）
// ================================
typedef struct file_operations {
    int (*open)(struct inode *inode, struct file *file);
    int (*close)(struct file *file);
    int (*read)(struct file *file, char *buffer, uint32_t size);
    int (*write)(struct file *file, const char *buffer, uint32_t size);
    int (*lseek)(struct file *file, int64_t offset, int whence);
    int (*ioctl)(struct file *file, uint32_t cmd, uint32_t arg);
} file_operations_t;

// ================================
// Inode 操作函数表（VFS 接口）
// ================================
typedef struct inode_operations {
    int (*lookup)(struct inode *dir, const char *name, struct dentry **result);
    int (*create)(struct inode *dir, const char *name, int mode, struct dentry **result);
    int (*mkdir)(struct inode *dir, const char *name, int mode);
    int (*rmdir)(struct inode *dir, const char *name);
    int (*unlink)(struct inode *dir, const char *name);
    int (*rename)(struct inode *old_dir, const char *old_name,
                  struct inode *new_dir, const char *new_name);
} inode_operations_t;

// ================================
// 打开标志（open 系统调用）
// ================================
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0100  // 如果文件不存在则创建
#define O_TRUNC     01000  // 如果文件存在则截断
#define O_APPEND    02000  // 追加模式

// ================================
// 文件系统初始化函数
// ================================
void fs_init(void);
super_block_t *ramfs_mount(void);
void vfs_set_root_sb(super_block_t *sb);

// ================================
// Inode 管理
// ================================
struct inode *iget(struct super_block *sb, uint32_t ino);
void iput(struct inode *inode);
struct inode *ramfs_alloc_inode(struct super_block *sb, int mode);
void ramfs_free_inode(struct inode *inode);

// ================================
// Dentry 管理
// ================================
struct dentry *d_lookup(struct inode *dir, const char *name);
struct dentry *d_alloc(struct inode *inode, const char *name);
void d_instantiate(struct dentry *dentry, struct inode *inode);
void d_add(struct dentry *dentry, struct inode *inode);

// ================================
// 文件操作（VFS 层）
// ================================
struct file *filp_open(const char *filename, int flags);
int filp_close(struct file *file);
int filp_read(struct file *file, char *buffer, uint32_t size);
int filp_write(struct file *file, const char *buffer, uint32_t size);
int filp_lseek(struct file *file, int64_t offset, int whence);

// ================================
// 路径解析
// ================================
struct inode *path_lookup(const char *path);
struct dentry *path_walk(const char *path, struct inode *root);

#endif // FS_H
