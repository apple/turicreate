/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <math.h>

#import <Accelerate/Accelerate.h>

#import "AudioPreprocessing.h"


// TODO: Make some of these parameters to the model
static const int inputLength = 15600;
static const NSUInteger windowLength = 400;
static const int hopLength = 160;

static const float minFrequency = 125;
static const float maxFrequency = 7500;

static const NSUInteger numBands = 64;
static const double sampleRate = 16000;
static const NSUInteger fftLength = 512;
static const int spectrumSize = 257;

static const float log_offset = 0.01;


API_AVAILABLE(macos(10.13),ios(11.0))
static void setErrorMsg(const char* errorMsg,  NSError** error) {
    if(error) {
      *error = [NSError errorWithDomain:MLModelErrorDomain
                                   code:MLModelErrorGeneric
                               userInfo:@{NSLocalizedDescriptionKey: @(*errorMsg)}];
    }
}

static inline float hzToMel(float freq) {
    return 1127.0f * logf(1 + freq/700.0f);
}


@interface TCSoundClassifierPowerSpectrum : NSObject

@property (readonly) float* PowerSpectrum;

-(nullable instancetype)initWithFFTLength:(NSUInteger)fftLength frameLength:(NSUInteger)frameLength;
-(void)applyHammingWindow:(float*)signal numSamples:(NSUInteger)numSamples;
-(void)calculatePowerSpectrum:(float*)signal numSamples:(NSUInteger)numSamples;

@end


@implementation TCSoundClassifierPowerSpectrum {
    float *_hammingWindow, *_workingFrame;

    DSPSplitComplex _spectrum;
    FFTSetup _fftSetup;
    
    NSUInteger _frameLength, _fftLength;
}


-(nullable instancetype)initWithFFTLength:(NSUInteger)fftLength frameLength:(NSUInteger)frameLength {
    
    self = [super init];
    if (!self) return nil;

    _fftLength = fftLength;
    _frameLength = frameLength;
    
    _hammingWindow = (float*)calloc(frameLength, sizeof(float));
    [self _generateHammingWindow:frameLength];
    
    _fftSetup = vDSP_create_fftsetup(log2(fftLength*2), FFT_RADIX2);
    _spectrum.realp = (float*)calloc(fftLength, sizeof(float));
    _spectrum.imagp = (float*)calloc(fftLength, sizeof(float));
    _PowerSpectrum = (float*)calloc(fftLength/2 + 1, sizeof(float));
    _workingFrame = (float*)calloc(fftLength, sizeof(float));
    
    return self;
}

-(void)dealloc {
    free(_hammingWindow);
    vDSP_destroy_fftsetup(_fftSetup);
    free(_spectrum.realp);
    free(_spectrum.imagp);
    free(_PowerSpectrum);
    free(_workingFrame);
}

-(void)applyHammingWindow:(float*)signal numSamples:(NSUInteger)numSamples {
    NSAssert(numSamples == _frameLength, @"Num samples and frame length must be the same.");
    vDSP_vmul(_hammingWindow, 1, signal, 1, signal, 1, numSamples);
}

-(void)calculatePowerSpectrum:(float*)signal numSamples:(NSUInteger)numSamples {
    // Copy signal into working frame.
    memcpy(_workingFrame, signal, numSamples * sizeof(float));

    // Pad signal with 0 to fftLength.
    memset(&_workingFrame[numSamples], 0, (_fftLength - numSamples) * sizeof(float));

    // Produce packed input necessary for vDSP_fft to work.
    vDSP_ctoz((DSPComplex *)_workingFrame, 2, &_spectrum, 1, _fftLength/2);

    // Perform the forward FFT.
    vDSP_fft_zrip(_fftSetup, &_spectrum, 1, log2(_fftLength), kFFTDirection_Forward);

    float scale = 0.5; // Factor of 2 needed due to vDSP_fft_zrip implementation detail.
    vDSP_zvabs(&_spectrum, 1, _PowerSpectrum, 1, _fftLength/2);
    vDSP_vsmul(_PowerSpectrum, 1, &scale, _PowerSpectrum, 1, _fftLength/2);
}

-(void)_generateHammingWindow:(NSUInteger)frameLength {
    // Don't use vDSP_hamm_window because it uses a slightly different implementation
    for(size_t i = 0; i < frameLength; i++) {
        _hammingWindow[i] = 0.5f - 0.5f*cosf((2*M_PI*i)/(frameLength));
    }
}

@end // TCSoundClassifierPowerSpectrum


@interface TCSoundClassifierMelFrequencyFilterBank : NSObject

-(nullable instancetype)initWithMinFrequency:(float)minFreq
                                maxFrequency:(float)maxFrequency
                                  sampleRate:(double)sampleRate
                                 numMelBands:(NSUInteger)numMelBands
                                     numBins:(NSUInteger)numBins;

