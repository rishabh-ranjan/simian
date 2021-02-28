/*
 * print.cpp: printing, debugging and graph vizualization functions
 * author: Rishabh Ranjan
 */

#include "verifier.h"

#include <ostream>

template< typename T >
std::ostream& operator<<(std::ostream& ost, const std::vector< T >& v) {
	for (int i = 0; i < (int)v.size(); ++i) {
		if (i != 0) ost << ' ';
		ost << v[i];
	}
	return ost;
}

std::ostream& operator<<(std::ostream& ost, CallType t) {
	switch (t) {
		case CallType::send: return ost << "send";
		case CallType::recv: return ost << "recv";
		case CallType::raw_barr: return ost << "raw_barr";
		case CallType::merged_barr: return ost << "merged_barr";
		case CallType::misc: return ost << "misc";
	}
	assert(false);
}

std::ostream& operator<<(std::ostream& ost, const std::pair< Operator, int >& p) {
	switch (p.first) {
		case Operator::equal: return ost << "== " << p.second;
		case Operator::not_equal: return ost << "!= " << p.second;
		case Operator::less: return ost << "< " << p.second;
		case Operator::greater: return ost << "> " << p.second;
		case Operator::less_or_equal: return ost << "<= " << p.second;
		case Operator::greater_or_equal: return ost << ">= " << p.second;
	}
	assert(false);
}

std::ostream& operator<<(std::ostream& ost, const Condition& c) {
	return ost << "(" << c.and_ << ")";
}

std::ostream& operator<<(std::ostream& ost, const Call& c) {
	ost << "idx=" << c.idx << " ";
	ost << "pid=" << c.pid << " ";
	ost << "loc=" << c.loc << " ";
	ost << "comm=" << c.comm << " ";
	ost << "bid=" << c.bid << " ";
	ost << "epoch=" << c.epoch << " ";
	ost << "type=" << c.type << " ";
	ost << "\n\t";
	ost << "data=" << c.data << " ";
	ost << "nbr=" << c.nbr << " ";
	ost << "tag=" << c.tag << " ";
	ost << "data_cnd=" << c.data_cnd << " ";
	ost << "src_cnd=" << c.src_cnd << " ";
	ost << "tag_cnd=" << c.tag_cnd << " ";
	ost << "\n\t";
	ost << "preds={" << c.preds << "} ";
	ost << "succs={" << c.succs << "} ";
	ost << "mplus={" << c.mplus << "} ";
	ost << "cnd_mplus={" << c.cnd_mplus << "} ";
	return ost << '\n';
}

void Verifier::write_calls(std::ostream& ost) {
	for (Call& c: calls_) ost << c << '\n';
}

void Verifier::write_dot_prog(std::ostream& ost) {
	ost <<
		"digraph{"
		"node[shape=record style=filled colorscheme=purd9 fillcolor=2];";
	for (int i = 0; i < (int)pid_offset_.size() - 1; ++i) {
		ost <<
			"subgraph cluster" << i << "{"
			"style=dashed;"
			"colorscheme=purd9;"
			"bgcolor=1;"
			"edge[arrowhead=empty];"
			"label=proc_" << i << ";";
		for (int j = pid_offset_[i]; j < pid_offset_[i+1]; ++j) {
			Call& c = calls_[j];
			ost << c.idx << "[label=\""
			 		<< c.pid << ":" << c.loc << " " << c.comm << " " << c.type << " " << c.nbr << " " << c.tag
					<< "\"];";
		}
		for (int j = pid_offset_[i]; j < pid_offset_[i+1]; ++j) {
			Call& c = calls_[j];
			for (int x: c.preds) {
				Call& p = calls_[x];
				ost << p.idx << "->" << c.idx << ";";
			}
		}
		ost << "}";
	}
	ost << "}";
}

void Verifier::write_dot_epoch(std::ostream& ost) {
	const int cluster_spacing = 3;
	ost <<
		"digraph{"
		"node[shape=none margin=0 style=filled colorscheme=ylorrd9 fillcolor=2];"
		"compound=true;"
		"splines=line;";
	std::vector< std::vector< int >> ep(epochs_.size());
	for (Call& c: calls_) {
		if (c.type == CallType::raw_barr) continue;
		ep[c.epoch].push_back(c.idx);
	}
	for (int e = 0; e < (int)epochs_.size(); ++e) {
		for (int i = 0; i < cluster_spacing; ++i) {
			ost <<
				"subgraph clusterdummy" << e << "_" << i << "{"
				"style=invis;";
		}
		ost <<
			"subgraph cluster" << e << "{"
			"style=dashed;"
			"colorscheme=ylorrd9;"
			"bgcolor=1;"
			"edge[arrowhead=empty];"
			"label=epoch_" << e << ";";
		for (int x: ep[e]) {
			Call& c = calls_[x];
			std::ostringstream coss;
			coss << c.pid << ":" << c.loc << " " << c.type;
			std::ostringstream moss;
			moss << " ";
			for (int x: c.mplus) {
				Call& m = calls_[x];
				moss << m.pid << ":" << m.loc << " ";
			}
			ost << c.idx <<
				"[label=<<table border=\"0\" cellborder=\"0\" cellspacing=\"0\">"
				"<tr><td>" << coss.str() << "</td></tr><hr/>"
				"<tr><td>" << moss.str() << "</td></tr>"
				"</table>>];";
			for (int y: c.preds) {
				Call& p = calls_[y];
				if (p.epoch != e) continue;
				ost << p.idx << "->" << c.idx << ";";
			}
		}
		ost << "}";
		for (int i = 0; i < cluster_spacing; ++i) {
			ost << "}";
		}
	}
	ost <<
		"subgraph{"
		"edge[color=darkgray arrowsize=1.5];";
	for (Call& c: calls_) {
		if (c.type == CallType::raw_barr) continue;
		for (int x: c.preds) {
			Call& p = calls_[x];
			if (p.epoch == c.epoch) continue;
			ost << p.idx << "->" << c.idx << ";";
		}
	}
	ost << "}";
	ost << "}";
}

std::ostream& operator<<(std::ostream& ost, const EpochInfo& epoch_info) {
	ost << "number of calls: " << epoch_info.size << '\n';
	ost << "symmetries: " << epoch_info.syms << '\n';
	ost << "solver time: " << epoch_info.solver_time << " s" << '\n';
	ost << "repetitions: " << epoch_info.reps << '\n';
	return ost;
}

void Verifier::print_epoch_infos(std::ostream& ost) {
	ost << '\n';
	ost << "=== Epoch Info ===" << '\n';
	for (auto& p: verified_hashes_) {
		for (auto& e: p.second) {
			ost << e;
			ost << "---" << '\n';
		}
	}
}

void Verifier::print_epoch_bc(std::ostream& ost) {
	int num_calls = 0;
	for (auto& p: verified_hashes_) {
		for (auto& e: p.second) {
			ost << "(" << e.size << "," << e.syms << "," << e.reps << "), ";
			num_calls += e.size * e.reps;
		}
	}
	ost << num_calls << ' ';
	//ost << algo_time_ - solver_time_ << ' ';
	//ost << solver_time_ << ' ';
}
