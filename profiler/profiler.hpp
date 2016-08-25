#ifndef PROFILER_HPP
#define PROFILER_HPP
#include <unordered_map>
#include <string>
#include <chrono>

class profiler
{
public:
    profiler()
        : _prev_point(std::chrono::high_resolution_clock::now())
    {

    }

    void startpoint()
    {
        _prev_point = std::chrono::high_resolution_clock::now();
    }

    void checkpoint(const std::string & pointname_)
    {
        auto cur_point = std::chrono::high_resolution_clock::now();
        auto map_iter = _dur_list.find(pointname_);
        if (map_iter == _dur_list.end()) {
            _dur_list.insert(std::pair<std::string, long>(pointname_, 0));
        }
        _dur_list[pointname_] += std::chrono::duration_cast<std::chrono::microseconds>(cur_point - _prev_point).count();
        _prev_point = std::chrono::high_resolution_clock::now();
    }
    const std::unordered_map<std::string, long> get_list()
    {
        return _dur_list;
    }

    const long get_duration(const std::string & name) const
    {
        auto f = _dur_list.find(name);
        if (f == _dur_list.end())
            return 0;
        return f->second;
    }

    static profiler & global()
    {
        static profiler __global_profiler;
        return __global_profiler;
    }

private:

    std::chrono::high_resolution_clock::time_point _prev_point;
    std::unordered_map<std::string, long> _dur_list;

};

#endif // PROFILER_HPP
