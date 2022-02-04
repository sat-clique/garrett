#pragma once

#include "types.h"

#include <gatekit/gate.h>

#include <functional>

auto is_valid_gate_structure(gatekit::gate_structure<ClauseHandle> const& structure,
                             std::size_t const n_threads,
                             std::function<void(std::size_t)> const& progress_callback) -> bool;
