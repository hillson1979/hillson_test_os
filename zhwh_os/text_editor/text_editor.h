/**
 * @file text_editor.h
 * @brief 文本编辑器头文件
 */

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include "stdint_compat.h"
#include "stddef.h"

// 编辑器配置
#define EDITOR_MAX_LINE_LEN   4096
#define EDITOR_MAX_LINES      10000
#define EDITOR_MAX_FILE_SIZE  (8 * 1024 * 1024)  // 8MB

// 键盘快捷键定义
#define KEY_CTRL_A    0x01
#define KEY_CTRL_C    0x03
#define KEY_CTRL_S    0x13
#define KEY_CTRL_F    0x06
#define KEY_CTRL_G    0x07
#define KEY_CTRL_V    0x16
#define KEY_CTRL_X    0x18
#define KEY_CTRL_Z    0x1A
#define KEY_CTRL_Y    0x19

// 编辑器状态结构
typedef struct {
    char        *buffer;           // 文本缓冲区
    size_t      buffer_size;       // 缓冲区大小
    size_t      text_len;          // 当前文本长度

    // 光标位置
    int         cursor_line;       // 当前行（从0开始）
    int         cursor_col;        // 当前列（从0开始）
    int         cursor_pos;        // 光标在缓冲区中的位置

    // 选择区域
    int         sel_start;         // 选择开始位置
    int         sel_end;           // 选择结束位置
    int         selecting;         // 是否正在选择

    // 视图状态
    int         scroll_line;       // 当前滚动行
    int         scroll_col;        // 当前滚动列

    // 文件信息
    char        filename[256];     // 当前文件名
    int         modified;          // 是否已修改

    // 剪贴板
    char        *clipboard;        // 剪贴板内容
    size_t      clipboard_len;     // 剪贴板长度

    // 撤销/重做
    // （简单版本暂不实现）
} editor_state_t;

// 语法高亮类型
typedef enum {
    HL_NORMAL,
    HL_COMMENT,
    HL_KEYWORD,
    HL_STRING,
    HL_NUMBER,
    HL_FUNCTION
} highlight_type_t;

// 函数声明
editor_state_t *editor_new(void);
void editor_free(editor_state_t *ed);

int editor_load_file(editor_state_t *ed, const char *filename);
int editor_save_file(editor_state_t *ed, const char *filename);

void editor_insert_char(editor_state_t *ed, char c);
void editor_insert_string(editor_state_t *ed, const char *str);
void editor_delete_char(editor_state_t *ed, int before);
void editor_delete_selection(editor_state_t *ed);

void editor_move_cursor(editor_state_t *ed, int dx, int dy);
void editor_move_to_line_start(editor_state_t *ed);
void editor_move_to_line_end(editor_state_t *ed);
void editor_move_to_file_start(editor_state_t *ed);
void editor_move_to_file_end(editor_state_t *ed);

void editor_select_all(editor_state_t *ed);
void editor_copy(editor_state_t *ed);
void editor_cut(editor_state_t *ed);
void editor_paste(editor_state_t *ed);

int editor_find(editor_state_t *ed, const char *text, int start_pos);
int editor_find_next(editor_state_t *ed, const char *text);
int editor_replace(editor_state_t *ed, const char *find, const char *replace, int all);

// 行操作
int editor_get_line_count(editor_state_t *ed);
const char *editor_get_line(editor_state_t *ed, int line_num, size_t *len);

// 辅助函数
void editor_undo(editor_state_t *ed);
void editor_redo(editor_state_t *ed);

#endif /* TEXT_EDITOR_H */
