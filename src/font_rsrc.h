#ifndef FONT_RSRC_H
#define FONT_RSRC_H

#include <snlsys/snlsys.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(FONT_SHARED_BUILD)
  #define FONT_API EXPORT_SYM
#else
  #define FONT_API IMPORT_SYM
#endif

#ifndef NDEBUG
  #define FONT(func) ASSERT(FONT_NO_ERROR == font_##func)
#else
  #define FONT(func) cmdsys_##func
#endif /* NDEBUG */

struct mem_allocator;

enum font_error {
  FONT_INTERNAL_ERROR,
  FONT_INVALID_ARGUMENT,
  FONT_INVALID_CALL,
  FONT_MEMORY_ERROR,
  FONT_NO_ERROR
};

/*******************************************************************************
 *
 * Font system
 *
 ******************************************************************************/
struct font_system;

#ifdef __cplusplus
extern "C" {
#endif

FONT_API enum font_error
font_system_create
  (struct mem_allocator* allocator, /* May be NULL */
   struct font_system** sys);

FONT_API enum font_error
font_system_ref_get
  (struct font_system* sys);

FONT_API enum font_error
font_system_ref_put
  (struct font_system* sys);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*******************************************************************************
 *
 * Font resource
 *
 ******************************************************************************/
struct font_rsrc; /* Font resource */

#ifdef __cplusplus
extern "C" {
#endif

FONT_API enum font_error
font_rsrc_create
  (struct font_system* sys,
   const char* path, /* May be NULL */
   struct font_rsrc** out_font);

FONT_API enum font_error
font_rsrc_ref_get
  (struct font_rsrc* font);

FONT_API enum font_error
font_rsrc_ref_put
  (struct font_rsrc* font);

FONT_API enum font_error
font_rsrc_load
  (struct font_rsrc* font,
   const char* path);

FONT_API enum font_error
font_rsrc_set_size
  (struct font_rsrc* font,
   const uint16_t width,  /* In pixels */
   const uint16_t height); /* In pixels */

FONT_API enum font_error
font_rsrc_get_line_space
  (const struct font_rsrc* font,
   uint16_t* line_space); /* In pixels */

FONT_API enum font_error
font_rsrc_is_scalable
  (const struct font_rsrc* font,
   bool* is_scalable);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*******************************************************************************
 *
 * Font glygh
 *
 ******************************************************************************/
struct font_glyph;
struct font_glyph_desc {
  struct {
    int x_min;
    int y_min;
    int x_max;
    int y_max;
  } bbox;
  size_t width;
  wchar_t character;
};

#ifdef __cplusplus
extern "C" {
#endif

FONT_API enum font_error
font_rsrc_get_glyph
  (struct font_rsrc* font,
   wchar_t ch,
   struct font_glyph** glyph);

FONT_API enum font_error
font_glyph_ref_get
  (struct font_glyph* glyph);

FONT_API enum font_error
font_glyph_ref_put
  (struct font_glyph* glyph);

FONT_API enum font_error
font_glyph_get_bitmap
  (struct font_glyph* glyph,
   const bool antialiasing,
   uint16_t* width, /* May be NULL */
   uint16_t* height, /* May be NULL */
   uint8_t* bytes_per_pixel, /* May be NULL */
   unsigned char* buffer); /* May be NULL */

FONT_API enum font_error
font_glyph_get_desc
  (const struct font_glyph* glyph,
   struct font_glyph_desc* desc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FONT_RSRC_H */

