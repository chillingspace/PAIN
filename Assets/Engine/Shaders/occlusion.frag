// occlusion.frag
// Discard all fragments — we only care about the depth test pass count.

#version 330 core

void main() {
    // Nothing to output; color writes are disabled during occlusion query pass.
}
