#include "font_rsrc.h"

#include <snlsys/mem_allocator.h>
#include <snlsys/ref_count.h>
#include <snlsys/snlsys.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <stdbool.h>

#ifndef NDEBUG
  #define FT(func) ASSERT(0 == FT_##func)
#else
  #define FT(func) FT_##func
#endif

struct font_system {
  struct ref ref;
  struct mem_allocator* allocator;
  FT_Library ft_handle;
};

struct font_rsrc {
  struct ref ref;
  struct font_system* sys;
  FT_Face ft_face;
};

struct font_glyph {
  struct ref ref;
  struct font_rsrc* font;
  wchar_t character;
  struct { /* Glyph bounding box in pixels */
    int x_min;
    int y_min;
    int x_max;
    int y_max;
  } bbox;
  FT_Glyph ft_glyph;
};

/*******************************************************************************
 *
 * Helper functions.
 *
 ******************************************************************************/
static enum font_error
ft_to_font_error(FT_Error ft_err)
{
  enum font_error font_err = FONT_NO_ERROR;

  switch(ft_err) {
    case FT_Err_Ok:
      font_err = FONT_NO_ERROR;
      break;
    default:
      font_err = FONT_INTERNAL_ERROR;
      break;
  }
  return font_err;
}

static int
sizeof_ft_pixel_mode(FT_Pixel_Mode mode)
{
  int size = 0;

  switch(mode) {
    case FT_PIXEL_MODE_NONE:
      size = 0;
      break;
    case FT_PIXEL_MODE_MONO:
    case FT_PIXEL_MODE_GRAY:
    case FT_PIXEL_MODE_GRAY2:
    case FT_PIXEL_MODE_GRAY4:
      size = 1;
      break;
    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
      size = 3;
      break;
    default:
      ASSERT(false);
      break;
  }
  return size;
}

static void
copy_bitmap_pixel
  (const FT_Bitmap* bitmap,
   const int x,
   const int y,
   unsigned char* pixel)
{
  const unsigned char* bmp_row = bitmap->buffer + y * bitmap->pitch;
  const unsigned char* bmp_byte = NULL;
  int bit_shift = 0;
  const char mode = bitmap->pixel_mode;

  switch(mode) {
    case FT_PIXEL_MODE_MONO:
      bmp_byte = bmp_row + x / 8;
      bit_shift = 7 - x % 8;
      *pixel = (unsigned char)((((*bmp_byte) >> bit_shift) & 0x01) * 255);
      break;
    case FT_PIXEL_MODE_GRAY:
      bmp_byte = bmp_row + x;
      *pixel = *bmp_byte;
      break;
    case FT_PIXEL_MODE_GRAY2:
      bmp_byte = bmp_row + x / 4;
      bit_shift = (3 - x % 4) * 2;
      *pixel = (unsigned char)((((*bmp_byte) >> bit_shift) & 0x03) * 85);
      break;
    case FT_PIXEL_MODE_GRAY4:
      bmp_byte = bmp_row + x / 2;
      bit_shift = (1 - x % 2) * 4;
      *pixel = (unsigned char)((((*bmp_byte) >> bit_shift) & 0x0F) * 17);
      break;
    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
      bmp_byte = bmp_row + x * 3;
      pixel[0] = bmp_byte[0];
      pixel[1] = bmp_byte[1];
      pixel[2] = bmp_byte[2];
      break;
    default: ASSERT(0); /* Unreachable code */ break;
  }
}

static void
release_font_system(struct ref* ref)
{
  struct font_system* sys = NULL;
  ASSERT(ref);

  sys = CONTAINER_OF(ref, struct font_system, ref);

  FT_Error ft_err = 0;
  (void)ft_err;

  ft_err = FT_Done_FreeType(sys->ft_handle);
  ASSERT(!ft_err);
  MEM_FREE(sys->allocator, sys);
}

static void
release_font(struct ref* ref)
{
  struct font_system* sys = NULL;
  struct font_rsrc* font = NULL;
  ASSERT(ref);

  font = CONTAINER_OF(ref, struct font_rsrc, ref);
  sys = font->sys;

  if(font->ft_face)
    FT(Done_Face(font->ft_face));

  MEM_FREE(sys->allocator, font);
  FONT(system_ref_put(sys));
}

static void
release_glyph(struct ref* ref)
{
  struct font_glyph* glyph = NULL;
  struct font_rsrc* font = NULL;

  glyph = CONTAINER_OF(ref, struct font_glyph, ref);
  font = glyph->font;

  if(glyph->ft_glyph)
    FT_Done_Glyph(glyph->ft_glyph);

  MEM_FREE(font->sys->allocator, glyph);
  FONT(rsrc_ref_put(font));
}

/*******************************************************************************
 *
 * Font system functions
 *
 ******************************************************************************/
enum font_error
font_system_create
  (struct mem_allocator* allocator,
   struct font_system** out_sys)
{
  struct mem_allocator* alloc = allocator ? allocator : &mem_default_allocator;
  struct font_system* sys = NULL;
  FT_Error ft_err = 0;
  enum font_error font_err = FONT_NO_ERROR;

