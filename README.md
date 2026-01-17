# BytePusher in C
### developed by owitrz
This file contains my attempt at a BytePusher implementation in C. 

A BytePusher is a specification created by Javamannen in 2010 that is effectively a simplified CHIP-8 emulator. It is a virtual machine that utilises the ByteByteJump One Instruction Set Computer architecture, where each instruction is structure A,B,C. Where A (the source address) is moved to B (the destination address) before jumping to C (the jump address)

There are a few components to a complete BytePusher implementation. I have successfully implemented:
- Video (256x256)
- Key Callback (16 Keys)
- Colours (216 fixed colours)
- Audio (partly) - I gave up on a weird bug haha


Important Links:
- https://github.com/raysan5/raylib/wiki/raylib-architecture
  - Raylib is a popular game programming library written for C and C++. I have chosen this because I wanted to learn it, it is meant to be relatively simple and easy to get going, and it able to support mono channel audio streams and easy drawing to the screen.
- https://esolangs.org/wiki/BytePusher
  - This is the esolangs specification for a BytePusher VM.