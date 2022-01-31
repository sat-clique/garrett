#pragma once

#include "types.h"

#include <gatekit/gate.h>

#include <chrono>
#include <filesystem>
#include <vector>

void print_stats(std::vector<Clause> const& problem,
                 gatekit::gate_structure<ClauseHandle> const& structure,
                 std::filesystem::path const& path,
                 std::chrono::milliseconds duration);
