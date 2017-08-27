#include <zmqpp/zmqpp.hpp>
#include <iostream>
#include <string>
#include <cassert>
#include <unordered_map>
#include <fstream>
#include <glob.h>
#include <cmath>

using namespace std;
using namespace zmqpp;

int get_file_parts(const string& fileName, int chunk = 512) {
  ifstream ifs(fileName, ios::binary | ios::ate);
  ifstream::pos_type pos = ifs.tellg();
  return ceil((double)pos / (chunk * 1024));
}

vector<char> readFileToBytes(const string& fileName, int part, int chunk, int hw) {
  ifstream ifs(fileName, ios::binary | ios::ate);
  int tsz = ifs.tellg();
  int kb = 1024;

  ifs.seekg(part * kb * chunk, ifs.beg);
  vector<char> result;
  if (part + 1 == hw) {
    result.resize(tsz - kb * part * chunk);
  }else {
    result.resize(chunk * kb);
  }
  ifs.read(result.data(), chunk * kb);

  return result;
}

void fileToMesage(const string& fileName, message& msg, int part, int hw) {
  vector<char> bytes = readFileToBytes(fileName, part, 512, hw);
  msg.add_raw(bytes.data(), bytes.size());
}

string get_song_name(const string &name ) {
  string cname = name.substr(name.find("/", 2) + 1);
  int sz = (int)cname.size();
  return cname.substr(0, sz - 4);
}

void getSongsList(unordered_map<string, string> &songs, const string& pattern){
  glob_t glob_result;
  glob(pattern.c_str(), GLOB_TILDE,NULL, &glob_result);

  for(unsigned int i = 0; i < glob_result.gl_pathc; ++i){
    string sname = get_song_name(glob_result.gl_pathv[i]);
    songs[sname] = string(glob_result.gl_pathv[i]);
  }
  globfree(&glob_result);
}

int main(int argc, char** argv) {
  context ctx;
  socket s(ctx, socket_type::rep);
  s.bind("tcp://*:5555");

  string path(argv[1]);
  unordered_map<string,string> songs;
  getSongsList(songs, path + "/*.ogg");

  cout << "Start serving requests!\n";
  while(true) {
    message m;
    s.receive(m);

    string op;
    m >> op;

    cout << "Action:  " << op << endl;
    if (op == "list") {
      message n;
      n << "list" << songs.size();
      for(const auto& p : songs)
        n << p.first;
      s.send(n);
    } else if (op == "init_song") {
      string songName;
      int part, hw;
      m >> songName >> part >> hw;
      cout << "sending song " << songName
           << " at " << songs[songName] << " part :" << part << endl;
      message n;
      n << "init";
      fileToMesage(songs[songName], n, part, hw);
      s.send(n);
    } else if (op == "get_part") {
      string songName;
      int part, hw;
      m >> songName >> part >> hw;
      cout << "sending song " << songName
           << " at " << songs[songName] << " part :" << part << endl;
      message n;
      n << "chunk";
      fileToMesage(songs[songName], n, part, hw);
      s.send(n);
    } else if (op == "play") {
      string songName;
      m >> songName;

      message n;
      if (songs.count(songName)) {
        n << "song";
        n << get_file_parts(songs[songName]);
      } else {
        n << "error";
      }

      s.send(n);
      } else {
        cout << "Invalid operation requested!!\n";
      }
  }
  cout << "Finished\n";
  return 0;
}
