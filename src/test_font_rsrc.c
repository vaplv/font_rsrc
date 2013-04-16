#include "font_rsrc.h"
#include <snlsys/image.h>
#include <snlsys/mem_allocator.h>
#include <snlsys/snlsys.h>
#include <stdbool.h>
#include <stdio.h>

#define OK FONT_NO_ERROR
#define BAD_ARG FONT_INVALID_ARGUMENT

int
main(int argc, char** argv)
{
  char buf[BUFSIZ];
  struct font_glyph_desc desc;
  struct font_system* sys = NULL;
  struct font_rsrc* font = NULL;
  struct font_glyph* glyph = NULL;
  const char* path = NULL;
  unsigned char* buffer = NULL;
  size_t buffer_size = 0;
  uint16_t h = 0;
  uint16_t w = 0;
  uint8_t Bpp = 0;
  uint16_t i = 0;
  bool b = false;

  if(argc != 2) {
    printf("usage: %s FONT\n", argv[0]);
    goto error;
  }
  path = argv[1];

  CHECK(font_system_create( NULL, NULL ), BAD_ARG);
  CHECK(font_system_create(NULL, &sys), OK);

  CHECK(font_rsrc_create(NULL, NULL, NULL), BAD_ARG);
  CHECK(font_rsrc_create(sys, NULL, NULL), BAD_ARG);
  CHECK(font_rsrc_create(NULL, NULL, &font), BAD_ARG);
  CHECK(font_rsrc_create(sys, NULL, &font), OK);

  CHECK(font_rsrc_load(NULL, NULL), BAD_ARG);
  CHECK(font_rsrc_load(font, NULL), BAD_ARG);
  CHECK(font_rsrc_load(NULL, path), BAD_ARG);
  CHECK(font_rsrc_load(font, path), OK);

  CHECK(font_rsrc_is_scalable(NULL, NULL), BAD_ARG);
  CHECK(font_rsrc_is_scalable(font, NULL), BAD_ARG);
  CHECK(font_rsrc_is_scalable(NULL, &b), BAD_ARG);
  CHECK(font_rsrc_is_scalable(font, &b), OK);

  if(b) {
    CHECK(font_rsrc_set_size(NULL, 0, 0), BAD_ARG);
    CHECK(font_rsrc_set_size(font, 0, 0), BAD_ARG);
    CHECK(font_rsrc_set_size(NULL, 32, 0), BAD_ARG);
    CHECK(font_rsrc_set_size(font, 32, 0), BAD_ARG);
    CHECK(font_rsrc_set_size(NULL, 0, 64), BAD_ARG);
    CHECK(font_rsrc_set_size(font, 0, 64), BAD_ARG);
    CHECK(font_rsrc_set_size(NULL, 32, 64), BAD_ARG);
    CHECK(font_rsrc_set_size(font, 32, 64), OK);
    CHECK(font_rsrc_set_size(font, 16, 16), OK);
  }

  CHECK(font_rsrc_get_line_space(NULL, NULL), BAD_ARG);
  CHECK(font_rsrc_get_line_space(font, NULL), BAD_ARG);
  CHECK(font_rsrc_get_line_space(NULL, &i), BAD_ARG);
  CHECK(font_rsrc_get_line_space(font, &i), OK);

  CHECK(font_rsrc_get_glyph(NULL, L'a', NULL), BAD_ARG);
  CHECK(font_rsrc_get_glyph(font, L'a', NULL), BAD_ARG);
  CHECK(font_rsrc_get_glyph(NULL, L'a', &glyph), BAD_ARG);
  CHECK(font_rsrc_get_glyph(font, L'a', &glyph), OK);

  CHECK(font_glyph_get_desc(NULL, NULL), BAD_ARG);
  CHECK(font_glyph_get_desc(glyph, NULL), BAD_ARG);
  CHECK(font_glyph_get_desc(NULL, &desc), BAD_ARG);
  CHECK(font_glyph_get_desc(glyph, &desc), OK);
  CHECK(desc.character, L'a');

  CHECK(font_glyph_get_bitmap(NULL, true, NULL, NULL, NULL, NULL), BAD_ARG);
  CHECK(font_glyph_get_bitmap(glyph, true, NULL, NULL, NULL, NULL), OK);
  CHECK(font_glyph_get_bitmap(glyph, true, &w, NULL, NULL, NULL), OK);
  CHECK(font_glyph_get_bitmap(glyph, true, NULL, &h, NULL, NULL), OK);
  CHECK(font_glyph_get_bitmap(glyph, true, &w, &h, NULL, NULL), OK);
  CHECK(font_glyph_get_bitmap(glyph, true, NULL, NULL, &Bpp, NULL), OK);
  CHECK(font_glyph_get_bitmap(glyph, true, &w, NULL, &Bpp, NULL), OK);
  CHECK(font_glyph_get_bitmap(glyph, true, NULL, &h, &Bpp, NULL), OK);
  CHECK(font_glyph_get_bitmap(glyph, true, &w, &h, &Bpp, NULL), OK);
  NCHECK(w, 0);
  NCHECK(h, 0);
  NCHECK(Bpp, 0);

  buffer = MEM_CALLOC
    (&mem_default_allocator, (size_t)w*h*Bpp, sizeof(unsigned char));
  NCHECK(buffer, NULL);
  buffer_size = (size_t)(w * h * Bpp) * sizeof(unsigned char);

  CHECK(font_glyph_get_bitmap(glyph, false, NULL, NULL, NULL, buffer), OK);
  CHECK(font_glyph_get_bitmap(glyph, false, &w, NULL, NULL, buffer), OK);
  CHECK(font_glyph_get_bitmap(glyph, false, NULL, &h, NULL, buffer), OK);
  CHECK(font_glyph_get_bitmap(glyph, false, &w, &h, NULL, buffer), OK);
  CHECK(font_glyph_get_bitmap(glyph, false, NULL, NULL, &Bpp, buffer), OK);
  CHECK(font_glyph_get_bitmap(glyph, false, &w, NULL, &Bpp, buffer), OK);
  CHECK(font_glyph_get_bitmap(glyph, false, NULL, &h, &Bpp, buffer), OK);
  CHECK(font_glyph_get_bitmap(glyph, false, &w, &h, &Bpp, buffer), OK);

  CHECK(font_glyph_ref_get(NULL), BAD_ARG);
  CHECK(font_glyph_ref_get(glyph), OK);
  CHECK(font_glyph_ref_put(NULL), BAD_ARG);
  CHECK(font_glyph_ref_put(glyph), OK);
  CHECK(font_glyph_ref_put(glyph), OK);

  b = true;
  for(i = 0; b && i < w*h*Bpp; ++i)
    b = ((int)buffer[i] == 0);
  CHECK(b, false);

  for(i = 32; i < 127; ++i) {
    size_t required_buffer_size = 0;
    CHECK(font_rsrc_get_glyph(font, (wchar_t)i, &glyph), OK);

    CHECK(font_glyph_get_bitmap(glyph, true, &w, &h, &Bpp, NULL), OK);
    required_buffer_size = (size_t)(w * h * Bpp) * sizeof(unsigned char);
    if(required_buffer_size > buffer_size) {
      buffer = MEM_REALLOC(&mem_default_allocator,buffer, required_buffer_size);
      NCHECK(buffer, NULL);
      buffer_size = required_buffer_size;
    }
    CHECK(font_glyph_get_bitmap(glyph, true, &w, &h, &Bpp, buffer), OK);
    NCHECK(snprintf(buf, BUFSIZ, "/tmp/%.3d.ppm", i - 32), BUFSIZ);
    CHECK(image_ppm_write(buf, w, h, Bpp, buffer), 0);
    CHECK(font_glyph_ref_put(glyph), OK);
  }
  MEM_FREE(&mem_default_allocator, buffer);

  CHECK(font_rsrc_ref_get(NULL), BAD_ARG);
  CHECK(font_rsrc_ref_get(font), OK);
  CHECK(font_rsrc_ref_put(NULL), BAD_ARG);
  CHECK(font_rsrc_ref_put(font), OK);
  CHECK(font_rsrc_ref_put(font), OK);

  CHECK(font_system_ref_get(NULL), BAD_ARG);
  CHECK(font_system_ref_get(sys), OK);
  CHECK(font_system_ref_put(NULL), BAD_ARG);
  CHECK(font_system_ref_put(sys), OK);
  CHECK(font_system_ref_put(sys), OK);

  CHECK(MEM_ALLOCATED_SIZE(&mem_default_allocator), 0);

  return 0;

error:
  return -1;
}

