#include "evaluation.h"
#include "types.h"
#include "validation.h"

#include <cnfkit/dimacs_parser.h>
#include <cnfkit/io/io_libarchive.h>

#include <gatekit/gate.h>
#include <gatekit/scanner.h>

#include <filesystem>
#include <iostream>
#include <vector>

namespace {
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

void evaluate_gate_structure(std::filesystem::path const& path)
{
  std::vector<Clause> problem = parse_problem(path);

  stopwatch gate_scan_stopwatch;
  auto gate_structure = scan_gates(problem);
  auto elapsed = gate_scan_stopwatch.duration_since_start();

  print_stats(problem,
              gate_structure,
              path,
              std::chrono::duration_cast<std::chrono::milliseconds>(elapsed));

  std::cout << (is_valid_gate_structure(gate_structure) ? "valid" : "invalid") << "\n";
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
