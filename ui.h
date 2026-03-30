#pragma once

#include <stdio.h>

#ifndef HEADLESS_ROM_FONT
#define HEADLESS_ROM_FONT 0
#endif

/* ZX Spectrum colour IDs */
#define ZX_COLOUR_BLACK  0
#define ZX_COLOUR_BLUE   1
#define ZX_COLOUR_RED    2
#define ZX_COLOUR_GREEN  4
#define ZX_COLOUR_WHITE  7
#define ZX_COLOUR_CYAN   5
#define ZX_COLOUR_YELLOW 6

#define ZX_ATTR_BASE   0x5800
#define ZX_PIXELS_BASE 0x4000
#define ZX_PIXELS_SIZE 0x1800U
#define ZX_ATTR_SIZE   0x300U

#define UI_TEXT_ROW_STYLE_BLANK   0U
#define UI_TEXT_ROW_STYLE_CONTROL 1U
#define UI_TEXT_ROW_STYLE_TEXT    2U
#define UI_TEXT_ROW_STYLE_RESULT  3U

/*
 * Initialise the RAM font buffer and redirect the z88dk terminal driver to
 * use it.  Must be called once at startup before any screen output.
 */
void init_ui_font(void);

/*
 * Optional idle-pump callback invoked once per row during
 * ui_render_hex_dump_panel.  Set to NULL to disable.  Use this to keep the
 * key-scan latch alive during long render operations (same pattern as
 * disk_operations_set_idle_pump).
 */
void ui_set_idle_pump(void (*pump)(void));

/*
 * Set the hex dump scroll row (0 = start of data).  The header shows
 * "DATA #N" where N = row + 1, so the user always sees their position.
 * Reset automatically by ui_reset_hex_dump_panel.
 */
void ui_set_hex_dump_scroll(unsigned int row);

/* Clear the terminal viewport and reset the scroll position. */
void ui_term_clear(void);

/* Write a single attribute byte to the given character cell. */
void ui_attr_set_cell(unsigned char row, unsigned char col,
                      unsigned char ink, unsigned char paper,
                      unsigned char bright);

/* Fill all 24×32 attribute cells with a single computed byte (one memset). */
void ui_attr_fill(unsigned char ink, unsigned char paper, unsigned char bright);

/* Colour N consecutive attribute cells starting at (row, start_col). */
void ui_attr_set_run(unsigned char row, unsigned char start_col,
                     unsigned char count,
                     unsigned char ink, unsigned char paper,
                     unsigned char bright);

/* Blit one character glyph directly into pixel RAM. */
void ui_screen_put_char(unsigned char row, unsigned char col, char ch);

/*
 * Invalidate the text-screen row cache so that the next call to
 * ui_render_text_screen unconditionally redraws every row.
 * Call this before switching to the main-menu printf path.
 */
void ui_reset_text_screen_cache(void);

/*
 * Drive status badge — always shown in the right 14 cols of row 23.
 *
 * ui_set_drive_motor: call from plus3_motor_on/off sites; on=1 when spinning.
 * ui_set_drive_st3:   call after every cmd_sense_drive_status(); decodes
 *                     READY (bit 5) and WPROT (bit 6) from the ST3 byte.
 */
void ui_set_drive_motor(unsigned char on);
void ui_set_drive_st3(unsigned char st3);

/*
 * Draw a labelled text screen with a title bar, controls line, body rows,
 * and a result line.  Uses a dirty-row cache so repeated calls with the
 * same content are near-zero cost.
 */
void ui_render_text_screen(const char* title, const char* controls,
                           const char* const* lines, unsigned char line_count,
                           const char* result_label, const char* result_value);

/*
 * Render a hex+ASCII dump panel in the lower screen area (rows 10-23).
 * Row 10 is a full-width header banner; rows 11-23 show 13 rows of 8 bytes
 * each in "XX XX ... AA" format (hex cols 0-22, ASCII cols 24-31).
 *
 * Redraws only when the data content has changed (checksum dirty tracking).
 * Bypasses the text-screen row cache so it persists between card redraws.
 */
void ui_render_hex_dump_panel(const unsigned char *data, unsigned int data_len);

/*
 * Invalidate the hex dump panel checksum so the next call to
 * ui_render_hex_dump_panel unconditionally redraws the panel.
 * Call when switching tracks or leaving the track-loop screen.
 */
void ui_reset_hex_dump_panel(void);

