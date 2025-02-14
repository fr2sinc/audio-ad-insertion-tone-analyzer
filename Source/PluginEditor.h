/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AudioadinsertiontoneanalyzerAudioProcessorEditor  : public juce::AudioProcessorEditor,
	public Slider::Listener
{
public:
    AudioadinsertiontoneanalyzerAudioProcessorEditor (AudioadinsertiontoneanalyzerAudioProcessor&);
    ~AudioadinsertiontoneanalyzerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioadinsertiontoneanalyzerAudioProcessor& audioProcessor;

	Slider mDelaySlider;
	void sliderValueChanged(Slider* slider) override;

	Label delayLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioadinsertiontoneanalyzerAudioProcessorEditor)
};
