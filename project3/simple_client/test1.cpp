#include <iostream>
#include <pthread.h>
#include <assert.h>

#include "HttpClient.h"

using namespace std;

void concurrent_test(int n);
void consequetive_test();
void *handle_thread(void *arg);

int PORT = 8080;

int main(int argc, char *argv[]) {
  //consequetive_test();
  concurrent_test(4);
  
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
  for (int i = 0; i < n; i++) {
    pthread_t thread1;
    pthread_t thread2;
    pthread_create(&thread1, NULL, handle_thread, (void *) "/bootstrap.html");
    pthread_create(&thread2, NULL, handle_thread, (void *) "/hello_world.html");
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
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