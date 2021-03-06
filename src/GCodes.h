/****************************************************************************************************

RepRapFirmware - G Codes

This class interprets G Codes from one or more sources, and calls the functions in Move, Heat etc
that drive the machine to do what the G Codes command.

-----------------------------------------------------------------------------------------------------

Version 0.1

13 February 2013

Adrian Bowyer
RepRap Professional Ltd
http://reprappro.com

Licence: GPL

****************************************************************************************************/

#ifndef GCODES_H
#define GCODES_H

#include "GCodeBuffer.h"

#if defined(LCD_UI)
#include "UIBuffer.h"
#endif

const unsigned int StackSize = 5;

const char feedrateLetter = 'F';						// GCode feedrate
const char extrudeLetter = 'E'; 						// GCode extrude

// Type for specifying which endstops we want to check
typedef uint16_t EndstopChecks;							// must be large enough to hold a bitmap of drive numbers or ZProbeActive
const EndstopChecks ZProbeActive = 1 << 15;				// must be distinct from 1 << (any drive number)

const float minutesToSeconds = 60.0;
const float secondsToMinutes = 1.0/minutesToSeconds;

// Enumeration to list all the possible states that the Gcode processing machine may be in
enum class GCodeState
{
	normal,												// not doing anything and ready to process a new GCode
	waitingForMoveToComplete,							// doing a homing move, so we must wait for it to finish before processing another GCode
	homing,
	setBed1,
	setBed2,
	setBed3,
	toolChange1,
	toolChange2,
	toolChange3,
	pausing1,
	pausing2,
	resuming1,
	resuming2,
	resuming3
};

// Small class to stack the state when we execute a macro file
class GCodeMachineState
{
public:
	GCodeState state;
	GCodeBuffer *gb;									// this may be null when executing config.g
	float feedrate;
	FileData fileState;
	bool drivesRelative;
	bool axesRelative;
	bool doingFileMacro;
};

//****************************************************************************************************

// The GCode interpreter

class GCodes
{   
public:
	struct RawMove
	{
		float coords[DRIVES];											// new positions for the axes, amount of movement for the extruders
		float feedRate;													// feed rate of this move
		FilePosition filePos;											// offset in the file being printed that this move was read from
		EndstopChecks endStopsToCheck;									// endstops to check
		uint8_t moveType;												// the S parameter from the G0 or G1 command, 0 for a normal move
		bool isFirmwareRetraction;										// true if this is a firmware retraction/un-retraction move
		bool usePressureAdvance;										// true if we want to us extruder pressure advance, if there is any extrusion
	};
  
#if defined(WEBSERVER)
    GCodes(Platform* p, Webserver* w);
#else
    GCodes(Platform* p);
#endif
    void Spin();														// Called in a tight loop to make this class work
    void Init();														// Set it up
    void Exit();														// Shut it down
    void Reset();														// Reset some parameter to defaults
    bool ReadMove(RawMove& m);											// Called by the Move class to get a movement set by the last G Code
    void ClearMove();
    void QueueFileToPrint(const char* fileName);						// Open a file of G Codes to run
    void DeleteFile(const char* fileName);								// Does what it says
    bool GetProbeCoordinates(int count, float& x, float& y, float& z) const;	// Get pre-recorded probe coordinates
    void GetCurrentCoordinates(StringRef& s) const;						// Write where we are into a string
    bool DoingFileMacro() const;										// Or still busy processing a macro file?
    float FractionOfFilePrinted() const;								// Get fraction of file printed
    void Diagnostics();													// Send helpful information out
    bool HaveIncomingData() const;										// Is there something that we have to do?
	size_t GetStackPointer() const;										// Returns the current stack pointer
    bool GetAxisIsHomed(uint8_t axis) const { return axisIsHomed[axis]; } // Is the axis at 0?
    void SetAxisIsHomed(uint8_t axis) { axisIsHomed[axis] = true; }		// Tell us that the axis is now homed

