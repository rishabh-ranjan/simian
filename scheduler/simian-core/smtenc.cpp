/*
 * smtenc.cpp: SMTEncoding memeber functions related to simian
 * authore: Rishabh Ranjan
 */

#include "../SMTEncoding.hpp"
#include "verifier.h"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

void SMTEncoding::init_ver() {
    std::map< std::string, int > comm_map;
    forall_transitionLists(iter, last_node->_tlist) {
        ver.push_proc();
        forall_transitions(titer, (*iter)->_tlist) {
            Envelope *env = titer->GetEnvelope();
            if (env->func == "Finalize") continue;
            auto it = comm_map.find(env->comm);
            int comm_id;
            if (it == comm_map.end()) {
                comm_id = comm_map.size();
                comm_map[env->comm] = comm_id;
            } else {
                comm_id = it->second;
            }
            ver.push_call(
                    comm_id,
                    env->func,
                    env->data,
                    env->src,
                    env->dest,
                    env->rtag,
                    env->stag,
                    std::vector< int >(
                        titer->get_ancestors().begin(),
                        titer->get_ancestors().end()
                        )
                    );
        }
    }
    ver.push_proc();
}

void SMTEncoding::add_condition(const std::string &ast) {
    std::string rem;
    for (char c: ast) {
        if (c == '(' || c == ')') continue;
        rem += c;
    }
    std::istringstream iss(rem);
    std::string head;
    iss >> head;
    Operator op;
    if (head == "not") {
        iss >> head;
        if (head == "=") op = Operator::not_equal;
        else if (head == "<") op = Operator::greater_or_equal;
        else if (head == ">") op = Operator::less_or_equal;
        else if (head == "<=") op = Operator::greater;
        else if (head == ">=") op = Operator::less;
        else assert(false);
    } else {
        if (head == "=") op = Operator::equal;
        else if (head == "<") op = Operator::less;
        else if (head == ">") op = Operator::greater;
        else if (head == "<=") op = Operator::less_or_equal;
        else if (head == ">=") op = Operator::greater_or_equal;
        else assert(false);
    }
    iss >> head;
    std::istringstream vss(head);
    std::string tmp;
    std::getline(vss, tmp, '_');
    assert(tmp == "R");
    std::getline(vss, tmp, '_');
    int pid = stoi(tmp);
    std::getline(vss, tmp, '_');
    int loc = stoi(tmp);
    std::getline(vss, tmp, '_');
    std::getline(vss, tmp, '_');
    std::getline(vss, tmp, '_');
    int val;
    iss >> val;
    if (tmp == "src") {
        ver.add_src_condition(pid, loc, op, val);
    } else if (tmp == "tag") {
        ver.add_tag_condition(pid, loc, op, val);
    } else if (tmp == "data") {
        ver.add_data_condition(pid, loc, op, val);
    } else {
        assert(false);
    }
}
