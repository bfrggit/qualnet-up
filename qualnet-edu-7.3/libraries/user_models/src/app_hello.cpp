#include <stdio.h>
#include "api.h"
#include "app_hello.h"

void AppHelloInit(Node* node, const NodeInput *nodeInput) {
	printf("Application layer Hello initialized.\n");
}

void AppHelloProcessEvent(Node *node, Message *packet) {
	printf("Application layer Hello processed an event.\n");
}

void AppHelloFinalize(Node *node) {
	printf("Application layer Hello finalized.\n");
}
