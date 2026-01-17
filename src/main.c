#include <stdio.h>
#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

unsigned char mem[256 * 256 * 256 + 8], *pc, *draw_addr, *audio_addr;
static int colourmap[256];
const int WIDTH = 512;
const int HEIGHT = 512;
const int SAMPLES = 256;
const int SAMPLE_RATE = 15360;
const int SAMPLE_SIZE = 8;
const int CHANNELS = 1;
const int INSTRUCTION_COUNTER = 65536;
const char* filename = "C:\\Users\\bears\\source\\bytepusher\\test\\keyboard_test.BytePusher";


void play_audio(const unsigned char* addr, AudioStream* stream) {
    while(!IsAudioStreamProcessed(*stream)){
        WaitTime(0.001f);
    }
    // Synchronization is now handled by `SetTargetFPS(60)` and Raylib's audio buffering.
    unsigned char audio_buffer[SAMPLES];
    for (int i = 0; i < SAMPLES; i++) {
        // Raylib 8-bit PCM is unsigned (0-255, 128 is center).
        // BytePusher is signed (-128 to 127).
        audio_buffer[i] = (unsigned char) ((int8_t) addr[i] + 128);
    }

    UpdateAudioStream(*stream, audio_buffer, SAMPLES);
}

void draw_video(unsigned char* addr, Texture2D* tex) {
    // Draw Pixels at addr for 64 KiB
    Color pixels[256*256];
    for (int i = 0; i < 256*256; i++){
        // The BytePusher spec uses a 6x6x6 color cube.
        // The original code was `colourmap[*addr++]` which increments addr after dereferencing.
        // This is correct for reading sequential pixel data.
        pixels[i] = GetColor(colourmap[*addr++]);
    }
    UpdateTexture(*tex, pixels);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextureEx(*tex, (Vector2){0, 0}, 0.0f, 2.0f, WHITE);
    DrawFPS(10, 10);
    EndDrawing();
}

void update_keyboard(unsigned char* mem){
    uint16_t key_matrix = 0;
    
    // Standard BytePusher Key Mapping
    // 1 2 3 4  ->  1 2 3 4
    // Q W E R  ->  5 6 7 8
    // A S D F  ->  9 A B C
    // Z X C V  ->  D E F 0
    if (IsKeyDown(KEY_ONE))   key_matrix |= 0x0002;
    if (IsKeyDown(KEY_TWO))   key_matrix |= 0x0004;
    if (IsKeyDown(KEY_THREE)) key_matrix |= 0x0008;
    if (IsKeyDown(KEY_FOUR))  key_matrix |= 0x0010;
    if (IsKeyDown(KEY_Q))     key_matrix |= 0x0020;
    if (IsKeyDown(KEY_W))     key_matrix |= 0x0040;
    if (IsKeyDown(KEY_E))     key_matrix |= 0x0080;
    if (IsKeyDown(KEY_R))     key_matrix |= 0x0100;
    if (IsKeyDown(KEY_A))     key_matrix |= 0x0200;
    if (IsKeyDown(KEY_S))     key_matrix |= 0x0400;
    if (IsKeyDown(KEY_D))     key_matrix |= 0x0800;
    if (IsKeyDown(KEY_F))     key_matrix |= 0x1000;
    if (IsKeyDown(KEY_Z))     key_matrix |= 0x2000;
    if (IsKeyDown(KEY_X))     key_matrix |= 0x4000;
    if (IsKeyDown(KEY_C))     key_matrix |= 0x8000;
    if (IsKeyDown(KEY_V))     key_matrix |= 0x0001;

    // Write state to memory address 0 and 1 (Big Endian)
    mem[0] = (key_matrix >> 8) & 0xFF;
    mem[1] = key_matrix & 0xFF;
}

void init_colour_map(){
    for (int r = 0; r < 6; r++)
        for (int g = 0; g < 6; g++)
            for (int b = 0; b < 6; b++)
                // Raylib GetColor expects 0xRRGGBBAA
                colourmap[r*36 + g*6 + b] = (r*0x33)<<24 | (g*0x33)<<16 | (b*0x33)<<8 | 0xFF;
}

void init_memory(){
    // Read memory from the file set in 'filename' constant
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    int filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    fread(mem, 1, filesize, file);
    fclose(file);
    memset(mem+filesize, 0, sizeof(mem)-filesize); // Use sizeof(mem) for total size
}

int main(void) {
    // Initial Work
    SetConfigFlags(FLAG_VSYNC_HINT);
    init_memory();
    init_colour_map();

    // AudioDevice init
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        fprintf(stderr, "Error: Audio device not ready\n");
        exit(1);
    }

    // Set a larger audio buffer to prevent buffer overruns due to timing jitter.
    SetAudioStreamBufferSizeDefault(SAMPLES * 2);

    // Window init
    InitWindow(WIDTH, HEIGHT, "BytePusher");
    if (!IsWindowReady()) {
        fprintf(stderr, "Error: Window not ready\n");
        exit(1);
    }

    // AudioStream init
    AudioStream audio_stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    PlayAudioStream(audio_stream);

    // Create a silence buffer (128 is "0" volume for 8-bit unsigned audio)
    unsigned char silence[SAMPLES];
    memset(silence, 128, SAMPLES);

    // Push 1 frame of silence to start. This primes the buffer but leaves a slot
    // open for the first frame's audio, preventing an overrun warning on the first frame.
    UpdateAudioStream(audio_stream, silence, SAMPLES);

    // Texture init - used to show pixels using GPU instead of CPU
    Image frame = GenImageColor(256, 256, BLACK);
    Texture2D tex = LoadTextureFromImage(frame);
    UnloadImage(frame);

    // Lock to 60 FPS as per BytePusher spec
    //SetTargetFPS(60);
    while(!WindowShouldClose()){
        // Outer Loop - Iterates once per frame
        // Update Input State
        update_keyboard(mem);

        // Get process counter
        // BytePusher addresses are 24-bit, stored in 3 bytes (Big Endian)
        pc = mem + ((uint32_t)mem[2]<<16 | (uint32_t)mem[3]<<8 | (uint32_t)mem[4]);

        // Go through all instructions
        for (int i = 0; i < INSTRUCTION_COUNTER; i++){
            // Instruction format: A, B, C
            // A = source address (pc[0], pc[1], pc[2])
            // B = destination address (pc[3], pc[4], pc[5])
            // C = jump address (pc[6], pc[7], pc[8])
            uint32_t source_addr = ((uint32_t)pc[0]<<16 | (uint32_t)pc[1]<<8 | (uint32_t)pc[2]);
            uint32_t dest_addr = ((uint32_t)pc[3]<<16 | (uint32_t)pc[4]<<8 | (uint32_t)pc[5]);
            uint32_t jump_addr = ((uint32_t)pc[6]<<16 | (uint32_t)pc[7]<<8 | (uint32_t)pc[8]);

            mem[dest_addr] = mem[source_addr];
            pc = mem + jump_addr;
        }
        // Audio address is 2 bytes (MSB, MID) at mem[6], mem[7]
        audio_addr = mem + ((uint32_t)mem[6]<<16 | (uint32_t)mem[7]<<8);
        play_audio(audio_addr, &audio_stream);

        // Draw address is 1 byte (MSB) at mem[5]
        draw_addr = mem + ((uint32_t)mem[5]<<16);
        draw_video(draw_addr, &tex);
    }

    // Cleanup
    UnloadTexture(tex);
    UnloadAudioStream(audio_stream);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
