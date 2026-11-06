#include "image.h"
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../../external/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../external/stb_image_write.h"
void save_png(const std::string& path,const std::vector<Color>& img,int w,int h) {
    auto parent=std::filesystem::path(path).parent_path(); if(!parent.empty()) std::filesystem::create_directories(parent);
    std::vector<unsigned char> bytes(size_t(w)*h*3);
    for(size_t i=0;i<img.size();++i) { auto c=clamp(img[i],0,1); bytes[3*i]=(unsigned char)(255*c.x); bytes[3*i+1]=(unsigned char)(255*c.y); bytes[3*i+2]=(unsigned char)(255*c.z); }
    if(!stbi_write_png(path.c_str(),w,h,3,bytes.data(),w*3)) throw std::runtime_error("Cannot write "+path);
}

std::string save_comparison(const std::string& name, const std::string& reference,
                            const std::vector<Color>& image, int w, int h) {
    const std::string output = "out/" + name + "_compare.png";
    // Do not leave an old comparison that could be mistaken for this run.
    std::error_code error;
    std::filesystem::remove(output, error);
    if (reference.empty()) return "skipped: no reference matches these options";
    int rw, rh, channels;
    unsigned char* pixels = stbi_load(reference.c_str(), &rw, &rh, &channels, 3);
    if (!pixels) return "skipped: cannot read " + reference;
    if (rw != w || rh != h) {
        stbi_image_free(pixels);
        return "skipped: reference size does not match";
    }
    const int gap = 8;
    const int width = 2 * w + gap;
    std::vector<Color> pair(size_t(width) * h, {0.5f, 0.5f, 0.5f});
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t i = size_t(y) * w + x;
            pair[size_t(y) * width + x] = image[i];
            pair[size_t(y) * width + w + gap + x] =
                {pixels[3*i] / 255.0f, pixels[3*i+1] / 255.0f, pixels[3*i+2] / 255.0f};
        }
    }
    stbi_image_free(pixels);
    save_png(output, pair, width, h);
    return output + " (left: yours; right: reference)";
}
