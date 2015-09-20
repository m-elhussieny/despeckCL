#ifndef TIMINGS_H
#define TIMINGS_H

#include <map>
#include <string>

namespace timings {

    typedef std::map<std::string, double> map;

    map join(const map& tm1, const map& tm2);

    double total_time(const map &tm);

    void print(const map& tm);
}

#endif
