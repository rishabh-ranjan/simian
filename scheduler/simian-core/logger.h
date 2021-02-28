/*
 * logger.h: header only logging code
 * author: Rishabh Ranjan
 */

#pragma once

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

inline static unsigned long long unix_time_ms() {
	return std::chrono::duration_cast< std::chrono::milliseconds >(
			std::chrono::system_clock::now().time_since_epoch()).count();
}

static std::string utc_timestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto now_time_t = std::chrono::system_clock::to_time_t(now);
  const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()) % 1000;
  std::stringstream now_ss;
  now_ss
      << std::put_time(std::localtime(&now_time_t), "%a %b %d %Y %T")
      << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
  return now_ss.str();
}

class Logger {
	std::string logpath_;
	std::ostream* ost_;

public:
	void open(const std::string& logpath) {
		logpath_ = logpath;
		auto tmp = new std::ofstream();
		tmp->rdbuf()->pubsetbuf(nullptr, 0); // no buffering
		tmp->open(logpath/*, std::ios::app*/); // append
		if (!tmp->is_open()) {
			std::cerr << "Error: unable to open " << logpath << std::endl;
			std::exit(-1);
		}
		ost_ = tmp;
		std::cout << "Logging in " << logpath << std::endl;
	}

	explicit Logger(std::ostream& ost): ost_(&ost) { *ost_ << '\n'; }

	explicit Logger(const std::string& logpath) { open(logpath); }

	Logger() {}

	template <typename T>
	friend std::ostream& operator<<(Logger&, const T&);
};

template <typename T>
std::ostream& operator<<(Logger& logger, const T& t) {
	*logger.ost_ << "\n[" << utc_timestamp() << "] ";
	return *logger.ost_ << t;
}

typedef unsigned long long ull;
extern ull start_time;
extern std::string dirname;
extern Logger logger;
extern std::ofstream statf;
extern ull scheduler_time;
extern ull trace_time;
extern ull path_time;
extern ull trace_solver_time;
extern ull path_solver_time;
extern std::vector< std::pair< int, int >> path_epoch_info;

#if 0
int main() {
	Logger logger;
	logger << "Hello World!";
	for (int i = 0; i < 5; ++i) {
		logger << '\t' << "i = " << i;
	}
	logger << "Bye!";
}
#endif
