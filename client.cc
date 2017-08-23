#include <iostream>
#include <string>
#include <fstream>
#include <queue>
#include <thread>

#include <mutex>
#include <condition_variable>

#include <SFML/Audio.hpp>
#include <zmqpp/zmqpp.hpp>

using namespace std;
using namespace zmqpp;
using namespace sf;

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

void messageToFile(const message & msg, const string &fileName) {
	const void *data;
	msg.get(&data, 1);
	size_t size = msg.size(1);

	ofstream ofs(fileName, ios::binary);
	ofs.write((char *)data, size);
	return;
}

void music_queue(Music &music, SafeQueue<string> &Q) {
	context ctx;
	socket s(ctx, socket_type::req);
	// cout << "Connecting to tcp port 5555\n";
	s.connect("tcp://localhost:5555");

	while (true) {
		string songName = Q.dequeue(), state;
		message m, answer;
		m << "song";
		m << songName;
		s.send(m);
		s.receive(answer);
		answer >> state;
		if (state == "file") {
			messageToFile(answer, songName + ".ogg");
			music.openFromFile(songName + ".ogg");
			music.play();
			cout << "playing: " << songName << endl;
			while (music.getStatus() == SoundSource::Playing) {}
		} else {
			cout << "Didn't get a valid file" << endl;
		}

	}
}

void server_interaction(Music &music, SafeQueue<string> &Q) {
	context ctx;
	socket s(ctx, socket_type::req);
	// cout << "Connecting to tcp port 5555\n";
	s.connect("tcp://localhost:5555");

	while (true) {
		message m, answer;
		string operation, song, result;

		cin >> operation;

		if (operation == "next") {
			music.stop();
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
			cout << "new song added " << newsong << endl;
			Q.enqueue(newmusic);
		}else {
			cout << "Don't know what to do :D" << endl;
		}
	}
}

int main(int argc, char** argv) {
	Music music;
	SafeQueue<string> Q;

	thread interact(&server_interaction, ref(music), ref(Q));
	thread playlist(&music_queue, ref(music), ref(Q));
	interact.join();
	playlist.join();
	return 0;
}


/*
thread music play
hay un problema con los while que validan las condiciones
ya que gastan muchos recursos
otro problema es la cola que es compartida en 2 hilos (
	los dos hilos pueden modificar la cola!
	race condition - semaforo

	disenar cola segura para concurrencia
		mutex = semaforo (investigar)
		cola
		variable de condicion = ?
)
while true
	while p.empty
	song = p.front
	descargar song
	music.load
	music.play
	while music.playing

poller zmq?
	poller p
	p.add(s, poller::poll_in)
	int stdinput = fileno(stdin)
	p.add(stdinput, poller::poll_in)
	while p.poll()
		if p.has_input(s)
			message m
			s.receive(m)
		if p.has_input(stdinput)
			normal interaction
*/
