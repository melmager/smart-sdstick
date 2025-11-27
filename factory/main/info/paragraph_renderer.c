//
// Created by susanne on 27.11.25.
//

#include "info_types.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef INFO_DEFAULT_MAX_WIDTH
#define INFO_DEFAULT_MAX_WIDTH 220
#endif

/* If your painter provides a width function, define HAVE_PAINTER_GET_TEXT_WIDTH in the project.
   Otherwise the fallback approximates width by font->Width or 8px per char. */
static int measure_text_width(const char *s, const font_t *font)
{
    if (!s) return 0;
#if defined(HAVE_PAINTER_GET_TEXT_WIDTH)
    return painter_get_text_width(s, font);
#else
    if (font && font->Width > 0) return (int)strlen(s) * font->Width;
    return (int)strlen(s) * 8;
#endif
}

static void draw_line_with_align(const char *line, const font_t *font, uint16_t color,
                                 int16_t base_x, int16_t effective_max_w, info_text_align_t align, int16_t y)
{
    int16_t x = base_x;
    if (align != INFO_ALIGN_LEFT && effective_max_w > 0) {
        int tw = measure_text_width(line, font);
        if (align == INFO_ALIGN_CENTER) {
            x = base_x + (effective_max_w - tw) / 2;
        } else if (align == INFO_ALIGN_RIGHT) {
            x = base_x + (effective_max_w - tw);
        }
        if (x < 0) x = base_x;
    }

    if (font) DISPLAY_PRINTF_SET_FONT(*font);
    painter_draw_string(x, y, line, font, color);
}

/* Render one paragraph (text may contain '\n'). Returns new Y (first free Y). */
int16_t render_paragraph(int16_t cur_x, int16_t cur_y, const info_page_config_t *page, const info_paragraph_t *p)
{
    if (!page || !p) return cur_y;

    const font_t *font = p->font ? p->font : page->default_font;
    uint16_t color = p->color ? p->color : page->default_color;
    int16_t max_w = (p->max_width != 0) ? p->max_width : (page->max_width != 0 ? page->max_width : INFO_DEFAULT_MAX_WIDTH);
    info_text_align_t align = (p->align != INFO_ALIGN_LEFT) ? p->align : page->default_align;
    int16_t base_x = cur_x + (p->indent);

    char work[INFO_MAX_LINE_BUF];
    const char *src = NULL;
    if (p->gen) {
        p->gen(work, sizeof(work));
        src = work;
    } else if (p->text) {
        src = p->text;
    } else {
        return cur_y;
    }

    const char *line_start = src;
    while (line_start && *line_start) {
        const char *nl = strchr(line_start, '\n');
        size_t len = nl ? (size_t)(nl - line_start) : strlen(line_start);
        char linebuf[INFO_MAX_LINE_BUF];
        if (len >= sizeof(linebuf)) len = sizeof(linebuf) - 1;
        memcpy(linebuf, line_start, len);
        linebuf[len] = '\0';

        if (p->wrap && max_w > 0) {
            /* naive word-wrap */
            char *token = linebuf;
            while (*token) {
                /* find largest chunk that fits */
                int last_fit = 0;
                int last_space = -1;
                for (int i = 1; token[i-1] != '\0' && i < (int)sizeof(linebuf); ++i) {
                    char tmp[INFO_MAX_LINE_BUF];
                    int tocopy = (i < (int)sizeof(tmp)) ? i : (int)sizeof(tmp)-1;
                    memcpy(tmp, token, tocopy);
                    tmp[tocopy] = '\0';
                    int w = measure_text_width(tmp, font);
                    if (w <= max_w) {
                        last_fit = i;
                        char *sp = strrchr(tmp, ' ');
                        if (sp) last_space = (int)(sp - tmp);
                    } else {
                        break;
                    }
                }
                if (last_fit == 0) {
                    /* nothing fits -> truncate and draw */
                    char tmp[INFO_MAX_LINE_BUF];
                    int take = (int)sizeof(tmp) - 1;
                    memcpy(tmp, token, take);
                    tmp[take] = '\0';
                    draw_line_with_align(tmp, font, color, base_x, max_w, align, cur_y);
                    cur_y += font->Height + page->line_padding;
                    break;
                } else {
                    int take = last_fit;
                    if (last_space >= 0 && last_space + 1 < take) take = last_space + 1;
                    char tmp[INFO_MAX_LINE_BUF];
                    int tocopy = (take < (int)sizeof(tmp)) ? take : (int)sizeof(tmp)-1;
                    memcpy(tmp, token, tocopy);
                    tmp[tocopy] = '\0';
                    /* trim trailing spaces */
                    while (tocopy > 0 && tmp[tocopy-1] == ' ') tmp[--tocopy] = '\0';
                    draw_line_with_align(tmp, font, color, base_x, max_w, align, cur_y);
                    cur_y += font->Height + page->line_padding;
                    token += take;
                    while (*token == ' ') token++;
                }
            }
        } else {
            /* no wrap: draw truncated if needs */
            if (max_w > 0) {
                int tw = measure_text_width(linebuf, font);
                if (tw > max_w) {
                    /* naive truncation by characters */
                    int allowed = (int)strlen(linebuf);
                    while (allowed > 0) {
                        char tmp[INFO_MAX_LINE_BUF];
                        int tocopy = (allowed < (int)sizeof(tmp)) ? allowed : (int)sizeof(tmp)-1;
                        memcpy(tmp, linebuf, tocopy);
                        tmp[tocopy] = '\0';
                        if (measure_text_width(tmp, font) <= max_w) break;
                        allowed--;
                    }
                    char tmp[INFO_MAX_LINE_BUF];
                    memcpy(tmp, linebuf, allowed);
                    tmp[allowed] = '\0';
                    draw_line_with_align(tmp, font, color, base_x, max_w, align, cur_y);
                } else {
                    draw_line_with_align(linebuf, font, color, base_x, max_w, align, cur_y);
                }
            } else {
                draw_line_with_align(linebuf, font, color, base_x, max_w, align, cur_y);
            }
            cur_y += font->Height + page->line_padding;
        }

        if (!nl) break;
        line_start = nl + 1;
    }

    return cur_y;
}

void render_info_page(const info_page_config_t *page)
{
    if (!page) return;
    int16_t y = page->default_y;
    for (size_t i = 0; i < page->paragraphs_count; ++i) {
        y = render_paragraph(page->default_x, y, page, &page->paragraphs[i]);
    }
}