    void PauseSDPrint();												// Pause the current print from SD card
    float GetSpeedFactor() const { return speedFactor * minutesToSeconds; }	// Return the current speed factor
    float GetExtrusionFactor(size_t extruder) { return extrusionFactors[extruder]; } // Return the current extrusion factors
    float GetRawExtruderPosition(size_t drive) const;					// Get the actual extruder position, after adjusting the extrusion factor
    float GetRawExtruderTotalByDrive(size_t extruder) const;			// Get the total extrusion since start of print, for one drive
    float GetTotalRawExtrusion() const { return rawExtruderTotal; }		// Get the total extrusion since start of print, all drives
    
    bool HaveAux() const { return auxDetected; }						// Any device on the AUX line?
	bool IsFlashing() const { return isFlashing; }						// Is a new firmware binary going to be flashed?
	OutputBuffer *GetAuxGCodeReply();									// Returns cached G-Code reply for AUX devices and clears its reference
    uint32_t GetAuxSeq() { return auxSeq; }

	bool IsPaused() const;
	bool IsPausing() const;
	bool IsResuming() const;
	bool IsRunning() const;

    bool AllAxesAreHomed() const;										// Return true if all axes are homed
    bool DoFileMacro(const char* fileName, bool reportMissing = true);	// Run a GCode macro in a file, optionally report error if not found

	bool SetExtrusionFactor(int extruder, float extrusionFactorPercentage); // Set extrusion factor % for extruder #extruder
    bool SetSpeedFactor(float speedFacto);                                  // Set speed factor (decimal)

#if defined(LCD_UI)
	bool DrivesRelative() const;
	void lcdUiBuffer(UIBuffer* buf) { lcdUiInput = buf; }
#endif

private:
  
    void StartNextGCode(StringRef& reply);								// Fetch a new GCode and process it
    void DoFilePrint(GCodeBuffer* gb, StringRef& reply);				// Get G Codes from a file and print them
    bool AllMovesAreFinishedAndMoveBufferIsLoaded();					// Wait for move queue to exhaust and the current position is loaded
    bool DoCannedCycleMove(EndstopChecks ce);							// Do a move from an internally programmed canned cycle
    void FileMacroCyclesReturn();										// End a macro
    bool ActOnCode(GCodeBuffer* gb, StringRef& reply);					// Do a G, M or T Code
    bool HandleGcode(GCodeBuffer* gb, StringRef& reply);				// Do a G code
    bool HandleMcode(GCodeBuffer* gb, StringRef& reply);				// Do an M code
    bool HandleTcode(GCodeBuffer* gb, StringRef& reply);				// Do a T code
    void CancelPrint();													// Cancel the current print
    int SetUpMove(GCodeBuffer* gb, StringRef& reply);					// Pass a move on to the Move module
    bool DoDwell(GCodeBuffer *gb);										// Wait for a bit
    bool DoDwellTime(float dwell);										// Really wait for a bit
    bool DoHome(GCodeBuffer *gb, StringRef& reply, bool& error);		// Home some axes
    bool DoSingleZProbeAtPoint(int probePointIndex, float heightAdjust); // Probe at a given point
    bool DoSingleZProbe(bool reportOnly, float heightAdjust);			// Probe where we are
    int DoZProbe(float distance);										// Do a Z probe cycle up to the maximum specified distance
    bool SetSingleZProbeAtAPosition(GCodeBuffer *gb, StringRef& reply);	// Probes at a given position - see the comment at the head of the function itself
    void SetBedEquationWithProbe(int sParam, StringRef& reply);			// Probes a series of points and sets the bed equation
    bool SetPrintZProbe(GCodeBuffer *gb, StringRef& reply);				// Either return the probe value, or set its threshold
    void SetOrReportOffsets(StringRef& reply, GCodeBuffer *gb);			// Deal with a G10
    bool SetPositions(GCodeBuffer *gb);									// Deal with a G92
    bool LoadMoveBufferFromGCode(GCodeBuffer *gb,  						// Set up a move for the Move class
    		bool doingG92, bool applyLimits);
    bool NoHome() const;												// Are we homing and not finished?
    void Push();														// Push feedrate etc on the stack
    void Pop();															// Pop feedrate etc
    void DisableDrives();												// Turn the motors off
    void SetEthernetAddress(GCodeBuffer *gb, int mCode);				// Does what it says
    void SetMACAddress(GCodeBuffer *gb);								// Deals with an M540
	void HandleReply(GCodeBuffer *gb, bool error, const char *reply);	// Handle G-Code replies
	void HandleReply(GCodeBuffer *gb, bool error, OutputBuffer *reply);
    bool OpenFileToWrite(const char* directory,							// Start saving GCodes in a file
    		const char* fileName, GCodeBuffer *gb);
    void WriteGCodeToFile(GCodeBuffer *gb);								// Write this GCode into a file
    bool SendConfigToLine();											// Deal with M503
#if defined(WEBSERVER)
    void WriteHTMLToFile(char b, GCodeBuffer *gb);						// Save an HTML file (usually to upload a new web interface)
#endif
    bool OffsetAxes(GCodeBuffer *gb);									// Set offsets - deprecated, use G10
    void SetPidParameters(GCodeBuffer *gb, int heater, StringRef& reply);	// Set the P/I/D parameters for a heater
    void SetHeaterParameters(GCodeBuffer *gb, StringRef& reply);		 // Set the thermistor and ADC parameters for a heater
    void ManageTool(GCodeBuffer *gb, StringRef& reply);					// Create a new tool definition
    void SetToolHeaters(Tool *tool, float temperature);					// Set all a tool's heaters to the temperature.  For M104...
    bool ToolHeatersAtSetTemperatures(const Tool *tool) const;			// Wait for the heaters associated with the specified tool to reach their set temperatures
    void SetAllAxesNotHomed();											// Flag all axes as not homed
    void SetPositions(float positionNow[DRIVES]);						// Set the current position to be this
    const char *TranslateEndStopResult(EndStopHit es);					// Translate end stop result to text
    bool RetractFilament(bool retract);									// Retract or un-retract filaments

