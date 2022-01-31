#include <cnfkit/dimacs_parser.h>
#include <cnfkit/io/io_libarchive.h>

#include <gatekit/gate.h>
#include <gatekit/scanner.h>
#include <gatekit/traits.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>


namespace gatekit {
template <>
struct lit_traits<cnfkit::lit> {
  static auto negate(cnfkit::lit literal) -> cnfkit::lit { return -literal; }
  static auto to_index(cnfkit::lit literal) -> std::size_t { return literal.get_raw_value(); }
  static auto to_var_index(cnfkit::lit literal) -> std::size_t
  {
    return literal.get_var().get_raw_value();
  }
};
}


namespace {
using Clause = std::vector<cnfkit::lit>;
using ClauseHandle = Clause const*;

template <typename T>
class ptr_iterator {
public:
  explicit ptr_iterator(T* p) noexcept : m_ptr(p) {}

  auto operator*() const noexcept -> T* { return m_ptr; }

  auto operator++() noexcept -> ptr_iterator&
  {
    ++m_ptr;
    return *this;
  }

  auto operator+(size_t offset) noexcept -> ptr_iterator { return ptr_iterator{m_ptr + offset}; }

  auto operator!=(const ptr_iterator& rhs) const noexcept -> bool { return m_ptr != rhs.m_ptr; }

private:
  T* m_ptr;
};


auto operator<<(std::ostream& s, cnfkit::lit const& lit) -> std::ostream&
{
  if (!lit.is_positive()) {
    s << "-";
  }

  s << lit.get_var().get_raw_value();
  return s;
}

auto vars_in_problem(std::vector<Clause> const& problem) -> size_t
{
  std::unordered_set<uint32_t> vars;
  for (Clause const& clause : problem) {
    for (cnfkit::lit const& literal : clause) {
      vars.insert(literal.get_var().get_raw_value());
    }
  }
  return vars.size();
}

auto clauses_in_problem(std::vector<Clause> const& problem) -> size_t
{
  return problem.size();
}

auto unaries_in_problem(std::vector<Clause> const& problem) -> size_t
{
  return std::count_if(problem.begin(), problem.end(), [](auto const& x) { return x.size() == 1; });
}

auto clauses_in_gates(gatekit::gate_structure<ClauseHandle> const& structure) -> size_t
{
  size_t result = 0;
  for (gatekit::gate<ClauseHandle> const& gate : structure.gates) {
    result += gate.clauses.size();
  }
  return result;
}

template <typename T>
void print_stat(std::string name, T&& value)
{
  std::cout << name << ": " << value << "\n";
}

void print_stats(std::vector<Clause> const& problem,
                 gatekit::gate_structure<ClauseHandle> const& structure,
                 std::filesystem::path const& path,
                 std::chrono::milliseconds duration)
{
  print_stat("name", path.filename().string());
  print_stat("dur_gate_scan_seconds", static_cast<double>(duration.count()) / 1000.0);
  print_stat("num_vars_in_problem", vars_in_problem(problem));
  print_stat("num_clauses_in_problem", clauses_in_problem(problem));
  print_stat("num_unaries_in_problem", unaries_in_problem(problem));
  print_stat("num_clauses_in_gates", clauses_in_gates(structure));
  print_stat("num_gates", structure.gates.size());
  print_stat("num_roots", structure.roots.size());
  print_stat("num_gates/num_vars_in_problem",
             static_cast<double>(structure.gates.size()) /
                 static_cast<double>(vars_in_problem(problem)));
  print_stat("num_clauses_in_gates/num_clauses_in_problem",
             static_cast<double>(clauses_in_gates(structure)) /
                 static_cast<double>(clauses_in_problem(problem)));
}

template <typename Clock = std::chrono::steady_clock>
class stopwatch {
public:
  stopwatch() : m_start{Clock::now()} {}

  auto duration_since_start() const -> typename Clock::duration { return Clock::now() - m_start; }


private:
  typename Clock::time_point m_start;
};


void scan_gates(std::filesystem::path const& path)
{
  std::vector<Clause> problem;

  cnfkit::libarchive_source cnf_source{path};
  cnfkit::parse_cnf(cnf_source, [&problem](std::vector<cnfkit::lit> const& clause) {
    problem.emplace_back(clause);
  });

  stopwatch gate_scan_stopwatch;

  auto start = ptr_iterator<Clause const>{&problem.front()};
  gatekit::gate_structure<ClauseHandle> gates =
      gatekit::scan_gates<ClauseHandle>(start, start + problem.size());

  auto elapsed = gate_scan_stopwatch.duration_since_start();

  print_stats(problem, gates, path, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed));
}
}

auto main(int argc, char** argv) -> int
{
  if (argc != 2) {
    std::cerr << "Usage: garrett FILE\n";
    return 1;
  }

  try {
    scan_gates(argv[1]);
  }
  catch (std::exception const& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
