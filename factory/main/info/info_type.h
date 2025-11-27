//
// Created by susanne on 27.11.25.
//

#ifndef S32_OTG_INFO_TYPE_H
#define S32_OTG_INFO_TYPE_H

#endif //S32_OTG_INFO_TYPE_H

#ifndef INFO_TYPES_H
#define INFO_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "display_printf.h"   /* Font type and DISPLAY_PRINTF_SET_FONT */
#include "display_painter.h"  /* painter_draw_string / painter_get_text_width (if available) */

#ifdef __cplusplus
extern "C" {
#endif

#define INFO_MAX_LINE_BUF 128
#define INFO_DEFAULT_MAX_WIDTH 220
#define INFO_USE_DEFAULT_INT16  (INT16_MIN)

typedef enum { INFO_ALIGN_LEFT = 0, INFO_ALIGN_CENTER, INFO_ALIGN_RIGHT } info_text_align_t;

typedef void (*info_line_gen_t)(char *buf, size_t len);

/* A paragraph: may contain '\n' to indicate explicit line breaks.
   If .gen != NULL it will be called to fill a single string (may contain '\n'). */
typedef struct {
    const char *text;            /* literal string (may contain '\n') OR NULL */
    info_line_gen_t gen;         /* generator callback OR NULL */
    int16_t indent;              /* additional X offset relative to page default_x */
    const font_t *font;          /* NULL => use page default_font */
    uint16_t color;              /* 0 => use page default_color */
    bool wrap;                   /* true -> wrap lines at max_width, false -> truncate */
    info_text_align_t align;     /* override alignment for this paragraph; use page default if unset */
    int16_t max_width;           /* 0 -> use page max_width or default */
} info_paragraph_t;

typedef struct {
    int16_t default_x;
    int16_t default_y;
    int16_t line_padding;            /* extra space between lines (added to font->Height) */
    const font_t *default_font;
    uint16_t default_color;
    info_text_align_t default_align;
    int16_t max_width;               /* 0 = fallback to INFO_DEFAULT_MAX_WIDTH */
    const info_paragraph_t *paragraphs;
    size_t paragraphs_count;
} info_page_config_t;

/* Renderer API */
int16_t render_paragraph(int16_t cur_x, int16_t cur_y, const info_page_config_t *page, const info_paragraph_t *p);
void render_info_page(const info_page_config_t *page);

#ifdef __cplusplus
}
#endif

#endif /* INFO_TYPES_H */