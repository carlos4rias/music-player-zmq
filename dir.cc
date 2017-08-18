#include <string>
#include <iostream>
#include <vector>
#include <glob.h>

using namespace std;

vector<string> globVector(const string& pattern){
    glob_t glob_result;
    glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result);
    vector<string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        files.push_back(string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    for (int i = 0; i < files.size(); i++) cout << files[i] << endl;
    return files;
}

int main (int argc, char ** argv) {
  string dir(argv[1]);
  globVector(dir + "/*.ogg");
}
