#include "rasterizer.h"

using namespace std;

namespace CGL {

RasterizerImp::RasterizerImp(PixelSampleMethod psm, LevelSampleMethod lsm,
                             size_t width, size_t height,
                             unsigned int sample_rate) {
  this->psm = psm;
  this->lsm = lsm;
  this->width = width;
  this->height = height;
  this->sample_rate = sample_rate;

  sample_buffer.resize(width * height * sample_rate, Color::White);
}

// Used by rasterize_point and rasterize_line
void RasterizerImp::fill_pixel(size_t x, size_t y, size_t t, Color c) {
  // TODO: Task 2: You might need to this function to fix points and lines (such
  // as the black rectangle border in test4.svg) NOTE: You are not required to
  // implement proper supersampling for points and lines It is sufficient to use
  // the same color for all supersamples of a pixel for points and lines (not
  // triangles

  sample_buffer[(y * width + x) * sample_rate + t] = c;
}

// Rasterize a point: simple example to help you start familiarizing
// yourself with the starter code.
//
void RasterizerImp::rasterize_point(float x, float y, Color color) {
  // fill in the nearest pixel
  int sx = (int)floor(x);
  int sy = (int)floor(y);

  // check bounds
  if (sx < 0 || sx >= width)
    return;
  if (sy < 0 || sy >= height)
    return;

  for (int t = 0; t < sample_rate; ++t) {
    fill_pixel(sx, sy, t, color);
  }
  return;
}

// Rasterize a line.
void RasterizerImp::rasterize_line(float x0, float y0, float x1, float y1,
                                   Color color) {
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  float pt[] = {x0, y0};
  float m = (y1 - y0) / (x1 - x0);
  float dpt[] = {1, m};
  int steep = abs(m) > 1;
  if (steep) {
    dpt[0] = x1 == x0 ? 0 : 1 / abs(m);
    dpt[1] = x1 == x0 ? (y1 - y0) / abs(y1 - y0) : m / abs(m);
  }

  while (floor(pt[0]) <= floor(x1) && abs(pt[1] - y0) <= abs(y1 - y0)) {
    for (int i = 0; i < this->sample_rate; ++i) {
      rasterize_point(pt[0], pt[1],  color);
    }
    pt[0] += dpt[0];
    pt[1] += dpt[1];
  }
}

// Rasterize a triangle.
void RasterizerImp::rasterize_triangle(float x0, float y0, float x1, float y1,
                                       float x2, float y2, Color color) {
  // TODO: Task 1: Implement basic triangle rasterization here, no supersampling

  auto islefttop = [](float x0, float y0, float x1, float y1) -> bool {
    if (y0 == y1) {
      return x1 > x0;
    }
    return y1 > y0;
  };

  bool lefttop0 = islefttop(x0, y0, x1, y1);
  bool lefttop1 = islefttop(x1, y1, x2, y2);
  bool lefttop2 = islefttop(x2, y2, x0, y0);

  auto inside = [=](float x, float y) -> bool {
    auto cross = [](float x, float y, float x0, float y0, float x1,
                    float y1) -> float {
      float dx = x1 - x0;
      float dy = y1 - y0;
      return (x - x0) * dy - (y - y0) * dx;
    };

    float e0 = cross(x, y, x0, y0, x1, y1);
    float e1 = cross(x, y, x1, y1, x2, y2);
    float e2 = cross(x, y, x2, y2, x0, y0);

    // check opengel edge rule
    bool all_positive = (e0 > 0 || (e0 == 0 && lefttop0)) &&
                        (e1 > 0 || (e1 == 0 && lefttop1)) &&
                        (e2 > 0 || (e2 == 0 && lefttop2));
    if (all_positive) {
      return true;
    }

    bool all_negative = (e0 < 0 || (e0 == 0 && lefttop0)) &&
                        (e1 < 0 || (e1 == 0 && lefttop1)) &&
                        (e2 < 0 || (e2 == 0 && lefttop2));
    return all_negative;
  };

  // calculate bounding box
  float xl = std::min({x0, x1, x2});
  float xr = std::max({x0, x1, x2});
  float yu = std::min({y0, y1, y2});
  float yd = std::max({y0, y1, y2});

  // fix: For per row, we can find left most and right most and fill pixel
  // between both it.
  int n = static_cast<int>(std::sqrt(this->sample_rate));
  float step = 1.0f / n;

  for (int x = (int)xl; x <= xr; ++x) {
    for (int y = (int)yu; y <= yd; ++y) {
      if (x >= width || y >= height) {
        continue;
      }
      for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
          float x_ = x + (i + 0.5) * step;
          float y_ = y + (j + 0.5) * step;
          if (inside(x_, y_)) {
            fill_pixel(x, y, i * n + j, color);
          }
        }
      }
    }
  }

  // TODO: Task 2: Update to implement super-sampled rasterization
}

void RasterizerImp::rasterize_interpolated_color_triangle(float x0, float y0,
                                                          Color c0, float x1,
                                                          float y1, Color c1,
                                                          float x2, float y2,
                                                          Color c2) {
  // TODO: Task 4: Rasterize the triangle, calculating barycentric coordinates
  // and using them to interpolate vertex colors across the triangle Hint: You
  // can reuse code from rasterize_triangle
}

void RasterizerImp::rasterize_textured_triangle(float x0, float y0, float u0,
                                                float v0, float x1, float y1,
                                                float u1, float v1, float x2,
                                                float y2, float u2, float v2,
                                                Texture &tex) {
  // TODO: Task 5: Fill in the SampleParams struct and pass it to the tex.sample
  // function.
  // TODO: Task 6: Set the correct barycentric differentials in the SampleParams
  // struct. Hint: You can reuse code from
  // rasterize_triangle/rasterize_interpolated_color_triangle
}

void RasterizerImp::set_sample_rate(unsigned int rate) {
  // TODO: Task 2: You may want to update this function for supersampling
  // support

  this->sample_rate = rate;

  this->sample_buffer.resize(width * height * this->sample_rate, Color::White);
  clear_buffers();
  // std::fill(this->sample_buffer.begin(), this->sample_buffer.end(), Color::White);
}

void RasterizerImp::set_framebuffer_target(unsigned char *rgb_framebuffer,
                                           size_t width, size_t height) {
  // TODO: Task 2: You may want to update this function for supersampling
  // support

  this->width = width;
  this->height = height;
  this->rgb_framebuffer_target = rgb_framebuffer;

  this->sample_buffer.resize(width * height * this->sample_rate, Color::White);
  clear_buffers();
}

void RasterizerImp::clear_buffers() {
  std::fill(rgb_framebuffer_target, rgb_framebuffer_target + 3 * width * height,
            255);
  std::fill(sample_buffer.begin(), sample_buffer.end(), Color::White);
}

// This function is called at the end of rasterizing all elements of the
// SVG file.  If you use a supersample buffer to rasterize SVG elements
// for antialising, you could use this call to fill the target framebuffer
// pixels from the supersample buffer data.
//
void RasterizerImp::resolve_to_framebuffer() {
  // TODO: Task 2: You will likely want to update this function for
  // supersampling support

  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      Color color = Color::Black;

      for (int t = 0; t < sample_rate; ++t) {
        color += sample_buffer[(y * width + x) * sample_rate + t];
      }
      color *= 1.0 / sample_rate;

      for (int k = 0; k < 3; ++k) {
        this->rgb_framebuffer_target[3 * (y * width + x) + k] =
            (&color.r)[k] * 255;
      }
    }
  }
}

Rasterizer::~Rasterizer() {}

} // namespace CGL
