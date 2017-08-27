#include <iostream>
#include <string>
#include <fstream>
#include <queue>
#include <thread>
#include <stdlib.h>

#include <mutex>
#include <condition_variable>

#include <SFML/Audio.hpp>
#include <zmqpp/zmqpp.hpp>

using namespace std;
using namespace zmqpp;
using namespace sf;


class data{
public:
	string name;
	int n_parts;
	data () {}
	data (string x, int np) : name(x), n_parts(np) {}
};

typedef data data;

#ifndef SAFE_QUEUE
#define SAFE_QUEUE

// A threadsafe-queue.
template <class T>
class SafeQueue {
public:
	SafeQueue(void)
		: q()
		, m()
		, c()
	{}

	~SafeQueue(void)
	{}

	// Add an element to the queue.
	void enqueue(T t) {
		std::lock_guard<std::mutex> lock(m);
		q.push(t);
		c.notify_one();
	}

	int size() {
		return  q.size();
	}

	bool empty() {
		return q.empty();
	}

	// Get the "front"-element.
	// If the queue is empty, wait till a element is avaiable.
	T dequeue(void) {
		std::unique_lock<std::mutex> lock(m);
		while(q.empty()) {
			// release lock as long as the wait and reaquire it afterwards.
			c.wait(lock);
		}
		T val = q.front();
		q.pop();
		return val;
	}

private:
	std::queue<T> q;
	mutable std::mutex m;
	std::condition_variable c;
};
#endif

void messageToFile(const message & msg, const string &fileName, bool append) {
	const void *data;
	msg.get(&data, 1);

	size_t size = msg.size(1);

  if (append) {
    ofstream ofs(fileName, ios::app);
    ofs.write((char *)data, size);
  }else {
    ofstream ofs(fileName, ios::binary);
    ofs.write((char *)data, size);
  }
  return;
}

void music_queue(Music &music, SafeQueue<data> &Q) {
	context ctx;
	socket s(ctx, socket_type::req);
	s.connect("tcp://localhost:5555");

	while (true) {
		string  state, songName;
		data song = Q.dequeue();
		songName = song.name;
		int cur_p = 0;

		message m, answer;
		m << "init_song";
		m << songName << cur_p << song.n_parts;

		s.send(m);
		s.receive(answer);
		answer >> state;

		if (state == "init") {
			messageToFile(answer, songName + ".ogg", cur_p);
			music.openFromFile(songName + ".ogg");
			music.play();
			double ltime = music.getPlayingOffset().asSeconds();
			cout << "playing: " << songName << endl;
			cur_p++;
		  while (cur_p < song.n_parts && music.getStatus()) {
				sf::sleep(sf::milliseconds(1000));
				double cur_time = music.getPlayingOffset().asSeconds();
				if (cur_time - ltime >= 20.0) {
					message mm, ans;
					mm << "get_part";
					mm << songName << cur_p << song.n_parts;
					s.send(mm);
					s.receive(ans);
					ans >> state;
					cout << "getting the part: " << cur_p << endl;
					if (state == "chunk") {
						messageToFile(ans, songName + ".ogg", cur_p > 0);
						cur_p++;
					} else {
						cout << "Didn't get the part " << cur_p << endl;
					}
					ltime = cur_time;
				}
		  }
			while (music.getStatus()) {
				sf::sleep(sf::milliseconds(1000));
			}
		} else {
			cout << "Didn't get a valid file" << endl;
		}

	}
}

void server_interaction(Music &music, SafeQueue<data> &Q) {
	context ctx;
	socket s(ctx, socket_type::req);

	s.connect("tcp://localhost:5555");
	cout << "options :" << endl;
	cout << "\tlist - get the songs list available on the server." << endl;
	cout << "\tplay #songname - add the songname song to the playlist if it's available." << endl;
	cout << "\tpause - pause the current song." << endl;
	cout << "\tresume - resume the current song" << endl;
	cout << "\tnext - play the next song in the queue if there is one." << endl;

	while (true) {
		message m, answer;
		string operation, song, result;

		cin >> operation;

		if (operation == "next") {
			music.stop();
			cout << music.getStatus() << endl;
			continue;
		} else if (operation == "pause") {
			music.pause();
			continue;
		} else if (operation == "resume") {
			music.play();
			continue;
		}

		m << operation;
		if (operation == "play") {
			cin >> song;
			m << song;
		}

		s.send(m);
		s.receive(answer);

		answer >> result;
		if (result == "list") {
			size_t numSongs;
			answer >> numSongs;
			cout << "Available songs: " << numSongs << endl;
			for(int i = 0; i < (int)numSongs; i++) {
				string s;
				answer >> s;
				cout << s << endl;
			}
		} else if(result == "song") {
			string newsong = song;
			int nparts; answer >> nparts;
			cout << "new song added " << newsong << " #parts: " << nparts << endl;
			Q.enqueue(data(newsong, nparts));
		}else {
			cout << "Don't know what to do :D" << endl;
		}
	}
}


int main(int argc, char** argv) {
	Music music;
	SafeQueue<data> Q;

	thread interact(&server_interaction, ref(music), ref(Q));
	thread playlist(&music_queue, ref(music), ref(Q));
	interact.join();
	playlist.join();
	return 0;
}
