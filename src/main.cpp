#include "evaluation.h"
#include "progress_indicator.h"
#include "types.h"
#include "validation.h"

#include <cnfkit/dimacs_parser.h>
#include <cnfkit/io/io_libarchive.h>

#include <gatekit/gate.h>
#include <gatekit/scanner.h>

#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

namespace {
constexpr size_t progress_bar_width = 50;

auto parse_problem(std::filesystem::path const& path) -> std::vector<Clause>
{
  std::vector<Clause> problem;

  cnfkit::libarchive_source cnf_source{path};
  cnfkit::parse_cnf(cnf_source, [&problem](std::vector<cnfkit::lit> const& clause) {
    problem.emplace_back(clause);
  });

  return problem;
}

auto scan_gates(std::vector<Clause> const& problem) -> gatekit::gate_structure<ClauseHandle>
{
  auto start = ptr_iterator<Clause const>{&problem.front()};
  return gatekit::scan_gates<ClauseHandle>(start, start + problem.size());
}

enum class gate_structure_validity { valid, invalid };

auto verify_gate_structure(gatekit::gate_structure<ClauseHandle> const& structure)
    -> gate_structure_validity
{
  progress_indicator bar{progress_bar_width, "Verifying gates"};

  auto progress_fn = [&bar, &structure](std::size_t num_verified_gates) {
    bar.set_progress(static_cast<double>(num_verified_gates) / structure.gates.size());
  };

  unsigned int num_threads = std::min(2u, std::thread::hardware_concurrency());
  bool const result = is_valid_gate_structure(structure, num_threads, progress_fn);

  return result ? gate_structure_validity::valid : gate_structure_validity::invalid;
}

void evaluate_gate_structure(std::filesystem::path const& path)
{
  std::vector<Clause> problem = parse_problem(path);

  progress_indicator bar{progress_bar_width, "Scanning gates"};

  stopwatch gate_scan_stopwatch;
  auto gate_structure = scan_gates(problem);
  auto elapsed = gate_scan_stopwatch.duration_since_start();

  bar.set_finished();

  gate_structure_validity valid = verify_gate_structure(gate_structure);

  print_stats(problem,
              gate_structure,
              path,
              std::chrono::duration_cast<std::chrono::milliseconds>(elapsed));
  std::cout << "valid: " << (valid == gate_structure_validity::valid) << "\n";
}
}

auto main(int argc, char** argv) -> int
{
  if (argc != 2) {
    std::cerr << "Usage: garrett FILE\n";
    return 1;
  }

  try {
    evaluate_gate_structure(argv[1]);
  }
  catch (std::exception const& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
