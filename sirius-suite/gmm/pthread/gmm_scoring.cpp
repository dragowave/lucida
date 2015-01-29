#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <assert.h>

#include <pthread.h>

int NTHREADS;
int iterations;

float feature_vect[] = {2.240018,    2.2570236,    0.11304555,   -0.21307051,
                        0.8988138,   0.039065503,  0.023874786,  0.13153112,
                        0.15324382,  0.16986738,   -0.020297153, -0.26773554,
                        0.40202165,  0.35923952,   0.060746543,  0.35402644,
                        0.086052455, -0.10499257,  0.04395058,   0.026407119,
                        -0.48301497, 0.120889395,  0.67980754,   -0.19875681,
                        -0.5443737,  -0.039534688, 0.20888293,   0.054865785,
                        -0.4846478,  0.1,          0.1,          0.1};

float *means_vect;
float *precs_vect;
float *weight_vect;
float *factor_vect;

float *cpu_score_vect;
float *pthread_score_vect;

float logZero = -3.4028235E38;

float logBase = 1.0001;

float maxLogValue = 7097004.5;
float minLogValue = -7443538.0;

float naturalLogBase = (float)1.00011595E-4;
float inverseNaturalLogBase = 9998.841;

int comp_size = 32;
int feat_size = 29;
int senone_size = 5120;

void computeScore_seq(float *feature_vect, float *means_vect, float *precs_vect,
                      float *weight_vect, float *factor_vect,
                      float *cpu_score_vect) {

  float logZero = -3.4028235E38;
  float maxLogValue = 7097004.5;
  float minLogValue = -7443538.0;
  float naturalLogBase = (float)1.00011595E-4;
  float inverseNaturalLogBase = 9998.841;

  int comp_size = 32;
  int feat_size = 29;
  int senone_size = 5120;

  for (int i = 0; i < senone_size; i++) {
    cpu_score_vect[i] = logZero;

    for (int j = 0; j < comp_size; j++) {
      float logDval = 0.0f;

      for (int k = 0; k < feat_size; k++) {
        int idx = k + comp_size * j + i * comp_size * comp_size;
        float logDiff = feature_vect[k] - means_vect[idx];
        logDval += logDiff * logDiff * precs_vect[idx];
      }

      // Convert to the appropriate base.
      if (logDval != logZero) {
        logDval = logDval * inverseNaturalLogBase;
      }

      int idx2 = j + i * comp_size;
      logDval -= factor_vect[idx2];

      if (logDval < logZero) {
        logDval = logZero;
      }

      float logVal2 = logDval + weight_vect[idx2];

      float logHighestValue = cpu_score_vect[i];
      float logDifference = cpu_score_vect[i] - logVal2;

      // difference is always a positive number
      if (logDifference < 0) {
        logHighestValue = logVal2;
        logDifference = -logDifference;
      }

      float logValue = -logDifference;
      float logInnerSummation;
      if (logValue < minLogValue) {
        logInnerSummation = 0.0;
      } else if (logValue > maxLogValue) {
        logInnerSummation = FLT_MAX;

      } else {
        if (logValue == logZero) {
          logValue = logZero;
        } else {
          logValue = logValue * naturalLogBase;
        }
        logInnerSummation = exp(logValue);
      }

      logInnerSummation += 1.0;

      float returnLogValue;
      if (logInnerSummation <= 0.0) {
        returnLogValue = logZero;

      } else {
        returnLogValue =
            (float)(log(logInnerSummation) * inverseNaturalLogBase);
        if (returnLogValue > FLT_MAX) {
          returnLogValue = FLT_MAX;
        } else if (returnLogValue < -FLT_MAX) {
          returnLogValue = -FLT_MAX;
        }
      }
      // sum log
      cpu_score_vect[i] = logHighestValue + returnLogValue;
    }
  }
}

