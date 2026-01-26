# MotionCam MCRAW decoder

A simple library for decoding files recorded by [MotionCam Pro](https://www.motioncamapp.com/).

## Usage

Look in `example.cpp` for a simple example on how to extract the RAW frames into DNGs and the audio into a WAV file.

To build the example:

```
mkdir build

cd build

cmake ..

make
```

To extract the first frame and audio from a `.mcraw` file run:

`./example <path to mcraw file> -n 1`


## Sample Files

You can download a sample file from [here](https://storage.googleapis.com/motioncamapp.com/samples/007-VIDEO_24mm-240328_141729.0.mcraw).

## MotionCam Pro

MotionCam Pro is an app for Android that provides the ability to record RAW videos. Get it from the [Play Store](https://play.google.com/store/apps/details?id=com.motioncam.pro&hl=en&gl=US).
