#include <iostream>
#include <pthread.h>
#include <assert.h>
#include <vector>

#include "HttpClient.h"

using namespace std;

void concurrent_test(int n);
void consequetive_test();
void slow_test();
void *handle_thread(void *arg);

int PORT = 8080;

int main(int argc, char *argv[]) {
  // consequetive_test();
  concurrent_test(16);
  // slow_test();

  return 0;
}

void *handle_thread(void *arg) {
  string filename = (char *) arg;

  HttpClient client("localhost", PORT);
  HTTPResponse *response = client.get(filename);
  assert(response->status() == 200);
  delete response;
  return NULL;
}

void concurrent_test(int n) {
  cout << n << " threads test" << endl;
  vector<pthread_t> threads;
  for (int i = 0; i < n; i++) {
    pthread_t thread1;
    pthread_create(&thread1, NULL, handle_thread, (void *) "/slow.html");
    threads.push_back(thread1);
  }
  for (size_t i = 0; i < threads.size(); i++) {
    pthread_join(threads[i], NULL);
  }
  cout << "passed" << endl;
}

void consequetive_test() {
  cout << "consequetive test" << endl;
  for (int i = 0; i < 100; i++) {
    HttpClient client("localhost", PORT);
    HTTPResponse *response = client.get("/hello_world.html");
    assert(response->status() == 200);
    delete response;
  }
  cout << "passed" << endl;
}

void slow_test() {
  cout << "slow test" << endl;
  vector<pthread_t> threads;
  for (int i = 0; i < 4; i++) {
    pthread_t thread1;
    pthread_create(&thread1, NULL, handle_thread, (void *) "/slow.html");
    threads.push_back(thread1);
  }
  for (int i = 0; i < 4; i++) {
    pthread_t thread1;
    pthread_create(&thread1, NULL, handle_thread, (void *) "/hello_world.html");
    threads.push_back(thread1);
  }
  for (size_t i = 0; i < threads.size(); i++) {
    pthread_join(threads[i], NULL);
  }
  cout << "passed" << endl;
}