#define TABLE_SIZE   (200)
#define SAMPLE_RATE   (44100)
#define NUM_SECONDS 5

int buffersize = 30000;

#ifndef M_PI
#define M_PI  (3.14159265)
#endif
typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
    char message[20];
}
paTestData;

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData );
static void StreamFinished( void* userData );
