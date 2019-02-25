/* Taken from http://cairographics.org/samples/ */

#include <cairo.h>
#include <math.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
  char* filename;
  if (argc != 2) {
    fprintf(stderr, "Usage: %s OUTPUT_FILENAME\n", argv[0]);
    return 1;
  }
  filename = argv[1];
  double xc = 128.0;
  double yc = 128.0;
  double radius = 100.0;
  double angle1 = 45.0 * (M_PI / 180.0);  /* angles are specified */
  double angle2 = 180.0 * (M_PI / 180.0); /* in radians           */

  cairo_surface_t* im =
    cairo_image_surface_create(CAIRO_FORMAT_ARGB32, xc * 2, yc * 2);
  cairo_t* cr = cairo_create(im);

  cairo_set_line_width(cr, 10.0);
  cairo_arc(cr, xc, yc, radius, angle1, angle2);
  cairo_stroke(cr);

  /* draw helping lines */
  cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.6);
  cairo_set_line_width(cr, 6.0);

  cairo_arc(cr, xc, yc, 10.0, 0, 2 * M_PI);
  cairo_fill(cr);

  cairo_arc(cr, xc, yc, radius, angle1, angle1);
  cairo_line_to(cr, xc, yc);
  cairo_arc(cr, xc, yc, radius, angle2, angle2);
  cairo_line_to(cr, xc, yc);
  cairo_stroke(cr);

  cairo_status_t status = cairo_surface_write_to_png(im, filename);
  cairo_surface_destroy(im);
  if (status != CAIRO_STATUS_SUCCESS) {
    fprintf(stderr, "Could not save png to '%s'\n", filename);
  }

  cairo_destroy(cr);
  return 0;
}
