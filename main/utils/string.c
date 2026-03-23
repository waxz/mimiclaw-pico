#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "./string.h"

/* ── Sanitize standard Markdown for Telegram legacy Markdown ── */

/* ── UTF-8 helpers ──────────────────────────────────────────── */

 int utf8_char_len(const char *p, size_t remaining)
{
    unsigned char c = (unsigned char)*p;
    int len;
    if (c < 0x80)        len = 1;
    else if (c < 0xC0)   return 0;   /* stray continuation byte */
    else if (c < 0xE0)   len = 2;
    else if (c < 0xF0)   len = 3;
    else if (c < 0xF8)   len = 4;
    else                  return 0;

    if ((size_t)len > remaining) return 0;          /* truncated */
    for (int i = 1; i < len; i++) {
        if (((unsigned char)p[i] & 0xC0) != 0x80)  /* bad continuation */
            return 0;
    }
    return len;
}

/* Strip invalid UTF-8 bytes in-place, return new length */
 size_t strip_invalid_utf8(char *s, size_t len)
{
    size_t r = 0, w = 0;
    while (r < len) {
        int cl = utf8_char_len(s + r, len - r);
        if (cl > 0) {
            if (w != r) memmove(s + w, s + r, cl);
            w += cl;
            r += cl;
        } else {
            r++;   /* skip bad byte */
        }
    }
    s[w] = '\0';
    return w;
}

/* Find largest chunk ≤ max_chunk that doesn't split a UTF-8 char */
 size_t utf8_safe_chunk(const char *text, size_t text_len, size_t max_chunk)
{
    if (text_len <= max_chunk) return text_len;
    size_t chunk = max_chunk;
    /* Walk back while sitting on a continuation byte (10xxxxxx) */
    while (chunk > 0 && ((unsigned char)text[chunk] & 0xC0) == 0x80)
        chunk--;
    return chunk > 0 ? chunk : max_chunk;
}
 char *sanitize_telegram_markdown(const char *src)
{
    if (!src) return NULL;
    size_t slen = strlen(src);

    /* Work on a UTF-8-clean copy */
    char *clean = malloc(slen + 1);
    if (!clean) return strdup(src);
    memcpy(clean, src, slen + 1);
    slen = strip_invalid_utf8(clean, slen);

    size_t cap = slen + 64;          /* output ≤ input + closing ``` */
    char *buf  = malloc(cap);
    if (!buf) { free(clean); return strdup(src); }

    size_t o = 0;
    bool in_code_block = false;

    #define EMIT(c)    do { if (o < cap-1) buf[o++] = (c); } while(0)
    #define EMIT_N(p,n) do { for(int _k=0;_k<(n)&&o<cap-1;_k++) buf[o++]=(p)[_k]; } while(0)

    for (size_t i = 0; i < slen; ) {
        unsigned char c = (unsigned char)clean[i];

        /* ── Multi-byte UTF-8: copy whole character untouched ── */
        if (c >= 0x80) {
            int cl = utf8_char_len(clean + i, slen - i);
            if (cl > 0) { EMIT_N(clean + i, cl); i += cl; }
            else i++;
            continue;
        }

        /* ── Code block ``` ── */
        if (i+2 < slen && c=='`' && clean[i+1]=='`' && clean[i+2]=='`') {
            if (!in_code_block) {
                if (strstr(clean + i + 3, "```")) {
                    in_code_block = true;
                    EMIT('`'); EMIT('`'); EMIT('`'); i += 3;
                    while (i < slen && clean[i] != '\n') { EMIT(clean[i]); i++; }
                } else {
                    i += 3;   /* no closing → drop */
                }
            } else {
                in_code_block = false;
                EMIT('`'); EMIT('`'); EMIT('`'); i += 3;
            }
            continue;
        }
        if (in_code_block) { EMIT(clean[i]); i++; continue; }

        /* ── Inline code ` ── */
        if (c == '`') {
            const char *cl2 = strchr(clean + i + 1, '`');
            if (cl2) {
                size_t span = (size_t)(cl2 - clean) - i + 1;
                EMIT_N(clean + i, (int)span); i += span;
            } else { i++; }
            continue;
        }

        /* ── # Heading at line start → strip markers ── */
        if (c == '#' && (i == 0 || clean[i-1] == '\n')) {
            while (i < slen && clean[i] == '#') i++;
            while (i < slen && clean[i] == ' ') i++;
            continue;
        }

        /* ── *** → * ── */
        if (c=='*' && i+2<slen && clean[i+1]=='*' && clean[i+2]=='*') {
            EMIT('*'); i += 3; continue;
        }
        /* ── ** → * ── */
        if (c=='*' && i+1<slen && clean[i+1]=='*') {
            EMIT('*'); i += 2; continue;
        }
        /* ── * bullet → - ── */
        if (c=='*' && (i==0||clean[i-1]=='\n') && i+1<slen && clean[i+1]==' ') {
            EMIT('-'); i++; continue;
        }
        /* ── __ → _ ── */
        if (c=='_' && i+1<slen && clean[i+1]=='_') {
            EMIT('_'); i += 2; continue;
        }
        /* ── [text](url) keep, bare [ drop ── */
        if (c == '[') {
            const char *cb = memchr(clean+i+1, ']', slen-i-1);
            if (cb && cb+1 < clean+slen && *(cb+1)=='(') {
                const char *cp = memchr(cb+2, ')', (size_t)(clean+slen-cb-2));
                if (cp) {
                    size_t span = (size_t)(cp-clean)-i+1;
                    EMIT_N(clean+i, (int)span); i += span; continue;
                }
            }
            i++; continue;
        }

        EMIT(clean[i]); i++;
    }

    if (in_code_block) { EMIT('\n'); EMIT('`'); EMIT('`'); EMIT('`'); }
    buf[o] = '\0';

    /* ── Pass 2: strip * or _ if count is odd (unbalanced) ── */
    int stars = 0, unders = 0;
    for (size_t j = 0; j < o; j++) {
        unsigned char x = (unsigned char)buf[j];
        if (x >= 0x80) { /* skip multibyte */
            int cl3 = utf8_char_len(buf+j, o-j);
            if (cl3 > 1) j += cl3 - 1;
            continue;
        }
        if (x == '`') { /* skip code spans */
            if (j+2<o && buf[j+1]=='`' && buf[j+2]=='`') {
                const char *e = strstr(buf+j+3,"```");
                if (e) { j = (size_t)(e-buf)+2; continue; }
            }
            const char *e2 = strchr(buf+j+1,'`');
            if (e2) { j = (size_t)(e2-buf); continue; }
        }
        if (x == '*') stars++;
        if (x == '_') unders++;
    }
    if ((stars%2) || (unders%2)) {
        bool ss = (stars%2 != 0), su = (unders%2 != 0);
        size_t w = 0;
        for (size_t j = 0; j < o; j++) {
            unsigned char x = (unsigned char)buf[j];
            if (x >= 0x80) {
                int cl3 = utf8_char_len(buf+j, o-j);
                if (cl3 > 0) { if(w!=j) memmove(buf+w,buf+j,cl3); w+=cl3; j+=cl3-1; }
                continue;
            }
            if ((x=='*' && ss) || (x=='_' && su)) continue;
            buf[w++] = buf[j];
        }
        buf[w] = '\0'; o = w;
    }

    free(clean);
    #undef EMIT
    #undef EMIT_N
    return buf;
}


