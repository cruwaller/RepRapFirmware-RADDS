/****************************************************************************************************

RepRapFirmware - Tool

This class implements a tool in the RepRap machine, usually (though not necessarily) an extruder.

Tools may have zero or more drives associated with them and zero or more heaters.  There are a fixed number
of tools in a given RepRap, with fixed heaters and drives.  All this is specified on reboot, and cannot
be altered dynamically.  This restriction may be lifted in the future.  Tool descriptions are stored in
GCode macros that are loaded on reboot.

-----------------------------------------------------------------------------------------------------

Version 0.1

Created on: Apr 11, 2014

Adrian Bowyer
RepRap Professional Ltd
http://reprappro.com

Licence: GPL

****************************************************************************************************/

#ifndef TOOL_H_
#define TOOL_H_

class Tool
{
public:

	static Tool * Create(int toolNumber, long d[], size_t dCount, long h[], size_t hCount);
	static void Delete(Tool *t);

	const float *GetOffset() const;
	void SetOffset(const float offs[AXES]);
	size_t DriveCount() const;
	int Drive(int driveNumber) const;
	bool ToolCanDrive(bool extrude);
	size_t HeaterCount() const;
	int Heater(int heaterNumber) const;
	int Number() const;
	void SetVariables(const float* standby, const float* active);
	void GetVariables(float* standby, float* active) const;
	void DefineMix(float* m);
	const float* GetMix() const;
	void SetMixing(bool b);
	bool GetMixing() const;
	float MaxFeedrate() const;
	float InstantDv() const;
	void Print(StringRef& reply);
#if defined(LCD_UI)
        float MaxAcceleration() const;
#endif
	friend class RepRap;

protected:

	Tool* Next() const;
	void Activate(Tool* currentlyActive);
	void Standby();
	void FlagTemperatureFault(int8_t dudHeater);
	void ClearTemperatureFault(int8_t wasDudHeater);
	void UpdateExtruderAndHeaterCount(uint16_t &extruders, uint16_t &heaters) const;
	bool DisplayColdExtrudeWarning();

private:
	static Tool *freelist;

	void SetTemperatureFault(int8_t dudHeater);
	void ResetTemperatureFault(int8_t wasDudHeater);
	bool AllHeatersAtHighTemperature(bool forExtrusion) const;
	int myNumber;
	int drives[DRIVES - AXES];
	float mix[DRIVES - AXES];
	bool mixing;
	size_t driveCount;
	int heaters[HEATERS];
	float activeTemperatures[HEATERS];
	float standbyTemperatures[HEATERS];
	size_t heaterCount;
	Tool* next;
	bool active;
	bool heaterFault;
	float offset[AXES];

	volatile bool displayColdExtrudeWarning;
};

inline int Tool::Drive(int driveNumber) const
{
	return drives[driveNumber];
}

inline size_t Tool::HeaterCount() const
{
	return heaterCount;
}

inline int Tool::Heater(int heaterNumber) const
{
	return heaters[heaterNumber];
}

inline Tool* Tool::Next() const
{
	return next;
}

inline int Tool::Number() const
{
	return myNumber;
}

inline void Tool::DefineMix(float* m)
{
	for(size_t drive = 0; drive < driveCount; drive++)
	{
		mix[drive] = m[drive];
	}
}

inline const float* Tool::GetMix() const
{
	return mix;
}

inline void Tool::SetMixing(bool b)
{
	mixing = b;
}

inline bool Tool::GetMixing() const
{
	return mixing;
}

inline size_t Tool::DriveCount() const
{
	return driveCount;
}

inline const float *Tool::GetOffset() const
{
	return offset;
}

inline void Tool::SetOffset(const float offs[AXES])
{
	for(size_t i = 0; i < AXES; ++i)
	{
		offset[i] = offs[i];
	}
}

#endif /* TOOL_H_ */