    Platform* platform;							// The RepRap machine
#if defined(WEBSERVER)
    Webserver* webserver;						// The webserver class

    GCodeBuffer* httpGCode;						// The sources...
    GCodeBuffer* telnetGCode;					// ...
#endif
    GCodeBuffer* fileGCode;						// ...
    GCodeBuffer* serialGCode;					// ...
    GCodeBuffer* auxGCode;						// this one is for the LCD display on the async serial interface
    GCodeBuffer* fileMacroGCode;				// ...
    GCodeBuffer *gbCurrent;
    bool active;								// Live and running?
    bool isPaused;								// true if the print has been paused
    bool dwellWaiting;							// We are in a dwell
    bool moveAvailable;							// Have we seen a move G Code and set it up?
    float dwellTime;							// How long a pause for a dwell (seconds)?
    float feedRate;								// The feed rate of the last G0/G1 command that had an F parameter
    RawMove moveBuffer;							// Move details to pass to Move class
    float savedMoveBuffer[DRIVES + 1];			// The position and feedrate when we started the current simulation
    float pausedMoveBuffer[DRIVES + 1]; 		// Move coordinates; last is feed rate
    GCodeState state;							// The main state variable of the GCode state machine
	bool drivesRelative;
	bool axesRelative;
    GCodeMachineState stack[StackSize];			// State that we save when calling macro files
    unsigned int stackPointer;					// Push and Pop stack pointer
    static const char axisLetters[AXES]; 		// 'X', 'Y', 'Z'
	float axisScaleFactors[AXES];				// Scale XYZ coordinates by this factor (for Delta configurations)
    float lastRawExtruderPosition[DRIVES - AXES];	// Extruder position of the last move fed into the Move class
    float rawExtruderTotalByDrive[DRIVES - AXES];	// Total extrusion amount fed to Move class since starting print, before applying extrusion factor, per drive
    float rawExtruderTotal;						// Total extrusion amount fed to Move class since starting print, before applying extrusion factor, summed over all drives
	float record[DRIVES+1];						// Temporary store for move positions
	float moveToDo[DRIVES+1];					// Where to go set by G1 etc
	bool activeDrive[DRIVES+1];					// Is this drive involved in a move?
	bool offSetSet;								// Are any axis offsets non-zero?
    float distanceScale;						// MM or inches
    FileData fileBeingPrinted;
    FileData fileToPrint;
    FileStore* fileBeingWritten;				// A file to write G Codes (or sometimes HTML) in
    uint16_t toBeHomed;							// Bitmap of axes still to be homed
    bool doingFileMacro;						// Are we executing a macro file?
    int oldToolNumber, newToolNumber;			// Tools being changed
    const char* eofString;						// What's at the end of an HTML file?
    uint8_t eofStringCounter;					// Check the...
    uint8_t eofStringLength;					// ... EoF string as we read.
    int probeCount;								// Counts multiple probe points
    int8_t cannedCycleMoveCount;				// Counts through internal (i.e. not macro) canned cycle moves
    bool cannedCycleMoveQueued;					// True if a canned cycle move has been set
    bool zProbesSet;							// True if all Z probing is done and we can set the bed equation
    float longWait;								// Timer for things that happen occasionally (seconds)
    bool limitAxes;								// Don't think outside the box.
    bool axisIsHomed[AXES];						// These record which of the axes have been homed
    float pausedFanValues[NUM_FANS];			// Fan speeds when the print was paused
    float speedFactor;							// speed factor, including the conversion from mm/min to mm/sec, normally 1/60
    float extrusionFactors[DRIVES - AXES];		// extrusion factors (normally 1.0)
    float lastProbedZ;							// the last height at which the Z probe stopped