/* ── Markdown table → code-block conversion ─────────────────── */

 bool is_tbl_sep(const char *s, size_t n)
{
    bool dash = false, pipe = false;
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (c == '-')      dash = true;
        else if (c == '|') pipe = true;
        else if (c != ':' && c != ' ' && c != '\t') return false;
    }
    return dash && pipe;
}

 bool has_pipes(const char *s, size_t n)
{
    int count = 0;
    for (size_t i = 0; i < n; i++)
        if (s[i] == '|') count++;
    return count >= 2;
}

 int split_tbl_cells(const char *line, size_t len,
                           const char **cells, size_t *clens, int max)
{
    const char *p = line, *e = line + len;
    if (p < e && *p == '|') p++;
    if (e > p && *(e - 1) == '|') e--;
    int n = 0;
    while (p < e && n < max) {
        const char *bar = memchr(p, '|', e - p);
        const char *ce = bar ? bar : e;
        const char *cs = p;
        size_t cl = ce - cs;
        while (cl > 0 && (*cs == ' ' || *cs == '\t')) { cs++; cl--; }
        while (cl > 0 && (cs[cl-1] == ' ' || cs[cl-1] == '\t')) cl--;
        cells[n] = cs;
        clens[n] = cl;
        n++;
        p = bar ? bar + 1 : e;
    }
    return n;
}

