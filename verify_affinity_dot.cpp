// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Author: hanhanW

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <optional>
#include <queue>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <vector>

using namespace std;

constexpr int MAXN = 1048576;
constexpr int kUnknown = -1;

int N, M, nColor;
int fromInit[MAXN], from[MAXN];
int color[MAXN], isInit[MAXN], requirement[MAXN];
bool skip[MAXN];
bool enableLog = false;
std::vector<int> edge[MAXN];
std::unordered_map<std::string, int> strToIdx;
std::unordered_map<int, std::string> idxToStr;
std::unordered_map<std::string, int> deviceToColor;

bool containStr(string str, string target) {
  return str.find(target) != std::string::npos;
}

int getColor(string str, string target) {
  if (!containStr(str, target)) {
    return kUnknown;
  }
  string magic = "#hal.device.affinity<";
  size_t pos = str.find(magic);
  if (pos == std::string::npos) {
    assert(false);
  }
  string device;
  for (size_t i = pos + magic.size(); i < str.size(); ++i) {
    if (str[i] == '>') {
      break;
    }
    device += str[i];
  }

  if (deviceToColor.find(device) == deviceToColor.end()) {
    deviceToColor[device] = nColor++;
  }
  return deviceToColor[device];
}

void dumpPath(int dest, std::optional<int> lastNode = std::nullopt) {
  std::vector<int> path;
  int u = dest;
  path.push_back(u);
  while (!isInit[u]) {
    u = from[u];
    path.push_back(u);
  }
  std::reverse(path.begin(), path.end());
  for (int i = 0, e = path.size(); i < e - 1; ++i) {
    std::cout << idxToStr[path[i]] << " -> ";
  }
  std::cout << idxToStr[path.back()];
  if (lastNode) {
    std::cout << " -> " << idxToStr[lastNode.value()];
  }
  std::cout << std::endl;
}

void parseNode(string input, int i) {
  assert(i < MAXN &&
         "hit the limitation of the number of nodes, please consider increase "
         "MAXN value");
  stringstream ss;
  ss << input;
  string str;
  ss >> str;
  strToIdx[str] = i;
  idxToStr[i] = str;
  if (enableLog) {
    std::cout << str << std::endl;
  }

  bool found = 0;
  while (ss >> str) {
    if (str == "label") {
      found = 1;
      break;
    }
  }
  assert(found);
  getline(ss, str);
  const char* kFlowTransferStr = "flow.tensor.transfer";
  const char* kFlowBarrierStr = "flow.tensor.barrier";
  color[i] = getColor(str, kFlowTransferStr);
  if (containStr(str, kFlowBarrierStr)) {
    requirement[i] = getColor(str, kFlowBarrierStr);
  }
  if (containStr(str, "hal.tensor.barrier")) {
    skip[i] = true;
    if (enableLog) {
      std::cout << "Mark node " << i << " skip" << std::endl;
    }
  }

  if (enableLog) {
    std::cout << i << ": " << color[i] << std::endl;
    if (requirement[i] != kUnknown) {
      std::cout << "requirement: " << requirement[i] << std::endl;
    }
  }
}

void parseEdge(string input) {
  stringstream ss;
  ss << input;
  string ustr, sep, vstr;
  ss >> ustr >> sep >> vstr;

  if (strToIdx.find(ustr) == strToIdx.end()) {
    std::cout << "GG u_i: " << ustr << "\n";
    assert(false);
  }
  if (strToIdx.find(vstr) == strToIdx.end()) {
    std::cout << "GG v_i: " << vstr << "\n";
    assert(false);
  }

  int u = strToIdx[ustr];
  int v = strToIdx[vstr];
  edge[u].push_back(v);
}

string trimSpaces(string str) {
  string res = "";
  for (auto c : str) {
    if (c != ' ') {
      res += c;
    }
  }
  return res;
}

int main(int argc, char* argv[]) {
  if (argc != 1 && argc != 2) {
    std::cerr << "Usage: " << argv[0] << " [any_string_to_enable_log]"
              << std::endl;
    return 1;
  }

  if (argc == 2) {
    enableLog = true;
  } else {
    enableLog = false;
  }

  std::fill(color, color + MAXN, kUnknown);
  std::fill(requirement, requirement + MAXN, kUnknown);
  std::fill(fromInit, fromInit + MAXN, kUnknown);
  std::fill(isInit, isInit + MAXN, false);
  std::fill(skip, skip + MAXN, false);
  nColor = 0;
  N = 0;
  M = 0;
  bool inSubGraph = false;
  string input;
  while (getline(cin, input)) {
    if (containStr(input, "digraph")) {
      continue;
    }
    // We don't care the last `}` for diagraph.
    if (trimSpaces(input) == "}") {
      inSubGraph = false;
      continue;
    }
    if (containStr(input, "subgraph")) {
      inSubGraph = true;
      continue;
    }

    stringstream ss;
    ss << input;
    // All the nodes starts with 'v'.
    string str;
    ss >> str;
    if (str[0] != 'v') {
      continue;
    }
    if (inSubGraph) {
      parseNode(input, N++);
    } else {
      M++;
      parseEdge(input);
    }
  }
  std::cout << "There are " << N << " nodes and " << M << " edges in total"
            << endl;

  std::queue<int> que;
  for (int i = 0; i < N; ++i) {
    if (color[i] != kUnknown) {
      isInit[i] = true;
      fromInit[i] = i;
      from[i] = i;
      que.push(i);
    }
  }

  while (!que.empty()) {
    int u = que.front();
    que.pop();
    for (auto v : edge[u]) {
      if (skip[v]) {
        continue;
      }
      if (color[v] == kUnknown) {
        color[v] = color[u];
        fromInit[v] = fromInit[u];
        from[v] = u;
        que.push(v);

        if (requirement[v] != kUnknown && requirement[v] != color[u]) {
          std::cout << "does not meet the requirement\n";
          std::cout << "u: " << idxToStr[u] << " fromInit "
                    << idxToStr[fromInit[u]] << "\n";
          std::cout << "v: " << idxToStr[v] << " fromInit "
                    << idxToStr[fromInit[v]] << "\n";
          dumpPath(u);
          dumpPath(v);
          return 1;
        }
      } else if (!isInit[v] && color[v] != color[u]) {
        std::cout << "Mismatch color @ " << idxToStr[v] << "!\n";
        std::cout << "Node " << idxToStr[u] << " has color " << color[u]
                  << std::endl;
        std::cout << "Node " << idxToStr[v] << " already has color " << color[v]
                  << std::endl;
        dumpPath(u, v);
        dumpPath(v);
        return 1;
      }
    }
  }
  std::cout << "input program may be valid\n";

  return 0;
}
