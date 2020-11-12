/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioadinsertiontoneanalyzerAudioProcessorEditor::AudioadinsertiontoneanalyzerAudioProcessorEditor (AudioadinsertiontoneanalyzerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
	mDelaySlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
	mDelaySlider.setTextBoxStyle(Slider::TextBoxBelow, true, 50, 20);
	mDelaySlider.setRange(0.0f, 2000.0f, 10.0f);
	mDelaySlider.setValue(0.0f);
	mDelaySlider.addListener(this);
	addAndMakeVisible(&mDelaySlider);

	delayLabel.setText("Delay Slider", dontSendNotification);
	delayLabel.attachToComponent(&mDelaySlider, false);

    setSize (400, 300);
}

AudioadinsertiontoneanalyzerAudioProcessorEditor::~AudioadinsertiontoneanalyzerAudioProcessorEditor()
{
}

//==============================================================================
void AudioadinsertiontoneanalyzerAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll(Colours::black);

}

void AudioadinsertiontoneanalyzerAudioProcessorEditor::resized()
{
	mDelaySlider.setBounds(getWidth() / 2 - 50, getHeight() / 2 - 75, 100, 150);
}

void AudioadinsertiontoneanalyzerAudioProcessorEditor::sliderValueChanged(Slider * slider)
{
	//check if the *slider points to a memory location
	if (slider == &mDelaySlider)
		audioProcessor.mDelay = mDelaySlider.getValue();
}
