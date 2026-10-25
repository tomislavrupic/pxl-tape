#include "../engine/WebBridge.cpp"
#include <cstdio>
int main(){tape_init(48000);tape_note(60,.8f,1,0);for(int i=0;i<100;i++){tape_render(128);std::fwrite(tape_output(),sizeof(float),256,stdout);}return 0;}