  if(!out_sys) {
    font_err = FONT_INVALID_ARGUMENT;
    goto error;
  }

  sys = MEM_CALLOC(alloc, 1, sizeof(struct font_system));
  if(!sys) {
    font_err = FONT_MEMORY_ERROR;
    goto error;
  }
  sys->allocator = alloc;
  ref_init(&sys->ref);

  ft_err = FT_Init_FreeType(&sys->ft_handle);
  if(ft_err != 0) {
    font_err = ft_to_font_error(ft_err);
    goto error;
  }

exit:
  if(out_sys)
    *out_sys = sys;
  return font_err;
error:
  if(sys) {
    FONT(system_ref_put(sys));
    sys = NULL;
  }
  goto exit;
}

enum font_error
font_system_ref_get(struct font_system* sys)
{
  if(!sys)
    return FONT_INVALID_ARGUMENT;
  ref_get(&sys->ref);
  return FONT_NO_ERROR;
}

enum font_error
font_system_ref_put(struct font_system* sys)
{
  if(!sys)
    return FONT_INVALID_ARGUMENT;
  ref_put(&sys->ref, release_font_system);
  return FONT_NO_ERROR;
}

/*******************************************************************************
 *
 * Font resource functions
 *
 ******************************************************************************/
enum font_error
font_rsrc_create
  (struct font_system* sys,
   const char* path,
   struct font_rsrc** out_font)
{
  struct font_rsrc* font = NULL;
  enum font_error font_err = FONT_NO_ERROR;

  if(!sys || !out_font) {
    font_err = FONT_INVALID_ARGUMENT;
    goto error;
  }
  font = MEM_CALLOC(sys->allocator, 1, sizeof(struct font_rsrc));
  if(!font) {
    font_err = FONT_MEMORY_ERROR;
    goto error;
  }
  font->sys = sys;
  FONT(system_ref_get(sys));
  ref_init(&font->ref);

  if(path) {
    font_err = font_rsrc_load(font, path);
    if(font_err != FONT_NO_ERROR)
      goto error;
  }

exit:
  if(out_font)
    *out_font = font;
  return font_err;
error:
  if(font) {
    FONT(rsrc_ref_put(font));
    font = NULL;
  }
  goto exit;
}

enum font_error
font_rsrc_ref_get(struct font_rsrc* font)
{
  if(!font)
    return FONT_INVALID_ARGUMENT;
  ref_get(&font->ref);
  return FONT_NO_ERROR;
}

enum font_error
font_rsrc_ref_put(struct font_rsrc* font)
{
  if(!font)
    return FONT_INVALID_ARGUMENT;
  ref_put(&font->ref, release_font);
  return FONT_NO_ERROR;
}

enum font_error
font_rsrc_load(struct font_rsrc* font, const char* path)
{
  FT_Error ft_err = 0;
  enum font_error font_err = FONT_NO_ERROR;

  if(!font || !path) {
    font_err = FONT_INVALID_ARGUMENT;
    goto error;
  }
  ft_err = FT_New_Face(font->sys->ft_handle, path, 0, &font->ft_face);
  if(0 != ft_err) {
    font_err = ft_to_font_error(ft_err);
    goto error;
  }
  /* Set a default char size of 16pt for a resolution of 96x96dpi. */
  if(FT_IS_SCALABLE(font->ft_face))
    FT(Set_Char_Size(font->ft_face, 0, 16*64, 0, 96));

exit:
  return font_err;
error:
  goto exit;
}

enum font_error
font_rsrc_set_size
  (struct font_rsrc* font,
   const int width,
   const int height)
{
  if(!font || !width || !height)
    return FONT_INVALID_ARGUMENT;
  if(!FT_IS_SCALABLE(font->ft_face))
     return FONT_INVALID_ARGUMENT;

  /* Ensure that that the API and the FT library are compatible. */
  STATIC_ASSERT(sizeof(int) <= sizeof(FT_UInt), Unexpected_type_size);
  FT(Set_Pixel_Sizes(font->ft_face, (FT_UInt)width, (FT_UInt)height));
  return FONT_NO_ERROR;
}

enum font_error
font_rsrc_get_line_space(const struct font_rsrc* font, int* line_space)
{
  if(!font || !line_space)
    return FONT_INVALID_ARGUMENT;

  if(FT_IS_SCALABLE(font->ft_face)) {
    /* The font metrics are encoded in 26.6 fixed point */
    const signed long height = font->ft_face->size->metrics.height >> 6;
    if(height < 0 || height > UINT16_MAX)
      return FONT_MEMORY_ERROR;
    *line_space = (int)height;
  } else {
    ASSERT(font->ft_face->num_fixed_sizes != 0);
    *line_space = (int)font->ft_face->available_sizes[0].height;
  }
  return FONT_NO_ERROR;
}

