#include "video.cpp"
#include "audio.h"

using namespace std;
using namespace cv;

double out_freq = 500.0;
double out_ampl = 1.0;
double out_phase = 0.0;

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    //paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    printf("sending %ld, %f %f\n", framesPerBuffer, timeInfo -> currentTime, timeInfo -> outputBufferDacTime);
    for( i=0; i<framesPerBuffer; i++ )
    {
        out_phase += out_freq * M_PI * 2.0 / SAMPLE_RATE;
        double data = sin(out_phase) * out_ampl;
        *out++ = data;
        *out++ = data;
    }

    return paContinue;
}

static void StreamFinished( void* userData )
{
   paTestData *data = (paTestData *) userData;
   printf( "Stream Completed: %s\n", data->message );
}

int main( void )
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;

    const int numbg = 30;
    VideoCapture capture;
    Mat frame;
    Mat bg[numbg] = {};
    Mat background;
    int i = 0;

    //-- 2. Read the video stream
    capture.open( -1 );
    if ( ! capture.isOpened() ) { printf("--(!)Error opening video capture\n"); return -1; }

    while ( capture.read(frame) && i < numbg)
    {
        if( frame.empty() )
        {
            printf(" --(!) No captured frame -- Break!");
            break;
        }
        bg[i] = frame;
        i++;
    }
    //bg_temp = bg[0];

    int rc = bg[0].rows * bg[0].cols;
    double* bg_avg_r = (double*)calloc(rc, sizeof(double));
    double* bg_avg_g = (double*)calloc(rc, sizeof(double));
    double* bg_avg_b = (double*)calloc(rc, sizeof(double));
    for(i = 0; i < numbg; i ++) {
      double* bg_ir = read_channel_from_mat(bg[i], 2);
      double* bg_ig = read_channel_from_mat(bg[i], 1);
      double* bg_ib = read_channel_from_mat(bg[i], 0);
      for(int j = 0; j < rc; j ++) {
        bg_avg_r[j] += bg_ir[j];
        bg_avg_g[j] += bg_ig[j];
        bg_avg_b[j] += bg_ib[j];
      }
      free(bg_ib);
      free(bg_ig);
      free(bg_ir);
    }

    for(int j = 0; j < rc; j ++) {
      bg_avg_r[j] /= numbg;
      bg_avg_g[j] /= numbg;
      bg_avg_b[j] /= numbg;
    }
    background = bg[0];
    read_mat_from_channels(background, bg_avg_r, bg_avg_g, bg_avg_b);
    free(bg_avg_r);
    free(bg_avg_g);
    free(bg_avg_b);

    Mat bgblur;
    GaussianBlur(background, bgblur, Size(11,11), 3, 3);
    imwrite("bg.png", bgblur);

    // printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL,
              &outputParameters,
              SAMPLE_RATE,
              buffersize,
              paClipOff,
              patestCallback,
              &data );
    if( err != paNoError ) goto error;

    sprintf( data.message, "No Message" );
    err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    goto noerror;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;

noerror:

    int count = 0;
    while( capture.read(frame)) {
        const clock_t begin = clock();
        detectAndDisplay( frame, bgblur, out_freq);
        cout << float(clock() - begin) / CLOCKS_PER_SEC << endl;
        int c = waitKey(10);

        count ++;
        if(count == 100)
          buffersize = 5000;
        if( (char)c == 27 ) { break; } // escape
    }

/*
    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    Pa_Terminate();*/
    printf("Test finished.\n");
    return 0;
}