void *computeScore_thread(void *tid) {
  float logZero = -3.4028235E38;

  float logBase = 1.0001;

  float maxLogValue = 7097004.5;
  float minLogValue = -7443538.0;

  float naturalLogBase = (float)1.00011595E-4;
  float inverseNaturalLogBase = 9998.841;

  int comp_size = 32;
  int feat_size = 29;
  int senone_size = 5120;

  int i, start=0, *mytid, end=0;

  mytid = (int *)tid;
  start = (*mytid * iterations);
  end = start + iterations;

  for (i = start; i < end; i++) {
    pthread_score_vect[i] = logZero;

    for (int j = 0; j < comp_size; j++) {
      // getScore
      float logDval = 0.0f;
      for (int k = 0; k < feat_size; k++) {
        int idx = k + comp_size * j + i * comp_size * comp_size;
        float logDiff = feature_vect[k] - means_vect[idx];
        logDval += logDiff * logDiff * precs_vect[idx];
      }

      // Convert to the appropriate base.
      if (logDval != logZero) {
        logDval = logDval * inverseNaturalLogBase;
      }

      int idx2 = j + i * comp_size;

      // Add the precomputed factor, with the appropriate sign.
      logDval -= factor_vect[idx2];

      if (logDval < logZero) {
        logDval = logZero;
      }
      // end of getScore

      float logVal2 = logDval + weight_vect[idx2];

      float logHighestValue = pthread_score_vect[i];
      float logDifference = pthread_score_vect[i] - logVal2;

      // difference is always a positive number
      if (logDifference < 0) {
        logHighestValue = logVal2;
        logDifference = -logDifference;
      }

      float logValue = -logDifference;
      float logInnerSummation;
      if (logValue < minLogValue) {
        logInnerSummation = 0.0;
      } else if (logValue > maxLogValue) {
        logInnerSummation = FLT_MAX;

      } else {
        if (logValue == logZero) {
          logValue = logZero;
        } else {
          logValue = logValue * naturalLogBase;
        }
        logInnerSummation = exp(logValue);
      }

      logInnerSummation += 1.0;

      float returnLogValue;
      if (logInnerSummation <= 0.0) {
        returnLogValue = logZero;

      } else {
        returnLogValue =
            (float)(log(logInnerSummation) * inverseNaturalLogBase);
        if (returnLogValue > FLT_MAX) {
          returnLogValue = FLT_MAX;
        } else if (returnLogValue < -FLT_MAX) {
          returnLogValue = -FLT_MAX;
        }
      }
      // sum log
      pthread_score_vect[i] = logHighestValue + returnLogValue;
    }
  }

  pthread_exit(NULL);
}

float calculateMiliseconds(timeval t1, timeval t2) {
  float elapsedTime;
  elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
  return elapsedTime;
}

int main(int argc, char *argv[]) {
  // Timing
  timeval t1, t2;
  float cpu_elapsedTime;
  float par_elapsedTime;

  int means_array_size = senone_size * comp_size * comp_size;
  int comp_array_size = senone_size * comp_size;

  means_vect = (float *)malloc(means_array_size * sizeof(float));
  precs_vect = (float *)malloc(means_array_size * sizeof(float));
  weight_vect = (float *)malloc(comp_array_size * sizeof(float));
  factor_vect = (float *)malloc(comp_array_size * sizeof(float));

  cpu_score_vect = (float *)malloc(senone_size * sizeof(float));
  pthread_score_vect = (float *)malloc(senone_size * sizeof(float));

  NTHREADS = atoi(argv[1]);

  int tids[NTHREADS];
  pthread_t threads[NTHREADS];
  pthread_attr_t attr;
  printf("Threads: %d array size: %d\n", NTHREADS, senone_size);

  // load model from file
  FILE *fp = fopen(argv[2], "r");
  if (fp == NULL) {
    printf("Can’t open file\n");
    exit(-1);
  }

  int idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      for (int k = 0; k < comp_size; k++) {
        float elem;
        fscanf(fp, "%f", &elem);
        means_vect[idx] = elem;
        ++idx;
      }
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      for (int k = 0; k < comp_size; k++) {
        float elem;
        fscanf(fp, "%f", &elem);
        precs_vect[idx] = elem;
        idx = idx + 1;
      }
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      float elem;
      fscanf(fp, "%f", &elem);
      weight_vect[idx] = elem;
      idx = idx + 1;
    }
  }

  idx = 0;
  for (int i = 0; i < senone_size; i++) {
    for (int j = 0; j < comp_size; j++) {
      float elem;
      fscanf(fp, "%f", &elem);
      factor_vect[idx] = elem;
      idx = idx + 1;
    }
  }

  fclose(fp);

  gettimeofday(&t1, NULL);
  computeScore_seq(feature_vect, means_vect, precs_vect, weight_vect,
          factor_vect, cpu_score_vect);
  gettimeofday(&t2, NULL);

  cpu_elapsedTime = calculateMiliseconds(t1, t2);

  gettimeofday(&t1, NULL);
  iterations = senone_size / NTHREADS;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (int i = 0; i < NTHREADS; ++i) {
    tids[i] = i;
    pthread_create(&threads[i], &attr, computeScore_thread, (void *)&tids[i]);
  }

  for (int i = 0; i < NTHREADS; ++i) pthread_join(threads[i], NULL);

  gettimeofday(&t2, NULL);
  
  for(int i = 0; i < senone_size; ++i)
      assert(cpu_score_vect[i] == pthread_score_vect[i]);

  par_elapsedTime = calculateMiliseconds(t1, t2);
  printf("CPU Time=%4.3f ms\n", (float)cpu_elapsedTime);
  printf("CPU PThread Time=%4.3f ms\n", par_elapsedTime);
  printf("Speedup=%4.3f\n", (float)cpu_elapsedTime/(float)par_elapsedTime);

  /* Clean up and exit */
  free(means_vect);
  free(precs_vect);

  free(weight_vect);
  free(factor_vect);

  free(cpu_score_vect);
  free(pthread_score_vect);

  pthread_attr_destroy(&attr);
  pthread_exit(NULL);

  return 0;
}