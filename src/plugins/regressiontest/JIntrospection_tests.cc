

#include "JIntrospection.h"

int main() {

    JEvent event;
    JIntrospection sut(&event);
    auto result = sut.DoReplLoop(22);
    return result;
}

