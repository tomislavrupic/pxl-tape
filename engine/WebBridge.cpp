#include "Demo.h"
#include "TapeEngine.h"
#include "AlbumCapture.h"
#include <cstring>
#include <new>
static pxl::Engine e;static pxl::AlbumCapture album;static float albumAudio[8192]{};
static float audio[256]{},input[256]{},status[1552]{};
static void defaults(){e.sequence[0]=e.sequence[4]=e.sequence[8]=e.sequence[12]=true;for(int s=0;s<16;s++){e.drumSequence[0][s]=s%4==0;e.drumSequence[1][s]=s==4||s==12;e.drumSequence[2][s]=s%2==0;e.drumSequence[3][s]=s==14;}}
extern "C" {
int tape_init(int rate){if(rate<8000||rate>96000)return 0;e.prepare(rate);defaults();return 1;}
int tape_init_fx(int rate){if(rate<8000||rate>96000)return 0;e.prepare(rate,1);return 1;}
void tape_checkpoint(int track){if(!e.recording&&track>=0&&track<4)e.checkpoint(track);}
int tape_parameter_count(){return 37+pxl::deviceParameterCount;}
void tape_mod_cc(float value){if(std::isfinite(value))e.modulationCC=std::clamp(value,0.f,1.f);}
void tape_album_start(){album.start(int(e.rate));}
void tape_album_stop(){album.stop();}
int tape_album_recording(){return album.recording()?1:0;}
int tape_album_overflow(){return album.overflowed()?1:0;}
int tape_album_pending(){return int(album.available());}
int tape_album_frames(){return int(album.frames());}
int tape_album_read(){return album.read(albumAudio,4096);}
float* tape_album_output(){return albumAudio;}
int tape_storage(int track){return track>=0&&track<4?int(e.tape[track][0].size()):0;}
void tape_reserve(int track,int frames){e.reserveTrack(track,frames);}
int tape_capacity(){return e.capacity;}float* tape_output(){return audio;}float* tape_input(){return input;}
float* tape_track(int track,int channel){return track>=0&&track<4&&channel>=0&&channel<2?e.tape[track][channel].data():nullptr;}
int tape_used(int track,int frames){if(track<0||track>3)return 0;if(frames>=0)e.used[track]=std::clamp(frames,0,e.capacity);return e.used[track];}
int tape_sample_size(int frames){if(frames>=0){if(frames>e.rate*30)return -1;for(auto&c:e.sample)c.resize(frames);}return int(e.sample[0].size());}
float* tape_sample(int channel){return channel>=0&&channel<2?e.sample[channel].data():nullptr;}
int tape_kit_size(int frames){if(frames>=0){if(frames>e.rate*16)return -1;for(auto& c:e.drumKit)c.resize(frames);}return int(e.drumKit[0].size());}
float* tape_kit(int channel){return channel>=0&&channel<2?e.drumKit[channel].data():nullptr;}
void tape_set(int p,float v){if(!std::isfinite(v))return;switch(p){
 case 0:e.sound=std::clamp(int(v),0,12);break;case 1:e.tone=std::clamp(v,0.f,1.f);break;case 2:e.shape=std::clamp(v,0.f,1.f);break;case 3:e.attack=std::clamp(v,.001f,2.f);break;case 4:e.release=std::clamp(v,.02f,5.f);break;case 5:e.instrumentGain=std::clamp(v,0.f,1.f);break;case 6:e.drumGain=std::clamp(v,0.f,1.f);break;case 7:e.bpm=std::clamp(v,40.f,240.f);break;case 8:e.speed=e.recording?1:std::clamp(v,-2.f,2.f);break;case 9:e.swing=std::clamp(v,0.f,.5f);break;case 10:e.drive=std::clamp(v,0.f,1.f);break;case 11:e.delayMix=std::clamp(v,0.f,1.f);break;case 12:e.delayFeedback=std::clamp(v,0.f,.85f);break;case 13:e.reverbMix=std::clamp(v,0.f,1.f);break;case 14:e.master=std::clamp(v,0.f,1.f);break;case 15:e.monitor=v>.5;break;case 16:e.seqEnabled=v>.5;if(!e.seqEnabled)e.allNotesOff();break;case 17:e.drumSeqEnabled=v>.5;break;case 18:break;case 19:e.scale=std::clamp(int(v),0,2);break;case 20:e.root=std::clamp(int(v),0,11);break;
 default:if(p>=37){pxl::setDeviceParameter(e.device,p-37,v);break;}if(p>=21&&p<37){int t=(p-21)/4;switch((p-21)%4){case 0:e.trackGain[t]=std::clamp(v,0.f,1.5f);break;case 1:e.trackPan[t]=std::clamp(v,-1.f,1.f);break;case 2:e.trackMute[t]=v>.5;break;case 3:e.trackSolo[t]=v>.5;break;}}break;}}
void tape_note(int note,float velocity,int on,int drum){if(note<0||note>127||!std::isfinite(velocity))return;if(drum){if(on)e.triggerDrum((note-36+128)%8,std::clamp(velocity,0.f,1.f));}else if(on)e.noteOn(note,std::clamp(velocity,0.f,1.f));else e.noteOff(note);}
void tape_sustain(int on){e.setSustain(on!=0);}
void tape_stop_drums(){e.stopDrums();}
void tape_drum(int index){if(index>=0&&index<8)e.triggerDrum(index,.9f);}
void tape_arm(int track){if(!e.recording)e.armed=std::clamp(track,0,3);}
void tape_seek(int frame){if(!e.recording)e.position=std::clamp(frame,e.loopStart,e.loopEnd-1);}
void tape_select(int a,int b){if(!e.recording){e.selectStart=std::clamp(std::min(a,b),e.loopStart,e.loopEnd);e.selectEnd=std::clamp(std::max(a,b),e.loopStart,e.loopEnd);}}
int tape_loop_start(){return e.loopStart;}
void tape_loop(int bars){if(!e.recording){e.loopStart=0;e.loopEnd=bars==0?e.capacity:std::clamp(int(e.rate*60/e.bpm*4*std::clamp(bars,1,256)),1,e.capacity);e.selectStart=e.selectEnd=0;if(e.position>=e.loopEnd)e.rewind();}}
void tape_region(int start,int end,int position){if(start>=0&&end>start&&end<=e.capacity){e.loopStart=start;e.loopEnd=end;e.position=std::clamp(position,start,end-1);}}
void tape_step(int row,int step,int on,int note){if(row<0||row>8||step<0||step>127)return;if(row==0){e.sequence[step]=on!=0;e.seqNotes[step]=std::clamp(note,0,127);}else e.drumSequence[row-1][step]=on!=0;}
int tape_action(int action){
 if(action>=4&&action<=10&&e.recording)return 0;
 switch(action){case 0:e.playing=!e.playing;if(!e.playing){e.recording=false;e.allNotesOff();}break;
 case 1:if(!e.recording){e.checkpoint(e.armed);e.reserveTrack(e.armed,e.capacity);e.speed=1;}e.recording=!e.recording;e.playing=true;break;
 case 2:e.rewind();break;case 3:if(!e.recording)e.speed=e.speed<0?1:-1;break;
 case 4:e.copy(e.armed);break;case 5:e.cut(e.armed);break;case 6:e.paste(e.armed);break;case 7:e.reverseSelection(e.armed);break;case 8:e.clear(e.armed);break;case 9:return e.undo()?1:0;case 10:e.bounce();break;
 case 11:{if(e.recording)return 0;auto s=e.selection();if(s.second-s.first>e.rate*30)return 0;e.panic();for(int c=0;c<2;c++){e.sample[c].resize(size_t(s.second-s.first));for(int i=s.first;i<s.second;i++)e.sample[c][size_t(i-s.first)]=e.readTape(e.armed,c,i);}e.sound=4;break;}
 case 12:e.playing=e.recording=false;e.clearAll();e.clearUndo();break;
 case 13:return pxl::loadDemo(e)?1:0;
 case 14:e.panic();break;case 15:e.playing=e.recording=false;e.allNotesOff();break;case 16:e.clearUndo();break;default:return 0;}
 return 1;
}
void tape_render(int frames){frames=std::clamp(frames,0,128);for(int i=0;i<frames;i++){auto sample=e.tick(input[i*2],input[i*2+1]);audio[i*2]=sample.first;audio[i*2+1]=sample.second;album.push(sample.first,sample.second);}}
float* tape_status(){status[0]=float(e.position);status[1]=e.playing;status[2]=e.recording;status[3]=e.armed;status[4]=e.loopEnd;status[5]=float(e.rate);status[6]=float(e.bpm);status[7]=e.currentStep;status[8]=e.meterL;status[9]=e.meterR;status[10]=e.sourceMeter;status[11]=float(e.sample[0].size());for(int t=0;t<4;t++)status[12+t]=float(e.used[t]);for(int i=0;i<256;i++)status[16+i]=e.scope[(e.scopeHead+i)%256];for(int t=0;t<4;t++)for(int i=0;i<320;i++){float peak=0;int a=i*e.loopEnd/320,b=(i+1)*e.loopEnd/320;for(int j=a;j<b;j+=std::max(1,(b-a)/24))peak=std::max(peak,std::max(std::abs(e.readTape(t,0,j)),std::abs(e.readTape(t,1,j))));status[272+t*320+i]=peak;}return status;}
}