-(void)apply:(const float*)powerSpectrum
      output:(double*)out;

@end


@implementation TCSoundClassifierMelFrequencyFilterBank {
    float _minFreq, _maxFreq, _sampleRate;
    NSUInteger _numMelBands, _numBins;
    NSMutableArray* _filters;
    float** _filterBanks;
}

-(nullable instancetype)initWithMinFrequency:(float)minFreq
                                maxFrequency:(float)maxFreq
                                  sampleRate:(double)sampleRate
                                 numMelBands:(NSUInteger)numMelBands
                                     numBins:(NSUInteger)numBins {

    self = [super init];
    if (!self) return nil;

    _minFreq = minFreq;
    _maxFreq = maxFreq;
    _sampleRate = sampleRate;
    _numMelBands = numMelBands;
    _numBins = numBins;
    _filterBanks = calloc(_numMelBands, sizeof(float *));

    NSAssert(_minFreq < _maxFreq, @"Min mel frequency must be less than max mel frequency.");
    NSAssert(_maxFreq != 0, @"Max frequency cannot be 0.");
    NSAssert(_sampleRate != 0, @"Sample rate cannot be 0.");
    NSAssert(_numMelBands != 0, @"numMelBands cannot be 0.");
    NSAssert(_numBins != 0, @"numBins cannot be 0.");

    _filters = [[NSMutableArray alloc] initWithCapacity:numMelBands];

    float melMin = hzToMel(_minFreq);
    float melMax = hzToMel(_maxFreq);

    // Get points equally spaced in the Mel Scale.
    float melBandEdges[_numMelBands + 2];
    float increment = (melMax - melMin) / (_numMelBands + 1);
    vDSP_vramp(&melMin, &increment, melBandEdges, 1, _numMelBands + 2);

    // Get points equally spaced in the Hz Scale.
    float spectrogramBins[spectrumSize];
    float minHz = 0.0;
    float HzIncrement = (sampleRate/2) / (spectrumSize - 1);
    vDSP_vramp(&minHz, &HzIncrement, spectrogramBins, 1, spectrumSize);

    // Convert Hz to Mel
    for(int i = 0; i < spectrumSize; i++) {
        spectrogramBins[i] = hzToMel(spectrogramBins[i]);
    }

    // For each band edge, create a new filter (basically a float array that we vDSP_dot later)
    for(size_t i = 0; i < _numMelBands; i++) {
        // Grab the low, center, and bottom edges
        float lower_edge = melBandEdges[i];
        float center_edge = melBandEdges[i+1];
        float upper_edge = melBandEdges[i+2];

        // Compute lower and upper slopes
        float* lowerBank = (float*)calloc(spectrumSize, sizeof(float));
        float* upperBank = (float*)calloc(spectrumSize, sizeof(float));
        float* filterBank = (float*)calloc(spectrumSize, sizeof(float));
        float add = -lower_edge;
        float div = center_edge - lower_edge;
        vDSP_vsadd(spectrogramBins, 1, &add, lowerBank, 1, spectrumSize);
        vDSP_vsdiv(lowerBank, 1, &div, lowerBank, 1, spectrumSize);

        float negative_one = -1;
        add = upper_edge;
        div = upper_edge - center_edge ;
        vDSP_vsmul(spectrogramBins, 1, &negative_one, upperBank, 1, spectrumSize);
        vDSP_vsadd(upperBank, 1, &add, upperBank, 1, spectrumSize);
        vDSP_vsdiv(upperBank, 1, &div, upperBank, 1, spectrumSize);

        // find intersection
        for(int k = 0; k < spectrumSize; k++) {
            filterBank[k] = (k == 0) ? 0.0 : fmax(0.0, fmin(lowerBank[k], upperBank[k]));
        }

        _filterBanks[i] = filterBank;
        free(lowerBank);
        free(upperBank);
    }

    return self;
}

-(void)dealloc {
  for(size_t i = 0; i < _numMelBands; i++) {
    free(_filterBanks[i]);
  }
  free(_filterBanks);
}

-(void)apply:(const float*)powerSpectrum
      output:(double*)out {
  float res;
  for(size_t i = 0; i < _numMelBands; i++) {
    vDSP_dotpr(powerSpectrum, 1, _filterBanks[i], 1, &res, spectrumSize);
    out[i] = log(res + log_offset);
  }
}

@end   //  TCSoundClassifierMelFrequencyFilterBank


@implementation TCSoundClassifierPreprocessing {
  TCSoundClassifierPowerSpectrum* _powerSpectrum;
  TCSoundClassifierMelFrequencyFilterBank* _melFilterBank;
}

