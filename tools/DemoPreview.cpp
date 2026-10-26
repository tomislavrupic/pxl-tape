#include "Demo.h"
#include <iostream>
int main(){pxl::Engine e;e.prepare(48000);pxl::loadDemo(e);std::cout<<"{\"name\":\"Neon Transit\",\"tempo\":112,\"frames\":"<<e.loopEnd<<",\"peaks\":[";for(int t=0;t<4;t++)for(int b=0;b<320;b++){float peak=0;for(int i=b*e.loopEnd/320;i<(b+1)*e.loopEnd/320;i++)peak=std::max(peak,std::max(std::abs(e.tape[t][0][i]),std::abs(e.tape[t][1][i])));if(t||b)std::cout<<",";std::cout<<peak;}std::cout<<"]}";}