enum font_error
font_rsrc_is_scalable(const struct font_rsrc* font, bool* is_scalable)
{
  if(!font || !is_scalable)
    return FONT_INVALID_ARGUMENT;
  *is_scalable = FT_IS_SCALABLE(font->ft_face);
  return FONT_NO_ERROR;
}

/*******************************************************************************
 *
 * Font glyph functions
 *
 ******************************************************************************/
enum font_error
font_rsrc_get_glyph
  (struct font_rsrc* font,
   wchar_t ch,
   struct font_glyph** out_glyph)
{
  FT_BBox box;
  struct font_glyph* glyph = NULL;
  FT_UInt glyph_index = 0;
  enum font_error font_err = FONT_NO_ERROR;

  if(!font || !out_glyph) {
    font_err = FONT_INVALID_ARGUMENT;
    goto error;
  }
  glyph_index = FT_Get_Char_Index(font->ft_face, (FT_ULong)ch);
  if(0 == glyph_index) {
    font_err = FONT_INVALID_ARGUMENT;
    goto error;
  }
  glyph = MEM_CALLOC(font->sys->allocator, 1, sizeof(struct font_glyph));
  if(!glyph) {
    font_err = FONT_MEMORY_ERROR;
    goto error;
  }
  ref_init(&glyph->ref);
  glyph->font = font;
  FONT(rsrc_ref_get(font));
  glyph->character = ch;

  FT(Load_Glyph(font->ft_face, (FT_ULong)glyph_index, FT_LOAD_DEFAULT));
  FT(Get_Glyph(font->ft_face->glyph, &glyph->ft_glyph));

  FT_Glyph_Get_CBox(glyph->ft_glyph, FT_GLYPH_BBOX_PIXELS, &box);
  glyph->bbox.x_min = (int)box.xMin;
  glyph->bbox.y_min = (int)box.yMin;
  glyph->bbox.x_max = (int)box.xMax;
  glyph->bbox.y_max = (int)box.yMax;

exit:
  if(out_glyph)
    *out_glyph = glyph;
  return font_err;
error:
  if(glyph) {
    FONT(glyph_ref_put(glyph));
    glyph = NULL;
  }
  goto exit;
}

enum font_error
font_glyph_ref_get(struct font_glyph* glyph)
{
  if(!glyph)
    return FONT_INVALID_ARGUMENT;
  ref_get(&glyph->ref);
  return FONT_NO_ERROR;
}

enum font_error
font_glyph_ref_put(struct font_glyph* glyph)
{
  if(!glyph)
    return FONT_INVALID_ARGUMENT;
  ref_put(&glyph->ref, release_glyph);
  return FONT_NO_ERROR;
}

enum font_error
font_glyph_get_bitmap
  (struct font_glyph* glyph,
   bool antialiasing,
   int* width,
   int* height,
   int* bytes_per_pixel,
   unsigned char* buffer)
{
  const FT_Bitmap* bmp = NULL;
  int Bpp = 0;
  enum font_error font_err = FONT_NO_ERROR;

  if(!glyph) {
    font_err = FONT_INVALID_ARGUMENT;
    goto error;
  }
  if(antialiasing) {
    FT(Glyph_To_Bitmap(&glyph->ft_glyph, FT_RENDER_MODE_NORMAL, NULL, 1));
  } else {
    FT(Glyph_To_Bitmap(&glyph->ft_glyph, FT_RENDER_MODE_MONO, NULL, 1));
  }
  bmp = &((FT_BitmapGlyph)glyph->ft_glyph)->bitmap;
  Bpp = sizeof_ft_pixel_mode(bmp->pixel_mode);

  if(width)
    *width = (int)bmp->width;
  if(height)
    *height = (int)bmp->rows;
  if(bytes_per_pixel)
    *bytes_per_pixel = Bpp;
  if(buffer) {
    const size_t pitch = (size_t)(Bpp * bmp->width);
    int x, y;
    for(y = 0; y < bmp->rows; ++y) {
      unsigned char* row = buffer + (size_t)y * pitch;
      for(x = 0; x < bmp->width; ++x) {
        unsigned char* pixel = row + x * Bpp;
        copy_bitmap_pixel(bmp, x, y, pixel);
      }
    }
  }
exit:
  return font_err;
error:
  goto exit;
}

enum font_error
font_glyph_get_desc
  (const struct font_glyph* glyph,
   struct font_glyph_desc* desc)
{
  if(!glyph || !desc)
    return FONT_INVALID_ARGUMENT;

  desc->character = glyph->character;
  desc->bbox.x_min = glyph->bbox.x_min;
  desc->bbox.y_min = glyph->bbox.y_min;
  desc->bbox.x_max = glyph->bbox.x_max;
  desc->bbox.y_max = glyph->bbox.y_max;

  /* 16.16 Fixed point */
  const signed long ft_width = (glyph->ft_glyph->advance.x) >> 16;

  ASSERT(ft_width <= INT_MAX);
  desc->width =  (int)ft_width;
  return FONT_NO_ERROR;
}

