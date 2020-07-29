SoundAnalysisPreprocessing
________________________________________________________________________________

A model which takes audio signal samples as input and outputs an array of
preprocessed samples according to the specified preprocessing types


.. code-block:: proto

	message SoundAnalysisPreprocessing {
	
	    // Specific preprocessing types for sound analysis
	
	       be fed to Vggish feature extractor.
	       c.f. https://arxiv.org/pdf/1609.09430.pdf
	
	       The preprocessing takes input a single channel (monophonic) audio samples
	       975 miliseconds long, sampled at 16KHz, i.e., 15600 samples 1D multiarray
	       and produces preprocessed samples in multiarray of shape [1, 96, 64]
	
	     (1) Splits the input audio samples into overlapping frames, where each
	         frame is 25 milliseconds long and hops forward by 10 milliseconds.
	         Any partial frames at the end are dropped.
	
	     (2) Hann window: apply a periodic Hann with a window_length of
	         25 milliseconds, which translates to 400 samples in 16KHz sampling rate
	
	         w(n) = 0.5 - 0.5 * cos(2*pi*n/window_length_sample),
	         where 0 <= n <= window_lenth_samples - 1 and window_lenth_samples = 400
	
	         Then, the Hann window is applied to each frame as below
	
	         windowed_frame(n) = frame(n) * w(n)
	         where 0 <= n <= window_lenth_samples - 1 and window_lenth_samples = 400
	
	     (3) Power spectrum: calculate short-time Fourier transfor magnitude, with
	         an FFT length of 512
	
	     (4) Log Mel filter bank: calculates a log magnitude mel-frequency
	         spectrogram minimum frequency of 125Hz and maximum frequency of 7500Hz,
	         number of mel bins is 64, log_offset is 0.01, number of spectrum bins
	         is 64.
	
	    message Vggish {
	        // no specific parameter
	    }
	
	    // Vision feature print type
	    oneof SoundAnalysisPreprocessingType {
	        Vggish vggish = 20;
	    }
	
	}






SoundAnalysisPreprocessing.Vggish
--------------------------------------------------------------------------------




.. code-block:: proto

	    message Vggish {
	        // no specific parameter
	    }