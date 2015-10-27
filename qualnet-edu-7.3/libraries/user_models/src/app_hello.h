#ifndef _HELLO_APP_H
#define _HELLO_APP_H

void AppHelloInit(Node *node, const NodeInput *nodeInput);
void AppHelloProcessEvent(Node *node, Message *packet);
void AppHelloFinalize(Node *node);

#endif