#include <iostream>
#include <string>
#include <fstream>
#include <deque>

#include <SFML/Audio.hpp>
#include <zmqpp/zmqpp.hpp>

using namespace std;
using namespace zmqpp;
using namespace sf;

void messageToFile(const message & msg, const string &fileName) {
	const void *data;
	msg.get(&data, 1);
	size_t size = msg.size(1);

	ofstream ofs(fileName, ios::binary);
	ofs.write((char *)data, size);
	return;
}

int main(int argc, char** argv) {
	cout << "This is the client!!\n";

	context ctx;
	socket s(ctx, socket_type::req);

	sf::Music music;

	cout << "Connecting to tcp port 5555\n";
	s.connect("tcp://localhost:5555");

	cout << "Sending  some work!\n";
	while (true) {
		message m;
		string operation, song; cin >> operation;
		m << operation;
		if (operation == "play") {
			cin >> song;
			m << song;
		}

		s.send(m);



		message answer;
		s.receive(answer);

		string result;
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

		} else if(result == "file") {
			string newmusic = "new.ogg";
			messageToFile(answer, newmusic);
			music.openFromFile(newmusic);
			music.play();

			while (music.getStatus() == SoundSource::Playing) {
				cout << "is playing" << endl;
			}

		}else {
			cout << "Don't know what to do :D" << endl;
		}

	}
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
*/
