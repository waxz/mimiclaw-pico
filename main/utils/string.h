#pragma once

char *sanitize_telegram_markdown(const char *src);
int utf8_char_len(const char *p, size_t remaining);
size_t strip_invalid_utf8(char *s, size_t len);
size_t utf8_safe_chunk(const char *text, size_t text_len, size_t max_chunk);


#define TBL_MAX_COLS 16
#define TBL_MAX_ROWS 64

 bool is_tbl_sep(const char *s, size_t n);
 bool has_pipes(const char *s, size_t n);
 
 int split_tbl_cells(const char *line, size_t len,
                           const char **cells, size_t *clens, int max);

char *convert_markdown_tables(const char *src);


