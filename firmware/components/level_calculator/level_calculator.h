#pragma once

#include <stdint.h>

namespace level_calculator {

struct Model {
    int level_max_cm;    // e.g., 450
    int sensor_offset_cm;// e.g., 20
    int vol_max_l;       // e.g., 80000
};

struct Result {
    int level_cm;
    int percentual; // 0..100
    int volume_l;   // liters
};

inline Result compute(int distance_cm, const Model &m) {
    int base = m.level_max_cm + m.sensor_offset_cm;
    int level_cm = base - distance_cm;
    if (level_cm < 0) level_cm = 0;
    if (level_cm > m.level_max_cm) level_cm = m.level_max_cm;

    int percentual = (level_cm * 100) / m.level_max_cm;
    int64_t vol = ((int64_t)level_cm * (int64_t)m.vol_max_l) / (int64_t)m.level_max_cm;
    Result r{level_cm, percentual, (int)vol};
    return r;
}

} // namespace level_calculator
