#include <zmqpp/zmqpp.hpp>
#include <iostream>
#include <string>
#include <cassert>
#include <unordered_map>
#include <fstream>
#include <glob.h>

using namespace std;
using namespace zmqpp;

vector<char> readFileToBytes(const string& fileName) {
	ifstream ifs(fileName, ios::binary | ios::ate);
	ifstream::pos_type pos = ifs.tellg();

	vector<char> result(pos);

	ifs.seekg(0, ios::beg);
	ifs.read(result.data(), pos);

	return result;
}

void fileToMesage(const string& fileName, message& msg) {
	vector<char> bytes = readFileToBytes(fileName);
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
    if (op == "list") {  // Use case 1: Send the songs
      message n;
      n << "list" << songs.size();
      for(const auto& p : songs)
        n << p.first;
      s.send(n);
    } else if(op == "play") {
      // Use case 2: Send song file
      string songName;
      m >> songName;
      cout << "sending song " << songName
           << " at " << songs[songName] << endl;
			message n;
			n << "file";
			cout << songs.count(songName) << endl;
			fileToMesage(songs[songName], n);
			s.send(n);
    } else {
      cout << "Invalid operation requested!!\n";
    }
  }

  cout << "Finished\n";
  return 0;
}
