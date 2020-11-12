/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioadinsertiontoneanalyzerAudioProcessor::AudioadinsertiontoneanalyzerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	formatManager.registerBasicFormats();       // [1]	
}

AudioadinsertiontoneanalyzerAudioProcessor::~AudioadinsertiontoneanalyzerAudioProcessor()
{
}

//==============================================================================
const juce::String AudioadinsertiontoneanalyzerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioadinsertiontoneanalyzerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioadinsertiontoneanalyzerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioadinsertiontoneanalyzerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioadinsertiontoneanalyzerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioadinsertiontoneanalyzerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioadinsertiontoneanalyzerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioadinsertiontoneanalyzerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AudioadinsertiontoneanalyzerAudioProcessor::getProgramName (int index)
{
    return {};
}

void AudioadinsertiontoneanalyzerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AudioadinsertiontoneanalyzerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback
	const int numInputChannels = getTotalNumInputChannels();
	//set the mSampleRate variable to be used in all functions
	mSampleRate = sampleRate;

	//now I need the num of samples to fit in the delay buffer
	const int delayBufferSize = 2 * (sampleRate + samplesPerBlock); //2sec of delay buffer
	mDelayBuffer.setSize(numInputChannels, delayBufferSize);

	gAnalyzer.setSampleRate(sampleRate);
	transportSource.prepareToPlay(samplesPerBlock, sampleRate);
}

void AudioadinsertiontoneanalyzerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioadinsertiontoneanalyzerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AudioadinsertiontoneanalyzerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	const int bufferLength = buffer.getNumSamples();
	const int delayBufferLength = mDelayBuffer.getNumSamples();

	changeToneState();
	doToneAnalysis(0, bufferLength, buffer.getReadPointer(0));

	//audio injection from audioFile into main Buffer, one time for all channels
	if (toneState == On) {
		//injection
		transportSource.getNextAudioBlock(AudioSourceChannelInfo(buffer));
	}
	
	for (int channel = 0; channel < totalNumInputChannels; ++channel) {

		const float* bufferData = buffer.getReadPointer(channel);
		const float* delayBufferData = mDelayBuffer.getReadPointer(channel);
				
		//delay actions
		fillDelayBuffer(channel, bufferLength, delayBufferLength, bufferData, delayBufferData);
		getFromDelayBuffer(buffer, channel, bufferLength, delayBufferLength, bufferData, delayBufferData);

	}	

	mWritePosition += bufferLength;
	mWritePosition %= delayBufferLength;
	timeCounter++;
}

void AudioadinsertiontoneanalyzerAudioProcessor::doToneAnalysis(int channel, const int bufferLength, const float* bufferData) {

	//calculate FFT only in one channel, if I need to calculate fft on two channel I have to create e mono channel from two channel in a trivial way
	if (channel == 0) {

		if (timeCounter > 150) { //10ms * 100 in samples
			bool fftResult = gAnalyzer.checkGoertzelFrequencies();
			if (fftResult) {
				newToneState = On;
			}
		}
		else {
			newToneState = Off;
		}

		//fill fifo buffer to perform after frequency analysis
		for (int sample = 0; sample < bufferLength; ++sample) {
			gAnalyzer.pushSampleIntoFifo(bufferData[sample]);
		}

	}
}

void AudioadinsertiontoneanalyzerAudioProcessor::changeToneState() {

	if (toneState == On) {
		if (newToneState == On) {
			toneState = Off;			
			timeCounter = 0;
		}
	}
	else if (toneState = Off) {
		if (newToneState == On) {
			toneState = On;
			loadInjectionFile();
			timeCounter = 0;
		}
	}
}

void AudioadinsertiontoneanalyzerAudioProcessor::loadInjectionFile() {

	//load random audio files from audioInjection directory
	File dir = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\audioInjection");
	Array<File> injFiles = dir.findChildFiles(2, ".mp3; .wav");

	if (injFiles.size() != 0) {

		Random rand = Random();
		int x = rand.nextInt(Range<int>(0, injFiles.size() - 1));
		File file = injFiles[x];
		auto* reader = formatManager.createReaderFor(file);
		if (reader != nullptr) {
			std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
			transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
			readerSource.reset(newSource.release());
		}
		transportSource.start();
	}

}

void AudioadinsertiontoneanalyzerAudioProcessor::fillDelayBuffer(int channel, const int bufferLength, const int delayBufferLength,
	const float* bufferData, const float* delayBufferData) {

	//copy data from main buffer to delay buffer
	if (delayBufferLength > bufferLength + mWritePosition) {
		mDelayBuffer.copyFrom(channel, mWritePosition, bufferData, bufferLength);
	}
	else {
		const int bufferRemaining = delayBufferLength - mWritePosition;
		mDelayBuffer.copyFrom(channel, mWritePosition, bufferData, bufferRemaining);
		mDelayBuffer.copyFrom(channel, 0, bufferData + bufferRemaining, bufferLength - bufferRemaining);
	}
}

void AudioadinsertiontoneanalyzerAudioProcessor::getFromDelayBuffer(AudioBuffer<float>& buffer, int channel, const int bufferLength,
	const int delayBufferLength, const float* bufferData, const float* delayBufferData) {

	int delayTime = mDelay;//mDelay;//ex 500 ms
	//(mSampleRate * delayTime / 1000)
	const int readPosition = static_cast<int> (delayBufferLength + mWritePosition - (mSampleRate * delayTime / 1000)) % delayBufferLength;

	if (delayBufferLength > bufferLength + readPosition) {
		buffer.copyFrom(channel, 0, delayBufferData + readPosition, bufferLength);
	}
	else {
		const int bufferRemaining = delayBufferLength - readPosition;
		buffer.copyFrom(channel, 0, delayBufferData + readPosition, bufferRemaining);
		buffer.copyFrom(channel, bufferRemaining, delayBufferData, bufferLength - bufferRemaining);
	}
}

//==============================================================================
bool AudioadinsertiontoneanalyzerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioadinsertiontoneanalyzerAudioProcessor::createEditor()
{
    return new AudioadinsertiontoneanalyzerAudioProcessorEditor (*this);
}

//==============================================================================
void AudioadinsertiontoneanalyzerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AudioadinsertiontoneanalyzerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioadinsertiontoneanalyzerAudioProcessor();
}
