
/*
 * Idea:
 *
 * Multiple types of state classes  (rather than enums) with members that are used
 *	to evaluate a state's appropriateness? (i.e. appropriate when there are more high-tones
 *	than low-tones, appropriate when high amplitude, etc.) Would determine state changes
 *	by when the appropriateness of one state rises above the appropriateness of
 *	the current state.
 *
 */


//	Determines the state of the visualization based on provided audio data. Does NOT generate color
//		values for the visualization. (Not yet used)

enum ColorProgressionState
{
	SlowFlow, // Continuously, slowly changes color
	JumpToBeat // Only does large color shifts based on beat
};

enum ColorTargetSelectionState
{
	Random,
	RandomVariantOfSingleColor,
	FrequencyColorMap
};

enum BrightnessSelectionState
{
	ConstantBrightest,
	LinearByAmplitudeWithThreshold
};

//	How to control the individual LEDs
enum StringColoringState
{
	AllSingleColor,
	MovingLeftToRight,
	MovingRightToLeft,
	MovingInOut
};

class VisualizationStateController
{
public:
	void UpdateWithPCMData( float * monoPcmData, int length );

};