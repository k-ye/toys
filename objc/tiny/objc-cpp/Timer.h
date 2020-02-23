#include <chrono>

// https://stackoverflow.com/a/14391562/12003165
class Timer {
public:
  using Clock = std::chrono::high_resolution_clock;
  using Duration = Clock::duration;

  void start() {
    start_ = Clock::now();
    duration_ = Duration::zero();
  }

  void stop() { duration_ = Clock::now() - start_; }

  Duration get_duration() const { return duration_; }

  int get_micros() const {
    return std::chrono::duration_cast<std::chrono::microseconds>(duration_)
        .count();
  }

private:
  std::chrono::time_point<Clock> start_;
  Duration duration_;
};