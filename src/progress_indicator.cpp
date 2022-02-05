#include "progress_indicator.h"

#include <iostream>

progress_indicator::progress_indicator(std::size_t width, std::string label)
  : m_width{width}, m_label{label}
{
  redraw(0.0);
}

progress_indicator::~progress_indicator()
{
  set_finished();
}

void progress_indicator::set_progress(double percentage)
{
  redraw(percentage);
}

void progress_indicator::set_finished()
{
  if (!m_is_finished) {
    set_progress(1.0);
    std::cerr << "\n";
    std::cerr.flush();
  }

  m_is_finished = true;
}

void progress_indicator::set_label(std::string const& label)
{
  m_label = label;
}

void progress_indicator::redraw(double percentage)
{
  std::cerr << "\r [";

  int percentage_100 = 100.0 * percentage;
  std::size_t progress_width = percentage * m_width;
  for (std::size_t i = 0; i < m_width; ++i) {
    if (i < progress_width) {
      std::cerr << "=";
    }
    else if (i == progress_width) {
      std::cerr << ">";
    }
    else {
      std::cerr << (i < progress_width ? "=" : " ");
    }
  }

  std::cerr << "]  " << (percentage_100 < 100 ? " " : "") << percentage_100 << "%  " << m_label;

  std::cerr.flush();
}