char *convert_markdown_tables(const char *src)
{
    if (!src || !strchr(src, '|')) return NULL;

    size_t slen = strlen(src);
    const char *end = src + slen;

    /* Quick scan: any separator line at all? */
    bool any_sep = false;
    for (const char *s = src; s < end && !any_sep; ) {
        const char *eol = memchr(s, '\n', end - s);
        size_t ll = eol ? (size_t)(eol - s) : (size_t)(end - s);
        if (ll > 0 && s[ll-1] == '\r') ll--;
        if (is_tbl_sep(s, ll)) any_sep = true;
        s = eol ? eol + 1 : end;
    }
    if (!any_sep) return NULL;

    size_t cap = slen + 1024;
    char *out = malloc(cap);
    if (!out) return NULL;
    size_t o = 0;
    bool in_code = false;

    #define T_ENSURE(n) do { \
        while (o + (n) >= cap) { \
            cap *= 2; \
            char *_t = realloc(out, cap); \
            if (!_t) { free(out); return NULL; } \
            out = _t; \
        } \
    } while(0)

    const char *p = src;
    while (p < end) {
        const char *eol = memchr(p, '\n', end - p);
        size_t raw = eol ? (size_t)(eol - p + 1) : (size_t)(end - p);
        size_t ll = raw;
        if (ll > 0 && p[ll-1] == '\n') ll--;
        if (ll > 0 && p[ll-1] == '\r') ll--;

        /* Track existing code blocks — don't touch them */
        if (ll >= 3 && p[0] == '`' && p[1] == '`' && p[2] == '`') {
            in_code = !in_code;
            T_ENSURE(raw);
            memcpy(out + o, p, raw); o += raw;
            p += raw; continue;
        }
        if (in_code) {
            T_ENSURE(raw);
            memcpy(out + o, p, raw); o += raw;
            p += raw; continue;
        }

        /* Detect table start: line with ≥2 pipes */
        if (has_pipes(p, ll)) {
            /* Collect all consecutive table-like lines */
            struct { const char *s; size_t n; bool sep; } rows[TBL_MAX_ROWS];
            int nrows = 0;
            bool got_sep = false;
            const char *bp = p;

            while (bp < end && nrows < TBL_MAX_ROWS) {
                const char *beol = memchr(bp, '\n', end - bp);
                size_t braw = beol ? (size_t)(beol - bp + 1) : (size_t)(end - bp);
                size_t bll = braw;
                if (bll > 0 && bp[bll-1] == '\n') bll--;
                if (bll > 0 && bp[bll-1] == '\r') bll--;

                bool sep = is_tbl_sep(bp, bll);
                bool row = has_pipes(bp, bll);
                if (!sep && !row) break;

                if (sep) got_sep = true;
                rows[nrows].s   = bp;
                rows[nrows].n   = bll;
                rows[nrows].sep = sep;
                nrows++;
                bp += braw;
            }

            if (got_sep && nrows >= 2) {
                /* ── Parse cells & compute column widths ── */
                int ncols = 0;
                int widths[TBL_MAX_COLS] = {0};

                for (int r = 0; r < nrows; r++) {
                    if (rows[r].sep) continue;
                    const char *cells[TBL_MAX_COLS];
                    size_t clens[TBL_MAX_COLS];
                    int nc = split_tbl_cells(rows[r].s, rows[r].n,
                                            cells, clens, TBL_MAX_COLS);
                    if (nc > ncols) ncols = nc;
                    for (int c = 0; c < nc; c++)
                        if ((int)clens[c] > widths[c])
                            widths[c] = (int)clens[c];
                }

                if (ncols == 0) {
                    /* Not really a table — copy through */
                    size_t chunk = bp - p;
                    T_ENSURE(chunk);
                    memcpy(out + o, p, chunk); o += chunk;
                    p = bp; continue;
                }

                /* ── Estimate output size ── */
                int total_w = 0;
                for (int c = 0; c < ncols; c++) total_w += widths[c];
                size_t need = 16 + (size_t)(nrows + 2) *
                              ((size_t)total_w + (size_t)ncols * 4 + 8);
                T_ENSURE(need);

                /* ── Opening ``` ── */
                memcpy(out + o, "```\n", 4); o += 4;

                for (int r = 0; r < nrows; r++) {
                    if (rows[r].sep) {
                        /* ── Separator row ── */
                        for (int c = 0; c < ncols; c++) {
                            if (c > 0) {
                                out[o++] = '-';
                                out[o++] = '+';
                                out[o++] = '-';
                            } else {
                                out[o++] = '-';
                            }
                            for (int d = 0; d < widths[c]; d++)
                                out[o++] = '-';
                            out[o++] = '-';
                        }
                        out[o++] = '\n';
                    } else {
                        /* ── Data / header row ── */
                        const char *cells[TBL_MAX_COLS];
                        size_t clens[TBL_MAX_COLS];
                        int nc = split_tbl_cells(rows[r].s, rows[r].n,
                                                cells, clens, TBL_MAX_COLS);
                        for (int c = 0; c < ncols; c++) {
                            if (c > 0) {
                                out[o++] = ' ';
                                out[o++] = '|';
                                out[o++] = ' ';
                            } else {
                                out[o++] = ' ';
                            }
                            int cl = (c < nc) ? (int)clens[c] : 0;
                            if (cl > 0) {
                                memcpy(out + o, cells[c], cl);
                                o += cl;
                            }
                            for (int pad = cl; pad < widths[c]; pad++)
                                out[o++] = ' ';
                            out[o++] = ' ';
                        }
                        out[o++] = '\n';
                    }
                }

                /* ── Closing ``` ── */
                memcpy(out + o, "```\n", 4); o += 4;
                p = bp;
            } else {
                /* Not a real table — copy through unchanged */
                size_t chunk = bp - p;
                T_ENSURE(chunk);
                memcpy(out + o, p, chunk); o += chunk;
                p = bp;
            }
            continue;
        }

        /* Regular line — copy through */
        T_ENSURE(raw);
        memcpy(out + o, p, raw); o += raw;
        p += raw;
    }

    T_ENSURE(1);
    out[o] = '\0';

    #undef T_ENSURE
    return out;
}