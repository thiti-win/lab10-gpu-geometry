#pragma once
#include "vec3.h"
#include <string>
#include <vector>
void save_png(const std::string& path,const std::vector<Color>& image,int w,int h);
// Return a panel message. Missing or mismatched references are safe to skip.
std::string save_comparison(const std::string& name, const std::string& reference,
                            const std::vector<Color>& image, int w, int h);
