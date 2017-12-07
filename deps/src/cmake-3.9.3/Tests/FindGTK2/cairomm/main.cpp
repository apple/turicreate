// Taken from
// http://cgit.freedesktop.org/cairomm/plain/examples/surfaces/image-surface.cc

/* M_PI is defined in math.h in the case of Microsoft Visual C++, Solaris,
 * et. al.
 */
#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif

#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <cairommconfig.h>
#include <iostream>
#include <string>

#include <cmath>

int main()
{
  Cairo::RefPtr<Cairo::ImageSurface> surface =
    Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 600, 400);

  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);

  cr->save(); // save the state of the context
  cr->set_source_rgb(0.86, 0.85, 0.47);
  cr->paint();   // fill image with the color
  cr->restore(); // color is back to black now

  cr->save();
  // draw a border around the image
  cr->set_line_width(20.0); // make the line wider
  cr->rectangle(0.0, 0.0, surface->get_width(), surface->get_height());
  cr->stroke();

  cr->set_source_rgba(0.0, 0.0, 0.0, 0.7);
  // draw a circle in the center of the image
  cr->arc(surface->get_width() / 2.0, surface->get_height() / 2.0,
          surface->get_height() / 4.0, 0.0, 2.0 * M_PI);
  cr->stroke();

  // draw a diagonal line
  cr->move_to(surface->get_width() / 4.0, surface->get_height() / 4.0);
  cr->line_to(surface->get_width() * 3.0 / 4.0,
              surface->get_height() * 3.0 / 4.0);
  cr->stroke();
  cr->restore();

#ifdef CAIRO_HAS_PNG_FUNCTIONS

  std::string filename = "image.png";
  surface->write_to_png(filename);

  std::cout << "Wrote png file \"" << filename << "\"" << std::endl;

#else

  std::cout
    << "You must compile cairo with PNG support for this example to work."
    << std::endl;

#endif
}
