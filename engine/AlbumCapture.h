#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cstdint>
namespace pxl {
// Single producer audio callback, single consumer storage thread/worklet drain.
// No allocation, locks or IO; overflow preserves the complete captured prefix.
class AlbumCapture {
 public:
 static constexpr uint32_t capacity=16384;
 void start(int rate,int minutes=30){stop();while(producer.test_and_set(std::memory_order_acquire)){};readAt=0;writeAt=0;total=0;lost=false;limit=uint32_t(rate)*60*uint32_t(std::clamp(minutes,1,30));running=true;producer.clear(std::memory_order_release);}
 void stop(){running.store(false,std::memory_order_release);while(producer.test_and_set(std::memory_order_acquire)){}producer.clear(std::memory_order_release);}
 bool recording()const{return running.load(std::memory_order_acquire);}
 bool overflowed()const{return lost.load(std::memory_order_acquire);}
 uint32_t available()const{return (writeAt.load(std::memory_order_acquire)+capacity-readAt.load(std::memory_order_acquire))%capacity;}
 uint32_t frames()const{return total.load(std::memory_order_acquire);}
 void push(float l,float r){if(producer.test_and_set(std::memory_order_acquire))return;if(!recording()){producer.clear(std::memory_order_release);return;}auto w=writeAt.load(std::memory_order_relaxed),next=(w+1)%capacity;if(next==readAt.load(std::memory_order_acquire)){lost=true;running=false;}else{samples[w*2]=l;samples[w*2+1]=r;writeAt.store(next,std::memory_order_release);if(total.fetch_add(1,std::memory_order_release)+1>=limit)running=false;}producer.clear(std::memory_order_release);}
 int read(float* out,int maxFrames){int n=0;auto r=readAt.load(std::memory_order_relaxed),w=writeAt.load(std::memory_order_acquire);while(r!=w&&n<maxFrames){out[n*2]=samples[r*2];out[n*2+1]=samples[r*2+1];r=(r+1)%capacity;++n;}readAt.store(r,std::memory_order_release);return n;}
 private:
 std::array<float,capacity*2> samples{};
 std::atomic_flag producer=ATOMIC_FLAG_INIT;
 std::atomic<uint32_t> readAt{0},writeAt{0},total{0};std::atomic<bool> running{false},lost{false};uint32_t limit=0;
};
}
