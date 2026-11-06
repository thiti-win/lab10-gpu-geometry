#pragma once
#include "vec3.h"
// Tiny given font for the step-count legend.
inline void image_number(std::vector<Color>& image,int w,int h,int x,int y,const std::string& text){
  const char* digits[10]={"111101101101111","010110010010111","111001111100111","111001111001111","101101111001001","111100111001111","111100111101111","111001001001001","111101111101111","111101111001111"};
  int scale=std::max(1,w/320);
  for(char c:text){for(int j=0;j<5;++j)for(int i=0;i<3;++i)if((c=='+' ? (j==2 || (i==1 && j>=1 && j<=3)) : digits[c-'0'][j*3+i]=='1'))for(int dy=0;dy<scale;++dy)for(int dx=0;dx<scale;++dx){int px=x+i*scale+dx,py=y+j*scale+dy;if(px>=0&&px<w&&py>=0&&py<h)image[py*w+px]={1,1,1};}x+=4*scale;}
}