- (nullable instancetype)initWithModelDescription:(MLModelDescription *)modelDescription
                              parameterDictionary:(NSDictionary<NSString *, id> *)parameters
                                            error:(NSError **)error {

  self = [super init];
  if (!self) return nil;

  // Validate input
  if(modelDescription.inputDescriptionsByName.count != 1) {
    setErrorMsg("Model must have only one input", error);
    return nil;
  }
  _inputFeatureName = modelDescription.inputDescriptionsByName.allKeys[0];
  MLFeatureDescription* inputDesc = modelDescription.inputDescriptionsByName.allValues[0];
  if(inputDesc.type != MLFeatureTypeMultiArray) {
    setErrorMsg("Input must an MLMultiArray", error);
    return nil;
  }
  if(inputDesc.type != MLFeatureTypeMultiArray) {
    setErrorMsg("Input must an MLMultiArray", error);
    return nil;
  }
  if(inputDesc.multiArrayConstraint.dataType != MLMultiArrayDataTypeFloat32) {
    setErrorMsg("Input array must have type float", error);
    return nil;
  }

  // Validate output
  if(modelDescription.outputDescriptionsByName.count != 1) {
    setErrorMsg("Model must have only one output", error);
    return nil;
  }
  _outputFeatureName = modelDescription.outputDescriptionsByName.allKeys[0];
  MLFeatureDescription* outputDesc = modelDescription.outputDescriptionsByName.allValues[0];
  if(outputDesc.type != MLFeatureTypeMultiArray) {
    setErrorMsg("Output must an MLMultiArray", error);
    return nil;
  }
  if(outputDesc.type != MLFeatureTypeMultiArray) {
    setErrorMsg("Output must an MLMultiArray", error);
    return nil;
  }
  if(outputDesc.multiArrayConstraint.dataType != MLMultiArrayDataTypeDouble) {
    setErrorMsg("Output array must have type double", error);
    return nil;
  }

  _powerSpectrum = [[TCSoundClassifierPowerSpectrum alloc] initWithFFTLength:fftLength
                                                             frameLength:windowLength];

  _melFilterBank = [[TCSoundClassifierMelFrequencyFilterBank alloc] initWithMinFrequency:minFrequency
                                                                            maxFrequency:maxFrequency
                                                                              sampleRate:sampleRate
                                                                             numMelBands:numBands
                                                                                 numBins:fftLength/2];
  
  return self;
}


- (nullable id<MLFeatureProvider>)predictionFromFeatures:(id<MLFeatureProvider>)input
                                                 options:(MLPredictionOptions *)options
                                                   error:(NSError **)error {

  // Get input data
  MLFeatureValue* inputValue = [input featureValueForName: self.inputFeatureName];
  if(inputValue == nil) {
    setErrorMsg("Input value not found", error);
    return nil;
  }
  if(inputValue.type != MLFeatureTypeMultiArray) {
    setErrorMsg("Input must be an MLMultiArray", error);
    return nil;
  }
  MLMultiArray* inputArray = inputValue.multiArrayValue;
  if(inputArray.dataType != MLMultiArrayDataTypeFloat32) {
    setErrorMsg("Input array must be of type float32", error);
    return nil;
  }
  if(![inputArray.shape isEqualToArray:@[@(inputLength)]]) {
    setErrorMsg("Input array not of correct shape", error);
    return nil;
  }

  // Create output
  int numFrames = 1 + floor((inputLength - windowLength) / hopLength);
  const MLMultiArray* output = [[MLMultiArray alloc] initWithShape:@[@(1), @(numFrames), @(numBands)]
                                                    dataType:MLMultiArrayDataTypeDouble
                                                       error:error];
  if(output == nil) {
    setErrorMsg("Can not create MLMultiArray output", error);
    return nil;
  }
  double* outputPtr = (double*) output.dataPointer;
  const size_t stride = output.strides[1].intValue;
  NSAssert(output.strides[2].intValue == 1, @"Inner stride is not one.");

  // Process input
  float curFrame[windowLength];
  const float* curWindowStart = (float*) inputArray.dataPointer;
  for(int i = 0; i < numFrames; i ++) {
    memcpy(curFrame, curWindowStart, windowLength * sizeof(float));
    curWindowStart += hopLength;

    [_powerSpectrum applyHammingWindow:curFrame numSamples:windowLength];
    [_powerSpectrum calculatePowerSpectrum:curFrame numSamples:windowLength];

    size_t offset = i * stride;
    [_melFilterBank apply:_powerSpectrum.PowerSpectrum output:&(outputPtr[offset])];
  }

  // Set output
  id<MLFeatureProvider> result = [[MLDictionaryFeatureProvider alloc]
                                   initWithDictionary:@{self.outputFeatureName: output}
                                                error:error];
  if(result == nil) {
    setErrorMsg("Can not set output", error);
    return nil;
  }
  return result;
}

@end  // TCSoundClassifierPreprocessing
