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
const char* filename = "C:\\Users\\bears\\source\\bytepusher\\test\\audio_test.BytePusher";


void play_audio(const unsigned char* addr, AudioStream* stream){
    // TODO incomplete
    // Queue Audio at the addr for SAMPLES bytes
    unsigned char audio_buffer[256];
    for (int i = 0; i < SAMPLES; i++){
        audio_buffer[i] = addr[i];
    }
    if (IsAudioStreamProcessed(*stream)){
        UpdateAudioStream(*stream, audio_buffer, SAMPLES);
    }
}

void draw_video(unsigned char* addr, Texture2D* tex) {
    // Draw Pixels at addr for 64 KiB
    // FIXME colours are a bit wack - pixels are working tho lol
    Color pixels[256*256];
    for (int i = 0; i < 256*256; i++){
        pixels[i] = GetColor(colourmap[*addr++]);
    }
    UpdateTexture(*tex, pixels);

    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextureEx(*tex, (Vector2){0, 0}, 0.0f, 2.0f, WHITE);
    EndDrawing();
}

void handle_keybits(uint16_t addr){
    // TODO implement this
}

void init_colour_map(){
    for (int r = 0; r < 6; r++)
        for (int g = 0; g < 6; g++)
            for (int b = 0; b < 6; b++)
                colourmap[r*36 + g*6 + b] = r*0x33<<16 | g*0x33<<8 | b*0x33;
}

void init_memory(){
    // Read memory from the file set in 'filename' constant
    FILE* file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    int filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    fread(mem, 1, filesize, file);
    fclose(file);
    memset(mem+filesize, 0, 0x1000000-filesize);
}

int main(void) {
    // Initial Work
    init_memory();
    init_colour_map();

    // AudioDevice init
    InitAudioDevice();
    if (!IsAudioDeviceReady()) exit(0);

    // Window init
    InitWindow(WIDTH, HEIGHT, "BytePusher");
    if (!IsWindowReady()) exit(0);

    // AudioStream init
    SetAudioStreamBufferSizeDefault(SAMPLES);
    AudioStream audio_stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    PlayAudioStream(audio_stream);

    // Texture init - used to show pixels using GPU instead of CPU
    Image frame = GenImageColor(256, 256, BLACK);
    Texture2D tex = LoadTextureFromImage(frame);
    UnloadImage(frame);

    // Lock to 60 FPS as per BytePusher spec
    SetTargetFPS(60);
    while(!WindowShouldClose()){
        // Outer Loop - Iterates once per frame
        // Key State addr
        uint16_t key = (mem[0] << 8 | mem[1]);
        handle_keybits(key);

        // Get pc
        pc = mem + (mem[2]<<16 | mem[3]<<8 | mem[4]);
        // Go through all instructions
        for (int i = 0; i < INSTRUCTION_COUNTER; i++){
            mem[pc[3]<<16 | pc[4]<<8 | pc[5]] =
                    mem[pc[0]<<16 | pc[1]<<8 | pc[2]];
            pc = mem + (pc[6]<<16 | pc[7]<<8 | pc[8]);
        }

        // Draw
        draw_addr = mem + (mem[5]<<16);
        draw_video(draw_addr, &tex);

        // Play
        audio_addr = mem + (mem[6]<<16 | mem[7]<<8);
        play_audio(audio_addr, &audio_stream);
    }

    // Cleanup
    UnloadTexture(tex);
    UnloadAudioStream(audio_stream);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