    bool auxDetected;							// Have we processed at least one G-Code from an AUX device?
	OutputBuffer *auxGCodeReply;				// G-Code reply for AUX devices (special one because it is actually encapsulated before sending)
	uint32_t auxSeq;							// Sequence number for AUX devices
    bool simulating;
    float simulationTime;
	bool isFlashing;							// Is a new firmware binary going to be flashed?
    FilePosition filePos;						// The position we got up to in the file being printed

    // Firmware retraction settings
    float retractLength, retractExtra;			// retraction length and extra length to un-retract
    float retractSpeed;							// retract speed in mm/min
    float retractHop;							// Z hop when retracting

#if defined(LCD_UI)
    GCodeBuffer* lcdGCode;
    UIBuffer* lcdUiInput;
#endif
};

//*****************************************************************************************************

inline bool GCodes::DoingFileMacro() const
{
	return doingFileMacro;
}

inline bool GCodes::HaveIncomingData() const
{
// ???? LCD_UI
	return fileBeingPrinted.IsLive() ||
#if defined(WEBSERVER)
			webserver->GCodeAvailable(WebSource::HTTP) ||
			webserver->GCodeAvailable(WebSource::Telnet) ||
#endif
			platform->GCodeAvailable(SerialSource::USB) ||
			platform->GCodeAvailable(SerialSource::AUX);
}

#if defined(LCD_UI)
inline bool GCodes::DrivesRelative() const
{
	return drivesRelative;
}
#endif

inline bool GCodes::AllAxesAreHomed() const
{
	return axisIsHomed[X_AXIS] && axisIsHomed[Y_AXIS] && axisIsHomed[Z_AXIS];
}

inline void GCodes::SetAllAxesNotHomed()
{
	axisIsHomed[X_AXIS] = axisIsHomed[Y_AXIS] = axisIsHomed[Z_AXIS] = false;
}

inline size_t GCodes::GetStackPointer() const
{
	return stackPointer;
}

inline OutputBuffer *GCodes::GetAuxGCodeReply()
{
	OutputBuffer *temp = auxGCodeReply;
	auxGCodeReply = nullptr;
	return temp;
}

#endif
