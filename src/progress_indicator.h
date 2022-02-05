#pragma once

#include <cstdint>
#include <string>

class progress_indicator {
public:
  progress_indicator(std::size_t width, std::string label);
  ~progress_indicator();

  void set_progress(double percentage);
  void set_finished();
  void set_label(std::string const& label);

private:
  void redraw(double percentage);

  std::size_t m_width;
  std::string m_label;
  bool m_is_finished = false;
};
